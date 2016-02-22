/* Copyright (c) 2015-2016 The Khronos Group Inc.
 * Copyright (c) 2015-2016 Valve Corporation
 * Copyright (c) 2015-2016 LunarG, Inc.
 * Copyright (C) 2015-2016 Google Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and/or associated documentation files (the "Materials"), to
 * deal in the Materials without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Materials, and to permit persons to whom the Materials
 * are furnished to do so, subject to the following conditions:
 *
 * The above copyright notice(s) and this permission notice shall be included
 * in all copies or substantial portions of the Materials.
 *
 * THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE MATERIALS OR THE
 * USE OR OTHER DEALINGS IN THE MATERIALS
 *
 * Author: Ian Elliott <ian@lunarg.com>
 * Author: Ian Elliott <ianelliott@google.com>
 */

#include <stdio.h>
#include <string.h>
#include <vk_loader_platform.h>
#include <vulkan/vk_icd.h>
#include "swapchain.h"
#include "vk_layer_extension_utils.h"
#include "vk_enum_string_helper.h"

static int globalLockInitialized = 0;
static loader_platform_thread_mutex globalLock;

// The following is for logging error messages:
static std::unordered_map<void *, layer_data *> layer_data_map;

template layer_data *get_my_data_ptr<layer_data>(
        void *data_key,
        std::unordered_map<void *, layer_data *> &data_map);

static const VkExtensionProperties instance_extensions[] = {
    {
        VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
        VK_EXT_DEBUG_REPORT_SPEC_VERSION
    }
};

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceExtensionProperties(
        const char *pLayerName,
        uint32_t *pCount,
        VkExtensionProperties* pProperties)
{
    return util_GetExtensionProperties(ARRAY_SIZE(instance_extensions),
                                       instance_extensions, pCount,
                                       pProperties);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL
vkEnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice,
                                     const char *pLayerName, uint32_t *pCount,
                                     VkExtensionProperties *pProperties) {
    if (pLayerName == NULL) {
        dispatch_key key = get_dispatch_key(physicalDevice);
        layer_data *my_data = get_my_data_ptr(key, layer_data_map);
        return my_data->instance_dispatch_table
            ->EnumerateDeviceExtensionProperties(physicalDevice, NULL, pCount,
                                                 pProperties);
    } else {
        return util_GetExtensionProperties(0, nullptr, pCount, pProperties);
    }
}

static const VkLayerProperties swapchain_layers[] = {{
    "VK_LAYER_LUNARG_swapchain", VK_API_VERSION, 1, "LunarG Validation Layer",
}};

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceLayerProperties(
        uint32_t *pCount,
        VkLayerProperties*    pProperties)
{
    return util_GetLayerProperties(ARRAY_SIZE(swapchain_layers),
                                   swapchain_layers, pCount, pProperties);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceLayerProperties(
    VkPhysicalDevice physicalDevice, uint32_t *pCount,
    VkLayerProperties *pProperties) {
    return util_GetLayerProperties(ARRAY_SIZE(swapchain_layers),
                                   swapchain_layers, pCount, pProperties);
}

static void createDeviceRegisterExtensions(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo* pCreateInfo, VkDevice device)
{
    uint32_t i;
    layer_data *my_device_data   = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    layer_data *my_instance_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);

    VkLayerDispatchTable *pDisp  = my_device_data->device_dispatch_table;
    PFN_vkGetDeviceProcAddr gpa  = pDisp->GetDeviceProcAddr;

    pDisp->CreateSwapchainKHR    = (PFN_vkCreateSwapchainKHR) gpa(device, "vkCreateSwapchainKHR");
    pDisp->DestroySwapchainKHR   = (PFN_vkDestroySwapchainKHR) gpa(device, "vkDestroySwapchainKHR");
    pDisp->GetSwapchainImagesKHR = (PFN_vkGetSwapchainImagesKHR) gpa(device, "vkGetSwapchainImagesKHR");
    pDisp->AcquireNextImageKHR   = (PFN_vkAcquireNextImageKHR) gpa(device, "vkAcquireNextImageKHR");
    pDisp->QueuePresentKHR       = (PFN_vkQueuePresentKHR) gpa(device, "vkQueuePresentKHR");
    pDisp->GetDeviceQueue        = (PFN_vkGetDeviceQueue) gpa(device, "vkGetDeviceQueue");

    SwpPhysicalDevice *pPhysicalDevice = &my_instance_data->physicalDeviceMap[physicalDevice];
    if (pPhysicalDevice) {
        my_device_data->deviceMap[device].pPhysicalDevice = pPhysicalDevice;
        pPhysicalDevice->pDevice = &my_device_data->deviceMap[device];
    } else {
        // TBD: Should we leave error in (since Swapchain really needs this
        // link)?
        log_msg(my_instance_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT,
                (uint64_t)physicalDevice , __LINE__, SWAPCHAIN_INVALID_HANDLE, "Swapchain",
                "vkCreateDevice() called with a non-valid VkPhysicalDevice.");
    }
    my_device_data->deviceMap[device].device = device;
    my_device_data->deviceMap[device].swapchainExtensionEnabled = false;

    // Record whether the WSI device extension was enabled for this VkDevice.
    // No need to check if the extension was advertised by
    // vkEnumerateDeviceExtensionProperties(), since the loader handles that.
    for (i = 0; i < pCreateInfo->enabledExtensionCount; i++) {
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {

            my_device_data->deviceMap[device].swapchainExtensionEnabled = true;
        }
    }
}

static void createInstanceRegisterExtensions(const VkInstanceCreateInfo* pCreateInfo, VkInstance instance)
{
    uint32_t i;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    VkLayerInstanceDispatchTable *pDisp  = my_data->instance_dispatch_table;
    PFN_vkGetInstanceProcAddr gpa = pDisp->GetInstanceProcAddr;
#ifdef VK_USE_PLATFORM_ANDROID_KHR
    pDisp->CreateAndroidSurfaceKHR = (PFN_vkCreateAndroidSurfaceKHR) gpa(instance, "vkCreateAndroidSurfaceKHR");
#endif // VK_USE_PLATFORM_ANDROID_KHR
#ifdef VK_USE_PLATFORM_MIR_KHR
    pDisp->CreateMirSurfaceKHR = (PFN_vkCreateMirSurfaceKHR) gpa(instance, "vkCreateMirSurfaceKHR");
    pDisp->GetPhysicalDeviceMirPresentationSupportKHR = (PFN_vkGetPhysicalDeviceMirPresentationSupportKHR) gpa(instance, "vkGetPhysicalDeviceMirPresentationSupportKHR");
#endif // VK_USE_PLATFORM_MIR_KHR
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
    pDisp->CreateWaylandSurfaceKHR = (PFN_vkCreateWaylandSurfaceKHR) gpa(instance, "vkCreateWaylandSurfaceKHR");
    pDisp->GetPhysicalDeviceWaylandPresentationSupportKHR = (PFN_vkGetPhysicalDeviceWaylandPresentationSupportKHR) gpa(instance, "vkGetPhysicalDeviceWaylandPresentationSupportKHR");
#endif // VK_USE_PLATFORM_WAYLAND_KHR
#ifdef VK_USE_PLATFORM_WIN32_KHR
    pDisp->CreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR) gpa(instance, "vkCreateWin32SurfaceKHR");
    pDisp->GetPhysicalDeviceWin32PresentationSupportKHR = (PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR) gpa(instance, "vkGetPhysicalDeviceWin32PresentationSupportKHR");
#endif // VK_USE_PLATFORM_WIN32_KHR
#ifdef VK_USE_PLATFORM_XCB_KHR
    pDisp->CreateXcbSurfaceKHR = (PFN_vkCreateXcbSurfaceKHR) gpa(instance, "vkCreateXcbSurfaceKHR");
    pDisp->GetPhysicalDeviceXcbPresentationSupportKHR = (PFN_vkGetPhysicalDeviceXcbPresentationSupportKHR) gpa(instance, "vkGetPhysicalDeviceXcbPresentationSupportKHR");
#endif // VK_USE_PLATFORM_XCB_KHR
#ifdef VK_USE_PLATFORM_XLIB_KHR
    pDisp->CreateXlibSurfaceKHR = (PFN_vkCreateXlibSurfaceKHR) gpa(instance, "vkCreateXlibSurfaceKHR");
    pDisp->GetPhysicalDeviceXlibPresentationSupportKHR = (PFN_vkGetPhysicalDeviceXlibPresentationSupportKHR) gpa(instance, "vkGetPhysicalDeviceXlibPresentationSupportKHR");
#endif // VK_USE_PLATFORM_XLIB_KHR
    pDisp->DestroySurfaceKHR = (PFN_vkDestroySurfaceKHR) gpa(instance, "vkDestroySurfaceKHR");
    pDisp->GetPhysicalDeviceSurfaceSupportKHR = (PFN_vkGetPhysicalDeviceSurfaceSupportKHR) gpa(instance, "vkGetPhysicalDeviceSurfaceSupportKHR");
    pDisp->GetPhysicalDeviceSurfaceCapabilitiesKHR = (PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR) gpa(instance, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
    pDisp->GetPhysicalDeviceSurfaceFormatsKHR = (PFN_vkGetPhysicalDeviceSurfaceFormatsKHR) gpa(instance, "vkGetPhysicalDeviceSurfaceFormatsKHR");
    pDisp->GetPhysicalDeviceSurfacePresentModesKHR = (PFN_vkGetPhysicalDeviceSurfacePresentModesKHR) gpa(instance, "vkGetPhysicalDeviceSurfacePresentModesKHR");

    // Remember this instance, and whether the VK_KHR_surface extension
    // was enabled for it:
    my_data->instanceMap[instance].instance = instance;
    my_data->instanceMap[instance].surfaceExtensionEnabled = false;
#ifdef VK_USE_PLATFORM_ANDROID_KHR
    my_data->instanceMap[instance].androidSurfaceExtensionEnabled = false;
#endif // VK_USE_PLATFORM_ANDROID_KHR
#ifdef VK_USE_PLATFORM_MIR_KHR
    my_data->instanceMap[instance].mirSurfaceExtensionEnabled = false;
#endif // VK_USE_PLATFORM_MIR_KHR
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
    my_data->instanceMap[instance].waylandSurfaceExtensionEnabled = false;
#endif // VK_USE_PLATFORM_WAYLAND_KHR
#ifdef VK_USE_PLATFORM_WIN32_KHR
    my_data->instanceMap[instance].win32SurfaceExtensionEnabled = false;
#endif // VK_USE_PLATFORM_WIN32_KHR
#ifdef VK_USE_PLATFORM_XCB_KHR
    my_data->instanceMap[instance].xcbSurfaceExtensionEnabled = false;
#endif // VK_USE_PLATFORM_XCB_KHR
#ifdef VK_USE_PLATFORM_XLIB_KHR
    my_data->instanceMap[instance].xlibSurfaceExtensionEnabled = false;
#endif // VK_USE_PLATFORM_XLIB_KHR


    // Record whether the WSI instance extension was enabled for this
    // VkInstance.  No need to check if the extension was advertised by
    // vkEnumerateInstanceExtensionProperties(), since the loader handles that.
    for (i = 0; i < pCreateInfo->enabledExtensionCount; i++) {
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_SURFACE_EXTENSION_NAME) == 0) {

            my_data->instanceMap[instance].surfaceExtensionEnabled = true;
        }
#ifdef VK_USE_PLATFORM_ANDROID_KHR
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_ANDROID_SURFACE_EXTENSION_NAME) == 0) {

            my_data->instanceMap[instance].androidSurfaceExtensionEnabled = true;
#endif // VK_USE_PLATFORM_ANDROID_KHR
#ifdef VK_USE_PLATFORM_MIR_KHR
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_MIR_SURFACE_EXTENSION_NAME) == 0) {

            my_data->instanceMap[instance].mirSurfaceExtensionEnabled = true;
#endif // VK_USE_PLATFORM_MIR_KHR
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME) == 0) {

            my_data->instanceMap[instance].waylandSurfaceExtensionEnabled = true;
#endif // VK_USE_PLATFORM_WAYLAND_KHR
#ifdef VK_USE_PLATFORM_WIN32_KHR
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_WIN32_SURFACE_EXTENSION_NAME) == 0) {

            my_data->instanceMap[instance].win32SurfaceExtensionEnabled = true;
#endif // VK_USE_PLATFORM_WIN32_KHR
#ifdef VK_USE_PLATFORM_XCB_KHR
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_XCB_SURFACE_EXTENSION_NAME) == 0) {

            my_data->instanceMap[instance].xcbSurfaceExtensionEnabled = true;
#endif // VK_USE_PLATFORM_XCB_KHR
#ifdef VK_USE_PLATFORM_XLIB_KHR
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_XLIB_SURFACE_EXTENSION_NAME) == 0) {

            my_data->instanceMap[instance].xlibSurfaceExtensionEnabled = true;
#endif // VK_USE_PLATFORM_XLIB_KHR
        }
    }
}


#include "vk_dispatch_table_helper.h"
static void initSwapchain(layer_data *my_data, const VkAllocationCallbacks *pAllocator)
{
    uint32_t report_flags = 0;
    uint32_t debug_action = 0;
    FILE *log_output = NULL;
    const char *option_str;
    VkDebugReportCallbackEXT callback;

    // Initialize Swapchain options:
    report_flags = getLayerOptionFlags("SwapchainReportFlags", 0);
    getLayerOptionEnum("SwapchainDebugAction", (uint32_t *) &debug_action);

    if (debug_action & VK_DBG_LAYER_ACTION_LOG_MSG)
    {
        // Turn on logging, since it was requested:
        option_str = getLayerOption("SwapchainLogFilename");
        log_output = getLayerLogOutput(option_str, "Swapchain");
        VkDebugReportCallbackCreateInfoEXT dbgInfo;
        memset(&dbgInfo, 0, sizeof(dbgInfo));
        dbgInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
        dbgInfo.pfnCallback = log_callback;
        dbgInfo.pUserData = log_output;
        dbgInfo.flags = report_flags;
        layer_create_msg_callback(my_data->report_data,
                                  &dbgInfo,
                                  pAllocator,
                                  &callback);
        my_data->logging_callback.push_back(callback);
    }
    if (debug_action & VK_DBG_LAYER_ACTION_DEBUG_OUTPUT) {
        VkDebugReportCallbackCreateInfoEXT dbgInfo;
        memset(&dbgInfo, 0, sizeof(dbgInfo));
        dbgInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
        dbgInfo.pfnCallback = win32_debug_output_msg;
        dbgInfo.pUserData = log_output;
        dbgInfo.flags = report_flags;
        layer_create_msg_callback(my_data->report_data, &dbgInfo, pAllocator, &callback);
        my_data->logging_callback.push_back(callback);
    }
    if (!globalLockInitialized)
    {
        loader_platform_thread_create_mutex(&globalLock);
        globalLockInitialized = 1;
    }
}

static const char *surfaceTransformStr(VkSurfaceTransformFlagBitsKHR value)
{
    // Return a string corresponding to the value:
    return string_VkSurfaceTransformFlagBitsKHR(value);
}

static const char *surfaceCompositeAlphaStr(VkCompositeAlphaFlagBitsKHR value)
{
    // Return a string corresponding to the value:
    return string_VkCompositeAlphaFlagBitsKHR(value);
}

static const char *presentModeStr(VkPresentModeKHR value)
{
    // Return a string corresponding to the value:
    return string_VkPresentModeKHR(value);
}

static const char *sharingModeStr(VkSharingMode value)
{
    // Return a string corresponding to the value:
    return string_VkSharingMode(value);
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
    if (result != VK_SUCCESS) {
        return result;
    }

    layer_data *my_data = get_my_data_ptr(get_dispatch_key(*pInstance), layer_data_map);
    my_data->instance_dispatch_table = new VkLayerInstanceDispatchTable;
    layer_init_instance_dispatch_table(*pInstance, my_data->instance_dispatch_table, fpGetInstanceProcAddr);

    my_data->report_data = debug_report_create_instance(
                               my_data->instance_dispatch_table,
                               *pInstance,
                               pCreateInfo->enabledExtensionCount,
                               pCreateInfo->ppEnabledExtensionNames);

    // Call the following function after my_data is initialized:
    createInstanceRegisterExtensions(pCreateInfo, *pInstance);
    initSwapchain(my_data, pAllocator);

    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyInstance(VkInstance instance, const VkAllocationCallbacks* pAllocator)
{
    dispatch_key key = get_dispatch_key(instance);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    SwpInstance *pInstance = &(my_data->instanceMap[instance]);

    // Call down the call chain:
    my_data->instance_dispatch_table->DestroyInstance(instance, pAllocator);

    loader_platform_thread_lock_mutex(&globalLock);

    // Do additional internal cleanup:
    if (pInstance) {
        // Delete all of the SwpPhysicalDevice's, SwpSurface's, and the
        // SwpInstance associated with this instance:
        for (auto it = pInstance->physicalDevices.begin() ;
             it != pInstance->physicalDevices.end() ; it++) {

            // Free memory that was allocated for/by this SwpPhysicalDevice:
            SwpPhysicalDevice *pPhysicalDevice = it->second;
            if (pPhysicalDevice) {
                if (pPhysicalDevice->pDevice) {
                    LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, instance, "VkInstance",
                              SWAPCHAIN_DEL_OBJECT_BEFORE_CHILDREN,
                              "%s() called before all of its associated "
                              "VkDevices were destroyed.",
                              __FUNCTION__);
                }
                free(pPhysicalDevice->pSurfaceFormats);
                free(pPhysicalDevice->pPresentModes);
            }

            // Erase the SwpPhysicalDevice's from the my_data->physicalDeviceMap (which
            // are simply pointed to by the SwpInstance):
            my_data->physicalDeviceMap.erase(it->second->physicalDevice);
        }
        for (auto it = pInstance->surfaces.begin() ;
             it != pInstance->surfaces.end() ; it++) {

            // Free memory that was allocated for/by this SwpPhysicalDevice:
            SwpSurface *pSurface = it->second;
            if (pSurface) {
                LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, instance, "VkInstance",
                          SWAPCHAIN_DEL_OBJECT_BEFORE_CHILDREN,
                          "%s() called before all of its associated "
                          "VkSurfaceKHRs were destroyed.",
                          __FUNCTION__);
            }
        }
        my_data->instanceMap.erase(instance);
    }

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
        // Release mutex when destroying last instance
        loader_platform_thread_unlock_mutex(&globalLock);
        loader_platform_thread_delete_mutex(&globalLock);
        globalLockInitialized = 0;
    } else {
        loader_platform_thread_unlock_mutex(&globalLock);
    }
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceQueueFamilyProperties(
    VkPhysicalDevice physicalDevice,
    uint32_t* pQueueFamilyPropertyCount,
    VkQueueFamilyProperties* pQueueFamilyProperties)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);

    // Call down the call chain:
    my_data->instance_dispatch_table->GetPhysicalDeviceQueueFamilyProperties(
            physicalDevice,
            pQueueFamilyPropertyCount,
            pQueueFamilyProperties);

    // Record the result of this query:
    loader_platform_thread_lock_mutex(&globalLock);
    SwpPhysicalDevice *pPhysicalDevice = &my_data->physicalDeviceMap[physicalDevice];
    if (pPhysicalDevice &&
        pQueueFamilyPropertyCount && !pQueueFamilyProperties) {
        pPhysicalDevice->gotQueueFamilyPropertyCount = true;
        pPhysicalDevice->numOfQueueFamilies =
            *pQueueFamilyPropertyCount;
    }
    loader_platform_thread_unlock_mutex(&globalLock);
}

#ifdef VK_USE_PLATFORM_ANDROID_KHR
VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateAndroidSurfaceKHR(
    VkInstance                                  instance,
    const VkAndroidSurfaceCreateInfoKHR*        pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkSurfaceKHR*                               pSurface)
{
    VkResult result = VK_SUCCESS;
    VkBool32 skipCall = VK_FALSE;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    SwpInstance *pInstance = &(my_data->instanceMap[instance]);

    // Validate that the platform extension was enabled:
    if (pInstance && !pInstance->androidSurfaceExtensionEnabled) {
        skipCall |= LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT,
                              pInstance,
                              "VkInstance",
                              SWAPCHAIN_EXT_NOT_ENABLED_BUT_USED,
                              "%s() called even though the %s extension was not enabled for this VkInstance.",
                              __FUNCTION__, VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
    }

    if (!pCreateInfo) {
        skipCall |= LOG_ERROR_NULL_POINTER(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                                           device,
                                           "pCreateInfo");
    } else {
        if (pCreateInfo->sType != VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR) {
            skipCall |= LOG_ERROR_WRONG_STYPE(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                                              device,
                                              "pCreateInfo",
                                              "VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR");
        }
        if (pCreateInfo->pNext != NULL) {
            skipCall |= LOG_INFO_WRONG_NEXT(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                                             device,
                                             "pCreateInfo");
        }
    }

    if (VK_FALSE == skipCall) {
        // Call down the call chain:
        loader_platform_thread_unlock_mutex(&globalLock);
        result = my_data->instance_dispatch_table->CreateAndroidSurfaceKHR(
                instance, pCreateInfo, pAllocator, pSurface);
        loader_platform_thread_lock_mutex(&globalLock);

        // Obtain this pointer again after locking:
        pInstance = &(my_data->instanceMap[instance]);
        if ((result == VK_SUCCESS) && pInstance && pSurface) {
            // Record the VkSurfaceKHR returned by the ICD:
            my_data->surfaceMap[*pSurface].surface = *pSurface;
            my_data->surfaceMap[*pSurface].pInstance = pInstance;
            my_data->surfaceMap[*pSurface].usedAllocatorToCreate =
                (pAllocator != NULL);
            my_data->surfaceMap[*pSurface].numQueueFamilyIndexSupport = 0;
            my_data->surfaceMap[*pSurface].pQueueFamilyIndexSupport = NULL;
            // Point to the associated SwpInstance:
            pInstance->surfaces[*pSurface] = &my_data->surfaceMap[*pSurface];
        }
        loader_platform_thread_unlock_mutex(&globalLock);
        return result;
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    return VK_ERROR_VALIDATION_FAILED_EXT;
}
#endif // VK_USE_PLATFORM_ANDROID_KHR

#ifdef VK_USE_PLATFORM_MIR_KHR
VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateMirSurfaceKHR(
    VkInstance                                  instance,
    const VkMirSurfaceCreateInfoKHR*            pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkSurfaceKHR*                               pSurface)
{
    VkResult result = VK_SUCCESS;
    VkBool32 skipCall = VK_FALSE;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    SwpInstance *pInstance = &(my_data->instanceMap[instance]);

    // Validate that the platform extension was enabled:
    if (pInstance && !pInstance->mirSurfaceExtensionEnabled) {
        skipCall |= LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT,
                              pInstance,
                              "VkInstance",
                              SWAPCHAIN_EXT_NOT_ENABLED_BUT_USED,
                              "%s() called even though the %s extension was not enabled for this VkInstance.",
                              __FUNCTION__, VK_KHR_MIR_SURFACE_EXTENSION_NAME);
    }

    if (!pCreateInfo) {
        skipCall |= LOG_ERROR_NULL_POINTER(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                                           device,
                                           "pCreateInfo");
    } else {
        if (pCreateInfo->sType != VK_STRUCTURE_TYPE_MIR_SURFACE_CREATE_INFO_KHR) {
            skipCall |= LOG_ERROR_WRONG_STYPE(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                                              device,
                                              "pCreateInfo",
                                              "VK_STRUCTURE_TYPE_MIR_SURFACE_CREATE_INFO_KHR");
        }
        if (pCreateInfo->pNext != NULL) {
            skipCall |= LOG_INFO_WRONG_NEXT(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                                             device,
                                             "pCreateInfo");
        }
    }

    if (VK_FALSE == skipCall) {
        // Call down the call chain:
        loader_platform_thread_unlock_mutex(&globalLock);
        result = my_data->instance_dispatch_table->CreateMirSurfaceKHR(
                instance, pCreateInfo, pAllocator, pSurface);
        loader_platform_thread_lock_mutex(&globalLock);

        // Obtain this pointer again after locking:
        pInstance = &(my_data->instanceMap[instance]);
        if ((result == VK_SUCCESS) && pInstance && pSurface) {
            // Record the VkSurfaceKHR returned by the ICD:
            my_data->surfaceMap[*pSurface].surface = *pSurface;
            my_data->surfaceMap[*pSurface].pInstance = pInstance;
            my_data->surfaceMap[*pSurface].usedAllocatorToCreate =
                (pAllocator != NULL);
            my_data->surfaceMap[*pSurface].numQueueFamilyIndexSupport = 0;
            my_data->surfaceMap[*pSurface].pQueueFamilyIndexSupport = NULL;
            // Point to the associated SwpInstance:
            pInstance->surfaces[*pSurface] = &my_data->surfaceMap[*pSurface];
        }
        loader_platform_thread_unlock_mutex(&globalLock);
        return result;
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    return VK_ERROR_VALIDATION_FAILED_EXT;
}

VK_LAYER_EXPORT VKAPI_ATTR VkBool32 VKAPI_CALL vkGetPhysicalDeviceMirPresentationSupportKHR(
    VkPhysicalDevice physicalDevice,
    uint32_t queueFamilyIndex,
    MirConnection* connection)
{
    VkBool32 result = VK_FALSE;
    VkBool32 skipCall = VK_FALSE;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    SwpPhysicalDevice *pPhysicalDevice = &my_data->physicalDeviceMap[physicalDevice];

    // Validate that the platform extension was enabled:
    if (pPhysicalDevice && pPhysicalDevice->pInstance &&
        !pPhysicalDevice->pInstance->mirSurfaceExtensionEnabled) {
        skipCall |= LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT,
                              pPhysicalDevice->pInstance,
                              "VkInstance",
                              SWAPCHAIN_EXT_NOT_ENABLED_BUT_USED,
                              "%s() called even though the %s extension was not enabled for this VkInstance.",
                              __FUNCTION__, VK_KHR_MIR_SURFACE_EXTENSION_NAME);
    }
    if (pPhysicalDevice->gotQueueFamilyPropertyCount &&
        (queueFamilyIndex >= pPhysicalDevice->numOfQueueFamilies)) {
        skipCall |= LOG_ERROR_QUEUE_FAMILY_INDEX_TOO_LARGE(VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT,
                                                           pPhysicalDevice,
                                                           "VkPhysicalDevice",
                                                           queueFamilyIndex,
                                                           pPhysicalDevice->numOfQueueFamilies);
    }
    loader_platform_thread_unlock_mutex(&globalLock);

    if (VK_FALSE == skipCall) {
        // Call down the call chain:
        result = my_data->instance_dispatch_table->GetPhysicalDeviceMirPresentationSupportKHR(
                physicalDevice, queueFamilyIndex, connection);
    }
    return result;
}
#endif // VK_USE_PLATFORM_MIR_KHR

#ifdef VK_USE_PLATFORM_WAYLAND_KHR
VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateWaylandSurfaceKHR(
    VkInstance                                  instance,
    const VkWaylandSurfaceCreateInfoKHR*        pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkSurfaceKHR*                               pSurface)
{
    VkResult result = VK_SUCCESS;
    VkBool32 skipCall = VK_FALSE;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    SwpInstance *pInstance = &(my_data->instanceMap[instance]);

    // Validate that the platform extension was enabled:
    if (pInstance && !pInstance->waylandSurfaceExtensionEnabled) {
        skipCall |= LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT,
                              pInstance,
                              "VkInstance",
                              SWAPCHAIN_EXT_NOT_ENABLED_BUT_USED,
                              "%s() called even though the %s extension was not enabled for this VkInstance.",
                              __FUNCTION__, VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
    }

    if (!pCreateInfo) {
        skipCall |= LOG_ERROR_NULL_POINTER(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                                           device,
                                           "pCreateInfo");
    } else {
        if (pCreateInfo->sType != VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR) {
            skipCall |= LOG_ERROR_WRONG_STYPE(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                                              device,
                                              "pCreateInfo",
                                              "VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR");
        }
        if (pCreateInfo->pNext != NULL) {
            skipCall |= LOG_INFO_WRONG_NEXT(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                                             device,
                                             "pCreateInfo");
        }
    }

    if (VK_FALSE == skipCall) {
        // Call down the call chain:
        loader_platform_thread_unlock_mutex(&globalLock);
        result = my_data->instance_dispatch_table->CreateWaylandSurfaceKHR(
                instance, pCreateInfo, pAllocator, pSurface);
        loader_platform_thread_lock_mutex(&globalLock);

        // Obtain this pointer again after locking:
        pInstance = &(my_data->instanceMap[instance]);
        if ((result == VK_SUCCESS) && pInstance && pSurface) {
            // Record the VkSurfaceKHR returned by the ICD:
            my_data->surfaceMap[*pSurface].surface = *pSurface;
            my_data->surfaceMap[*pSurface].pInstance = pInstance;
            my_data->surfaceMap[*pSurface].usedAllocatorToCreate =
                (pAllocator != NULL);
            my_data->surfaceMap[*pSurface].numQueueFamilyIndexSupport = 0;
            my_data->surfaceMap[*pSurface].pQueueFamilyIndexSupport = NULL;
            // Point to the associated SwpInstance:
            pInstance->surfaces[*pSurface] = &my_data->surfaceMap[*pSurface];
        }
        loader_platform_thread_unlock_mutex(&globalLock);
        return result;
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    return VK_ERROR_VALIDATION_FAILED_EXT;
}

VK_LAYER_EXPORT VKAPI_ATTR VkBool32 VKAPI_CALL vkGetPhysicalDeviceWaylandPresentationSupportKHR(
    VkPhysicalDevice physicalDevice,
    uint32_t queueFamilyIndex,
    struct wl_display* display)
{
    VkBool32 result = VK_FALSE;
    VkBool32 skipCall = VK_FALSE;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    SwpPhysicalDevice *pPhysicalDevice = &my_data->physicalDeviceMap[physicalDevice];

    // Validate that the platform extension was enabled:
    if (pPhysicalDevice && pPhysicalDevice->pInstance &&
        !pPhysicalDevice->pInstance->waylandSurfaceExtensionEnabled) {
        skipCall |= LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT,
                              pPhysicalDevice->pInstance,
                              "VkInstance",
                              SWAPCHAIN_EXT_NOT_ENABLED_BUT_USED,
                              "%s() called even though the %s extension was not enabled for this VkInstance.",
                              __FUNCTION__, VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
    }
    if (pPhysicalDevice->gotQueueFamilyPropertyCount &&
        (queueFamilyIndex >= pPhysicalDevice->numOfQueueFamilies)) {
        skipCall |= LOG_ERROR_QUEUE_FAMILY_INDEX_TOO_LARGE(VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT,
                                                           pPhysicalDevice,
                                                           "VkPhysicalDevice",
                                                           queueFamilyIndex,
                                                           pPhysicalDevice->numOfQueueFamilies);
    }
    loader_platform_thread_unlock_mutex(&globalLock);

    if (VK_FALSE == skipCall) {
        // Call down the call chain:
        result = my_data->instance_dispatch_table->GetPhysicalDeviceWaylandPresentationSupportKHR(
                physicalDevice, queueFamilyIndex, display);
    }
    return result;
}
#endif // VK_USE_PLATFORM_WAYLAND_KHR

#ifdef VK_USE_PLATFORM_WIN32_KHR
VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateWin32SurfaceKHR(
    VkInstance                                  instance,
    const VkWin32SurfaceCreateInfoKHR*          pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkSurfaceKHR*                               pSurface)
{
    VkResult result = VK_SUCCESS;
    VkBool32 skipCall = VK_FALSE;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    SwpInstance *pInstance = &(my_data->instanceMap[instance]);

    // Validate that the platform extension was enabled:
    if (pInstance && !pInstance->win32SurfaceExtensionEnabled) {
        skipCall |= LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT,
                              pInstance,
                              "VkInstance",
                              SWAPCHAIN_EXT_NOT_ENABLED_BUT_USED,
                              "%s() called even though the %s extension was not enabled for this VkInstance.",
                              __FUNCTION__, VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
    }

    if (!pCreateInfo) {
        skipCall |= LOG_ERROR_NULL_POINTER(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                                           device,
                                           "pCreateInfo");
    } else {
        if (pCreateInfo->sType != VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR) {
            skipCall |= LOG_ERROR_WRONG_STYPE(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                                              device,
                                              "pCreateInfo",
                                              "VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR");
        }
        if (pCreateInfo->pNext != NULL) {
            skipCall |= LOG_INFO_WRONG_NEXT(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                                             device,
                                             "pCreateInfo");
        }
    }

    if (VK_FALSE == skipCall) {
        // Call down the call chain:
        loader_platform_thread_unlock_mutex(&globalLock);
        result = my_data->instance_dispatch_table->CreateWin32SurfaceKHR(
                instance, pCreateInfo, pAllocator, pSurface);
        loader_platform_thread_lock_mutex(&globalLock);

        // Obtain this pointer again after locking:
        pInstance = &(my_data->instanceMap[instance]);
        if ((result == VK_SUCCESS) && pInstance && pSurface) {
            // Record the VkSurfaceKHR returned by the ICD:
            my_data->surfaceMap[*pSurface].surface = *pSurface;
            my_data->surfaceMap[*pSurface].pInstance = pInstance;
            my_data->surfaceMap[*pSurface].usedAllocatorToCreate =
                (pAllocator != NULL);
            my_data->surfaceMap[*pSurface].numQueueFamilyIndexSupport = 0;
            my_data->surfaceMap[*pSurface].pQueueFamilyIndexSupport = NULL;
            // Point to the associated SwpInstance:
            pInstance->surfaces[*pSurface] = &my_data->surfaceMap[*pSurface];
        }
        loader_platform_thread_unlock_mutex(&globalLock);
        return result;
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    return VK_ERROR_VALIDATION_FAILED_EXT;
}

VK_LAYER_EXPORT VKAPI_ATTR VkBool32 VKAPI_CALL vkGetPhysicalDeviceWin32PresentationSupportKHR(
    VkPhysicalDevice physicalDevice,
    uint32_t queueFamilyIndex)
{
    VkBool32 result = VK_FALSE;
    VkBool32 skipCall = VK_FALSE;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    SwpPhysicalDevice *pPhysicalDevice = &my_data->physicalDeviceMap[physicalDevice];

    // Validate that the platform extension was enabled:
    if (pPhysicalDevice && pPhysicalDevice->pInstance &&
        !pPhysicalDevice->pInstance->win32SurfaceExtensionEnabled) {
        skipCall |= LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT,
                              pPhysicalDevice->pInstance,
                              "VkInstance",
                              SWAPCHAIN_EXT_NOT_ENABLED_BUT_USED,
                              "%s() called even though the %s extension was not enabled for this VkInstance.",
                              __FUNCTION__, VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
    }
    if (pPhysicalDevice->gotQueueFamilyPropertyCount &&
        (queueFamilyIndex >= pPhysicalDevice->numOfQueueFamilies)) {
        skipCall |= LOG_ERROR_QUEUE_FAMILY_INDEX_TOO_LARGE(VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT,
                                                           pPhysicalDevice,
                                                           "VkPhysicalDevice",
                                                           queueFamilyIndex,
                                                           pPhysicalDevice->numOfQueueFamilies);
    }
    loader_platform_thread_unlock_mutex(&globalLock);

    if (VK_FALSE == skipCall) {
        // Call down the call chain:
        result = my_data->instance_dispatch_table->GetPhysicalDeviceWin32PresentationSupportKHR(
                physicalDevice, queueFamilyIndex);
    }
    return result;
}
#endif // VK_USE_PLATFORM_WIN32_KHR

#ifdef VK_USE_PLATFORM_XCB_KHR
VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateXcbSurfaceKHR(
    VkInstance                                  instance,
    const VkXcbSurfaceCreateInfoKHR*            pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkSurfaceKHR*                               pSurface)
{
    VkResult result = VK_SUCCESS;
    VkBool32 skipCall = VK_FALSE;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    SwpInstance *pInstance = &(my_data->instanceMap[instance]);

    // Validate that the platform extension was enabled:
    if (pInstance && !pInstance->xcbSurfaceExtensionEnabled) {
        skipCall |= LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT,
                              pInstance,
                              "VkInstance",
                              SWAPCHAIN_EXT_NOT_ENABLED_BUT_USED,
                              "%s() called even though the %s extension was not enabled for this VkInstance.",
                              __FUNCTION__, VK_KHR_XCB_SURFACE_EXTENSION_NAME);
    }

    if (!pCreateInfo) {
        skipCall |= LOG_ERROR_NULL_POINTER(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                                           device,
                                           "pCreateInfo");
    } else {
        if (pCreateInfo->sType != VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR) {
            skipCall |= LOG_ERROR_WRONG_STYPE(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                                              device,
                                              "pCreateInfo",
                                              "VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR");
        }
        if (pCreateInfo->pNext != NULL) {
            skipCall |= LOG_INFO_WRONG_NEXT(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                                             device,
                                             "pCreateInfo");
        }
    }

    if (VK_FALSE == skipCall) {
        // Call down the call chain:
        loader_platform_thread_unlock_mutex(&globalLock);
        result = my_data->instance_dispatch_table->CreateXcbSurfaceKHR(
                instance, pCreateInfo, pAllocator, pSurface);
        loader_platform_thread_lock_mutex(&globalLock);

        // Obtain this pointer again after locking:
        pInstance = &(my_data->instanceMap[instance]);
        if ((result == VK_SUCCESS) && pInstance && pSurface) {
            // Record the VkSurfaceKHR returned by the ICD:
            my_data->surfaceMap[*pSurface].surface = *pSurface;
            my_data->surfaceMap[*pSurface].pInstance = pInstance;
            my_data->surfaceMap[*pSurface].usedAllocatorToCreate =
                (pAllocator != NULL);
            my_data->surfaceMap[*pSurface].numQueueFamilyIndexSupport = 0;
            my_data->surfaceMap[*pSurface].pQueueFamilyIndexSupport = NULL;
            // Point to the associated SwpInstance:
            pInstance->surfaces[*pSurface] = &my_data->surfaceMap[*pSurface];
        }
        loader_platform_thread_unlock_mutex(&globalLock);
        return result;
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    return VK_ERROR_VALIDATION_FAILED_EXT;
}

VK_LAYER_EXPORT VKAPI_ATTR VkBool32 VKAPI_CALL vkGetPhysicalDeviceXcbPresentationSupportKHR(
    VkPhysicalDevice physicalDevice,
    uint32_t queueFamilyIndex,
    xcb_connection_t* connection,
    xcb_visualid_t visual_id)
{
    VkBool32 result = VK_FALSE;
    VkBool32 skipCall = VK_FALSE;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    SwpPhysicalDevice *pPhysicalDevice = &my_data->physicalDeviceMap[physicalDevice];

    // Validate that the platform extension was enabled:
    if (pPhysicalDevice && pPhysicalDevice->pInstance &&
        !pPhysicalDevice->pInstance->xcbSurfaceExtensionEnabled) {
        skipCall |= LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT,
                              pPhysicalDevice->pInstance,
                              "VkInstance",
                              SWAPCHAIN_EXT_NOT_ENABLED_BUT_USED,
                              "%s() called even though the %s extension was not enabled for this VkInstance.",
                              __FUNCTION__, VK_KHR_XCB_SURFACE_EXTENSION_NAME);
    }
    if (pPhysicalDevice->gotQueueFamilyPropertyCount &&
        (queueFamilyIndex >= pPhysicalDevice->numOfQueueFamilies)) {
        skipCall |= LOG_ERROR_QUEUE_FAMILY_INDEX_TOO_LARGE(VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT,
                                                           pPhysicalDevice,
                                                           "VkPhysicalDevice",
                                                           queueFamilyIndex,
                                                           pPhysicalDevice->numOfQueueFamilies);
    }
    loader_platform_thread_unlock_mutex(&globalLock);

    if (VK_FALSE == skipCall) {
        // Call down the call chain:
        result = my_data->instance_dispatch_table->GetPhysicalDeviceXcbPresentationSupportKHR(
                physicalDevice, queueFamilyIndex, connection, visual_id);
    }
    return result;
}
#endif // VK_USE_PLATFORM_XCB_KHR

#ifdef VK_USE_PLATFORM_XLIB_KHR
VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateXlibSurfaceKHR(
    VkInstance                                  instance,
    const VkXlibSurfaceCreateInfoKHR*           pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkSurfaceKHR*                               pSurface)
{
    VkResult result = VK_SUCCESS;
    VkBool32 skipCall = VK_FALSE;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    SwpInstance *pInstance = &(my_data->instanceMap[instance]);

    // Validate that the platform extension was enabled:
    if (pInstance && !pInstance->xlibSurfaceExtensionEnabled) {
        skipCall |= LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT,
                              pInstance,
                              "VkInstance",
                              SWAPCHAIN_EXT_NOT_ENABLED_BUT_USED,
                              "%s() called even though the %s extension was not enabled for this VkInstance.",
                              __FUNCTION__, VK_KHR_XLIB_SURFACE_EXTENSION_NAME);
    }

    if (!pCreateInfo) {
        skipCall |= LOG_ERROR_NULL_POINTER(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                                           device,
                                           "pCreateInfo");
    } else {
        if (pCreateInfo->sType != VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR) {
            skipCall |= LOG_ERROR_WRONG_STYPE(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                                              device,
                                              "pCreateInfo",
                                              "VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR");
        }
        if (pCreateInfo->pNext != NULL) {
            skipCall |= LOG_INFO_WRONG_NEXT(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                                             device,
                                             "pCreateInfo");
        }
    }

    if (VK_FALSE == skipCall) {
        // Call down the call chain:
        loader_platform_thread_unlock_mutex(&globalLock);
        result = my_data->instance_dispatch_table->CreateXlibSurfaceKHR(
                instance, pCreateInfo, pAllocator, pSurface);
        loader_platform_thread_lock_mutex(&globalLock);

        // Obtain this pointer again after locking:
        pInstance = &(my_data->instanceMap[instance]);
        if ((result == VK_SUCCESS) && pInstance && pSurface) {
            // Record the VkSurfaceKHR returned by the ICD:
            my_data->surfaceMap[*pSurface].surface = *pSurface;
            my_data->surfaceMap[*pSurface].pInstance = pInstance;
            my_data->surfaceMap[*pSurface].usedAllocatorToCreate =
                (pAllocator != NULL);
            my_data->surfaceMap[*pSurface].numQueueFamilyIndexSupport = 0;
            my_data->surfaceMap[*pSurface].pQueueFamilyIndexSupport = NULL;
            // Point to the associated SwpInstance:
            pInstance->surfaces[*pSurface] = &my_data->surfaceMap[*pSurface];
        }
        loader_platform_thread_unlock_mutex(&globalLock);
        return result;
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    return VK_ERROR_VALIDATION_FAILED_EXT;
}

VK_LAYER_EXPORT VKAPI_ATTR VkBool32 VKAPI_CALL vkGetPhysicalDeviceXlibPresentationSupportKHR(
    VkPhysicalDevice physicalDevice,
    uint32_t queueFamilyIndex,
    Display* dpy,
    VisualID visualID)
{
    VkBool32 result = VK_FALSE;
    VkBool32 skipCall = VK_FALSE;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    SwpPhysicalDevice *pPhysicalDevice = &my_data->physicalDeviceMap[physicalDevice];

    // Validate that the platform extension was enabled:
    if (pPhysicalDevice && pPhysicalDevice->pInstance &&
        !pPhysicalDevice->pInstance->xlibSurfaceExtensionEnabled) {
        skipCall |= LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT,
                              pPhysicalDevice->pInstance,
                              "VkInstance",
                              SWAPCHAIN_EXT_NOT_ENABLED_BUT_USED,
                              "%s() called even though the %s extension was not enabled for this VkInstance.",
                              __FUNCTION__, VK_KHR_XLIB_SURFACE_EXTENSION_NAME);
    }
    if (pPhysicalDevice->gotQueueFamilyPropertyCount &&
        (queueFamilyIndex >= pPhysicalDevice->numOfQueueFamilies)) {
        skipCall |= LOG_ERROR_QUEUE_FAMILY_INDEX_TOO_LARGE(VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT,
                                                           pPhysicalDevice,
                                                           "VkPhysicalDevice",
                                                           queueFamilyIndex,
                                                           pPhysicalDevice->numOfQueueFamilies);
    }
    loader_platform_thread_unlock_mutex(&globalLock);

    if (VK_FALSE == skipCall) {
        // Call down the call chain:
        result = my_data->instance_dispatch_table->GetPhysicalDeviceXlibPresentationSupportKHR(
                physicalDevice, queueFamilyIndex, dpy, visualID);
    }
    return result;
}
#endif // VK_USE_PLATFORM_XLIB_KHR

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroySurfaceKHR(VkInstance  instance, VkSurfaceKHR  surface, const VkAllocationCallbacks* pAllocator)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    SwpSurface *pSurface = &my_data->surfaceMap[surface];

    // Regardless of skipCall value, do some internal cleanup:
    if (pSurface) {
        // Delete the SwpSurface associated with this surface:
        if (pSurface->pInstance) {
            pSurface->pInstance->surfaces.erase(surface);
        }
        if (!pSurface->swapchains.empty()) {
            LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, instance, "VkInstance",
                      SWAPCHAIN_DEL_OBJECT_BEFORE_CHILDREN,
                      "%s() called before all of its associated "
                      "VkSwapchainKHRs were destroyed.",
                      __FUNCTION__);
            // Empty and then delete all SwpSwapchain's
            for (auto it = pSurface->swapchains.begin() ;
                 it != pSurface->swapchains.end() ; it++) {
                // Delete all SwpImage's
                it->second->images.clear();
                // In case the swapchain's device hasn't been destroyed yet
                // (which isn't likely, but is possible), delete its
                // association with this swapchain (i.e. so we can't point to
                // this swpchain from that device, later on):
                if (it->second->pDevice) {
                    it->second->pDevice->swapchains.clear();
                }
            }
            pSurface->swapchains.clear();
        }
        if ((pAllocator != NULL) != pSurface->usedAllocatorToCreate) {
            LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, instance, "VkInstance",
                      SWAPCHAIN_INCOMPATIBLE_ALLOCATOR,
                      "%s() called with incompatible pAllocator from when "
                      "the object was created.",
                      __FUNCTION__);
        }
        my_data->surfaceMap.erase(surface);
    }
    loader_platform_thread_unlock_mutex(&globalLock);

    if (VK_FALSE == skipCall) {
        // Call down the call chain:
        my_data->instance_dispatch_table->DestroySurfaceKHR(
                instance, surface, pAllocator);
    }
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumeratePhysicalDevices(VkInstance instance, uint32_t* pPhysicalDeviceCount, VkPhysicalDevice* pPhysicalDevices)
{
    VkResult result = VK_SUCCESS;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);

    // Call down the call chain:
    result = my_data->instance_dispatch_table->EnumeratePhysicalDevices(
            instance, pPhysicalDeviceCount, pPhysicalDevices);

    loader_platform_thread_lock_mutex(&globalLock);
    SwpInstance *pInstance = &(my_data->instanceMap[instance]);
    if ((result == VK_SUCCESS) && pInstance && pPhysicalDevices &&
        (*pPhysicalDeviceCount > 0)) {
        // Record the VkPhysicalDevices returned by the ICD:
        for (uint32_t i = 0; i < *pPhysicalDeviceCount; i++) {
            my_data->physicalDeviceMap[pPhysicalDevices[i]].physicalDevice =
                pPhysicalDevices[i];
            my_data->physicalDeviceMap[pPhysicalDevices[i]].pInstance = pInstance;
            my_data->physicalDeviceMap[pPhysicalDevices[i]].pDevice = NULL;
            my_data->physicalDeviceMap[pPhysicalDevices[i]].gotQueueFamilyPropertyCount = false;
            my_data->physicalDeviceMap[pPhysicalDevices[i]].gotSurfaceCapabilities = false;
            my_data->physicalDeviceMap[pPhysicalDevices[i]].surfaceFormatCount = 0;
            my_data->physicalDeviceMap[pPhysicalDevices[i]].pSurfaceFormats = NULL;
            my_data->physicalDeviceMap[pPhysicalDevices[i]].presentModeCount = 0;
            my_data->physicalDeviceMap[pPhysicalDevices[i]].pPresentModes = NULL;
            // Point to the associated SwpInstance:
            if (pInstance) {
                pInstance->physicalDevices[pPhysicalDevices[i]] =
                    &my_data->physicalDeviceMap[pPhysicalDevices[i]];
            }
        }
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDevice* pDevice)
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

    VkResult result = fpCreateDevice(physicalDevice, pCreateInfo, pAllocator, pDevice);
    if (result != VK_SUCCESS) {
        return result;
    }

    loader_platform_thread_lock_mutex(&globalLock);
    layer_data *my_instance_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
    layer_data *my_device_data = get_my_data_ptr(get_dispatch_key(*pDevice), layer_data_map);

    // Setup device dispatch table
    my_device_data->device_dispatch_table = new VkLayerDispatchTable;
    layer_init_device_dispatch_table(*pDevice, my_device_data->device_dispatch_table, fpGetDeviceProcAddr);

    my_device_data->report_data = layer_debug_report_create_device(my_instance_data->report_data, *pDevice);
    createDeviceRegisterExtensions(physicalDevice, pCreateInfo, *pDevice);
    loader_platform_thread_unlock_mutex(&globalLock);

    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyDevice(VkDevice device, const VkAllocationCallbacks* pAllocator)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);

    // Call down the call chain:
    my_data->device_dispatch_table->DestroyDevice(device, pAllocator);

    // Do some internal cleanup:
    loader_platform_thread_lock_mutex(&globalLock);
    SwpDevice *pDevice = &my_data->deviceMap[device];
    if (pDevice) {
        // Delete the SwpDevice associated with this device:
        if (pDevice->pPhysicalDevice) {
            pDevice->pPhysicalDevice->pDevice = NULL;
        }
        if (!pDevice->swapchains.empty()) {
            LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, device, "VkDevice",
                      SWAPCHAIN_DEL_OBJECT_BEFORE_CHILDREN,
                      "%s() called before all of its associated "
                      "VkSwapchainKHRs were destroyed.",
                      __FUNCTION__);
            // Empty and then delete all SwpSwapchain's
            for (auto it = pDevice->swapchains.begin() ;
                 it != pDevice->swapchains.end() ; it++) {
                // Delete all SwpImage's
                it->second->images.clear();
                // In case the swapchain's surface hasn't been destroyed yet
                // (which is likely) delete its association with this swapchain
                // (i.e. so we can't point to this swpchain from that surface,
                // later on):
                if (it->second->pSurface) {
                    it->second->pSurface->swapchains.clear();
                }
            }
            pDevice->swapchains.clear();
        }
        my_data->deviceMap.erase(device);
    }
    delete my_data->device_dispatch_table;
    layer_data_map.erase(key);
    loader_platform_thread_unlock_mutex(&globalLock);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceSurfaceSupportKHR(
    VkPhysicalDevice physicalDevice,
    uint32_t queueFamilyIndex,
    VkSurfaceKHR surface,
    VkBool32* pSupported)
{
    VkResult result = VK_SUCCESS;
    VkBool32 skipCall = VK_FALSE;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    SwpPhysicalDevice *pPhysicalDevice = &my_data->physicalDeviceMap[physicalDevice];

    // Validate that the surface extension was enabled:
    if (pPhysicalDevice && pPhysicalDevice->pInstance &&
        !pPhysicalDevice->pInstance->surfaceExtensionEnabled) {
        skipCall |= LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT,
                              pPhysicalDevice->pInstance,
                              "VkInstance",
                              SWAPCHAIN_EXT_NOT_ENABLED_BUT_USED,
                              "%s() called even though the %s extension was not enabled for this VkInstance.",
                              __FUNCTION__, VK_KHR_SURFACE_EXTENSION_NAME);
    }
    if (!pPhysicalDevice->gotQueueFamilyPropertyCount) {
        skipCall |= LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT,
                              pPhysicalDevice,
                              "VkPhysicalDevice",
                              SWAPCHAIN_DID_NOT_QUERY_QUEUE_FAMILIES,
                              "%s() called before calling the "
                              "vkGetPhysicalDeviceQueueFamilyProperties "
                              "function.",
                              __FUNCTION__);
    } else if (pPhysicalDevice->gotQueueFamilyPropertyCount &&
               (queueFamilyIndex >= pPhysicalDevice->numOfQueueFamilies)) {
        skipCall |= LOG_ERROR_QUEUE_FAMILY_INDEX_TOO_LARGE(VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT,
                                                           pPhysicalDevice,
                                                           "VkPhysicalDevice",
                                                           queueFamilyIndex,
                                                           pPhysicalDevice->numOfQueueFamilies);
    }
    if (!pSupported) {
        skipCall |= LOG_ERROR_NULL_POINTER(VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT,
                                           physicalDevice,
                                           "pSupported");
    }

    if (VK_FALSE == skipCall) {
        // Call down the call chain:
        loader_platform_thread_unlock_mutex(&globalLock);
        result = my_data->instance_dispatch_table->GetPhysicalDeviceSurfaceSupportKHR(
                physicalDevice, queueFamilyIndex, surface,
                pSupported);
        loader_platform_thread_lock_mutex(&globalLock);

        // Obtain this pointer again after locking:
        pPhysicalDevice = &my_data->physicalDeviceMap[physicalDevice];
        if ((result == VK_SUCCESS) && pSupported && pPhysicalDevice) {
            // Record the result of this query:
            SwpInstance *pInstance = pPhysicalDevice->pInstance;
            SwpSurface *pSurface =
                (pInstance) ? pInstance->surfaces[surface] : NULL;
            if (pSurface) {
                pPhysicalDevice->supportedSurfaces[surface] = pSurface;
                if (!pSurface->numQueueFamilyIndexSupport) {
                    if (pPhysicalDevice->gotQueueFamilyPropertyCount) {
                        pSurface->pQueueFamilyIndexSupport = (VkBool32 *)
                            malloc(pPhysicalDevice->numOfQueueFamilies *
                                   sizeof(VkBool32));
                        if (pSurface->pQueueFamilyIndexSupport != NULL) {
                            pSurface->numQueueFamilyIndexSupport =
                                pPhysicalDevice->numOfQueueFamilies;
                        }
                    }
                }
                if (pSurface->numQueueFamilyIndexSupport) {
                    pSurface->pQueueFamilyIndexSupport[queueFamilyIndex] =
                        *pSupported;
                }
            }
        }
        loader_platform_thread_unlock_mutex(&globalLock);
        return result;
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    return VK_ERROR_VALIDATION_FAILED_EXT;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
    VkPhysicalDevice physicalDevice,
    VkSurfaceKHR surface,
    VkSurfaceCapabilitiesKHR* pSurfaceCapabilities)
{
    VkResult result = VK_SUCCESS;
    VkBool32 skipCall = VK_FALSE;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    SwpPhysicalDevice *pPhysicalDevice = &my_data->physicalDeviceMap[physicalDevice];

    // Validate that the surface extension was enabled:
    if (pPhysicalDevice && pPhysicalDevice->pInstance &&
        !pPhysicalDevice->pInstance->surfaceExtensionEnabled) {
        skipCall |= LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT,
                              pPhysicalDevice->pInstance,
                              "VkInstance",
                              SWAPCHAIN_EXT_NOT_ENABLED_BUT_USED,
                              "%s() called even though the %s extension was not enabled for this VkInstance.",
                              __FUNCTION__, VK_KHR_SURFACE_EXTENSION_NAME);
    }
    if (!pSurfaceCapabilities) {
        skipCall |= LOG_ERROR_NULL_POINTER(VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT,
                                           physicalDevice,
                                           "pSurfaceCapabilities");
    }

    if (VK_FALSE == skipCall) {
        // Call down the call chain:
        loader_platform_thread_unlock_mutex(&globalLock);
        result = my_data->instance_dispatch_table->GetPhysicalDeviceSurfaceCapabilitiesKHR(
                physicalDevice, surface, pSurfaceCapabilities);
        loader_platform_thread_lock_mutex(&globalLock);

        // Obtain this pointer again after locking:
        pPhysicalDevice = &my_data->physicalDeviceMap[physicalDevice];
        if ((result == VK_SUCCESS) && pPhysicalDevice) {
            // Record the result of this query:
            pPhysicalDevice->gotSurfaceCapabilities = true;
// FIXME: NEED TO COPY THIS DATA, BECAUSE pSurfaceCapabilities POINTS TO APP-ALLOCATED DATA
            pPhysicalDevice->surfaceCapabilities = *pSurfaceCapabilities;
        }
        loader_platform_thread_unlock_mutex(&globalLock);
        return result;
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    return VK_ERROR_VALIDATION_FAILED_EXT;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceSurfaceFormatsKHR(
    VkPhysicalDevice physicalDevice,
    VkSurfaceKHR surface,
    uint32_t* pSurfaceFormatCount,
    VkSurfaceFormatKHR* pSurfaceFormats)
{
    VkResult result = VK_SUCCESS;
    VkBool32 skipCall = VK_FALSE;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    SwpPhysicalDevice *pPhysicalDevice = &my_data->physicalDeviceMap[physicalDevice];

    // Validate that the surface extension was enabled:
    if (pPhysicalDevice && pPhysicalDevice->pInstance &&
        !pPhysicalDevice->pInstance->surfaceExtensionEnabled) {
        skipCall |= LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT,
                              pPhysicalDevice->pInstance,
                              "VkInstance",
                              SWAPCHAIN_EXT_NOT_ENABLED_BUT_USED,
                              "%s() called even though the %s extension was not enabled for this VkInstance.",
                              __FUNCTION__, VK_KHR_SURFACE_EXTENSION_NAME);
    }
    if (!pSurfaceFormatCount) {
        skipCall |= LOG_ERROR_NULL_POINTER(VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT,
                                           physicalDevice,
                                           "pSurfaceFormatCount");
    }

    if (VK_FALSE == skipCall) {
        // Call down the call chain:
        loader_platform_thread_unlock_mutex(&globalLock);
        result = my_data->instance_dispatch_table->GetPhysicalDeviceSurfaceFormatsKHR(
                physicalDevice, surface, pSurfaceFormatCount, pSurfaceFormats);
        loader_platform_thread_lock_mutex(&globalLock);

        // Obtain this pointer again after locking:
        pPhysicalDevice = &my_data->physicalDeviceMap[physicalDevice];
        if ((result == VK_SUCCESS) && pPhysicalDevice && !pSurfaceFormats &&
            pSurfaceFormatCount) {
            // Record the result of this preliminary query:
            pPhysicalDevice->surfaceFormatCount = *pSurfaceFormatCount;
        }
        else if ((result == VK_SUCCESS) && pPhysicalDevice && pSurfaceFormats &&
                 pSurfaceFormatCount) {
            // Compare the preliminary value of *pSurfaceFormatCount with the
            // value this time:
            if (*pSurfaceFormatCount > pPhysicalDevice->surfaceFormatCount) {
                LOG_ERROR_INVALID_COUNT(VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT,
                                        physicalDevice,
                                        "pSurfaceFormatCount",
                                        "pSurfaceFormats",
                                        *pSurfaceFormatCount,
                                        pPhysicalDevice->surfaceFormatCount);
            }
            else if (*pSurfaceFormatCount > 0) {
                // Record the result of this query:
                pPhysicalDevice->surfaceFormatCount = *pSurfaceFormatCount;
                pPhysicalDevice->pSurfaceFormats = (VkSurfaceFormatKHR *)
                    malloc(*pSurfaceFormatCount * sizeof(VkSurfaceFormatKHR));
                if (pPhysicalDevice->pSurfaceFormats) {
                    for (uint32_t i = 0 ; i < *pSurfaceFormatCount ; i++) {
                        pPhysicalDevice->pSurfaceFormats[i] = pSurfaceFormats[i];
                    }
                } else {
                    pPhysicalDevice->surfaceFormatCount = 0;
                }
            }
        }
        loader_platform_thread_unlock_mutex(&globalLock);
        return result;
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    return VK_ERROR_VALIDATION_FAILED_EXT;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceSurfacePresentModesKHR(
    VkPhysicalDevice physicalDevice,
    VkSurfaceKHR surface,
    uint32_t* pPresentModeCount,
    VkPresentModeKHR* pPresentModes)
{
    VkResult result = VK_SUCCESS;
    VkBool32 skipCall = VK_FALSE;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    SwpPhysicalDevice *pPhysicalDevice = &my_data->physicalDeviceMap[physicalDevice];

    // Validate that the surface extension was enabled:
    if (pPhysicalDevice && pPhysicalDevice->pInstance &&
        !pPhysicalDevice->pInstance->surfaceExtensionEnabled) {
        skipCall |= LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT,
                              pPhysicalDevice->pInstance,
                              "VkInstance",
                              SWAPCHAIN_EXT_NOT_ENABLED_BUT_USED,
                              "%s() called even though the %s extension was not enabled for this VkInstance.",
                              __FUNCTION__, VK_KHR_SURFACE_EXTENSION_NAME);
    }
    if (!pPresentModeCount) {
        skipCall |= LOG_ERROR_NULL_POINTER(VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT,
                                           physicalDevice,
                                           "pPresentModeCount");
    }

    if (VK_FALSE == skipCall) {
        // Call down the call chain:
        loader_platform_thread_unlock_mutex(&globalLock);
        result = my_data->instance_dispatch_table->GetPhysicalDeviceSurfacePresentModesKHR(
                physicalDevice, surface, pPresentModeCount, pPresentModes);
        loader_platform_thread_lock_mutex(&globalLock);

        // Obtain this pointer again after locking:
        pPhysicalDevice = &my_data->physicalDeviceMap[physicalDevice];
        if ((result == VK_SUCCESS) && pPhysicalDevice && !pPresentModes &&
            pPresentModeCount) {
            // Record the result of this preliminary query:
            pPhysicalDevice->presentModeCount = *pPresentModeCount;
        }
        else if ((result == VK_SUCCESS) && pPhysicalDevice && pPresentModes &&
                 pPresentModeCount) {
            // Compare the preliminary value of *pPresentModeCount with the
            // value this time:
            if (*pPresentModeCount > pPhysicalDevice->presentModeCount) {
                LOG_ERROR_INVALID_COUNT(VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT,
                                        physicalDevice,
                                        "pPresentModeCount",
                                        "pPresentModes",
                                        *pPresentModeCount,
                                        pPhysicalDevice->presentModeCount);
            }
            else if (*pPresentModeCount > 0) {
                // Record the result of this query:
                pPhysicalDevice->presentModeCount = *pPresentModeCount;
                pPhysicalDevice->pPresentModes = (VkPresentModeKHR *)
                    malloc(*pPresentModeCount * sizeof(VkPresentModeKHR));
                if (pPhysicalDevice->pPresentModes) {
                    for (uint32_t i = 0 ; i < *pPresentModeCount ; i++) {
                        pPhysicalDevice->pPresentModes[i] = pPresentModes[i];
                    }
                } else {
                    pPhysicalDevice->presentModeCount = 0;
                }
            }
        }
        loader_platform_thread_unlock_mutex(&globalLock);
        return result;
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    return VK_ERROR_VALIDATION_FAILED_EXT;
}

// This function does the up-front validation work for vkCreateSwapchainKHR(),
// and returns VK_TRUE if a logging callback indicates that the call down the
// chain should be skipped:
static VkBool32 validateCreateSwapchainKHR(
    VkDevice device,
    const VkSwapchainCreateInfoKHR* pCreateInfo,
    VkSwapchainKHR* pSwapchain)
{
// TODO: Validate cases of re-creating a swapchain (the current code
// assumes a new swapchain is being created).
    VkBool32 skipCall = VK_FALSE;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    char fn[] = "vkCreateSwapchainKHR";
    SwpDevice *pDevice = &my_data->deviceMap[device];

    // Validate that the swapchain extension was enabled:
    if (pDevice && !pDevice->swapchainExtensionEnabled) {
        return LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, device, "VkDevice",
                         SWAPCHAIN_EXT_NOT_ENABLED_BUT_USED,
                         "%s() called even though the %s extension was not enabled for this VkDevice.",
                         fn, VK_KHR_SWAPCHAIN_EXTENSION_NAME );
    }
    if (!pCreateInfo) {
        skipCall |= LOG_ERROR_NULL_POINTER(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                                           device,
                                           "pCreateInfo");
    } else {
        if (pCreateInfo->sType != VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR) {
            skipCall |= LOG_ERROR_WRONG_STYPE(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                                              device,
                                              "pCreateInfo",
                                              "VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR");
        }
        if (pCreateInfo->pNext != NULL) {
            skipCall |= LOG_INFO_WRONG_NEXT(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                                             device,
                                             "pCreateInfo");
        }
    }
    if (!pSwapchain) {
        skipCall |= LOG_ERROR_NULL_POINTER(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                                           device,
                                           "pSwapchain");
    }

    // Keep around a useful pointer to pPhysicalDevice:
    SwpPhysicalDevice *pPhysicalDevice = pDevice->pPhysicalDevice;

    // Validate pCreateInfo values with the results of
    // vkGetPhysicalDeviceSurfaceCapabilitiesKHR():
    if (!pPhysicalDevice || !pPhysicalDevice->gotSurfaceCapabilities) {
        skipCall |= LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, device, "VkDevice",
                              SWAPCHAIN_CREATE_SWAP_WITHOUT_QUERY,
                              "%s() called before calling "
                              "vkGetPhysicalDeviceSurfaceCapabilitiesKHR().",
                              fn);
    } else if (pCreateInfo) {
        // Validate pCreateInfo->surface to make sure that
        // vkGetPhysicalDeviceSurfaceSupportKHR() reported this as a supported
        // surface:
        SwpSurface *pSurface =
            ((pPhysicalDevice) ?
             pPhysicalDevice->supportedSurfaces[pCreateInfo->surface] : NULL);
        if (!pSurface) {
            skipCall |= LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, device, "VkDevice",
                                  SWAPCHAIN_CREATE_UNSUPPORTED_SURFACE,
                                  "%s() called with pCreateInfo->surface that "
                                  "was not returned by "
                                  "vkGetPhysicalDeviceSurfaceSupportKHR() "
                                  "for the device.",
                                  fn);
        }

        // Validate pCreateInfo->minImageCount against
        // VkSurfaceCapabilitiesKHR::{min|max}ImageCount:
        VkSurfaceCapabilitiesKHR *pCapabilities = &pPhysicalDevice->surfaceCapabilities;
        if ((pCreateInfo->minImageCount < pCapabilities->minImageCount) ||
            ((pCapabilities->maxImageCount > 0) &&
             (pCreateInfo->minImageCount > pCapabilities->maxImageCount))) {
            skipCall |= LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, device, "VkDevice",
                                  SWAPCHAIN_CREATE_SWAP_BAD_MIN_IMG_COUNT,
                                  "%s() called with pCreateInfo->minImageCount "
                                  "= %d, which is outside the bounds returned "
                                  "by vkGetPhysicalDeviceSurfaceCapabilitiesKHR() (i.e. "
                                  "minImageCount = %d, maxImageCount = %d).",
                                  fn,
                                  pCreateInfo->minImageCount,
                                  pCapabilities->minImageCount,
                                  pCapabilities->maxImageCount);
        }
        // Validate pCreateInfo->imageExtent against
        // VkSurfaceCapabilitiesKHR::{current|min|max}ImageExtent:
        if ((pCapabilities->currentExtent.width == -1) &&
            ((pCreateInfo->imageExtent.width < pCapabilities->minImageExtent.width) ||
             (pCreateInfo->imageExtent.width > pCapabilities->maxImageExtent.width) ||
             (pCreateInfo->imageExtent.height < pCapabilities->minImageExtent.height) ||
             (pCreateInfo->imageExtent.height > pCapabilities->maxImageExtent.height))) {
            skipCall |= LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, device, "VkDevice",
                                  SWAPCHAIN_CREATE_SWAP_OUT_OF_BOUNDS_EXTENTS,
                                  "%s() called with pCreateInfo->imageExtent = "
                                  "(%d,%d), which is outside the bounds "
                                  "returned by vkGetPhysicalDeviceSurfaceCapabilitiesKHR(): "
                                  "currentExtent = (%d,%d), minImageExtent = "
                                  "(%d,%d), maxImageExtent = (%d,%d).",
                                  fn,
                                  pCreateInfo->imageExtent.width,
                                  pCreateInfo->imageExtent.height,
                                  pCapabilities->currentExtent.width,
                                  pCapabilities->currentExtent.height,
                                  pCapabilities->minImageExtent.width,
                                  pCapabilities->minImageExtent.height,
                                  pCapabilities->maxImageExtent.width,
                                  pCapabilities->maxImageExtent.height);
        }
        if ((pCapabilities->currentExtent.width != -1) &&
            ((pCreateInfo->imageExtent.width != pCapabilities->currentExtent.width) ||
             (pCreateInfo->imageExtent.height != pCapabilities->currentExtent.height))) {
            skipCall |= LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, device, "VkDevice",
                                  SWAPCHAIN_CREATE_SWAP_EXTENTS_NO_MATCH_WIN,
                                  "%s() called with pCreateInfo->imageExtent = "
                                  "(%d,%d), which is not equal to the "
                                  "currentExtent = (%d,%d) returned by "
                                  "vkGetPhysicalDeviceSurfaceCapabilitiesKHR().",
                                  fn,
                                  pCreateInfo->imageExtent.width,
                                  pCreateInfo->imageExtent.height,
                                  pCapabilities->currentExtent.width,
                                  pCapabilities->currentExtent.height);
        }
        // Validate pCreateInfo->preTransform has one bit set (1st two
        // lines of if-statement), which bit is also set in
        // VkSurfaceCapabilitiesKHR::supportedTransforms (3rd line of if-statement):
        if (!pCreateInfo->preTransform ||
            (pCreateInfo->preTransform & (pCreateInfo->preTransform - 1)) ||
            !(pCreateInfo->preTransform & pCapabilities->supportedTransforms)) {
            // This is an error situation; one for which we'd like to give
            // the developer a helpful, multi-line error message.  Build it
            // up a little at a time, and then log it:
            std::string errorString = "";
            char str[1024];
            // Here's the first part of the message:
            sprintf(str, "%s() called with a non-supported "
                    "pCreateInfo->preTransform (i.e. %s).  "
                    "Supported values are:\n",
                    fn,
                    surfaceTransformStr(pCreateInfo->preTransform));
            errorString += str;
            for (int i = 0; i < 32; i++) {
                // Build up the rest of the message:
                if ((1 << i) & pCapabilities->supportedTransforms) {
                    const char *newStr =
                        surfaceTransformStr((VkSurfaceTransformFlagBitsKHR) (1 << i));
                    sprintf(str, "    %s\n", newStr);
                    errorString += str;
                }
            }
            // Log the message that we've built up:
            skipCall |= debug_report_log_msg(my_data->report_data,
                                             VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                             VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                                             (uint64_t) device, __LINE__,
                                             SWAPCHAIN_CREATE_SWAP_BAD_PRE_TRANSFORM,
                                             LAYER_NAME,
                                             errorString.c_str());
        }
        // Validate pCreateInfo->compositeAlpha has one bit set (1st two
        // lines of if-statement), which bit is also set in
        // VkSurfaceCapabilitiesKHR::supportedCompositeAlpha (3rd line of if-statement):
        if (!pCreateInfo->compositeAlpha ||
            (pCreateInfo->compositeAlpha & (pCreateInfo->compositeAlpha - 1)) ||
            !((pCreateInfo->compositeAlpha) & pCapabilities->supportedCompositeAlpha)) {
            // This is an error situation; one for which we'd like to give
            // the developer a helpful, multi-line error message.  Build it
            // up a little at a time, and then log it:
            std::string errorString = "";
            char str[1024];
            // Here's the first part of the message:
            sprintf(str, "%s() called with a non-supported "
                    "pCreateInfo->compositeAlpha (i.e. %s).  "
                    "Supported values are:\n",
                    fn,
                    surfaceCompositeAlphaStr(pCreateInfo->compositeAlpha));
            errorString += str;
            for (int i = 0; i < 32; i++) {
                // Build up the rest of the message:
                if ((1 << i) & pCapabilities->supportedCompositeAlpha) {
                    const char *newStr =
                        surfaceCompositeAlphaStr((VkCompositeAlphaFlagBitsKHR) (1 << i));
                    sprintf(str, "    %s\n", newStr);
                    errorString += str;
                }
            }
            // Log the message that we've built up:
            skipCall |= debug_report_log_msg(my_data->report_data,
                                             VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                             VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                                             (uint64_t) device, 0,
                                             SWAPCHAIN_CREATE_SWAP_BAD_COMPOSITE_ALPHA,
                                             LAYER_NAME,
                                             errorString.c_str());
        }
        // Validate pCreateInfo->imageArraySize against
        // VkSurfaceCapabilitiesKHR::maxImageArraySize:
        if ((pCreateInfo->imageArrayLayers < 1) ||
            (pCreateInfo->imageArrayLayers > pCapabilities->maxImageArrayLayers)) {
            skipCall |= LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, device, "VkDevice",
                                  SWAPCHAIN_CREATE_SWAP_BAD_IMG_ARRAY_SIZE,
                                  "%s() called with a non-supported "
                                  "pCreateInfo->imageArraySize (i.e. %d).  "
                                  "Minimum value is 1, maximum value is %d.",
                                  fn,
                                  pCreateInfo->imageArrayLayers,
                                  pCapabilities->maxImageArrayLayers);
        }
        // Validate pCreateInfo->imageUsage against
        // VkSurfaceCapabilitiesKHR::supportedUsageFlags:
        if (pCreateInfo->imageUsage !=
            (pCreateInfo->imageUsage & pCapabilities->supportedUsageFlags)) {
            skipCall |= LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, device, "VkDevice",
                                  SWAPCHAIN_CREATE_SWAP_BAD_IMG_USAGE_FLAGS,
                                  "%s() called with a non-supported "
                                  "pCreateInfo->imageUsage (i.e. 0x%08x)."
                                  "  Supported flag bits are 0x%08x.",
                                  fn,
                                  pCreateInfo->imageUsage,
                                  pCapabilities->supportedUsageFlags);
        }
    }

    // Validate pCreateInfo values with the results of
    // vkGetPhysicalDeviceSurfaceFormatsKHR():
    if (!pPhysicalDevice || !pPhysicalDevice->surfaceFormatCount) {
        skipCall |= LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, device, "VkDevice",
                              SWAPCHAIN_CREATE_SWAP_WITHOUT_QUERY,
                              "%s() called before calling "
                              "vkGetPhysicalDeviceSurfaceFormatsKHR().",
                              fn);
    } else if (pCreateInfo) {
        // Validate pCreateInfo->imageFormat against
        // VkSurfaceFormatKHR::format:
        bool foundFormat = false;
        bool foundColorSpace = false;
        bool foundMatch = false;
        for (uint32_t i = 0 ; i < pPhysicalDevice->surfaceFormatCount ; i++) {
            if (pCreateInfo->imageFormat == pPhysicalDevice->pSurfaceFormats[i].format) {
                // Validate pCreateInfo->imageColorSpace against
                // VkSurfaceFormatKHR::colorSpace:
                foundFormat = true;
                if (pCreateInfo->imageColorSpace == pPhysicalDevice->pSurfaceFormats[i].colorSpace) {
                    foundMatch = true;
                    break;
                }
            } else {
                if (pCreateInfo->imageColorSpace == pPhysicalDevice->pSurfaceFormats[i].colorSpace) {
                    foundColorSpace = true;
                }
            }
        }
        if (!foundMatch) {
            if (!foundFormat) {
                if (!foundColorSpace) {
                    skipCall |= LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, device,
                                          "VkDevice",
                                          SWAPCHAIN_CREATE_SWAP_BAD_IMG_FMT_CLR_SP,
                                          "%s() called with neither a "
                                          "supported pCreateInfo->imageFormat "
                                          "(i.e. %d) nor a supported "
                                          "pCreateInfo->imageColorSpace "
                                          "(i.e. %d).",
                                          fn,
                                          pCreateInfo->imageFormat,
                                          pCreateInfo->imageColorSpace);
                } else {
                    skipCall |= LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, device,
                                          "VkDevice",
                                          SWAPCHAIN_CREATE_SWAP_BAD_IMG_FORMAT,
                                          "%s() called with a non-supported "
                                          "pCreateInfo->imageFormat (i.e. %d).",
                                          fn, pCreateInfo->imageFormat);
                }
            } else if (!foundColorSpace) {
                skipCall |= LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, device, "VkDevice",
                                      SWAPCHAIN_CREATE_SWAP_BAD_IMG_COLOR_SPACE,
                                      "%s() called with a non-supported "
                                      "pCreateInfo->imageColorSpace (i.e. %d).",
                                      fn, pCreateInfo->imageColorSpace);
            }
        }
    }

    // Validate pCreateInfo values with the results of
    // vkGetPhysicalDeviceSurfacePresentModesKHR():
    if (!pPhysicalDevice || !pPhysicalDevice->presentModeCount) {
        skipCall |= LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, device, "VkDevice",
                              SWAPCHAIN_CREATE_SWAP_WITHOUT_QUERY,
                              "%s() called before calling "
                              "vkGetPhysicalDeviceSurfacePresentModesKHR().",
                              fn);
    } else if (pCreateInfo) {
        // Validate pCreateInfo->presentMode against
        // vkGetPhysicalDeviceSurfacePresentModesKHR():
        bool foundMatch = false;
        for (uint32_t i = 0 ; i < pPhysicalDevice->presentModeCount ; i++) {
            if (pPhysicalDevice->pPresentModes[i] == pCreateInfo->presentMode) {
                foundMatch = true;
                break;
            }
        }
        if (!foundMatch) {
            skipCall |= LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, device, "VkDevice",
                                  SWAPCHAIN_CREATE_SWAP_BAD_PRESENT_MODE,
                                  "%s() called with a non-supported "
                                  "pCreateInfo->presentMode (i.e. %s).",
                                  fn,
                                  presentModeStr(pCreateInfo->presentMode));
        }
    }

    // Validate pCreateInfo->imageSharingMode and related values:
    if (pCreateInfo->imageSharingMode == VK_SHARING_MODE_CONCURRENT) {
        if ((pCreateInfo->queueFamilyIndexCount <= 1) ||
            !pCreateInfo->pQueueFamilyIndices) {
            skipCall |= LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, device, "VkDevice",
                                  SWAPCHAIN_CREATE_SWAP_BAD_SHARING_VALUES,
                                  "%s() called with a supported "
                                  "pCreateInfo->sharingMode of (i.e. %s),"
                                  "but with a bad value(s) for "
                                  "pCreateInfo->queueFamilyIndexCount or "
                                  "pCreateInfo->pQueueFamilyIndices).",
                                  fn,
                                  sharingModeStr(pCreateInfo->imageSharingMode));
        }
    } else if (pCreateInfo->imageSharingMode != VK_SHARING_MODE_EXCLUSIVE) {
        skipCall |= LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, device, "VkDevice",
                              SWAPCHAIN_CREATE_SWAP_BAD_SHARING_MODE,
                              "%s() called with a non-supported "
                              "pCreateInfo->imageSharingMode (i.e. %s).",
                              fn,
                              sharingModeStr(pCreateInfo->imageSharingMode));
    }

    // Validate pCreateInfo->clipped:
    if (pCreateInfo &&
        (pCreateInfo->clipped != VK_FALSE) &&
        (pCreateInfo->clipped != VK_TRUE)) {
        skipCall |= LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                              device, "VkDevice",
                              SWAPCHAIN_BAD_BOOL,
                              "%s() called with a VkBool32 value that is "
                              "neither VK_TRUE nor VK_FALSE, but has the "
                              "numeric value of %d.",
                              fn,
                              pCreateInfo->clipped);
    }

    // Validate pCreateInfo->oldSwapchain:
    if (pCreateInfo && pCreateInfo->oldSwapchain) {
        SwpSwapchain *pOldSwapchain = &my_data->swapchainMap[pCreateInfo->oldSwapchain];
        if (pOldSwapchain) {
            if (device != pOldSwapchain->pDevice->device) {
                skipCall |= LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                                      device, "VkDevice",
                                      SWAPCHAIN_DESTROY_SWAP_DIFF_DEVICE,
                                      "%s() called with a different VkDevice "
                                      "than the VkSwapchainKHR was created with.",
                                      __FUNCTION__);
            }
            if (pCreateInfo->surface != pOldSwapchain->pSurface->surface) {
                skipCall |= LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                                      device, "VkDevice",
                                      SWAPCHAIN_CREATE_SWAP_DIFF_SURFACE,
                                      "%s() called with pCreateInfo->oldSwapchain "
                                      "that has a different VkSurfaceKHR than "
                                      "pCreateInfo->surface.",
                                      fn);
            }
        } else {
            // TBD: Leave this in (not sure object_track will check this)?
            skipCall |= LOG_ERROR_NON_VALID_OBJ(VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT,
                                                pCreateInfo->oldSwapchain,
                                                "VkSwapchainKHR");
        }
    }

    return skipCall;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateSwapchainKHR(
    VkDevice device,
    const VkSwapchainCreateInfoKHR* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkSwapchainKHR* pSwapchain)
{
    VkResult result = VK_SUCCESS;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    VkBool32 skipCall = validateCreateSwapchainKHR(device, pCreateInfo,
                                                   pSwapchain);

    if (VK_FALSE == skipCall) {
        // Call down the call chain:
        loader_platform_thread_unlock_mutex(&globalLock);
        result = my_data->device_dispatch_table->CreateSwapchainKHR(
                device, pCreateInfo, pAllocator, pSwapchain);
        loader_platform_thread_lock_mutex(&globalLock);

        if (result == VK_SUCCESS) {
            // Remember the swapchain's handle, and link it to the device:
            SwpDevice *pDevice = &my_data->deviceMap[device];

            my_data->swapchainMap[*pSwapchain].swapchain = *pSwapchain;
            if (pDevice) {
                pDevice->swapchains[*pSwapchain] =
                    &my_data->swapchainMap[*pSwapchain];
            }
            my_data->swapchainMap[*pSwapchain].pDevice = pDevice;
            my_data->swapchainMap[*pSwapchain].imageCount = 0;
            my_data->swapchainMap[*pSwapchain].usedAllocatorToCreate =
                (pAllocator != NULL);
            // Store a pointer to the surface
            SwpPhysicalDevice *pPhysicalDevice = pDevice->pPhysicalDevice;
            SwpInstance *pInstance =
                (pPhysicalDevice) ? pPhysicalDevice->pInstance : NULL;
            layer_data *my_instance_data =
                ((pInstance) ?
                 get_my_data_ptr(get_dispatch_key(pInstance->instance), layer_data_map) :
                 NULL);
            SwpSurface *pSurface =
                ((my_data && pCreateInfo) ?
                 &my_instance_data->surfaceMap[pCreateInfo->surface] : NULL);
            my_data->swapchainMap[*pSwapchain].pSurface = pSurface;
            if (pSurface) {
                pSurface->swapchains[*pSwapchain] =
                    &my_data->swapchainMap[*pSwapchain];
            }
        }
        loader_platform_thread_unlock_mutex(&globalLock);
        return result;
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    return VK_ERROR_VALIDATION_FAILED_EXT;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroySwapchainKHR(
    VkDevice device,
    VkSwapchainKHR swapchain,
    const VkAllocationCallbacks* pAllocator)
{
// TODOs:
//
// - Implement a check for validity language that reads: All uses of
//   presentable images acquired from pname:swapchain and owned by the
//   application must: have completed execution
    VkBool32 skipCall = VK_FALSE;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    SwpDevice *pDevice = &my_data->deviceMap[device];

    // Validate that the swapchain extension was enabled:
    if (pDevice && !pDevice->swapchainExtensionEnabled) {
        skipCall |= LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, device, "VkDevice",
                              SWAPCHAIN_EXT_NOT_ENABLED_BUT_USED,
                              "%s() called even though the %s extension was not enabled for this VkDevice.",
                              __FUNCTION__, VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    }

    // Regardless of skipCall value, do some internal cleanup:
    SwpSwapchain *pSwapchain = &my_data->swapchainMap[swapchain];
    if (pSwapchain) {
        // Delete the SwpSwapchain associated with this swapchain:
        if (pSwapchain->pDevice) {
            pSwapchain->pDevice->swapchains.erase(swapchain);
            if (device != pSwapchain->pDevice->device) {
                LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, device, "VkDevice",
                          SWAPCHAIN_DESTROY_SWAP_DIFF_DEVICE,
                          "%s() called with a different VkDevice than the "
                          "VkSwapchainKHR was created with.",
                          __FUNCTION__);
            }
        }
        if (pSwapchain->pSurface) {
            pSwapchain->pSurface->swapchains.erase(swapchain);
        }
        if (pSwapchain->imageCount) {
            pSwapchain->images.clear();
        }
        if ((pAllocator != NULL) != pSwapchain->usedAllocatorToCreate) {
            LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, instance, "VkInstance",
                      SWAPCHAIN_INCOMPATIBLE_ALLOCATOR,
                      "%s() called with incompatible pAllocator from when "
                      "the object was created.",
                      __FUNCTION__);
        }
        my_data->swapchainMap.erase(swapchain);
    }
    loader_platform_thread_unlock_mutex(&globalLock);

    if (VK_FALSE == skipCall) {
        // Call down the call chain:
        my_data->device_dispatch_table->DestroySwapchainKHR(device, swapchain, pAllocator);
    }
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkGetSwapchainImagesKHR(
    VkDevice                                    device,
    VkSwapchainKHR                              swapchain,
    uint32_t*                                   pSwapchainImageCount,
    VkImage*                                    pSwapchainImages)
{
    VkResult result = VK_SUCCESS;
    VkBool32 skipCall = VK_FALSE;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    SwpDevice *pDevice = &my_data->deviceMap[device];

    // Validate that the swapchain extension was enabled:
    if (pDevice && !pDevice->swapchainExtensionEnabled) {
        skipCall |= LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, device, "VkDevice",
                              SWAPCHAIN_EXT_NOT_ENABLED_BUT_USED,
                              "%s() called even though the %s extension was not enabled for this VkDevice.",
                              __FUNCTION__, VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    }
    SwpSwapchain *pSwapchain = &my_data->swapchainMap[swapchain];
    if (!pSwapchainImageCount) {
        skipCall |= LOG_ERROR_NULL_POINTER(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                                           device,
                                           "pSwapchainImageCount");
    }

    if (VK_FALSE == skipCall) {
        // Call down the call chain:
        loader_platform_thread_unlock_mutex(&globalLock);
        result = my_data->device_dispatch_table->GetSwapchainImagesKHR(
                device, swapchain, pSwapchainImageCount, pSwapchainImages);
        loader_platform_thread_lock_mutex(&globalLock);

        // Obtain this pointer again after locking:
        pSwapchain = &my_data->swapchainMap[swapchain];
        if ((result == VK_SUCCESS) && pSwapchain && !pSwapchainImages &&
            pSwapchainImageCount) {
            // Record the result of this preliminary query:
            pSwapchain->imageCount = *pSwapchainImageCount;
        }
        else if ((result == VK_SUCCESS) && pSwapchain && pSwapchainImages &&
                 pSwapchainImageCount) {
            // Compare the preliminary value of *pSwapchainImageCount with the
            // value this time:
            if (*pSwapchainImageCount > pSwapchain->imageCount) {
                LOG_ERROR_INVALID_COUNT(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                                        device,
                                        "pSwapchainImageCount",
                                        "pSwapchainImages",
                                        *pSwapchainImageCount,
                                        pSwapchain->imageCount);
            }
            else if (*pSwapchainImageCount > 0) {
                // Record the images and their state:
                pSwapchain->imageCount = *pSwapchainImageCount;
                for (uint32_t i = 0 ; i < *pSwapchainImageCount ; i++) {
                    pSwapchain->images[i].image = pSwapchainImages[i];
                    pSwapchain->images[i].pSwapchain = pSwapchain;
                    pSwapchain->images[i].ownedByApp = false;
                }
            }
        }
        loader_platform_thread_unlock_mutex(&globalLock);
        return result;
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    return VK_ERROR_VALIDATION_FAILED_EXT;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkAcquireNextImageKHR(
    VkDevice device,
    VkSwapchainKHR swapchain,
    uint64_t timeout,
    VkSemaphore semaphore,
    VkFence fence,
    uint32_t* pImageIndex)
{
// TODOs:
//
// - Address the timeout.  Possibilities include looking at the state of the
//   swapchain's images, depending on the timeout value.
// - Implement a check for validity language that reads: If pname:semaphore is
//   not sname:VK_NULL_HANDLE it must: be unsignalled
// - Implement a check for validity language that reads: If pname:fence is not
//   sname:VK_NULL_HANDLE it must: be unsignalled and mustnot: be associated
//   with any other queue command that has not yet completed execution on that
//   queue
// - Record/update the state of the swapchain, in case an error occurs
//   (e.g. VK_ERROR_OUT_OF_DATE_KHR).
    VkResult result = VK_SUCCESS;
    VkBool32 skipCall = VK_FALSE;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    SwpDevice *pDevice = &my_data->deviceMap[device];

    // Validate that the swapchain extension was enabled:
    if (pDevice && !pDevice->swapchainExtensionEnabled) {
        skipCall |= LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, device, "VkDevice",
                              SWAPCHAIN_EXT_NOT_ENABLED_BUT_USED,
                              "%s() called even though the %s extension was not enabled for this VkDevice.",
                              __FUNCTION__, VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    }
    SwpSwapchain *pSwapchain = &my_data->swapchainMap[swapchain];
    if (pSwapchain) {
        // Look to see if the application is trying to own too many images at
        // the same time (i.e. not leave any to display):
        uint32_t imagesOwnedByApp = 0;
        for (uint32_t i = 0 ; i < pSwapchain->imageCount ; i++) {
            if (pSwapchain->images[i].ownedByApp) {
                imagesOwnedByApp++;
            }
        }
        if (imagesOwnedByApp >= (pSwapchain->imageCount - 1)) {
            skipCall |= LOG_PERF_WARNING(VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT,
                                         swapchain,
                                         "VkSwapchainKHR",
                                         SWAPCHAIN_APP_OWNS_TOO_MANY_IMAGES,
                                         "%s() called when the application "
                                         "already owns all presentable images "
                                         "in this swapchain except for the "
                                         "image currently being displayed.  "
                                         "This call to %s() cannot succeed "
                                         "unless another thread calls the "
                                         "vkQueuePresentKHR() function in "
                                         "order to release ownership of one of "
                                         "the presentable images of this "
                                         "swapchain.",
                                         __FUNCTION__, __FUNCTION__);
        }
    }
    if (!pImageIndex) {
        skipCall |= LOG_ERROR_NULL_POINTER(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                                           device,
                                           "pImageIndex");
    }

    if (VK_FALSE == skipCall) {
        // Call down the call chain:
        loader_platform_thread_unlock_mutex(&globalLock);
        result = my_data->device_dispatch_table->AcquireNextImageKHR(
                device, swapchain, timeout, semaphore, fence, pImageIndex);
        loader_platform_thread_lock_mutex(&globalLock);

        // Obtain this pointer again after locking:
        pSwapchain = &my_data->swapchainMap[swapchain];
        if (((result == VK_SUCCESS) || (result == VK_SUBOPTIMAL_KHR)) &&
            pSwapchain) {
            // Change the state of the image (now owned by the application):
            pSwapchain->images[*pImageIndex].ownedByApp = true;
        }
        loader_platform_thread_unlock_mutex(&globalLock);
        return result;
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    return VK_ERROR_VALIDATION_FAILED_EXT;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkQueuePresentKHR(
    VkQueue queue,
    const VkPresentInfoKHR* pPresentInfo)
{
// TODOs:
//
// - Implement a check for validity language that reads: Any given element of
//   sname:VkSemaphore in pname:pWaitSemaphores must: refer to a prior signal
//   of that sname:VkSemaphore that won't be consumed by any other wait on that
//   semaphore
// - Record/update the state of the swapchain, in case an error occurs
//   (e.g. VK_ERROR_OUT_OF_DATE_KHR).
    VkResult result = VK_SUCCESS;
    VkBool32 skipCall = VK_FALSE;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(queue), layer_data_map);

    if (!pPresentInfo) {
        skipCall |= LOG_ERROR_NULL_POINTER(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                                           device,
                                           "pPresentInfo");
    } else {
        if (pPresentInfo->sType != VK_STRUCTURE_TYPE_PRESENT_INFO_KHR) {
            skipCall |= LOG_ERROR_WRONG_STYPE(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                                              device,
                                              "pPresentInfo",
                                              "VK_STRUCTURE_TYPE_PRESENT_INFO_KHR");
        }
        if (pPresentInfo->pNext != NULL) {
            skipCall |= LOG_INFO_WRONG_NEXT(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                                             device,
                                             "pPresentInfo");
        }
        if (!pPresentInfo->swapchainCount) {
            skipCall |= LOG_ERROR_ZERO_VALUE(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                                             device,
                                             "pPresentInfo->swapchainCount");
        }
        if (!pPresentInfo->pSwapchains) {
            skipCall |= LOG_ERROR_NULL_POINTER(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                                               device,
                                               "pPresentInfo->pSwapchains");
        }
        if (!pPresentInfo->pImageIndices) {
            skipCall |= LOG_ERROR_NULL_POINTER(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                                               device,
                                               "pPresentInfo->pImageIndices");
        }
        // Note: pPresentInfo->pResults is allowed to be NULL
    }

    loader_platform_thread_lock_mutex(&globalLock);
    for (uint32_t i = 0;
         pPresentInfo && (i < pPresentInfo->swapchainCount);
         i++) {
        uint32_t index = pPresentInfo->pImageIndices[i];
        SwpSwapchain *pSwapchain =
            &my_data->swapchainMap[pPresentInfo->pSwapchains[i]];
        if (pSwapchain) {
            if (!pSwapchain->pDevice->swapchainExtensionEnabled) {
                skipCall |= LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                                      pSwapchain->pDevice, "VkDevice",
                                      SWAPCHAIN_EXT_NOT_ENABLED_BUT_USED,
                                      "%s() called even though the %s extension was not enabled for this VkDevice.",
                                      __FUNCTION__, VK_KHR_SWAPCHAIN_EXTENSION_NAME);
            }
            if (index >= pSwapchain->imageCount) {
                skipCall |= LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT,
                                      pPresentInfo->pSwapchains[i],
                                      "VkSwapchainKHR",
                                      SWAPCHAIN_INDEX_TOO_LARGE,
                                      "%s() called for an index that is too "
                                      "large (i.e. %d).  There are only %d "
                                      "images in this VkSwapchainKHR.\n",
                                      __FUNCTION__, index,
                                      pSwapchain->imageCount);
            } else {
                if (!pSwapchain->images[index].ownedByApp) {
                    skipCall |= LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT,
                                          pPresentInfo->pSwapchains[i],
                                          "VkSwapchainKHR",
                                          SWAPCHAIN_INDEX_NOT_IN_USE,
                                          "%s() returned an index (i.e. %d) "
                                          "for an image that is not owned by "
                                          "the application.",
                                          __FUNCTION__, index);
                }
            }
            SwpQueue *pQueue = &my_data->queueMap[queue];
            SwpSurface *pSurface = pSwapchain->pSurface;
            if (pQueue && pSurface && pSurface->numQueueFamilyIndexSupport) {
                uint32_t queueFamilyIndex = pQueue->queueFamilyIndex;
                // Note: the 1st test is to ensure queueFamilyIndex is in range,
                // and the 2nd test is the validation check:
                if ((pSurface->numQueueFamilyIndexSupport > queueFamilyIndex) &&
                    (!pSurface->pQueueFamilyIndexSupport[queueFamilyIndex])) {
                    skipCall |= LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT,
                                          pPresentInfo->pSwapchains[i],
                                          "VkSwapchainKHR",
                                          SWAPCHAIN_SURFACE_NOT_SUPPORTED_WITH_QUEUE,
                                          "%s() called with a swapchain whose "
                                          "surface is not supported for "
                                          "presention on this device with the "
                                          "queueFamilyIndex (i.e. %d) of the "
                                          "given queue.",
                                          __FUNCTION__, queueFamilyIndex);
                }
            }
        }
    }

    if (VK_FALSE == skipCall) {
        // Call down the call chain:
        loader_platform_thread_unlock_mutex(&globalLock);
        result = my_data->device_dispatch_table->QueuePresentKHR(queue,
                                                                 pPresentInfo);
        loader_platform_thread_lock_mutex(&globalLock);

        if (pPresentInfo &&
            ((result == VK_SUCCESS) || (result == VK_SUBOPTIMAL_KHR))) {
            for (uint32_t i = 0; i < pPresentInfo->swapchainCount ; i++) {
                int index = pPresentInfo->pImageIndices[i];
                SwpSwapchain *pSwapchain =
                    &my_data->swapchainMap[pPresentInfo->pSwapchains[i]];
                if (pSwapchain) {
                    // Change the state of the image (no longer owned by the
                    // application):
                    pSwapchain->images[index].ownedByApp = false;
                }
            }
        }
        loader_platform_thread_unlock_mutex(&globalLock);
        return result;
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    return VK_ERROR_VALIDATION_FAILED_EXT;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetDeviceQueue(
    VkDevice                                    device,
    uint32_t                                    queueFamilyIndex,
    uint32_t                                    queueIndex,
    VkQueue*                                    pQueue)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);

    if (VK_FALSE == skipCall) {
        // Call down the call chain:
        my_data->device_dispatch_table->GetDeviceQueue(
                device, queueFamilyIndex, queueIndex, pQueue);

        // Remember the queue's handle, and link it to the device:
        loader_platform_thread_lock_mutex(&globalLock);
        SwpDevice *pDevice = &my_data->deviceMap[device];
        my_data->queueMap[&pQueue].queue = *pQueue;
        if (pDevice) {
            pDevice->queues[*pQueue] = &my_data->queueMap[*pQueue];
        }
        my_data->queueMap[&pQueue].pDevice = pDevice;
        my_data->queueMap[&pQueue].queueFamilyIndex = queueFamilyIndex;
        loader_platform_thread_unlock_mutex(&globalLock);
    }
}


VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugReportCallbackEXT(
        VkInstance                                      instance,
        const VkDebugReportCallbackCreateInfoEXT*       pCreateInfo,
        const VkAllocationCallbacks*                    pAllocator,
        VkDebugReportCallbackEXT*                       pMsgCallback)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    VkResult result = my_data->instance_dispatch_table->CreateDebugReportCallbackEXT(instance, pCreateInfo, pAllocator, pMsgCallback);
    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        result = layer_create_msg_callback(my_data->report_data, pCreateInfo, pAllocator, pMsgCallback);
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT msgCallback, const VkAllocationCallbacks *pAllocator)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    my_data->instance_dispatch_table->DestroyDebugReportCallbackEXT(instance, msgCallback, pAllocator);
    loader_platform_thread_lock_mutex(&globalLock);
    layer_destroy_msg_callback(my_data->report_data, msgCallback, pAllocator);
    loader_platform_thread_unlock_mutex(&globalLock);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDebugReportMessageEXT(
        VkInstance                                  instance,
        VkDebugReportFlagsEXT                       flags,
        VkDebugReportObjectTypeEXT                  objType,
        uint64_t                                    object,
        size_t                                      location,
        int32_t                                     msgCode,
        const char*                                 pLayerPrefix,
        const char*                                 pMsg)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    my_data->instance_dispatch_table->DebugReportMessageEXT(instance, flags, objType, object, location, msgCode, pLayerPrefix, pMsg);
}

VK_LAYER_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr(VkDevice device, const char* funcName)
{
    if (!strcmp("vkGetDeviceProcAddr", funcName))
        return (PFN_vkVoidFunction) vkGetDeviceProcAddr;
    if (!strcmp(funcName, "vkDestroyDevice"))
        return (PFN_vkVoidFunction) vkDestroyDevice;

    if (device == VK_NULL_HANDLE) {
        return NULL;
    }

    layer_data *my_data;

    my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkLayerDispatchTable *pDisp =  my_data->device_dispatch_table;
    if (my_data->deviceMap.size() != 0 &&
        my_data->deviceMap[device].swapchainExtensionEnabled)
    {
        if (!strcmp("vkCreateSwapchainKHR", funcName))
            return reinterpret_cast<PFN_vkVoidFunction>(vkCreateSwapchainKHR);
        if (!strcmp("vkDestroySwapchainKHR", funcName))
            return reinterpret_cast<PFN_vkVoidFunction>(vkDestroySwapchainKHR);
        if (!strcmp("vkGetSwapchainImagesKHR", funcName))
            return reinterpret_cast<PFN_vkVoidFunction>(vkGetSwapchainImagesKHR);
        if (!strcmp("vkAcquireNextImageKHR", funcName))
            return reinterpret_cast<PFN_vkVoidFunction>(vkAcquireNextImageKHR);
        if (!strcmp("vkQueuePresentKHR", funcName))
            return reinterpret_cast<PFN_vkVoidFunction>(vkQueuePresentKHR);
    }
    if (!strcmp("vkGetDeviceQueue", funcName))
        return reinterpret_cast<PFN_vkVoidFunction>(vkGetDeviceQueue);

    if (pDisp->GetDeviceProcAddr == NULL)
        return NULL;
    return pDisp->GetDeviceProcAddr(device, funcName);
}

VK_LAYER_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance instance, const char* funcName)
{
    if (!strcmp("vkGetInstanceProcAddr", funcName))
        return (PFN_vkVoidFunction) vkGetInstanceProcAddr;
    if (!strcmp(funcName, "vkCreateInstance"))
        return (PFN_vkVoidFunction) vkCreateInstance;
    if (!strcmp(funcName, "vkDestroyInstance"))
        return (PFN_vkVoidFunction) vkDestroyInstance;
    if (!strcmp(funcName, "vkCreateDevice"))
        return (PFN_vkVoidFunction) vkCreateDevice;
    if (!strcmp(funcName, "vkEnumeratePhysicalDevices"))
        return (PFN_vkVoidFunction) vkEnumeratePhysicalDevices;
    if (!strcmp(funcName, "vkEnumerateInstanceLayerProperties"))
        return (PFN_vkVoidFunction) vkEnumerateInstanceLayerProperties;
    if (!strcmp(funcName, "vkEnumerateDeviceLayerProperties"))
        return (PFN_vkVoidFunction)vkEnumerateDeviceLayerProperties;
    if (!strcmp(funcName, "vkEnumerateInstanceExtensionProperties"))
        return (PFN_vkVoidFunction) vkEnumerateInstanceExtensionProperties;
    if (!strcmp(funcName, "vkEnumerateDeviceExtensionProperties"))
        return (PFN_vkVoidFunction)vkEnumerateDeviceExtensionProperties;
    if (!strcmp(funcName, "vkGetPhysicalDeviceQueueFamilyProperties"))
        return (PFN_vkVoidFunction) vkGetPhysicalDeviceQueueFamilyProperties;

    if (instance == VK_NULL_HANDLE) {
        return NULL;
    }

    PFN_vkVoidFunction addr;

    layer_data *my_data;
    my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    VkLayerInstanceDispatchTable* pTable = my_data->instance_dispatch_table;
    addr = debug_report_get_instance_proc_addr(my_data->report_data, funcName);
    if (addr) {
        return addr;
    }

#ifdef VK_USE_PLATFORM_ANDROID_KHR
    if (my_data->instanceMap.size() != 0 &&
        my_data->instanceMap[instance].androidSurfaceExtensionEnabled)
    {
        if (!strcmp("vkCreateAndroidSurfaceKHR", funcName))
            return reinterpret_cast<PFN_vkVoidFunction>(vkCreateAndroidSurfaceKHR);
    }
#endif // VK_USE_PLATFORM_ANDROID_KHR
#ifdef VK_USE_PLATFORM_MIR_KHR
    if (my_data->instanceMap.size() != 0 &&
        my_data->instanceMap[instance].mirSurfaceExtensionEnabled)
    {
        if (!strcmp("vkCreateMirSurfaceKHR", funcName))
            return reinterpret_cast<PFN_vkVoidFunction>(vkCreateMirSurfaceKHR);
        if (!strcmp("vkGetPhysicalDeviceMirPresentationSupportKHR", funcName))
            return reinterpret_cast<PFN_vkVoidFunction>(vkGetPhysicalDeviceMirPresentationSupportKHR);
    }
#endif // VK_USE_PLATFORM_MIR_KHR
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
    if (my_data->instanceMap.size() != 0 &&
        my_data->instanceMap[instance].waylandSurfaceExtensionEnabled)
    {
        if (!strcmp("vkCreateWaylandSurfaceKHR", funcName))
            return reinterpret_cast<PFN_vkVoidFunction>(vkCreateWaylandSurfaceKHR);
        if (!strcmp("vkGetPhysicalDeviceWaylandPresentationSupportKHR", funcName))
            return reinterpret_cast<PFN_vkVoidFunction>(vkGetPhysicalDeviceWaylandPresentationSupportKHR);
    }
#endif // VK_USE_PLATFORM_WAYLAND_KHR
#ifdef VK_USE_PLATFORM_WIN32_KHR
    if (my_data->instanceMap.size() != 0 &&
        my_data->instanceMap[instance].win32SurfaceExtensionEnabled)
    {
        if (!strcmp("vkCreateWin32SurfaceKHR", funcName))
            return reinterpret_cast<PFN_vkVoidFunction>(vkCreateWin32SurfaceKHR);
        if (!strcmp("vkGetPhysicalDeviceWin32PresentationSupportKHR", funcName))
            return reinterpret_cast<PFN_vkVoidFunction>(vkGetPhysicalDeviceWin32PresentationSupportKHR);
    }
#endif // VK_USE_PLATFORM_WIN32_KHR
#ifdef VK_USE_PLATFORM_XCB_KHR
    if (my_data->instanceMap.size() != 0 &&
        my_data->instanceMap[instance].xcbSurfaceExtensionEnabled)
    {
        if (!strcmp("vkCreateXcbSurfaceKHR", funcName))
            return reinterpret_cast<PFN_vkVoidFunction>(vkCreateXcbSurfaceKHR);
        if (!strcmp("vkGetPhysicalDeviceXcbPresentationSupportKHR", funcName))
            return reinterpret_cast<PFN_vkVoidFunction>(vkGetPhysicalDeviceXcbPresentationSupportKHR);
    }
#endif // VK_USE_PLATFORM_XCB_KHR
#ifdef VK_USE_PLATFORM_XLIB_KHR
    if (my_data->instanceMap.size() != 0 &&
        my_data->instanceMap[instance].xlibSurfaceExtensionEnabled)
    {
        if (!strcmp("vkCreateXlibSurfaceKHR", funcName))
            return reinterpret_cast<PFN_vkVoidFunction>(vkCreateXlibSurfaceKHR);
        if (!strcmp("vkGetPhysicalDeviceXlibPresentationSupportKHR", funcName))
            return reinterpret_cast<PFN_vkVoidFunction>(vkGetPhysicalDeviceXlibPresentationSupportKHR);
    }
#endif // VK_USE_PLATFORM_XLIB_KHR
    if (my_data->instanceMap.size() != 0 &&
        my_data->instanceMap[instance].surfaceExtensionEnabled)
    {
        if (!strcmp("vkDestroySurfaceKHR", funcName))
            return reinterpret_cast<PFN_vkVoidFunction>(vkDestroySurfaceKHR);
        if (!strcmp("vkGetPhysicalDeviceSurfaceSupportKHR", funcName))
            return reinterpret_cast<PFN_vkVoidFunction>(vkGetPhysicalDeviceSurfaceSupportKHR);
        if (!strcmp("vkGetPhysicalDeviceSurfaceCapabilitiesKHR", funcName))
            return reinterpret_cast<PFN_vkVoidFunction>(vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
        if (!strcmp("vkGetPhysicalDeviceSurfaceFormatsKHR", funcName))
            return reinterpret_cast<PFN_vkVoidFunction>(vkGetPhysicalDeviceSurfaceFormatsKHR);
        if (!strcmp("vkGetPhysicalDeviceSurfacePresentModesKHR", funcName))
            return reinterpret_cast<PFN_vkVoidFunction>(vkGetPhysicalDeviceSurfacePresentModesKHR);
    }

    if (pTable->GetInstanceProcAddr == NULL)
        return NULL;
    return pTable->GetInstanceProcAddr(instance, funcName);
}

