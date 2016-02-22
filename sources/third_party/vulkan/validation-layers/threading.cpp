/*
 * Vulkan
 *
 * Copyright (C) 2015 Valve, Inc.
 * Copyright (C) 2016 Google, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unordered_map>
#include <list>

#include "vk_loader_platform.h"
#include "vulkan/vk_layer.h"
#include "vk_layer_config.h"
#include "vk_layer_extension_utils.h"
#include "vk_layer_utils.h"
#include "vk_enum_validate_helper.h"
#include "vk_struct_validate_helper.h"
#include "vk_layer_table.h"
#include "vk_layer_logging.h"
#include "threading.h"

#include "vk_dispatch_table_helper.h"
#include "vk_struct_string_helper_cpp.h"
#include "vk_layer_data.h"

#include "thread_check.h"

static void initThreading(layer_data *my_data, const VkAllocationCallbacks *pAllocator)
{

    uint32_t report_flags = 0;
    uint32_t debug_action = 0;
    FILE *log_output = NULL;
    const char *strOpt;
    VkDebugReportCallbackEXT callback;
    // initialize Threading options
    report_flags = getLayerOptionFlags("ThreadingReportFlags", 0);
    getLayerOptionEnum("ThreadingDebugAction", (uint32_t *) &debug_action);

    if (debug_action & VK_DBG_LAYER_ACTION_LOG_MSG)
    {
        strOpt = getLayerOption("ThreadingLogFilename");
        log_output = getLayerLogOutput(strOpt, "Threading");
        VkDebugReportCallbackCreateInfoEXT dbgCreateInfo;
        memset(&dbgCreateInfo, 0, sizeof(dbgCreateInfo));
        dbgCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
        dbgCreateInfo.flags = report_flags;
        dbgCreateInfo.pfnCallback = log_callback;
        dbgCreateInfo.pUserData = (void *) log_output;
        layer_create_msg_callback(my_data->report_data, &dbgCreateInfo, pAllocator, &callback);
        my_data->logging_callback.push_back(callback);
    }

    if (debug_action & VK_DBG_LAYER_ACTION_DEBUG_OUTPUT) {
        VkDebugReportCallbackCreateInfoEXT dbgCreateInfo;
        memset(&dbgCreateInfo, 0, sizeof(dbgCreateInfo));
        dbgCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
        dbgCreateInfo.flags = report_flags;
        dbgCreateInfo.pfnCallback = win32_debug_output_msg;
        dbgCreateInfo.pUserData = NULL;
        layer_create_msg_callback(my_data->report_data, &dbgCreateInfo, pAllocator, &callback);
        my_data->logging_callback.push_back(callback);
    }

    if (!threadingLockInitialized)
    {
        loader_platform_thread_create_mutex(&threadingLock);
        loader_platform_thread_init_cond(&threadingCond);
        threadingLockInitialized = 1;
    }
}


VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateInstance(const VkInstanceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkInstance* pInstance)
{
    VkLayerInstanceCreateInfo *chain_info = get_chain_info(pCreateInfo, VK_LAYER_LINK_INFO);

    assert(chain_info->u.pLayerInfo);
    PFN_vkGetInstanceProcAddr fpGetInstanceProcAddr = chain_info->u.pLayerInfo->pfnNextGetInstanceProcAddr;
    PFN_vkCreateInstance fpCreateInstance = (PFN_vkCreateInstance) fpGetInstanceProcAddr(NULL, "vkCreateInstance");
    if (fpCreateInstance == NULL) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    // Advance the link info for the next element on the chain
    chain_info->u.pLayerInfo = chain_info->u.pLayerInfo->pNext;

    VkResult result = fpCreateInstance(pCreateInfo, pAllocator, pInstance);
    if (result != VK_SUCCESS)
        return result;

    layer_data *my_data = get_my_data_ptr(get_dispatch_key(*pInstance), layer_data_map);
    my_data->instance_dispatch_table = new VkLayerInstanceDispatchTable;
    layer_init_instance_dispatch_table(*pInstance, my_data->instance_dispatch_table, fpGetInstanceProcAddr);

    my_data->report_data = debug_report_create_instance(
                               my_data->instance_dispatch_table,
                               *pInstance,
                               pCreateInfo->enabledExtensionCount,
                               pCreateInfo->ppEnabledExtensionNames);
    initThreading(my_data, pAllocator);
    return result;
}


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyInstance(VkInstance instance, const VkAllocationCallbacks* pAllocator)
{
    dispatch_key key = get_dispatch_key(instance);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerInstanceDispatchTable *pTable = my_data->instance_dispatch_table;
    startWriteObject(my_data, instance);
    pTable->DestroyInstance(instance, pAllocator);
    finishWriteObject(my_data, instance);

    // Clean up logging callback, if any
    while (my_data->logging_callback.size() > 0) {
        VkDebugReportCallbackEXT callback = my_data->logging_callback.back();
        layer_destroy_msg_callback(my_data->report_data, callback, pAllocator);
        my_data->logging_callback.pop_back();
    }

    layer_debug_report_destroy_instance(my_data->report_data);
    delete my_data->instance_dispatch_table;
    layer_data_map.erase(key);

    if (layer_data_map.empty()) {
        // Release mutex when destroying last instance.
        loader_platform_thread_delete_mutex(&threadingLock);
        threadingLockInitialized = 0;
    }
}


VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateDevice(VkPhysicalDevice gpu, const VkDeviceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDevice* pDevice)
{
    VkLayerDeviceCreateInfo *chain_info = get_chain_info(pCreateInfo, VK_LAYER_LINK_INFO);

    assert(chain_info->u.pLayerInfo);
    PFN_vkGetInstanceProcAddr fpGetInstanceProcAddr = chain_info->u.pLayerInfo->pfnNextGetInstanceProcAddr;
    PFN_vkGetDeviceProcAddr fpGetDeviceProcAddr = chain_info->u.pLayerInfo->pfnNextGetDeviceProcAddr;
    PFN_vkCreateDevice fpCreateDevice = (PFN_vkCreateDevice) fpGetInstanceProcAddr(NULL, "vkCreateDevice");
    if (fpCreateDevice == NULL) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    // Advance the link info for the next element on the chain
    chain_info->u.pLayerInfo = chain_info->u.pLayerInfo->pNext;

    VkResult result = fpCreateDevice(gpu, pCreateInfo, pAllocator, pDevice);
    if (result != VK_SUCCESS) {
        return result;
    }

    layer_data *my_instance_data = get_my_data_ptr(get_dispatch_key(gpu), layer_data_map);
    layer_data *my_device_data = get_my_data_ptr(get_dispatch_key(*pDevice), layer_data_map);

    // Setup device dispatch table
    my_device_data->device_dispatch_table = new VkLayerDispatchTable;
    layer_init_device_dispatch_table(*pDevice, my_device_data->device_dispatch_table, fpGetDeviceProcAddr);

    my_device_data->report_data = layer_debug_report_create_device(my_instance_data->report_data, *pDevice);
    return result;
}


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyDevice(VkDevice device, const VkAllocationCallbacks* pAllocator)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data* dev_data = get_my_data_ptr(key, layer_data_map);
    startWriteObject(dev_data, device);
    dev_data->device_dispatch_table->DestroyDevice(device, pAllocator);
    finishWriteObject(dev_data, device);
    layer_data_map.erase(key);
}

static const VkExtensionProperties threading_extensions[] = {
    {
        VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
        VK_EXT_DEBUG_REPORT_SPEC_VERSION
    }
};

VK_LAYER_EXPORT VkResult VKAPI_CALL vkEnumerateInstanceExtensionProperties(const char *pLayerName, uint32_t *pCount,  VkExtensionProperties* pProperties)
{
    return util_GetExtensionProperties(ARRAY_SIZE(threading_extensions), threading_extensions, pCount, pProperties);
}

static const VkLayerProperties globalLayerProps[] = {
    {
        "VK_LAYER_GOOGLE_threading",
        VK_API_VERSION, // specVersion
        1,
        "Google Validation Layer",
    }
};


VK_LAYER_EXPORT VkResult VKAPI_CALL vkEnumerateInstanceLayerProperties(uint32_t *pCount,  VkLayerProperties* pProperties)
{
    return util_GetLayerProperties(ARRAY_SIZE(globalLayerProps), globalLayerProps, pCount, pProperties);
}

static const VkLayerProperties deviceLayerProps[] = {
    {
        "VK_LAYER_GOOGLE_threading",
        VK_API_VERSION, // specVersion
        1,
        "Google Validation Layer",
    }
};

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceExtensionProperties(
        VkPhysicalDevice physicalDevice,
        const char *pLayerName,
        uint32_t *pCount,
        VkExtensionProperties* pProperties)
{
    if (pLayerName == NULL) {
        dispatch_key key = get_dispatch_key(physicalDevice);
        layer_data *my_data = get_my_data_ptr(key, layer_data_map);
        return my_data->instance_dispatch_table->EnumerateDeviceExtensionProperties(
                                                    physicalDevice,
                                                    NULL,
                                                    pCount,
                                                    pProperties);
    } else {
        // Threading layer does not have any device extensions
        return util_GetExtensionProperties(0,
                                           nullptr,
                                           pCount, pProperties);
    }
}

VK_LAYER_EXPORT VkResult VKAPI_CALL vkEnumerateDeviceLayerProperties(VkPhysicalDevice physicalDevice, uint32_t *pCount, VkLayerProperties* pProperties)
{
    return util_GetLayerProperties(ARRAY_SIZE(deviceLayerProps), deviceLayerProps, pCount, pProperties);
}

static inline PFN_vkVoidFunction layer_intercept_proc(const char *name)
{
    for (int i=0; i<sizeof(procmap)/sizeof(procmap[0]); i++) {
        if (!strcmp(name, procmap[i].name)) return procmap[i].pFunc;
    }
    return NULL;
}


static inline PFN_vkVoidFunction layer_intercept_instance_proc(const char *name)
{
    if (!name || name[0] != 'v' || name[1] != 'k')
        return NULL;

    name += 2;
    if (!strcmp(name, "CreateInstance"))
        return (PFN_vkVoidFunction) vkCreateInstance;
    if (!strcmp(name, "DestroyInstance"))
        return (PFN_vkVoidFunction) vkDestroyInstance;
    if (!strcmp(name, "EnumerateInstanceExtensionProperties"))
        return (PFN_vkVoidFunction) vkEnumerateInstanceExtensionProperties;
    if (!strcmp(name, "EnumerateInstanceLayerProperties"))
        return (PFN_vkVoidFunction) vkEnumerateInstanceLayerProperties;
    if (!strcmp(name, "EnumerateDeviceExtensionProperties"))
        return (PFN_vkVoidFunction) vkEnumerateDeviceExtensionProperties;
    if (!strcmp(name, "EnumerateDeviceLayerProperties"))
        return (PFN_vkVoidFunction) vkEnumerateDeviceLayerProperties;
    if (!strcmp(name, "CreateDevice"))
        return (PFN_vkVoidFunction) vkCreateDevice;
    if (!strcmp(name, "GetInstanceProcAddr"))
        return (PFN_vkVoidFunction) vkGetInstanceProcAddr;

    return NULL;
}

VK_LAYER_EXPORT PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr(VkDevice device, const char* funcName)
{
    PFN_vkVoidFunction addr;
    layer_data *dev_data;
    if (device == VK_NULL_HANDLE) {
        return NULL;
    }

    addr = layer_intercept_proc(funcName);
    if (addr)
        return addr;

    dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkLayerDispatchTable* pTable = dev_data->device_dispatch_table;

    if (pTable->GetDeviceProcAddr == NULL)
        return NULL;
    return pTable->GetDeviceProcAddr(device, funcName);
}

VK_LAYER_EXPORT PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance instance, const char* funcName)
{
    PFN_vkVoidFunction addr;
    layer_data* my_data;

    addr = layer_intercept_instance_proc(funcName);
    if (addr) {
        return addr;
    }

    if (instance == VK_NULL_HANDLE) {
        return NULL;
    }

    my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    addr = debug_report_get_instance_proc_addr(my_data->report_data, funcName);
    if (addr) {
        return addr;
    }

    VkLayerInstanceDispatchTable* pTable = my_data->instance_dispatch_table;
    if (pTable->GetInstanceProcAddr == NULL) {
        return NULL;
    }
    return pTable->GetInstanceProcAddr(instance, funcName);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugReportCallbackEXT(
    VkInstance                                      instance,
    const VkDebugReportCallbackCreateInfoEXT*       pCreateInfo,
    const VkAllocationCallbacks*                    pAllocator,
    VkDebugReportCallbackEXT*                       pMsgCallback)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    startReadObject(my_data, instance);
    VkResult result = my_data->instance_dispatch_table->CreateDebugReportCallbackEXT(instance, pCreateInfo, pAllocator, pMsgCallback);
    if (VK_SUCCESS == result) {
        result = layer_create_msg_callback(my_data->report_data, pCreateInfo, pAllocator, pMsgCallback);
    }
    finishReadObject(my_data, instance);
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyDebugReportCallbackEXT(
    VkInstance                                  instance,
    VkDebugReportCallbackEXT                    callback,
    const VkAllocationCallbacks*                pAllocator)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    startReadObject(my_data, instance);
    startWriteObject(my_data, callback);
    my_data->instance_dispatch_table->DestroyDebugReportCallbackEXT(instance, callback, pAllocator);
    layer_destroy_msg_callback(my_data->report_data, callback, pAllocator);
    finishReadObject(my_data, instance);
    finishWriteObject(my_data, callback);
}

VkResult VKAPI_CALL vkAllocateCommandBuffers(
    VkDevice                                    device,
    const VkCommandBufferAllocateInfo*          pAllocateInfo,
    VkCommandBuffer*                            pCommandBuffers)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    startReadObject(my_data, device);
    startWriteObject(my_data, pAllocateInfo->commandPool);

    result = pTable->AllocateCommandBuffers(device, pAllocateInfo, pCommandBuffers);
    finishReadObject(my_data, device);
    finishWriteObject(my_data, pAllocateInfo->commandPool);

    // Record mapping from command buffer to command pool
    if (VK_SUCCESS == result) {
        for (int index=0;index<pAllocateInfo->commandBufferCount;index++) {
            loader_platform_thread_lock_mutex(&threadingLock);
            command_pool_map[pCommandBuffers[index]] = pAllocateInfo->commandPool;
            loader_platform_thread_unlock_mutex(&threadingLock);
        }
    }

    return result;
}

void VKAPI_CALL vkFreeCommandBuffers(
    VkDevice                                    device,
    VkCommandPool                               commandPool,
    uint32_t                                    commandBufferCount,
    const VkCommandBuffer*                      pCommandBuffers)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    const bool lockCommandPool = false; // pool is already directly locked
    startReadObject(my_data, device);
    startWriteObject(my_data, commandPool);
    for (int index=0;index<commandBufferCount;index++) {
        startWriteObject(my_data, pCommandBuffers[index], lockCommandPool);
    }

    pTable->FreeCommandBuffers(device,commandPool,commandBufferCount,pCommandBuffers);
    finishReadObject(my_data, device);
    finishWriteObject(my_data, commandPool);
    for (int index=0;index<commandBufferCount;index++) {
        finishWriteObject(my_data, pCommandBuffers[index], lockCommandPool);
        loader_platform_thread_lock_mutex(&threadingLock);
        command_pool_map.erase(pCommandBuffers[index]);
        loader_platform_thread_unlock_mutex(&threadingLock);
    }
}

