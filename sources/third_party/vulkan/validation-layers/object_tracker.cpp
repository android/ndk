/* THIS FILE IS GENERATED.  DO NOT EDIT. */

/*
 * Copyright (c) 2015-2016 The Khronos Group Inc.
 * Copyright (c) 2015-2016 Valve Corporation
 * Copyright (c) 2015-2016 LunarG, Inc.
 * Copyright (c) 2015 Google, Inc.
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
 * Author: Tobin Ehlis <tobine@google.com>
 * Author: Courtney Goeltzenleuchter <courtneygo@google.com>
 * Author: Jon Ashburn <jon@lunarg.com>
 * Author: Mark Lobodzinski <mark@lunarg.com>
 * Author: Mike Stroyan <stroyan@google.com>
 * Author: Tony Barbour <tony@LunarG.com>
 */

// CODEGEN : file ../vk-layer-generate.py line #785
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "vulkan/vulkan.h"
#include "vk_loader_platform.h"

#include <unordered_map>
using namespace std;
#include "vulkan/vk_layer.h"
#include "vk_layer_config.h"
#include "vk_layer_table.h"
#include "vk_layer_data.h"
#include "vk_layer_logging.h"

#include "object_tracker.h"


unordered_map<uint64_t, OBJTRACK_NODE*> VkInstanceMap;
unordered_map<uint64_t, OBJTRACK_NODE*> VkPhysicalDeviceMap;
unordered_map<uint64_t, OBJTRACK_NODE*> VkDeviceMap;
unordered_map<uint64_t, OBJTRACK_NODE*> VkQueueMap;
unordered_map<uint64_t, OBJTRACK_NODE*> VkCommandBufferMap;
unordered_map<uint64_t, OBJTRACK_NODE*> VkCommandPoolMap;
unordered_map<uint64_t, OBJTRACK_NODE*> VkFenceMap;
unordered_map<uint64_t, OBJTRACK_NODE*> VkDeviceMemoryMap;
unordered_map<uint64_t, OBJTRACK_NODE*> VkBufferMap;
unordered_map<uint64_t, OBJTRACK_NODE*> VkImageMap;
unordered_map<uint64_t, OBJTRACK_NODE*> VkSemaphoreMap;
unordered_map<uint64_t, OBJTRACK_NODE*> VkEventMap;
unordered_map<uint64_t, OBJTRACK_NODE*> VkQueryPoolMap;
unordered_map<uint64_t, OBJTRACK_NODE*> VkBufferViewMap;
unordered_map<uint64_t, OBJTRACK_NODE*> VkImageViewMap;
unordered_map<uint64_t, OBJTRACK_NODE*> VkShaderModuleMap;
unordered_map<uint64_t, OBJTRACK_NODE*> VkPipelineCacheMap;
unordered_map<uint64_t, OBJTRACK_NODE*> VkPipelineLayoutMap;
unordered_map<uint64_t, OBJTRACK_NODE*> VkPipelineMap;
unordered_map<uint64_t, OBJTRACK_NODE*> VkDescriptorSetLayoutMap;
unordered_map<uint64_t, OBJTRACK_NODE*> VkSamplerMap;
unordered_map<uint64_t, OBJTRACK_NODE*> VkDescriptorPoolMap;
unordered_map<uint64_t, OBJTRACK_NODE*> VkDescriptorSetMap;
unordered_map<uint64_t, OBJTRACK_NODE*> VkRenderPassMap;
unordered_map<uint64_t, OBJTRACK_NODE*> VkFramebufferMap;
unordered_map<uint64_t, OBJTRACK_NODE*> VkSwapchainKHRMap;
unordered_map<uint64_t, OBJTRACK_NODE*> VkSurfaceKHRMap;
unordered_map<uint64_t, OBJTRACK_NODE*> VkDebugReportCallbackEXTMap;

// CODEGEN : file ../vk-layer-generate.py line #844
static void create_instance(VkInstance dispatchable_object, VkInstance vkObj, VkDebugReportObjectTypeEXT objType)
{
    log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, objType,(uint64_t)(vkObj), __LINE__, OBJTRACK_NONE, "OBJTRACK",
        "OBJ[%llu] : CREATE %s object 0x%" PRIxLEAST64 , object_track_index++, string_VkDebugReportObjectTypeEXT(objType),
        (uint64_t)(vkObj));

    OBJTRACK_NODE* pNewObjNode = new OBJTRACK_NODE;
    pNewObjNode->objType = objType;
    pNewObjNode->status  = OBJSTATUS_NONE;
    pNewObjNode->vkObj  = (uint64_t)(vkObj);
    VkInstanceMap[(uint64_t)vkObj] = pNewObjNode;
    uint32_t objIndex = objTypeToIndex(objType);
    numObjs[objIndex]++;
    numTotalObjs++;
}

// CODEGEN : file ../vk-layer-generate.py line #866
static void destroy_instance(VkInstance dispatchable_object, VkInstance object)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkInstanceMap.find(object_handle) != VkInstanceMap.end()) {
        OBJTRACK_NODE* pNode = VkInstanceMap[(uint64_t)object];
        uint32_t objIndex = objTypeToIndex(pNode->objType);
        assert(numTotalObjs > 0);
        numTotalObjs--;
        assert(numObjs[objIndex] > 0);
        numObjs[objIndex]--;
        log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, pNode->objType, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
           "OBJ_STAT Destroy %s obj 0x%" PRIxLEAST64 " (%" PRIu64 " total objs remain & %" PRIu64 " %s objs).",
            string_VkDebugReportObjectTypeEXT(pNode->objType), (uint64_t)(object), numTotalObjs, numObjs[objIndex],
            string_VkDebugReportObjectTypeEXT(pNode->objType));
        delete pNode;
        VkInstanceMap.erase(object_handle);
    } else {
        log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT ) 0, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
            "Unable to remove obj 0x%" PRIxLEAST64 ". Was it created? Has it already been destroyed?",
           object_handle);
    }
}

// CODEGEN : file ../vk-layer-generate.py line #893
static VkBool32 set_instance_status(VkInstance dispatchable_object, VkInstance object, VkDebugReportObjectTypeEXT objType, ObjectStatusFlags status_flag)
{
    if (object != VK_NULL_HANDLE) {
        uint64_t object_handle = (uint64_t)(object);
        if (VkInstanceMap.find(object_handle) != VkInstanceMap.end()) {
            OBJTRACK_NODE* pNode = VkInstanceMap[object_handle];
            pNode->status |= status_flag;
        }
        else {
            // If we do not find it print an error
            return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT ) 0, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
                "Unable to set status for non-existent object 0x%" PRIxLEAST64 " of %s type",
                object_handle, string_VkDebugReportObjectTypeEXT(objType));
        }
    }
    return VK_FALSE;
}

// CODEGEN : file ../vk-layer-generate.py line #915
static VkBool32 validate_instance_status(
VkInstance dispatchable_object, VkInstance object,
    VkDebugReportObjectTypeEXT     objType,
    ObjectStatusFlags   status_mask,
    ObjectStatusFlags   status_flag,
    VkFlags             msg_flags,
    OBJECT_TRACK_ERROR  error_code,
    const char         *fail_msg)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkInstanceMap.find(object_handle) != VkInstanceMap.end()) {
        OBJTRACK_NODE* pNode = VkInstanceMap[object_handle];
        if ((pNode->status & status_mask) != status_flag) {
            log_msg(mdd(dispatchable_object), msg_flags, pNode->objType, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
                "OBJECT VALIDATION WARNING: %s object 0x%" PRIxLEAST64 ": %s", string_VkDebugReportObjectTypeEXT(objType),
                 object_handle, fail_msg);
            return VK_FALSE;
        }
        return VK_TRUE;
    }
    else {
        // If we do not find it print an error
        log_msg(mdd(dispatchable_object), msg_flags, (VkDebugReportObjectTypeEXT) 0, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
            "Unable to obtain status for non-existent object 0x%" PRIxLEAST64 " of %s type",
            object_handle, string_VkDebugReportObjectTypeEXT(objType));
        return VK_FALSE;
    }
}

// CODEGEN : file ../vk-layer-generate.py line #948
static VkBool32 reset_instance_status(VkInstance dispatchable_object, VkInstance object, VkDebugReportObjectTypeEXT objType, ObjectStatusFlags status_flag)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkInstanceMap.find(object_handle) != VkInstanceMap.end()) {
        OBJTRACK_NODE* pNode = VkInstanceMap[object_handle];
        pNode->status &= ~status_flag;
    }
    else {
        // If we do not find it print an error
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
            "Unable to reset status for non-existent object 0x%" PRIxLEAST64 " of %s type",
            object_handle, string_VkDebugReportObjectTypeEXT(objType));
    }
    return VK_FALSE;
}

// CODEGEN : file ../vk-layer-generate.py line #844
static void create_physical_device(VkPhysicalDevice dispatchable_object, VkPhysicalDevice vkObj, VkDebugReportObjectTypeEXT objType)
{
    log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, objType,(uint64_t)(vkObj), __LINE__, OBJTRACK_NONE, "OBJTRACK",
        "OBJ[%llu] : CREATE %s object 0x%" PRIxLEAST64 , object_track_index++, string_VkDebugReportObjectTypeEXT(objType),
        (uint64_t)(vkObj));

    OBJTRACK_NODE* pNewObjNode = new OBJTRACK_NODE;
    pNewObjNode->objType = objType;
    pNewObjNode->status  = OBJSTATUS_NONE;
    pNewObjNode->vkObj  = (uint64_t)(vkObj);
    VkPhysicalDeviceMap[(uint64_t)vkObj] = pNewObjNode;
    uint32_t objIndex = objTypeToIndex(objType);
    numObjs[objIndex]++;
    numTotalObjs++;
}

// CODEGEN : file ../vk-layer-generate.py line #866
static void destroy_physical_device(VkPhysicalDevice dispatchable_object, VkPhysicalDevice object)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkPhysicalDeviceMap.find(object_handle) != VkPhysicalDeviceMap.end()) {
        OBJTRACK_NODE* pNode = VkPhysicalDeviceMap[(uint64_t)object];
        uint32_t objIndex = objTypeToIndex(pNode->objType);
        assert(numTotalObjs > 0);
        numTotalObjs--;
        assert(numObjs[objIndex] > 0);
        numObjs[objIndex]--;
        log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, pNode->objType, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
           "OBJ_STAT Destroy %s obj 0x%" PRIxLEAST64 " (%" PRIu64 " total objs remain & %" PRIu64 " %s objs).",
            string_VkDebugReportObjectTypeEXT(pNode->objType), (uint64_t)(object), numTotalObjs, numObjs[objIndex],
            string_VkDebugReportObjectTypeEXT(pNode->objType));
        delete pNode;
        VkPhysicalDeviceMap.erase(object_handle);
    } else {
        log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT ) 0, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
            "Unable to remove obj 0x%" PRIxLEAST64 ". Was it created? Has it already been destroyed?",
           object_handle);
    }
}

// CODEGEN : file ../vk-layer-generate.py line #893
static VkBool32 set_physical_device_status(VkPhysicalDevice dispatchable_object, VkPhysicalDevice object, VkDebugReportObjectTypeEXT objType, ObjectStatusFlags status_flag)
{
    if (object != VK_NULL_HANDLE) {
        uint64_t object_handle = (uint64_t)(object);
        if (VkPhysicalDeviceMap.find(object_handle) != VkPhysicalDeviceMap.end()) {
            OBJTRACK_NODE* pNode = VkPhysicalDeviceMap[object_handle];
            pNode->status |= status_flag;
        }
        else {
            // If we do not find it print an error
            return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT ) 0, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
                "Unable to set status for non-existent object 0x%" PRIxLEAST64 " of %s type",
                object_handle, string_VkDebugReportObjectTypeEXT(objType));
        }
    }
    return VK_FALSE;
}

// CODEGEN : file ../vk-layer-generate.py line #915
static VkBool32 validate_physical_device_status(
VkPhysicalDevice dispatchable_object, VkPhysicalDevice object,
    VkDebugReportObjectTypeEXT     objType,
    ObjectStatusFlags   status_mask,
    ObjectStatusFlags   status_flag,
    VkFlags             msg_flags,
    OBJECT_TRACK_ERROR  error_code,
    const char         *fail_msg)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkPhysicalDeviceMap.find(object_handle) != VkPhysicalDeviceMap.end()) {
        OBJTRACK_NODE* pNode = VkPhysicalDeviceMap[object_handle];
        if ((pNode->status & status_mask) != status_flag) {
            log_msg(mdd(dispatchable_object), msg_flags, pNode->objType, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
                "OBJECT VALIDATION WARNING: %s object 0x%" PRIxLEAST64 ": %s", string_VkDebugReportObjectTypeEXT(objType),
                 object_handle, fail_msg);
            return VK_FALSE;
        }
        return VK_TRUE;
    }
    else {
        // If we do not find it print an error
        log_msg(mdd(dispatchable_object), msg_flags, (VkDebugReportObjectTypeEXT) 0, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
            "Unable to obtain status for non-existent object 0x%" PRIxLEAST64 " of %s type",
            object_handle, string_VkDebugReportObjectTypeEXT(objType));
        return VK_FALSE;
    }
}

// CODEGEN : file ../vk-layer-generate.py line #948
static VkBool32 reset_physical_device_status(VkPhysicalDevice dispatchable_object, VkPhysicalDevice object, VkDebugReportObjectTypeEXT objType, ObjectStatusFlags status_flag)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkPhysicalDeviceMap.find(object_handle) != VkPhysicalDeviceMap.end()) {
        OBJTRACK_NODE* pNode = VkPhysicalDeviceMap[object_handle];
        pNode->status &= ~status_flag;
    }
    else {
        // If we do not find it print an error
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
            "Unable to reset status for non-existent object 0x%" PRIxLEAST64 " of %s type",
            object_handle, string_VkDebugReportObjectTypeEXT(objType));
    }
    return VK_FALSE;
}

// CODEGEN : file ../vk-layer-generate.py line #844
static void create_device(VkDevice dispatchable_object, VkDevice vkObj, VkDebugReportObjectTypeEXT objType)
{
    log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, objType,(uint64_t)(vkObj), __LINE__, OBJTRACK_NONE, "OBJTRACK",
        "OBJ[%llu] : CREATE %s object 0x%" PRIxLEAST64 , object_track_index++, string_VkDebugReportObjectTypeEXT(objType),
        (uint64_t)(vkObj));

    OBJTRACK_NODE* pNewObjNode = new OBJTRACK_NODE;
    pNewObjNode->objType = objType;
    pNewObjNode->status  = OBJSTATUS_NONE;
    pNewObjNode->vkObj  = (uint64_t)(vkObj);
    VkDeviceMap[(uint64_t)vkObj] = pNewObjNode;
    uint32_t objIndex = objTypeToIndex(objType);
    numObjs[objIndex]++;
    numTotalObjs++;
}

// CODEGEN : file ../vk-layer-generate.py line #866
static void destroy_device(VkDevice dispatchable_object, VkDevice object)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkDeviceMap.find(object_handle) != VkDeviceMap.end()) {
        OBJTRACK_NODE* pNode = VkDeviceMap[(uint64_t)object];
        uint32_t objIndex = objTypeToIndex(pNode->objType);
        assert(numTotalObjs > 0);
        numTotalObjs--;
        assert(numObjs[objIndex] > 0);
        numObjs[objIndex]--;
        log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, pNode->objType, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
           "OBJ_STAT Destroy %s obj 0x%" PRIxLEAST64 " (%" PRIu64 " total objs remain & %" PRIu64 " %s objs).",
            string_VkDebugReportObjectTypeEXT(pNode->objType), (uint64_t)(object), numTotalObjs, numObjs[objIndex],
            string_VkDebugReportObjectTypeEXT(pNode->objType));
        delete pNode;
        VkDeviceMap.erase(object_handle);
    } else {
        log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT ) 0, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
            "Unable to remove obj 0x%" PRIxLEAST64 ". Was it created? Has it already been destroyed?",
           object_handle);
    }
}

// CODEGEN : file ../vk-layer-generate.py line #893
static VkBool32 set_device_status(VkDevice dispatchable_object, VkDevice object, VkDebugReportObjectTypeEXT objType, ObjectStatusFlags status_flag)
{
    if (object != VK_NULL_HANDLE) {
        uint64_t object_handle = (uint64_t)(object);
        if (VkDeviceMap.find(object_handle) != VkDeviceMap.end()) {
            OBJTRACK_NODE* pNode = VkDeviceMap[object_handle];
            pNode->status |= status_flag;
        }
        else {
            // If we do not find it print an error
            return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT ) 0, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
                "Unable to set status for non-existent object 0x%" PRIxLEAST64 " of %s type",
                object_handle, string_VkDebugReportObjectTypeEXT(objType));
        }
    }
    return VK_FALSE;
}

// CODEGEN : file ../vk-layer-generate.py line #915
static VkBool32 validate_device_status(
VkDevice dispatchable_object, VkDevice object,
    VkDebugReportObjectTypeEXT     objType,
    ObjectStatusFlags   status_mask,
    ObjectStatusFlags   status_flag,
    VkFlags             msg_flags,
    OBJECT_TRACK_ERROR  error_code,
    const char         *fail_msg)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkDeviceMap.find(object_handle) != VkDeviceMap.end()) {
        OBJTRACK_NODE* pNode = VkDeviceMap[object_handle];
        if ((pNode->status & status_mask) != status_flag) {
            log_msg(mdd(dispatchable_object), msg_flags, pNode->objType, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
                "OBJECT VALIDATION WARNING: %s object 0x%" PRIxLEAST64 ": %s", string_VkDebugReportObjectTypeEXT(objType),
                 object_handle, fail_msg);
            return VK_FALSE;
        }
        return VK_TRUE;
    }
    else {
        // If we do not find it print an error
        log_msg(mdd(dispatchable_object), msg_flags, (VkDebugReportObjectTypeEXT) 0, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
            "Unable to obtain status for non-existent object 0x%" PRIxLEAST64 " of %s type",
            object_handle, string_VkDebugReportObjectTypeEXT(objType));
        return VK_FALSE;
    }
}

// CODEGEN : file ../vk-layer-generate.py line #948
static VkBool32 reset_device_status(VkDevice dispatchable_object, VkDevice object, VkDebugReportObjectTypeEXT objType, ObjectStatusFlags status_flag)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkDeviceMap.find(object_handle) != VkDeviceMap.end()) {
        OBJTRACK_NODE* pNode = VkDeviceMap[object_handle];
        pNode->status &= ~status_flag;
    }
    else {
        // If we do not find it print an error
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
            "Unable to reset status for non-existent object 0x%" PRIxLEAST64 " of %s type",
            object_handle, string_VkDebugReportObjectTypeEXT(objType));
    }
    return VK_FALSE;
}

// CODEGEN : file ../vk-layer-generate.py line #844
static void create_queue(VkQueue dispatchable_object, VkQueue vkObj, VkDebugReportObjectTypeEXT objType)
{
    log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, objType,(uint64_t)(vkObj), __LINE__, OBJTRACK_NONE, "OBJTRACK",
        "OBJ[%llu] : CREATE %s object 0x%" PRIxLEAST64 , object_track_index++, string_VkDebugReportObjectTypeEXT(objType),
        (uint64_t)(vkObj));

    OBJTRACK_NODE* pNewObjNode = new OBJTRACK_NODE;
    pNewObjNode->objType = objType;
    pNewObjNode->status  = OBJSTATUS_NONE;
    pNewObjNode->vkObj  = (uint64_t)(vkObj);
    VkQueueMap[(uint64_t)vkObj] = pNewObjNode;
    uint32_t objIndex = objTypeToIndex(objType);
    numObjs[objIndex]++;
    numTotalObjs++;
}

// CODEGEN : file ../vk-layer-generate.py line #866
static void destroy_queue(VkQueue dispatchable_object, VkQueue object)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkQueueMap.find(object_handle) != VkQueueMap.end()) {
        OBJTRACK_NODE* pNode = VkQueueMap[(uint64_t)object];
        uint32_t objIndex = objTypeToIndex(pNode->objType);
        assert(numTotalObjs > 0);
        numTotalObjs--;
        assert(numObjs[objIndex] > 0);
        numObjs[objIndex]--;
        log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, pNode->objType, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
           "OBJ_STAT Destroy %s obj 0x%" PRIxLEAST64 " (%" PRIu64 " total objs remain & %" PRIu64 " %s objs).",
            string_VkDebugReportObjectTypeEXT(pNode->objType), (uint64_t)(object), numTotalObjs, numObjs[objIndex],
            string_VkDebugReportObjectTypeEXT(pNode->objType));
        delete pNode;
        VkQueueMap.erase(object_handle);
    } else {
        log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT ) 0, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
            "Unable to remove obj 0x%" PRIxLEAST64 ". Was it created? Has it already been destroyed?",
           object_handle);
    }
}

// CODEGEN : file ../vk-layer-generate.py line #893
static VkBool32 set_queue_status(VkQueue dispatchable_object, VkQueue object, VkDebugReportObjectTypeEXT objType, ObjectStatusFlags status_flag)
{
    if (object != VK_NULL_HANDLE) {
        uint64_t object_handle = (uint64_t)(object);
        if (VkQueueMap.find(object_handle) != VkQueueMap.end()) {
            OBJTRACK_NODE* pNode = VkQueueMap[object_handle];
            pNode->status |= status_flag;
        }
        else {
            // If we do not find it print an error
            return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT ) 0, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
                "Unable to set status for non-existent object 0x%" PRIxLEAST64 " of %s type",
                object_handle, string_VkDebugReportObjectTypeEXT(objType));
        }
    }
    return VK_FALSE;
}

// CODEGEN : file ../vk-layer-generate.py line #915
static VkBool32 validate_queue_status(
VkQueue dispatchable_object, VkQueue object,
    VkDebugReportObjectTypeEXT     objType,
    ObjectStatusFlags   status_mask,
    ObjectStatusFlags   status_flag,
    VkFlags             msg_flags,
    OBJECT_TRACK_ERROR  error_code,
    const char         *fail_msg)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkQueueMap.find(object_handle) != VkQueueMap.end()) {
        OBJTRACK_NODE* pNode = VkQueueMap[object_handle];
        if ((pNode->status & status_mask) != status_flag) {
            log_msg(mdd(dispatchable_object), msg_flags, pNode->objType, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
                "OBJECT VALIDATION WARNING: %s object 0x%" PRIxLEAST64 ": %s", string_VkDebugReportObjectTypeEXT(objType),
                 object_handle, fail_msg);
            return VK_FALSE;
        }
        return VK_TRUE;
    }
    else {
        // If we do not find it print an error
        log_msg(mdd(dispatchable_object), msg_flags, (VkDebugReportObjectTypeEXT) 0, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
            "Unable to obtain status for non-existent object 0x%" PRIxLEAST64 " of %s type",
            object_handle, string_VkDebugReportObjectTypeEXT(objType));
        return VK_FALSE;
    }
}

// CODEGEN : file ../vk-layer-generate.py line #948
static VkBool32 reset_queue_status(VkQueue dispatchable_object, VkQueue object, VkDebugReportObjectTypeEXT objType, ObjectStatusFlags status_flag)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkQueueMap.find(object_handle) != VkQueueMap.end()) {
        OBJTRACK_NODE* pNode = VkQueueMap[object_handle];
        pNode->status &= ~status_flag;
    }
    else {
        // If we do not find it print an error
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
            "Unable to reset status for non-existent object 0x%" PRIxLEAST64 " of %s type",
            object_handle, string_VkDebugReportObjectTypeEXT(objType));
    }
    return VK_FALSE;
}

// CODEGEN : file ../vk-layer-generate.py line #844
static void create_command_buffer(VkCommandBuffer dispatchable_object, VkCommandBuffer vkObj, VkDebugReportObjectTypeEXT objType)
{
    log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, objType,(uint64_t)(vkObj), __LINE__, OBJTRACK_NONE, "OBJTRACK",
        "OBJ[%llu] : CREATE %s object 0x%" PRIxLEAST64 , object_track_index++, string_VkDebugReportObjectTypeEXT(objType),
        (uint64_t)(vkObj));

    OBJTRACK_NODE* pNewObjNode = new OBJTRACK_NODE;
    pNewObjNode->objType = objType;
    pNewObjNode->status  = OBJSTATUS_NONE;
    pNewObjNode->vkObj  = (uint64_t)(vkObj);
    VkCommandBufferMap[(uint64_t)vkObj] = pNewObjNode;
    uint32_t objIndex = objTypeToIndex(objType);
    numObjs[objIndex]++;
    numTotalObjs++;
}

// CODEGEN : file ../vk-layer-generate.py line #866
static void destroy_command_buffer(VkCommandBuffer dispatchable_object, VkCommandBuffer object)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkCommandBufferMap.find(object_handle) != VkCommandBufferMap.end()) {
        OBJTRACK_NODE* pNode = VkCommandBufferMap[(uint64_t)object];
        uint32_t objIndex = objTypeToIndex(pNode->objType);
        assert(numTotalObjs > 0);
        numTotalObjs--;
        assert(numObjs[objIndex] > 0);
        numObjs[objIndex]--;
        log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, pNode->objType, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
           "OBJ_STAT Destroy %s obj 0x%" PRIxLEAST64 " (%" PRIu64 " total objs remain & %" PRIu64 " %s objs).",
            string_VkDebugReportObjectTypeEXT(pNode->objType), (uint64_t)(object), numTotalObjs, numObjs[objIndex],
            string_VkDebugReportObjectTypeEXT(pNode->objType));
        delete pNode;
        VkCommandBufferMap.erase(object_handle);
    } else {
        log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT ) 0, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
            "Unable to remove obj 0x%" PRIxLEAST64 ". Was it created? Has it already been destroyed?",
           object_handle);
    }
}

// CODEGEN : file ../vk-layer-generate.py line #893
static VkBool32 set_command_buffer_status(VkCommandBuffer dispatchable_object, VkCommandBuffer object, VkDebugReportObjectTypeEXT objType, ObjectStatusFlags status_flag)
{
    if (object != VK_NULL_HANDLE) {
        uint64_t object_handle = (uint64_t)(object);
        if (VkCommandBufferMap.find(object_handle) != VkCommandBufferMap.end()) {
            OBJTRACK_NODE* pNode = VkCommandBufferMap[object_handle];
            pNode->status |= status_flag;
        }
        else {
            // If we do not find it print an error
            return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT ) 0, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
                "Unable to set status for non-existent object 0x%" PRIxLEAST64 " of %s type",
                object_handle, string_VkDebugReportObjectTypeEXT(objType));
        }
    }
    return VK_FALSE;
}

// CODEGEN : file ../vk-layer-generate.py line #915
static VkBool32 validate_command_buffer_status(
VkCommandBuffer dispatchable_object, VkCommandBuffer object,
    VkDebugReportObjectTypeEXT     objType,
    ObjectStatusFlags   status_mask,
    ObjectStatusFlags   status_flag,
    VkFlags             msg_flags,
    OBJECT_TRACK_ERROR  error_code,
    const char         *fail_msg)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkCommandBufferMap.find(object_handle) != VkCommandBufferMap.end()) {
        OBJTRACK_NODE* pNode = VkCommandBufferMap[object_handle];
        if ((pNode->status & status_mask) != status_flag) {
            log_msg(mdd(dispatchable_object), msg_flags, pNode->objType, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
                "OBJECT VALIDATION WARNING: %s object 0x%" PRIxLEAST64 ": %s", string_VkDebugReportObjectTypeEXT(objType),
                 object_handle, fail_msg);
            return VK_FALSE;
        }
        return VK_TRUE;
    }
    else {
        // If we do not find it print an error
        log_msg(mdd(dispatchable_object), msg_flags, (VkDebugReportObjectTypeEXT) 0, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
            "Unable to obtain status for non-existent object 0x%" PRIxLEAST64 " of %s type",
            object_handle, string_VkDebugReportObjectTypeEXT(objType));
        return VK_FALSE;
    }
}

// CODEGEN : file ../vk-layer-generate.py line #948
static VkBool32 reset_command_buffer_status(VkCommandBuffer dispatchable_object, VkCommandBuffer object, VkDebugReportObjectTypeEXT objType, ObjectStatusFlags status_flag)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkCommandBufferMap.find(object_handle) != VkCommandBufferMap.end()) {
        OBJTRACK_NODE* pNode = VkCommandBufferMap[object_handle];
        pNode->status &= ~status_flag;
    }
    else {
        // If we do not find it print an error
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
            "Unable to reset status for non-existent object 0x%" PRIxLEAST64 " of %s type",
            object_handle, string_VkDebugReportObjectTypeEXT(objType));
    }
    return VK_FALSE;
}

// CODEGEN : file ../vk-layer-generate.py line #844
static void create_command_pool(VkDevice dispatchable_object, VkCommandPool vkObj, VkDebugReportObjectTypeEXT objType)
{
    log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, objType,(uint64_t)(vkObj), __LINE__, OBJTRACK_NONE, "OBJTRACK",
        "OBJ[%llu] : CREATE %s object 0x%" PRIxLEAST64 , object_track_index++, string_VkDebugReportObjectTypeEXT(objType),
        (uint64_t)(vkObj));

    OBJTRACK_NODE* pNewObjNode = new OBJTRACK_NODE;
    pNewObjNode->objType = objType;
    pNewObjNode->status  = OBJSTATUS_NONE;
    pNewObjNode->vkObj  = (uint64_t)(vkObj);
    VkCommandPoolMap[(uint64_t)vkObj] = pNewObjNode;
    uint32_t objIndex = objTypeToIndex(objType);
    numObjs[objIndex]++;
    numTotalObjs++;
}

// CODEGEN : file ../vk-layer-generate.py line #866
static void destroy_command_pool(VkDevice dispatchable_object, VkCommandPool object)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkCommandPoolMap.find(object_handle) != VkCommandPoolMap.end()) {
        OBJTRACK_NODE* pNode = VkCommandPoolMap[(uint64_t)object];
        uint32_t objIndex = objTypeToIndex(pNode->objType);
        assert(numTotalObjs > 0);
        numTotalObjs--;
        assert(numObjs[objIndex] > 0);
        numObjs[objIndex]--;
        log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, pNode->objType, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
           "OBJ_STAT Destroy %s obj 0x%" PRIxLEAST64 " (%" PRIu64 " total objs remain & %" PRIu64 " %s objs).",
            string_VkDebugReportObjectTypeEXT(pNode->objType), (uint64_t)(object), numTotalObjs, numObjs[objIndex],
            string_VkDebugReportObjectTypeEXT(pNode->objType));
        delete pNode;
        VkCommandPoolMap.erase(object_handle);
    } else {
        log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT ) 0, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
            "Unable to remove obj 0x%" PRIxLEAST64 ". Was it created? Has it already been destroyed?",
           object_handle);
    }
}

// CODEGEN : file ../vk-layer-generate.py line #893
static VkBool32 set_command_pool_status(VkDevice dispatchable_object, VkCommandPool object, VkDebugReportObjectTypeEXT objType, ObjectStatusFlags status_flag)
{
    if (object != VK_NULL_HANDLE) {
        uint64_t object_handle = (uint64_t)(object);
        if (VkCommandPoolMap.find(object_handle) != VkCommandPoolMap.end()) {
            OBJTRACK_NODE* pNode = VkCommandPoolMap[object_handle];
            pNode->status |= status_flag;
        }
        else {
            // If we do not find it print an error
            return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT ) 0, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
                "Unable to set status for non-existent object 0x%" PRIxLEAST64 " of %s type",
                object_handle, string_VkDebugReportObjectTypeEXT(objType));
        }
    }
    return VK_FALSE;
}

// CODEGEN : file ../vk-layer-generate.py line #915
static VkBool32 validate_command_pool_status(
VkDevice dispatchable_object, VkCommandPool object,
    VkDebugReportObjectTypeEXT     objType,
    ObjectStatusFlags   status_mask,
    ObjectStatusFlags   status_flag,
    VkFlags             msg_flags,
    OBJECT_TRACK_ERROR  error_code,
    const char         *fail_msg)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkCommandPoolMap.find(object_handle) != VkCommandPoolMap.end()) {
        OBJTRACK_NODE* pNode = VkCommandPoolMap[object_handle];
        if ((pNode->status & status_mask) != status_flag) {
            log_msg(mdd(dispatchable_object), msg_flags, pNode->objType, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
                "OBJECT VALIDATION WARNING: %s object 0x%" PRIxLEAST64 ": %s", string_VkDebugReportObjectTypeEXT(objType),
                 object_handle, fail_msg);
            return VK_FALSE;
        }
        return VK_TRUE;
    }
    else {
        // If we do not find it print an error
        log_msg(mdd(dispatchable_object), msg_flags, (VkDebugReportObjectTypeEXT) 0, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
            "Unable to obtain status for non-existent object 0x%" PRIxLEAST64 " of %s type",
            object_handle, string_VkDebugReportObjectTypeEXT(objType));
        return VK_FALSE;
    }
}

// CODEGEN : file ../vk-layer-generate.py line #948
static VkBool32 reset_command_pool_status(VkDevice dispatchable_object, VkCommandPool object, VkDebugReportObjectTypeEXT objType, ObjectStatusFlags status_flag)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkCommandPoolMap.find(object_handle) != VkCommandPoolMap.end()) {
        OBJTRACK_NODE* pNode = VkCommandPoolMap[object_handle];
        pNode->status &= ~status_flag;
    }
    else {
        // If we do not find it print an error
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
            "Unable to reset status for non-existent object 0x%" PRIxLEAST64 " of %s type",
            object_handle, string_VkDebugReportObjectTypeEXT(objType));
    }
    return VK_FALSE;
}

// CODEGEN : file ../vk-layer-generate.py line #844
static void create_fence(VkDevice dispatchable_object, VkFence vkObj, VkDebugReportObjectTypeEXT objType)
{
    log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, objType,(uint64_t)(vkObj), __LINE__, OBJTRACK_NONE, "OBJTRACK",
        "OBJ[%llu] : CREATE %s object 0x%" PRIxLEAST64 , object_track_index++, string_VkDebugReportObjectTypeEXT(objType),
        (uint64_t)(vkObj));

    OBJTRACK_NODE* pNewObjNode = new OBJTRACK_NODE;
    pNewObjNode->objType = objType;
    pNewObjNode->status  = OBJSTATUS_NONE;
    pNewObjNode->vkObj  = (uint64_t)(vkObj);
    VkFenceMap[(uint64_t)vkObj] = pNewObjNode;
    uint32_t objIndex = objTypeToIndex(objType);
    numObjs[objIndex]++;
    numTotalObjs++;
}

// CODEGEN : file ../vk-layer-generate.py line #866
static void destroy_fence(VkDevice dispatchable_object, VkFence object)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkFenceMap.find(object_handle) != VkFenceMap.end()) {
        OBJTRACK_NODE* pNode = VkFenceMap[(uint64_t)object];
        uint32_t objIndex = objTypeToIndex(pNode->objType);
        assert(numTotalObjs > 0);
        numTotalObjs--;
        assert(numObjs[objIndex] > 0);
        numObjs[objIndex]--;
        log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, pNode->objType, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
           "OBJ_STAT Destroy %s obj 0x%" PRIxLEAST64 " (%" PRIu64 " total objs remain & %" PRIu64 " %s objs).",
            string_VkDebugReportObjectTypeEXT(pNode->objType), (uint64_t)(object), numTotalObjs, numObjs[objIndex],
            string_VkDebugReportObjectTypeEXT(pNode->objType));
        delete pNode;
        VkFenceMap.erase(object_handle);
    } else {
        log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT ) 0, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
            "Unable to remove obj 0x%" PRIxLEAST64 ". Was it created? Has it already been destroyed?",
           object_handle);
    }
}

// CODEGEN : file ../vk-layer-generate.py line #893
static VkBool32 set_fence_status(VkDevice dispatchable_object, VkFence object, VkDebugReportObjectTypeEXT objType, ObjectStatusFlags status_flag)
{
    if (object != VK_NULL_HANDLE) {
        uint64_t object_handle = (uint64_t)(object);
        if (VkFenceMap.find(object_handle) != VkFenceMap.end()) {
            OBJTRACK_NODE* pNode = VkFenceMap[object_handle];
            pNode->status |= status_flag;
        }
        else {
            // If we do not find it print an error
            return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT ) 0, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
                "Unable to set status for non-existent object 0x%" PRIxLEAST64 " of %s type",
                object_handle, string_VkDebugReportObjectTypeEXT(objType));
        }
    }
    return VK_FALSE;
}

// CODEGEN : file ../vk-layer-generate.py line #915
static VkBool32 validate_fence_status(
VkDevice dispatchable_object, VkFence object,
    VkDebugReportObjectTypeEXT     objType,
    ObjectStatusFlags   status_mask,
    ObjectStatusFlags   status_flag,
    VkFlags             msg_flags,
    OBJECT_TRACK_ERROR  error_code,
    const char         *fail_msg)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkFenceMap.find(object_handle) != VkFenceMap.end()) {
        OBJTRACK_NODE* pNode = VkFenceMap[object_handle];
        if ((pNode->status & status_mask) != status_flag) {
            log_msg(mdd(dispatchable_object), msg_flags, pNode->objType, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
                "OBJECT VALIDATION WARNING: %s object 0x%" PRIxLEAST64 ": %s", string_VkDebugReportObjectTypeEXT(objType),
                 object_handle, fail_msg);
            return VK_FALSE;
        }
        return VK_TRUE;
    }
    else {
        // If we do not find it print an error
        log_msg(mdd(dispatchable_object), msg_flags, (VkDebugReportObjectTypeEXT) 0, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
            "Unable to obtain status for non-existent object 0x%" PRIxLEAST64 " of %s type",
            object_handle, string_VkDebugReportObjectTypeEXT(objType));
        return VK_FALSE;
    }
}

// CODEGEN : file ../vk-layer-generate.py line #948
static VkBool32 reset_fence_status(VkDevice dispatchable_object, VkFence object, VkDebugReportObjectTypeEXT objType, ObjectStatusFlags status_flag)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkFenceMap.find(object_handle) != VkFenceMap.end()) {
        OBJTRACK_NODE* pNode = VkFenceMap[object_handle];
        pNode->status &= ~status_flag;
    }
    else {
        // If we do not find it print an error
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
            "Unable to reset status for non-existent object 0x%" PRIxLEAST64 " of %s type",
            object_handle, string_VkDebugReportObjectTypeEXT(objType));
    }
    return VK_FALSE;
}

// CODEGEN : file ../vk-layer-generate.py line #844
static void create_device_memory(VkDevice dispatchable_object, VkDeviceMemory vkObj, VkDebugReportObjectTypeEXT objType)
{
    log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, objType,(uint64_t)(vkObj), __LINE__, OBJTRACK_NONE, "OBJTRACK",
        "OBJ[%llu] : CREATE %s object 0x%" PRIxLEAST64 , object_track_index++, string_VkDebugReportObjectTypeEXT(objType),
        (uint64_t)(vkObj));

    OBJTRACK_NODE* pNewObjNode = new OBJTRACK_NODE;
    pNewObjNode->objType = objType;
    pNewObjNode->status  = OBJSTATUS_NONE;
    pNewObjNode->vkObj  = (uint64_t)(vkObj);
    VkDeviceMemoryMap[(uint64_t)vkObj] = pNewObjNode;
    uint32_t objIndex = objTypeToIndex(objType);
    numObjs[objIndex]++;
    numTotalObjs++;
}

// CODEGEN : file ../vk-layer-generate.py line #866
static void destroy_device_memory(VkDevice dispatchable_object, VkDeviceMemory object)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkDeviceMemoryMap.find(object_handle) != VkDeviceMemoryMap.end()) {
        OBJTRACK_NODE* pNode = VkDeviceMemoryMap[(uint64_t)object];
        uint32_t objIndex = objTypeToIndex(pNode->objType);
        assert(numTotalObjs > 0);
        numTotalObjs--;
        assert(numObjs[objIndex] > 0);
        numObjs[objIndex]--;
        log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, pNode->objType, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
           "OBJ_STAT Destroy %s obj 0x%" PRIxLEAST64 " (%" PRIu64 " total objs remain & %" PRIu64 " %s objs).",
            string_VkDebugReportObjectTypeEXT(pNode->objType), (uint64_t)(object), numTotalObjs, numObjs[objIndex],
            string_VkDebugReportObjectTypeEXT(pNode->objType));
        delete pNode;
        VkDeviceMemoryMap.erase(object_handle);
    } else {
        log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT ) 0, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
            "Unable to remove obj 0x%" PRIxLEAST64 ". Was it created? Has it already been destroyed?",
           object_handle);
    }
}

// CODEGEN : file ../vk-layer-generate.py line #893
static VkBool32 set_device_memory_status(VkDevice dispatchable_object, VkDeviceMemory object, VkDebugReportObjectTypeEXT objType, ObjectStatusFlags status_flag)
{
    if (object != VK_NULL_HANDLE) {
        uint64_t object_handle = (uint64_t)(object);
        if (VkDeviceMemoryMap.find(object_handle) != VkDeviceMemoryMap.end()) {
            OBJTRACK_NODE* pNode = VkDeviceMemoryMap[object_handle];
            pNode->status |= status_flag;
        }
        else {
            // If we do not find it print an error
            return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT ) 0, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
                "Unable to set status for non-existent object 0x%" PRIxLEAST64 " of %s type",
                object_handle, string_VkDebugReportObjectTypeEXT(objType));
        }
    }
    return VK_FALSE;
}

// CODEGEN : file ../vk-layer-generate.py line #915
static VkBool32 validate_device_memory_status(
VkDevice dispatchable_object, VkDeviceMemory object,
    VkDebugReportObjectTypeEXT     objType,
    ObjectStatusFlags   status_mask,
    ObjectStatusFlags   status_flag,
    VkFlags             msg_flags,
    OBJECT_TRACK_ERROR  error_code,
    const char         *fail_msg)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkDeviceMemoryMap.find(object_handle) != VkDeviceMemoryMap.end()) {
        OBJTRACK_NODE* pNode = VkDeviceMemoryMap[object_handle];
        if ((pNode->status & status_mask) != status_flag) {
            log_msg(mdd(dispatchable_object), msg_flags, pNode->objType, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
                "OBJECT VALIDATION WARNING: %s object 0x%" PRIxLEAST64 ": %s", string_VkDebugReportObjectTypeEXT(objType),
                 object_handle, fail_msg);
            return VK_FALSE;
        }
        return VK_TRUE;
    }
    else {
        // If we do not find it print an error
        log_msg(mdd(dispatchable_object), msg_flags, (VkDebugReportObjectTypeEXT) 0, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
            "Unable to obtain status for non-existent object 0x%" PRIxLEAST64 " of %s type",
            object_handle, string_VkDebugReportObjectTypeEXT(objType));
        return VK_FALSE;
    }
}

// CODEGEN : file ../vk-layer-generate.py line #948
static VkBool32 reset_device_memory_status(VkDevice dispatchable_object, VkDeviceMemory object, VkDebugReportObjectTypeEXT objType, ObjectStatusFlags status_flag)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkDeviceMemoryMap.find(object_handle) != VkDeviceMemoryMap.end()) {
        OBJTRACK_NODE* pNode = VkDeviceMemoryMap[object_handle];
        pNode->status &= ~status_flag;
    }
    else {
        // If we do not find it print an error
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
            "Unable to reset status for non-existent object 0x%" PRIxLEAST64 " of %s type",
            object_handle, string_VkDebugReportObjectTypeEXT(objType));
    }
    return VK_FALSE;
}

// CODEGEN : file ../vk-layer-generate.py line #844
static void create_buffer(VkDevice dispatchable_object, VkBuffer vkObj, VkDebugReportObjectTypeEXT objType)
{
    log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, objType,(uint64_t)(vkObj), __LINE__, OBJTRACK_NONE, "OBJTRACK",
        "OBJ[%llu] : CREATE %s object 0x%" PRIxLEAST64 , object_track_index++, string_VkDebugReportObjectTypeEXT(objType),
        (uint64_t)(vkObj));

    OBJTRACK_NODE* pNewObjNode = new OBJTRACK_NODE;
    pNewObjNode->objType = objType;
    pNewObjNode->status  = OBJSTATUS_NONE;
    pNewObjNode->vkObj  = (uint64_t)(vkObj);
    VkBufferMap[(uint64_t)vkObj] = pNewObjNode;
    uint32_t objIndex = objTypeToIndex(objType);
    numObjs[objIndex]++;
    numTotalObjs++;
}

// CODEGEN : file ../vk-layer-generate.py line #866
static void destroy_buffer(VkDevice dispatchable_object, VkBuffer object)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkBufferMap.find(object_handle) != VkBufferMap.end()) {
        OBJTRACK_NODE* pNode = VkBufferMap[(uint64_t)object];
        uint32_t objIndex = objTypeToIndex(pNode->objType);
        assert(numTotalObjs > 0);
        numTotalObjs--;
        assert(numObjs[objIndex] > 0);
        numObjs[objIndex]--;
        log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, pNode->objType, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
           "OBJ_STAT Destroy %s obj 0x%" PRIxLEAST64 " (%" PRIu64 " total objs remain & %" PRIu64 " %s objs).",
            string_VkDebugReportObjectTypeEXT(pNode->objType), (uint64_t)(object), numTotalObjs, numObjs[objIndex],
            string_VkDebugReportObjectTypeEXT(pNode->objType));
        delete pNode;
        VkBufferMap.erase(object_handle);
    } else {
        log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT ) 0, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
            "Unable to remove obj 0x%" PRIxLEAST64 ". Was it created? Has it already been destroyed?",
           object_handle);
    }
}

// CODEGEN : file ../vk-layer-generate.py line #893
static VkBool32 set_buffer_status(VkDevice dispatchable_object, VkBuffer object, VkDebugReportObjectTypeEXT objType, ObjectStatusFlags status_flag)
{
    if (object != VK_NULL_HANDLE) {
        uint64_t object_handle = (uint64_t)(object);
        if (VkBufferMap.find(object_handle) != VkBufferMap.end()) {
            OBJTRACK_NODE* pNode = VkBufferMap[object_handle];
            pNode->status |= status_flag;
        }
        else {
            // If we do not find it print an error
            return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT ) 0, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
                "Unable to set status for non-existent object 0x%" PRIxLEAST64 " of %s type",
                object_handle, string_VkDebugReportObjectTypeEXT(objType));
        }
    }
    return VK_FALSE;
}

// CODEGEN : file ../vk-layer-generate.py line #915
static VkBool32 validate_buffer_status(
VkDevice dispatchable_object, VkBuffer object,
    VkDebugReportObjectTypeEXT     objType,
    ObjectStatusFlags   status_mask,
    ObjectStatusFlags   status_flag,
    VkFlags             msg_flags,
    OBJECT_TRACK_ERROR  error_code,
    const char         *fail_msg)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkBufferMap.find(object_handle) != VkBufferMap.end()) {
        OBJTRACK_NODE* pNode = VkBufferMap[object_handle];
        if ((pNode->status & status_mask) != status_flag) {
            log_msg(mdd(dispatchable_object), msg_flags, pNode->objType, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
                "OBJECT VALIDATION WARNING: %s object 0x%" PRIxLEAST64 ": %s", string_VkDebugReportObjectTypeEXT(objType),
                 object_handle, fail_msg);
            return VK_FALSE;
        }
        return VK_TRUE;
    }
    else {
        // If we do not find it print an error
        log_msg(mdd(dispatchable_object), msg_flags, (VkDebugReportObjectTypeEXT) 0, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
            "Unable to obtain status for non-existent object 0x%" PRIxLEAST64 " of %s type",
            object_handle, string_VkDebugReportObjectTypeEXT(objType));
        return VK_FALSE;
    }
}

// CODEGEN : file ../vk-layer-generate.py line #948
static VkBool32 reset_buffer_status(VkDevice dispatchable_object, VkBuffer object, VkDebugReportObjectTypeEXT objType, ObjectStatusFlags status_flag)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkBufferMap.find(object_handle) != VkBufferMap.end()) {
        OBJTRACK_NODE* pNode = VkBufferMap[object_handle];
        pNode->status &= ~status_flag;
    }
    else {
        // If we do not find it print an error
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
            "Unable to reset status for non-existent object 0x%" PRIxLEAST64 " of %s type",
            object_handle, string_VkDebugReportObjectTypeEXT(objType));
    }
    return VK_FALSE;
}

// CODEGEN : file ../vk-layer-generate.py line #844
static void create_image(VkDevice dispatchable_object, VkImage vkObj, VkDebugReportObjectTypeEXT objType)
{
    log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, objType,(uint64_t)(vkObj), __LINE__, OBJTRACK_NONE, "OBJTRACK",
        "OBJ[%llu] : CREATE %s object 0x%" PRIxLEAST64 , object_track_index++, string_VkDebugReportObjectTypeEXT(objType),
        (uint64_t)(vkObj));

    OBJTRACK_NODE* pNewObjNode = new OBJTRACK_NODE;
    pNewObjNode->objType = objType;
    pNewObjNode->status  = OBJSTATUS_NONE;
    pNewObjNode->vkObj  = (uint64_t)(vkObj);
    VkImageMap[(uint64_t)vkObj] = pNewObjNode;
    uint32_t objIndex = objTypeToIndex(objType);
    numObjs[objIndex]++;
    numTotalObjs++;
}

// CODEGEN : file ../vk-layer-generate.py line #866
static void destroy_image(VkDevice dispatchable_object, VkImage object)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkImageMap.find(object_handle) != VkImageMap.end()) {
        OBJTRACK_NODE* pNode = VkImageMap[(uint64_t)object];
        uint32_t objIndex = objTypeToIndex(pNode->objType);
        assert(numTotalObjs > 0);
        numTotalObjs--;
        assert(numObjs[objIndex] > 0);
        numObjs[objIndex]--;
        log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, pNode->objType, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
           "OBJ_STAT Destroy %s obj 0x%" PRIxLEAST64 " (%" PRIu64 " total objs remain & %" PRIu64 " %s objs).",
            string_VkDebugReportObjectTypeEXT(pNode->objType), (uint64_t)(object), numTotalObjs, numObjs[objIndex],
            string_VkDebugReportObjectTypeEXT(pNode->objType));
        delete pNode;
        VkImageMap.erase(object_handle);
    } else {
        log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT ) 0, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
            "Unable to remove obj 0x%" PRIxLEAST64 ". Was it created? Has it already been destroyed?",
           object_handle);
    }
}

// CODEGEN : file ../vk-layer-generate.py line #893
static VkBool32 set_image_status(VkDevice dispatchable_object, VkImage object, VkDebugReportObjectTypeEXT objType, ObjectStatusFlags status_flag)
{
    if (object != VK_NULL_HANDLE) {
        uint64_t object_handle = (uint64_t)(object);
        if (VkImageMap.find(object_handle) != VkImageMap.end()) {
            OBJTRACK_NODE* pNode = VkImageMap[object_handle];
            pNode->status |= status_flag;
        }
        else {
            // If we do not find it print an error
            return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT ) 0, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
                "Unable to set status for non-existent object 0x%" PRIxLEAST64 " of %s type",
                object_handle, string_VkDebugReportObjectTypeEXT(objType));
        }
    }
    return VK_FALSE;
}

// CODEGEN : file ../vk-layer-generate.py line #915
static VkBool32 validate_image_status(
VkDevice dispatchable_object, VkImage object,
    VkDebugReportObjectTypeEXT     objType,
    ObjectStatusFlags   status_mask,
    ObjectStatusFlags   status_flag,
    VkFlags             msg_flags,
    OBJECT_TRACK_ERROR  error_code,
    const char         *fail_msg)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkImageMap.find(object_handle) != VkImageMap.end()) {
        OBJTRACK_NODE* pNode = VkImageMap[object_handle];
        if ((pNode->status & status_mask) != status_flag) {
            log_msg(mdd(dispatchable_object), msg_flags, pNode->objType, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
                "OBJECT VALIDATION WARNING: %s object 0x%" PRIxLEAST64 ": %s", string_VkDebugReportObjectTypeEXT(objType),
                 object_handle, fail_msg);
            return VK_FALSE;
        }
        return VK_TRUE;
    }
    else {
        // If we do not find it print an error
        log_msg(mdd(dispatchable_object), msg_flags, (VkDebugReportObjectTypeEXT) 0, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
            "Unable to obtain status for non-existent object 0x%" PRIxLEAST64 " of %s type",
            object_handle, string_VkDebugReportObjectTypeEXT(objType));
        return VK_FALSE;
    }
}

// CODEGEN : file ../vk-layer-generate.py line #948
static VkBool32 reset_image_status(VkDevice dispatchable_object, VkImage object, VkDebugReportObjectTypeEXT objType, ObjectStatusFlags status_flag)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkImageMap.find(object_handle) != VkImageMap.end()) {
        OBJTRACK_NODE* pNode = VkImageMap[object_handle];
        pNode->status &= ~status_flag;
    }
    else {
        // If we do not find it print an error
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
            "Unable to reset status for non-existent object 0x%" PRIxLEAST64 " of %s type",
            object_handle, string_VkDebugReportObjectTypeEXT(objType));
    }
    return VK_FALSE;
}

// CODEGEN : file ../vk-layer-generate.py line #844
static void create_semaphore(VkDevice dispatchable_object, VkSemaphore vkObj, VkDebugReportObjectTypeEXT objType)
{
    log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, objType,(uint64_t)(vkObj), __LINE__, OBJTRACK_NONE, "OBJTRACK",
        "OBJ[%llu] : CREATE %s object 0x%" PRIxLEAST64 , object_track_index++, string_VkDebugReportObjectTypeEXT(objType),
        (uint64_t)(vkObj));

    OBJTRACK_NODE* pNewObjNode = new OBJTRACK_NODE;
    pNewObjNode->objType = objType;
    pNewObjNode->status  = OBJSTATUS_NONE;
    pNewObjNode->vkObj  = (uint64_t)(vkObj);
    VkSemaphoreMap[(uint64_t)vkObj] = pNewObjNode;
    uint32_t objIndex = objTypeToIndex(objType);
    numObjs[objIndex]++;
    numTotalObjs++;
}

// CODEGEN : file ../vk-layer-generate.py line #866
static void destroy_semaphore(VkDevice dispatchable_object, VkSemaphore object)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkSemaphoreMap.find(object_handle) != VkSemaphoreMap.end()) {
        OBJTRACK_NODE* pNode = VkSemaphoreMap[(uint64_t)object];
        uint32_t objIndex = objTypeToIndex(pNode->objType);
        assert(numTotalObjs > 0);
        numTotalObjs--;
        assert(numObjs[objIndex] > 0);
        numObjs[objIndex]--;
        log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, pNode->objType, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
           "OBJ_STAT Destroy %s obj 0x%" PRIxLEAST64 " (%" PRIu64 " total objs remain & %" PRIu64 " %s objs).",
            string_VkDebugReportObjectTypeEXT(pNode->objType), (uint64_t)(object), numTotalObjs, numObjs[objIndex],
            string_VkDebugReportObjectTypeEXT(pNode->objType));
        delete pNode;
        VkSemaphoreMap.erase(object_handle);
    } else {
        log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT ) 0, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
            "Unable to remove obj 0x%" PRIxLEAST64 ". Was it created? Has it already been destroyed?",
           object_handle);
    }
}

// CODEGEN : file ../vk-layer-generate.py line #893
static VkBool32 set_semaphore_status(VkDevice dispatchable_object, VkSemaphore object, VkDebugReportObjectTypeEXT objType, ObjectStatusFlags status_flag)
{
    if (object != VK_NULL_HANDLE) {
        uint64_t object_handle = (uint64_t)(object);
        if (VkSemaphoreMap.find(object_handle) != VkSemaphoreMap.end()) {
            OBJTRACK_NODE* pNode = VkSemaphoreMap[object_handle];
            pNode->status |= status_flag;
        }
        else {
            // If we do not find it print an error
            return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT ) 0, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
                "Unable to set status for non-existent object 0x%" PRIxLEAST64 " of %s type",
                object_handle, string_VkDebugReportObjectTypeEXT(objType));
        }
    }
    return VK_FALSE;
}

// CODEGEN : file ../vk-layer-generate.py line #915
static VkBool32 validate_semaphore_status(
VkDevice dispatchable_object, VkSemaphore object,
    VkDebugReportObjectTypeEXT     objType,
    ObjectStatusFlags   status_mask,
    ObjectStatusFlags   status_flag,
    VkFlags             msg_flags,
    OBJECT_TRACK_ERROR  error_code,
    const char         *fail_msg)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkSemaphoreMap.find(object_handle) != VkSemaphoreMap.end()) {
        OBJTRACK_NODE* pNode = VkSemaphoreMap[object_handle];
        if ((pNode->status & status_mask) != status_flag) {
            log_msg(mdd(dispatchable_object), msg_flags, pNode->objType, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
                "OBJECT VALIDATION WARNING: %s object 0x%" PRIxLEAST64 ": %s", string_VkDebugReportObjectTypeEXT(objType),
                 object_handle, fail_msg);
            return VK_FALSE;
        }
        return VK_TRUE;
    }
    else {
        // If we do not find it print an error
        log_msg(mdd(dispatchable_object), msg_flags, (VkDebugReportObjectTypeEXT) 0, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
            "Unable to obtain status for non-existent object 0x%" PRIxLEAST64 " of %s type",
            object_handle, string_VkDebugReportObjectTypeEXT(objType));
        return VK_FALSE;
    }
}

// CODEGEN : file ../vk-layer-generate.py line #948
static VkBool32 reset_semaphore_status(VkDevice dispatchable_object, VkSemaphore object, VkDebugReportObjectTypeEXT objType, ObjectStatusFlags status_flag)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkSemaphoreMap.find(object_handle) != VkSemaphoreMap.end()) {
        OBJTRACK_NODE* pNode = VkSemaphoreMap[object_handle];
        pNode->status &= ~status_flag;
    }
    else {
        // If we do not find it print an error
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
            "Unable to reset status for non-existent object 0x%" PRIxLEAST64 " of %s type",
            object_handle, string_VkDebugReportObjectTypeEXT(objType));
    }
    return VK_FALSE;
}

// CODEGEN : file ../vk-layer-generate.py line #844
static void create_event(VkDevice dispatchable_object, VkEvent vkObj, VkDebugReportObjectTypeEXT objType)
{
    log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, objType,(uint64_t)(vkObj), __LINE__, OBJTRACK_NONE, "OBJTRACK",
        "OBJ[%llu] : CREATE %s object 0x%" PRIxLEAST64 , object_track_index++, string_VkDebugReportObjectTypeEXT(objType),
        (uint64_t)(vkObj));

    OBJTRACK_NODE* pNewObjNode = new OBJTRACK_NODE;
    pNewObjNode->objType = objType;
    pNewObjNode->status  = OBJSTATUS_NONE;
    pNewObjNode->vkObj  = (uint64_t)(vkObj);
    VkEventMap[(uint64_t)vkObj] = pNewObjNode;
    uint32_t objIndex = objTypeToIndex(objType);
    numObjs[objIndex]++;
    numTotalObjs++;
}

// CODEGEN : file ../vk-layer-generate.py line #866
static void destroy_event(VkDevice dispatchable_object, VkEvent object)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkEventMap.find(object_handle) != VkEventMap.end()) {
        OBJTRACK_NODE* pNode = VkEventMap[(uint64_t)object];
        uint32_t objIndex = objTypeToIndex(pNode->objType);
        assert(numTotalObjs > 0);
        numTotalObjs--;
        assert(numObjs[objIndex] > 0);
        numObjs[objIndex]--;
        log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, pNode->objType, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
           "OBJ_STAT Destroy %s obj 0x%" PRIxLEAST64 " (%" PRIu64 " total objs remain & %" PRIu64 " %s objs).",
            string_VkDebugReportObjectTypeEXT(pNode->objType), (uint64_t)(object), numTotalObjs, numObjs[objIndex],
            string_VkDebugReportObjectTypeEXT(pNode->objType));
        delete pNode;
        VkEventMap.erase(object_handle);
    } else {
        log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT ) 0, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
            "Unable to remove obj 0x%" PRIxLEAST64 ". Was it created? Has it already been destroyed?",
           object_handle);
    }
}

// CODEGEN : file ../vk-layer-generate.py line #893
static VkBool32 set_event_status(VkDevice dispatchable_object, VkEvent object, VkDebugReportObjectTypeEXT objType, ObjectStatusFlags status_flag)
{
    if (object != VK_NULL_HANDLE) {
        uint64_t object_handle = (uint64_t)(object);
        if (VkEventMap.find(object_handle) != VkEventMap.end()) {
            OBJTRACK_NODE* pNode = VkEventMap[object_handle];
            pNode->status |= status_flag;
        }
        else {
            // If we do not find it print an error
            return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT ) 0, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
                "Unable to set status for non-existent object 0x%" PRIxLEAST64 " of %s type",
                object_handle, string_VkDebugReportObjectTypeEXT(objType));
        }
    }
    return VK_FALSE;
}

// CODEGEN : file ../vk-layer-generate.py line #915
static VkBool32 validate_event_status(
VkDevice dispatchable_object, VkEvent object,
    VkDebugReportObjectTypeEXT     objType,
    ObjectStatusFlags   status_mask,
    ObjectStatusFlags   status_flag,
    VkFlags             msg_flags,
    OBJECT_TRACK_ERROR  error_code,
    const char         *fail_msg)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkEventMap.find(object_handle) != VkEventMap.end()) {
        OBJTRACK_NODE* pNode = VkEventMap[object_handle];
        if ((pNode->status & status_mask) != status_flag) {
            log_msg(mdd(dispatchable_object), msg_flags, pNode->objType, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
                "OBJECT VALIDATION WARNING: %s object 0x%" PRIxLEAST64 ": %s", string_VkDebugReportObjectTypeEXT(objType),
                 object_handle, fail_msg);
            return VK_FALSE;
        }
        return VK_TRUE;
    }
    else {
        // If we do not find it print an error
        log_msg(mdd(dispatchable_object), msg_flags, (VkDebugReportObjectTypeEXT) 0, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
            "Unable to obtain status for non-existent object 0x%" PRIxLEAST64 " of %s type",
            object_handle, string_VkDebugReportObjectTypeEXT(objType));
        return VK_FALSE;
    }
}

// CODEGEN : file ../vk-layer-generate.py line #948
static VkBool32 reset_event_status(VkDevice dispatchable_object, VkEvent object, VkDebugReportObjectTypeEXT objType, ObjectStatusFlags status_flag)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkEventMap.find(object_handle) != VkEventMap.end()) {
        OBJTRACK_NODE* pNode = VkEventMap[object_handle];
        pNode->status &= ~status_flag;
    }
    else {
        // If we do not find it print an error
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
            "Unable to reset status for non-existent object 0x%" PRIxLEAST64 " of %s type",
            object_handle, string_VkDebugReportObjectTypeEXT(objType));
    }
    return VK_FALSE;
}

// CODEGEN : file ../vk-layer-generate.py line #844
static void create_query_pool(VkDevice dispatchable_object, VkQueryPool vkObj, VkDebugReportObjectTypeEXT objType)
{
    log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, objType,(uint64_t)(vkObj), __LINE__, OBJTRACK_NONE, "OBJTRACK",
        "OBJ[%llu] : CREATE %s object 0x%" PRIxLEAST64 , object_track_index++, string_VkDebugReportObjectTypeEXT(objType),
        (uint64_t)(vkObj));

    OBJTRACK_NODE* pNewObjNode = new OBJTRACK_NODE;
    pNewObjNode->objType = objType;
    pNewObjNode->status  = OBJSTATUS_NONE;
    pNewObjNode->vkObj  = (uint64_t)(vkObj);
    VkQueryPoolMap[(uint64_t)vkObj] = pNewObjNode;
    uint32_t objIndex = objTypeToIndex(objType);
    numObjs[objIndex]++;
    numTotalObjs++;
}

// CODEGEN : file ../vk-layer-generate.py line #866
static void destroy_query_pool(VkDevice dispatchable_object, VkQueryPool object)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkQueryPoolMap.find(object_handle) != VkQueryPoolMap.end()) {
        OBJTRACK_NODE* pNode = VkQueryPoolMap[(uint64_t)object];
        uint32_t objIndex = objTypeToIndex(pNode->objType);
        assert(numTotalObjs > 0);
        numTotalObjs--;
        assert(numObjs[objIndex] > 0);
        numObjs[objIndex]--;
        log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, pNode->objType, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
           "OBJ_STAT Destroy %s obj 0x%" PRIxLEAST64 " (%" PRIu64 " total objs remain & %" PRIu64 " %s objs).",
            string_VkDebugReportObjectTypeEXT(pNode->objType), (uint64_t)(object), numTotalObjs, numObjs[objIndex],
            string_VkDebugReportObjectTypeEXT(pNode->objType));
        delete pNode;
        VkQueryPoolMap.erase(object_handle);
    } else {
        log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT ) 0, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
            "Unable to remove obj 0x%" PRIxLEAST64 ". Was it created? Has it already been destroyed?",
           object_handle);
    }
}

// CODEGEN : file ../vk-layer-generate.py line #893
static VkBool32 set_query_pool_status(VkDevice dispatchable_object, VkQueryPool object, VkDebugReportObjectTypeEXT objType, ObjectStatusFlags status_flag)
{
    if (object != VK_NULL_HANDLE) {
        uint64_t object_handle = (uint64_t)(object);
        if (VkQueryPoolMap.find(object_handle) != VkQueryPoolMap.end()) {
            OBJTRACK_NODE* pNode = VkQueryPoolMap[object_handle];
            pNode->status |= status_flag;
        }
        else {
            // If we do not find it print an error
            return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT ) 0, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
                "Unable to set status for non-existent object 0x%" PRIxLEAST64 " of %s type",
                object_handle, string_VkDebugReportObjectTypeEXT(objType));
        }
    }
    return VK_FALSE;
}

// CODEGEN : file ../vk-layer-generate.py line #915
static VkBool32 validate_query_pool_status(
VkDevice dispatchable_object, VkQueryPool object,
    VkDebugReportObjectTypeEXT     objType,
    ObjectStatusFlags   status_mask,
    ObjectStatusFlags   status_flag,
    VkFlags             msg_flags,
    OBJECT_TRACK_ERROR  error_code,
    const char         *fail_msg)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkQueryPoolMap.find(object_handle) != VkQueryPoolMap.end()) {
        OBJTRACK_NODE* pNode = VkQueryPoolMap[object_handle];
        if ((pNode->status & status_mask) != status_flag) {
            log_msg(mdd(dispatchable_object), msg_flags, pNode->objType, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
                "OBJECT VALIDATION WARNING: %s object 0x%" PRIxLEAST64 ": %s", string_VkDebugReportObjectTypeEXT(objType),
                 object_handle, fail_msg);
            return VK_FALSE;
        }
        return VK_TRUE;
    }
    else {
        // If we do not find it print an error
        log_msg(mdd(dispatchable_object), msg_flags, (VkDebugReportObjectTypeEXT) 0, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
            "Unable to obtain status for non-existent object 0x%" PRIxLEAST64 " of %s type",
            object_handle, string_VkDebugReportObjectTypeEXT(objType));
        return VK_FALSE;
    }
}

// CODEGEN : file ../vk-layer-generate.py line #948
static VkBool32 reset_query_pool_status(VkDevice dispatchable_object, VkQueryPool object, VkDebugReportObjectTypeEXT objType, ObjectStatusFlags status_flag)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkQueryPoolMap.find(object_handle) != VkQueryPoolMap.end()) {
        OBJTRACK_NODE* pNode = VkQueryPoolMap[object_handle];
        pNode->status &= ~status_flag;
    }
    else {
        // If we do not find it print an error
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
            "Unable to reset status for non-existent object 0x%" PRIxLEAST64 " of %s type",
            object_handle, string_VkDebugReportObjectTypeEXT(objType));
    }
    return VK_FALSE;
}

// CODEGEN : file ../vk-layer-generate.py line #844
static void create_buffer_view(VkDevice dispatchable_object, VkBufferView vkObj, VkDebugReportObjectTypeEXT objType)
{
    log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, objType,(uint64_t)(vkObj), __LINE__, OBJTRACK_NONE, "OBJTRACK",
        "OBJ[%llu] : CREATE %s object 0x%" PRIxLEAST64 , object_track_index++, string_VkDebugReportObjectTypeEXT(objType),
        (uint64_t)(vkObj));

    OBJTRACK_NODE* pNewObjNode = new OBJTRACK_NODE;
    pNewObjNode->objType = objType;
    pNewObjNode->status  = OBJSTATUS_NONE;
    pNewObjNode->vkObj  = (uint64_t)(vkObj);
    VkBufferViewMap[(uint64_t)vkObj] = pNewObjNode;
    uint32_t objIndex = objTypeToIndex(objType);
    numObjs[objIndex]++;
    numTotalObjs++;
}

// CODEGEN : file ../vk-layer-generate.py line #866
static void destroy_buffer_view(VkDevice dispatchable_object, VkBufferView object)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkBufferViewMap.find(object_handle) != VkBufferViewMap.end()) {
        OBJTRACK_NODE* pNode = VkBufferViewMap[(uint64_t)object];
        uint32_t objIndex = objTypeToIndex(pNode->objType);
        assert(numTotalObjs > 0);
        numTotalObjs--;
        assert(numObjs[objIndex] > 0);
        numObjs[objIndex]--;
        log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, pNode->objType, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
           "OBJ_STAT Destroy %s obj 0x%" PRIxLEAST64 " (%" PRIu64 " total objs remain & %" PRIu64 " %s objs).",
            string_VkDebugReportObjectTypeEXT(pNode->objType), (uint64_t)(object), numTotalObjs, numObjs[objIndex],
            string_VkDebugReportObjectTypeEXT(pNode->objType));
        delete pNode;
        VkBufferViewMap.erase(object_handle);
    } else {
        log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT ) 0, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
            "Unable to remove obj 0x%" PRIxLEAST64 ". Was it created? Has it already been destroyed?",
           object_handle);
    }
}

// CODEGEN : file ../vk-layer-generate.py line #893
static VkBool32 set_buffer_view_status(VkDevice dispatchable_object, VkBufferView object, VkDebugReportObjectTypeEXT objType, ObjectStatusFlags status_flag)
{
    if (object != VK_NULL_HANDLE) {
        uint64_t object_handle = (uint64_t)(object);
        if (VkBufferViewMap.find(object_handle) != VkBufferViewMap.end()) {
            OBJTRACK_NODE* pNode = VkBufferViewMap[object_handle];
            pNode->status |= status_flag;
        }
        else {
            // If we do not find it print an error
            return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT ) 0, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
                "Unable to set status for non-existent object 0x%" PRIxLEAST64 " of %s type",
                object_handle, string_VkDebugReportObjectTypeEXT(objType));
        }
    }
    return VK_FALSE;
}

// CODEGEN : file ../vk-layer-generate.py line #915
static VkBool32 validate_buffer_view_status(
VkDevice dispatchable_object, VkBufferView object,
    VkDebugReportObjectTypeEXT     objType,
    ObjectStatusFlags   status_mask,
    ObjectStatusFlags   status_flag,
    VkFlags             msg_flags,
    OBJECT_TRACK_ERROR  error_code,
    const char         *fail_msg)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkBufferViewMap.find(object_handle) != VkBufferViewMap.end()) {
        OBJTRACK_NODE* pNode = VkBufferViewMap[object_handle];
        if ((pNode->status & status_mask) != status_flag) {
            log_msg(mdd(dispatchable_object), msg_flags, pNode->objType, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
                "OBJECT VALIDATION WARNING: %s object 0x%" PRIxLEAST64 ": %s", string_VkDebugReportObjectTypeEXT(objType),
                 object_handle, fail_msg);
            return VK_FALSE;
        }
        return VK_TRUE;
    }
    else {
        // If we do not find it print an error
        log_msg(mdd(dispatchable_object), msg_flags, (VkDebugReportObjectTypeEXT) 0, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
            "Unable to obtain status for non-existent object 0x%" PRIxLEAST64 " of %s type",
            object_handle, string_VkDebugReportObjectTypeEXT(objType));
        return VK_FALSE;
    }
}

// CODEGEN : file ../vk-layer-generate.py line #948
static VkBool32 reset_buffer_view_status(VkDevice dispatchable_object, VkBufferView object, VkDebugReportObjectTypeEXT objType, ObjectStatusFlags status_flag)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkBufferViewMap.find(object_handle) != VkBufferViewMap.end()) {
        OBJTRACK_NODE* pNode = VkBufferViewMap[object_handle];
        pNode->status &= ~status_flag;
    }
    else {
        // If we do not find it print an error
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
            "Unable to reset status for non-existent object 0x%" PRIxLEAST64 " of %s type",
            object_handle, string_VkDebugReportObjectTypeEXT(objType));
    }
    return VK_FALSE;
}

// CODEGEN : file ../vk-layer-generate.py line #844
static void create_image_view(VkDevice dispatchable_object, VkImageView vkObj, VkDebugReportObjectTypeEXT objType)
{
    log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, objType,(uint64_t)(vkObj), __LINE__, OBJTRACK_NONE, "OBJTRACK",
        "OBJ[%llu] : CREATE %s object 0x%" PRIxLEAST64 , object_track_index++, string_VkDebugReportObjectTypeEXT(objType),
        (uint64_t)(vkObj));

    OBJTRACK_NODE* pNewObjNode = new OBJTRACK_NODE;
    pNewObjNode->objType = objType;
    pNewObjNode->status  = OBJSTATUS_NONE;
    pNewObjNode->vkObj  = (uint64_t)(vkObj);
    VkImageViewMap[(uint64_t)vkObj] = pNewObjNode;
    uint32_t objIndex = objTypeToIndex(objType);
    numObjs[objIndex]++;
    numTotalObjs++;
}

// CODEGEN : file ../vk-layer-generate.py line #866
static void destroy_image_view(VkDevice dispatchable_object, VkImageView object)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkImageViewMap.find(object_handle) != VkImageViewMap.end()) {
        OBJTRACK_NODE* pNode = VkImageViewMap[(uint64_t)object];
        uint32_t objIndex = objTypeToIndex(pNode->objType);
        assert(numTotalObjs > 0);
        numTotalObjs--;
        assert(numObjs[objIndex] > 0);
        numObjs[objIndex]--;
        log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, pNode->objType, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
           "OBJ_STAT Destroy %s obj 0x%" PRIxLEAST64 " (%" PRIu64 " total objs remain & %" PRIu64 " %s objs).",
            string_VkDebugReportObjectTypeEXT(pNode->objType), (uint64_t)(object), numTotalObjs, numObjs[objIndex],
            string_VkDebugReportObjectTypeEXT(pNode->objType));
        delete pNode;
        VkImageViewMap.erase(object_handle);
    } else {
        log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT ) 0, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
            "Unable to remove obj 0x%" PRIxLEAST64 ". Was it created? Has it already been destroyed?",
           object_handle);
    }
}

// CODEGEN : file ../vk-layer-generate.py line #893
static VkBool32 set_image_view_status(VkDevice dispatchable_object, VkImageView object, VkDebugReportObjectTypeEXT objType, ObjectStatusFlags status_flag)
{
    if (object != VK_NULL_HANDLE) {
        uint64_t object_handle = (uint64_t)(object);
        if (VkImageViewMap.find(object_handle) != VkImageViewMap.end()) {
            OBJTRACK_NODE* pNode = VkImageViewMap[object_handle];
            pNode->status |= status_flag;
        }
        else {
            // If we do not find it print an error
            return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT ) 0, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
                "Unable to set status for non-existent object 0x%" PRIxLEAST64 " of %s type",
                object_handle, string_VkDebugReportObjectTypeEXT(objType));
        }
    }
    return VK_FALSE;
}

// CODEGEN : file ../vk-layer-generate.py line #915
static VkBool32 validate_image_view_status(
VkDevice dispatchable_object, VkImageView object,
    VkDebugReportObjectTypeEXT     objType,
    ObjectStatusFlags   status_mask,
    ObjectStatusFlags   status_flag,
    VkFlags             msg_flags,
    OBJECT_TRACK_ERROR  error_code,
    const char         *fail_msg)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkImageViewMap.find(object_handle) != VkImageViewMap.end()) {
        OBJTRACK_NODE* pNode = VkImageViewMap[object_handle];
        if ((pNode->status & status_mask) != status_flag) {
            log_msg(mdd(dispatchable_object), msg_flags, pNode->objType, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
                "OBJECT VALIDATION WARNING: %s object 0x%" PRIxLEAST64 ": %s", string_VkDebugReportObjectTypeEXT(objType),
                 object_handle, fail_msg);
            return VK_FALSE;
        }
        return VK_TRUE;
    }
    else {
        // If we do not find it print an error
        log_msg(mdd(dispatchable_object), msg_flags, (VkDebugReportObjectTypeEXT) 0, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
            "Unable to obtain status for non-existent object 0x%" PRIxLEAST64 " of %s type",
            object_handle, string_VkDebugReportObjectTypeEXT(objType));
        return VK_FALSE;
    }
}

// CODEGEN : file ../vk-layer-generate.py line #948
static VkBool32 reset_image_view_status(VkDevice dispatchable_object, VkImageView object, VkDebugReportObjectTypeEXT objType, ObjectStatusFlags status_flag)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkImageViewMap.find(object_handle) != VkImageViewMap.end()) {
        OBJTRACK_NODE* pNode = VkImageViewMap[object_handle];
        pNode->status &= ~status_flag;
    }
    else {
        // If we do not find it print an error
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
            "Unable to reset status for non-existent object 0x%" PRIxLEAST64 " of %s type",
            object_handle, string_VkDebugReportObjectTypeEXT(objType));
    }
    return VK_FALSE;
}

// CODEGEN : file ../vk-layer-generate.py line #844
static void create_shader_module(VkDevice dispatchable_object, VkShaderModule vkObj, VkDebugReportObjectTypeEXT objType)
{
    log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, objType,(uint64_t)(vkObj), __LINE__, OBJTRACK_NONE, "OBJTRACK",
        "OBJ[%llu] : CREATE %s object 0x%" PRIxLEAST64 , object_track_index++, string_VkDebugReportObjectTypeEXT(objType),
        (uint64_t)(vkObj));

    OBJTRACK_NODE* pNewObjNode = new OBJTRACK_NODE;
    pNewObjNode->objType = objType;
    pNewObjNode->status  = OBJSTATUS_NONE;
    pNewObjNode->vkObj  = (uint64_t)(vkObj);
    VkShaderModuleMap[(uint64_t)vkObj] = pNewObjNode;
    uint32_t objIndex = objTypeToIndex(objType);
    numObjs[objIndex]++;
    numTotalObjs++;
}

// CODEGEN : file ../vk-layer-generate.py line #866
static void destroy_shader_module(VkDevice dispatchable_object, VkShaderModule object)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkShaderModuleMap.find(object_handle) != VkShaderModuleMap.end()) {
        OBJTRACK_NODE* pNode = VkShaderModuleMap[(uint64_t)object];
        uint32_t objIndex = objTypeToIndex(pNode->objType);
        assert(numTotalObjs > 0);
        numTotalObjs--;
        assert(numObjs[objIndex] > 0);
        numObjs[objIndex]--;
        log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, pNode->objType, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
           "OBJ_STAT Destroy %s obj 0x%" PRIxLEAST64 " (%" PRIu64 " total objs remain & %" PRIu64 " %s objs).",
            string_VkDebugReportObjectTypeEXT(pNode->objType), (uint64_t)(object), numTotalObjs, numObjs[objIndex],
            string_VkDebugReportObjectTypeEXT(pNode->objType));
        delete pNode;
        VkShaderModuleMap.erase(object_handle);
    } else {
        log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT ) 0, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
            "Unable to remove obj 0x%" PRIxLEAST64 ". Was it created? Has it already been destroyed?",
           object_handle);
    }
}

// CODEGEN : file ../vk-layer-generate.py line #893
static VkBool32 set_shader_module_status(VkDevice dispatchable_object, VkShaderModule object, VkDebugReportObjectTypeEXT objType, ObjectStatusFlags status_flag)
{
    if (object != VK_NULL_HANDLE) {
        uint64_t object_handle = (uint64_t)(object);
        if (VkShaderModuleMap.find(object_handle) != VkShaderModuleMap.end()) {
            OBJTRACK_NODE* pNode = VkShaderModuleMap[object_handle];
            pNode->status |= status_flag;
        }
        else {
            // If we do not find it print an error
            return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT ) 0, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
                "Unable to set status for non-existent object 0x%" PRIxLEAST64 " of %s type",
                object_handle, string_VkDebugReportObjectTypeEXT(objType));
        }
    }
    return VK_FALSE;
}

// CODEGEN : file ../vk-layer-generate.py line #915
static VkBool32 validate_shader_module_status(
VkDevice dispatchable_object, VkShaderModule object,
    VkDebugReportObjectTypeEXT     objType,
    ObjectStatusFlags   status_mask,
    ObjectStatusFlags   status_flag,
    VkFlags             msg_flags,
    OBJECT_TRACK_ERROR  error_code,
    const char         *fail_msg)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkShaderModuleMap.find(object_handle) != VkShaderModuleMap.end()) {
        OBJTRACK_NODE* pNode = VkShaderModuleMap[object_handle];
        if ((pNode->status & status_mask) != status_flag) {
            log_msg(mdd(dispatchable_object), msg_flags, pNode->objType, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
                "OBJECT VALIDATION WARNING: %s object 0x%" PRIxLEAST64 ": %s", string_VkDebugReportObjectTypeEXT(objType),
                 object_handle, fail_msg);
            return VK_FALSE;
        }
        return VK_TRUE;
    }
    else {
        // If we do not find it print an error
        log_msg(mdd(dispatchable_object), msg_flags, (VkDebugReportObjectTypeEXT) 0, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
            "Unable to obtain status for non-existent object 0x%" PRIxLEAST64 " of %s type",
            object_handle, string_VkDebugReportObjectTypeEXT(objType));
        return VK_FALSE;
    }
}

// CODEGEN : file ../vk-layer-generate.py line #948
static VkBool32 reset_shader_module_status(VkDevice dispatchable_object, VkShaderModule object, VkDebugReportObjectTypeEXT objType, ObjectStatusFlags status_flag)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkShaderModuleMap.find(object_handle) != VkShaderModuleMap.end()) {
        OBJTRACK_NODE* pNode = VkShaderModuleMap[object_handle];
        pNode->status &= ~status_flag;
    }
    else {
        // If we do not find it print an error
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
            "Unable to reset status for non-existent object 0x%" PRIxLEAST64 " of %s type",
            object_handle, string_VkDebugReportObjectTypeEXT(objType));
    }
    return VK_FALSE;
}

// CODEGEN : file ../vk-layer-generate.py line #844
static void create_pipeline_cache(VkDevice dispatchable_object, VkPipelineCache vkObj, VkDebugReportObjectTypeEXT objType)
{
    log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, objType,(uint64_t)(vkObj), __LINE__, OBJTRACK_NONE, "OBJTRACK",
        "OBJ[%llu] : CREATE %s object 0x%" PRIxLEAST64 , object_track_index++, string_VkDebugReportObjectTypeEXT(objType),
        (uint64_t)(vkObj));

    OBJTRACK_NODE* pNewObjNode = new OBJTRACK_NODE;
    pNewObjNode->objType = objType;
    pNewObjNode->status  = OBJSTATUS_NONE;
    pNewObjNode->vkObj  = (uint64_t)(vkObj);
    VkPipelineCacheMap[(uint64_t)vkObj] = pNewObjNode;
    uint32_t objIndex = objTypeToIndex(objType);
    numObjs[objIndex]++;
    numTotalObjs++;
}

// CODEGEN : file ../vk-layer-generate.py line #866
static void destroy_pipeline_cache(VkDevice dispatchable_object, VkPipelineCache object)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkPipelineCacheMap.find(object_handle) != VkPipelineCacheMap.end()) {
        OBJTRACK_NODE* pNode = VkPipelineCacheMap[(uint64_t)object];
        uint32_t objIndex = objTypeToIndex(pNode->objType);
        assert(numTotalObjs > 0);
        numTotalObjs--;
        assert(numObjs[objIndex] > 0);
        numObjs[objIndex]--;
        log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, pNode->objType, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
           "OBJ_STAT Destroy %s obj 0x%" PRIxLEAST64 " (%" PRIu64 " total objs remain & %" PRIu64 " %s objs).",
            string_VkDebugReportObjectTypeEXT(pNode->objType), (uint64_t)(object), numTotalObjs, numObjs[objIndex],
            string_VkDebugReportObjectTypeEXT(pNode->objType));
        delete pNode;
        VkPipelineCacheMap.erase(object_handle);
    } else {
        log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT ) 0, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
            "Unable to remove obj 0x%" PRIxLEAST64 ". Was it created? Has it already been destroyed?",
           object_handle);
    }
}

// CODEGEN : file ../vk-layer-generate.py line #893
static VkBool32 set_pipeline_cache_status(VkDevice dispatchable_object, VkPipelineCache object, VkDebugReportObjectTypeEXT objType, ObjectStatusFlags status_flag)
{
    if (object != VK_NULL_HANDLE) {
        uint64_t object_handle = (uint64_t)(object);
        if (VkPipelineCacheMap.find(object_handle) != VkPipelineCacheMap.end()) {
            OBJTRACK_NODE* pNode = VkPipelineCacheMap[object_handle];
            pNode->status |= status_flag;
        }
        else {
            // If we do not find it print an error
            return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT ) 0, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
                "Unable to set status for non-existent object 0x%" PRIxLEAST64 " of %s type",
                object_handle, string_VkDebugReportObjectTypeEXT(objType));
        }
    }
    return VK_FALSE;
}

// CODEGEN : file ../vk-layer-generate.py line #915
static VkBool32 validate_pipeline_cache_status(
VkDevice dispatchable_object, VkPipelineCache object,
    VkDebugReportObjectTypeEXT     objType,
    ObjectStatusFlags   status_mask,
    ObjectStatusFlags   status_flag,
    VkFlags             msg_flags,
    OBJECT_TRACK_ERROR  error_code,
    const char         *fail_msg)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkPipelineCacheMap.find(object_handle) != VkPipelineCacheMap.end()) {
        OBJTRACK_NODE* pNode = VkPipelineCacheMap[object_handle];
        if ((pNode->status & status_mask) != status_flag) {
            log_msg(mdd(dispatchable_object), msg_flags, pNode->objType, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
                "OBJECT VALIDATION WARNING: %s object 0x%" PRIxLEAST64 ": %s", string_VkDebugReportObjectTypeEXT(objType),
                 object_handle, fail_msg);
            return VK_FALSE;
        }
        return VK_TRUE;
    }
    else {
        // If we do not find it print an error
        log_msg(mdd(dispatchable_object), msg_flags, (VkDebugReportObjectTypeEXT) 0, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
            "Unable to obtain status for non-existent object 0x%" PRIxLEAST64 " of %s type",
            object_handle, string_VkDebugReportObjectTypeEXT(objType));
        return VK_FALSE;
    }
}

// CODEGEN : file ../vk-layer-generate.py line #948
static VkBool32 reset_pipeline_cache_status(VkDevice dispatchable_object, VkPipelineCache object, VkDebugReportObjectTypeEXT objType, ObjectStatusFlags status_flag)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkPipelineCacheMap.find(object_handle) != VkPipelineCacheMap.end()) {
        OBJTRACK_NODE* pNode = VkPipelineCacheMap[object_handle];
        pNode->status &= ~status_flag;
    }
    else {
        // If we do not find it print an error
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
            "Unable to reset status for non-existent object 0x%" PRIxLEAST64 " of %s type",
            object_handle, string_VkDebugReportObjectTypeEXT(objType));
    }
    return VK_FALSE;
}

// CODEGEN : file ../vk-layer-generate.py line #844
static void create_pipeline_layout(VkDevice dispatchable_object, VkPipelineLayout vkObj, VkDebugReportObjectTypeEXT objType)
{
    log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, objType,(uint64_t)(vkObj), __LINE__, OBJTRACK_NONE, "OBJTRACK",
        "OBJ[%llu] : CREATE %s object 0x%" PRIxLEAST64 , object_track_index++, string_VkDebugReportObjectTypeEXT(objType),
        (uint64_t)(vkObj));

    OBJTRACK_NODE* pNewObjNode = new OBJTRACK_NODE;
    pNewObjNode->objType = objType;
    pNewObjNode->status  = OBJSTATUS_NONE;
    pNewObjNode->vkObj  = (uint64_t)(vkObj);
    VkPipelineLayoutMap[(uint64_t)vkObj] = pNewObjNode;
    uint32_t objIndex = objTypeToIndex(objType);
    numObjs[objIndex]++;
    numTotalObjs++;
}

// CODEGEN : file ../vk-layer-generate.py line #866
static void destroy_pipeline_layout(VkDevice dispatchable_object, VkPipelineLayout object)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkPipelineLayoutMap.find(object_handle) != VkPipelineLayoutMap.end()) {
        OBJTRACK_NODE* pNode = VkPipelineLayoutMap[(uint64_t)object];
        uint32_t objIndex = objTypeToIndex(pNode->objType);
        assert(numTotalObjs > 0);
        numTotalObjs--;
        assert(numObjs[objIndex] > 0);
        numObjs[objIndex]--;
        log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, pNode->objType, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
           "OBJ_STAT Destroy %s obj 0x%" PRIxLEAST64 " (%" PRIu64 " total objs remain & %" PRIu64 " %s objs).",
            string_VkDebugReportObjectTypeEXT(pNode->objType), (uint64_t)(object), numTotalObjs, numObjs[objIndex],
            string_VkDebugReportObjectTypeEXT(pNode->objType));
        delete pNode;
        VkPipelineLayoutMap.erase(object_handle);
    } else {
        log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT ) 0, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
            "Unable to remove obj 0x%" PRIxLEAST64 ". Was it created? Has it already been destroyed?",
           object_handle);
    }
}

// CODEGEN : file ../vk-layer-generate.py line #893
static VkBool32 set_pipeline_layout_status(VkDevice dispatchable_object, VkPipelineLayout object, VkDebugReportObjectTypeEXT objType, ObjectStatusFlags status_flag)
{
    if (object != VK_NULL_HANDLE) {
        uint64_t object_handle = (uint64_t)(object);
        if (VkPipelineLayoutMap.find(object_handle) != VkPipelineLayoutMap.end()) {
            OBJTRACK_NODE* pNode = VkPipelineLayoutMap[object_handle];
            pNode->status |= status_flag;
        }
        else {
            // If we do not find it print an error
            return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT ) 0, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
                "Unable to set status for non-existent object 0x%" PRIxLEAST64 " of %s type",
                object_handle, string_VkDebugReportObjectTypeEXT(objType));
        }
    }
    return VK_FALSE;
}

// CODEGEN : file ../vk-layer-generate.py line #915
static VkBool32 validate_pipeline_layout_status(
VkDevice dispatchable_object, VkPipelineLayout object,
    VkDebugReportObjectTypeEXT     objType,
    ObjectStatusFlags   status_mask,
    ObjectStatusFlags   status_flag,
    VkFlags             msg_flags,
    OBJECT_TRACK_ERROR  error_code,
    const char         *fail_msg)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkPipelineLayoutMap.find(object_handle) != VkPipelineLayoutMap.end()) {
        OBJTRACK_NODE* pNode = VkPipelineLayoutMap[object_handle];
        if ((pNode->status & status_mask) != status_flag) {
            log_msg(mdd(dispatchable_object), msg_flags, pNode->objType, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
                "OBJECT VALIDATION WARNING: %s object 0x%" PRIxLEAST64 ": %s", string_VkDebugReportObjectTypeEXT(objType),
                 object_handle, fail_msg);
            return VK_FALSE;
        }
        return VK_TRUE;
    }
    else {
        // If we do not find it print an error
        log_msg(mdd(dispatchable_object), msg_flags, (VkDebugReportObjectTypeEXT) 0, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
            "Unable to obtain status for non-existent object 0x%" PRIxLEAST64 " of %s type",
            object_handle, string_VkDebugReportObjectTypeEXT(objType));
        return VK_FALSE;
    }
}

// CODEGEN : file ../vk-layer-generate.py line #948
static VkBool32 reset_pipeline_layout_status(VkDevice dispatchable_object, VkPipelineLayout object, VkDebugReportObjectTypeEXT objType, ObjectStatusFlags status_flag)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkPipelineLayoutMap.find(object_handle) != VkPipelineLayoutMap.end()) {
        OBJTRACK_NODE* pNode = VkPipelineLayoutMap[object_handle];
        pNode->status &= ~status_flag;
    }
    else {
        // If we do not find it print an error
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
            "Unable to reset status for non-existent object 0x%" PRIxLEAST64 " of %s type",
            object_handle, string_VkDebugReportObjectTypeEXT(objType));
    }
    return VK_FALSE;
}

// CODEGEN : file ../vk-layer-generate.py line #844
static void create_pipeline(VkDevice dispatchable_object, VkPipeline vkObj, VkDebugReportObjectTypeEXT objType)
{
    log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, objType,(uint64_t)(vkObj), __LINE__, OBJTRACK_NONE, "OBJTRACK",
        "OBJ[%llu] : CREATE %s object 0x%" PRIxLEAST64 , object_track_index++, string_VkDebugReportObjectTypeEXT(objType),
        (uint64_t)(vkObj));

    OBJTRACK_NODE* pNewObjNode = new OBJTRACK_NODE;
    pNewObjNode->objType = objType;
    pNewObjNode->status  = OBJSTATUS_NONE;
    pNewObjNode->vkObj  = (uint64_t)(vkObj);
    VkPipelineMap[(uint64_t)vkObj] = pNewObjNode;
    uint32_t objIndex = objTypeToIndex(objType);
    numObjs[objIndex]++;
    numTotalObjs++;
}

// CODEGEN : file ../vk-layer-generate.py line #866
static void destroy_pipeline(VkDevice dispatchable_object, VkPipeline object)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkPipelineMap.find(object_handle) != VkPipelineMap.end()) {
        OBJTRACK_NODE* pNode = VkPipelineMap[(uint64_t)object];
        uint32_t objIndex = objTypeToIndex(pNode->objType);
        assert(numTotalObjs > 0);
        numTotalObjs--;
        assert(numObjs[objIndex] > 0);
        numObjs[objIndex]--;
        log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, pNode->objType, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
           "OBJ_STAT Destroy %s obj 0x%" PRIxLEAST64 " (%" PRIu64 " total objs remain & %" PRIu64 " %s objs).",
            string_VkDebugReportObjectTypeEXT(pNode->objType), (uint64_t)(object), numTotalObjs, numObjs[objIndex],
            string_VkDebugReportObjectTypeEXT(pNode->objType));
        delete pNode;
        VkPipelineMap.erase(object_handle);
    } else {
        log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT ) 0, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
            "Unable to remove obj 0x%" PRIxLEAST64 ". Was it created? Has it already been destroyed?",
           object_handle);
    }
}

// CODEGEN : file ../vk-layer-generate.py line #893
static VkBool32 set_pipeline_status(VkDevice dispatchable_object, VkPipeline object, VkDebugReportObjectTypeEXT objType, ObjectStatusFlags status_flag)
{
    if (object != VK_NULL_HANDLE) {
        uint64_t object_handle = (uint64_t)(object);
        if (VkPipelineMap.find(object_handle) != VkPipelineMap.end()) {
            OBJTRACK_NODE* pNode = VkPipelineMap[object_handle];
            pNode->status |= status_flag;
        }
        else {
            // If we do not find it print an error
            return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT ) 0, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
                "Unable to set status for non-existent object 0x%" PRIxLEAST64 " of %s type",
                object_handle, string_VkDebugReportObjectTypeEXT(objType));
        }
    }
    return VK_FALSE;
}

// CODEGEN : file ../vk-layer-generate.py line #915
static VkBool32 validate_pipeline_status(
VkDevice dispatchable_object, VkPipeline object,
    VkDebugReportObjectTypeEXT     objType,
    ObjectStatusFlags   status_mask,
    ObjectStatusFlags   status_flag,
    VkFlags             msg_flags,
    OBJECT_TRACK_ERROR  error_code,
    const char         *fail_msg)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkPipelineMap.find(object_handle) != VkPipelineMap.end()) {
        OBJTRACK_NODE* pNode = VkPipelineMap[object_handle];
        if ((pNode->status & status_mask) != status_flag) {
            log_msg(mdd(dispatchable_object), msg_flags, pNode->objType, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
                "OBJECT VALIDATION WARNING: %s object 0x%" PRIxLEAST64 ": %s", string_VkDebugReportObjectTypeEXT(objType),
                 object_handle, fail_msg);
            return VK_FALSE;
        }
        return VK_TRUE;
    }
    else {
        // If we do not find it print an error
        log_msg(mdd(dispatchable_object), msg_flags, (VkDebugReportObjectTypeEXT) 0, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
            "Unable to obtain status for non-existent object 0x%" PRIxLEAST64 " of %s type",
            object_handle, string_VkDebugReportObjectTypeEXT(objType));
        return VK_FALSE;
    }
}

// CODEGEN : file ../vk-layer-generate.py line #948
static VkBool32 reset_pipeline_status(VkDevice dispatchable_object, VkPipeline object, VkDebugReportObjectTypeEXT objType, ObjectStatusFlags status_flag)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkPipelineMap.find(object_handle) != VkPipelineMap.end()) {
        OBJTRACK_NODE* pNode = VkPipelineMap[object_handle];
        pNode->status &= ~status_flag;
    }
    else {
        // If we do not find it print an error
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
            "Unable to reset status for non-existent object 0x%" PRIxLEAST64 " of %s type",
            object_handle, string_VkDebugReportObjectTypeEXT(objType));
    }
    return VK_FALSE;
}

// CODEGEN : file ../vk-layer-generate.py line #844
static void create_descriptor_set_layout(VkDevice dispatchable_object, VkDescriptorSetLayout vkObj, VkDebugReportObjectTypeEXT objType)
{
    log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, objType,(uint64_t)(vkObj), __LINE__, OBJTRACK_NONE, "OBJTRACK",
        "OBJ[%llu] : CREATE %s object 0x%" PRIxLEAST64 , object_track_index++, string_VkDebugReportObjectTypeEXT(objType),
        (uint64_t)(vkObj));

    OBJTRACK_NODE* pNewObjNode = new OBJTRACK_NODE;
    pNewObjNode->objType = objType;
    pNewObjNode->status  = OBJSTATUS_NONE;
    pNewObjNode->vkObj  = (uint64_t)(vkObj);
    VkDescriptorSetLayoutMap[(uint64_t)vkObj] = pNewObjNode;
    uint32_t objIndex = objTypeToIndex(objType);
    numObjs[objIndex]++;
    numTotalObjs++;
}

// CODEGEN : file ../vk-layer-generate.py line #866
static void destroy_descriptor_set_layout(VkDevice dispatchable_object, VkDescriptorSetLayout object)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkDescriptorSetLayoutMap.find(object_handle) != VkDescriptorSetLayoutMap.end()) {
        OBJTRACK_NODE* pNode = VkDescriptorSetLayoutMap[(uint64_t)object];
        uint32_t objIndex = objTypeToIndex(pNode->objType);
        assert(numTotalObjs > 0);
        numTotalObjs--;
        assert(numObjs[objIndex] > 0);
        numObjs[objIndex]--;
        log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, pNode->objType, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
           "OBJ_STAT Destroy %s obj 0x%" PRIxLEAST64 " (%" PRIu64 " total objs remain & %" PRIu64 " %s objs).",
            string_VkDebugReportObjectTypeEXT(pNode->objType), (uint64_t)(object), numTotalObjs, numObjs[objIndex],
            string_VkDebugReportObjectTypeEXT(pNode->objType));
        delete pNode;
        VkDescriptorSetLayoutMap.erase(object_handle);
    } else {
        log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT ) 0, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
            "Unable to remove obj 0x%" PRIxLEAST64 ". Was it created? Has it already been destroyed?",
           object_handle);
    }
}

// CODEGEN : file ../vk-layer-generate.py line #893
static VkBool32 set_descriptor_set_layout_status(VkDevice dispatchable_object, VkDescriptorSetLayout object, VkDebugReportObjectTypeEXT objType, ObjectStatusFlags status_flag)
{
    if (object != VK_NULL_HANDLE) {
        uint64_t object_handle = (uint64_t)(object);
        if (VkDescriptorSetLayoutMap.find(object_handle) != VkDescriptorSetLayoutMap.end()) {
            OBJTRACK_NODE* pNode = VkDescriptorSetLayoutMap[object_handle];
            pNode->status |= status_flag;
        }
        else {
            // If we do not find it print an error
            return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT ) 0, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
                "Unable to set status for non-existent object 0x%" PRIxLEAST64 " of %s type",
                object_handle, string_VkDebugReportObjectTypeEXT(objType));
        }
    }
    return VK_FALSE;
}

// CODEGEN : file ../vk-layer-generate.py line #915
static VkBool32 validate_descriptor_set_layout_status(
VkDevice dispatchable_object, VkDescriptorSetLayout object,
    VkDebugReportObjectTypeEXT     objType,
    ObjectStatusFlags   status_mask,
    ObjectStatusFlags   status_flag,
    VkFlags             msg_flags,
    OBJECT_TRACK_ERROR  error_code,
    const char         *fail_msg)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkDescriptorSetLayoutMap.find(object_handle) != VkDescriptorSetLayoutMap.end()) {
        OBJTRACK_NODE* pNode = VkDescriptorSetLayoutMap[object_handle];
        if ((pNode->status & status_mask) != status_flag) {
            log_msg(mdd(dispatchable_object), msg_flags, pNode->objType, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
                "OBJECT VALIDATION WARNING: %s object 0x%" PRIxLEAST64 ": %s", string_VkDebugReportObjectTypeEXT(objType),
                 object_handle, fail_msg);
            return VK_FALSE;
        }
        return VK_TRUE;
    }
    else {
        // If we do not find it print an error
        log_msg(mdd(dispatchable_object), msg_flags, (VkDebugReportObjectTypeEXT) 0, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
            "Unable to obtain status for non-existent object 0x%" PRIxLEAST64 " of %s type",
            object_handle, string_VkDebugReportObjectTypeEXT(objType));
        return VK_FALSE;
    }
}

// CODEGEN : file ../vk-layer-generate.py line #948
static VkBool32 reset_descriptor_set_layout_status(VkDevice dispatchable_object, VkDescriptorSetLayout object, VkDebugReportObjectTypeEXT objType, ObjectStatusFlags status_flag)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkDescriptorSetLayoutMap.find(object_handle) != VkDescriptorSetLayoutMap.end()) {
        OBJTRACK_NODE* pNode = VkDescriptorSetLayoutMap[object_handle];
        pNode->status &= ~status_flag;
    }
    else {
        // If we do not find it print an error
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
            "Unable to reset status for non-existent object 0x%" PRIxLEAST64 " of %s type",
            object_handle, string_VkDebugReportObjectTypeEXT(objType));
    }
    return VK_FALSE;
}

// CODEGEN : file ../vk-layer-generate.py line #844
static void create_sampler(VkDevice dispatchable_object, VkSampler vkObj, VkDebugReportObjectTypeEXT objType)
{
    log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, objType,(uint64_t)(vkObj), __LINE__, OBJTRACK_NONE, "OBJTRACK",
        "OBJ[%llu] : CREATE %s object 0x%" PRIxLEAST64 , object_track_index++, string_VkDebugReportObjectTypeEXT(objType),
        (uint64_t)(vkObj));

    OBJTRACK_NODE* pNewObjNode = new OBJTRACK_NODE;
    pNewObjNode->objType = objType;
    pNewObjNode->status  = OBJSTATUS_NONE;
    pNewObjNode->vkObj  = (uint64_t)(vkObj);
    VkSamplerMap[(uint64_t)vkObj] = pNewObjNode;
    uint32_t objIndex = objTypeToIndex(objType);
    numObjs[objIndex]++;
    numTotalObjs++;
}

// CODEGEN : file ../vk-layer-generate.py line #866
static void destroy_sampler(VkDevice dispatchable_object, VkSampler object)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkSamplerMap.find(object_handle) != VkSamplerMap.end()) {
        OBJTRACK_NODE* pNode = VkSamplerMap[(uint64_t)object];
        uint32_t objIndex = objTypeToIndex(pNode->objType);
        assert(numTotalObjs > 0);
        numTotalObjs--;
        assert(numObjs[objIndex] > 0);
        numObjs[objIndex]--;
        log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, pNode->objType, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
           "OBJ_STAT Destroy %s obj 0x%" PRIxLEAST64 " (%" PRIu64 " total objs remain & %" PRIu64 " %s objs).",
            string_VkDebugReportObjectTypeEXT(pNode->objType), (uint64_t)(object), numTotalObjs, numObjs[objIndex],
            string_VkDebugReportObjectTypeEXT(pNode->objType));
        delete pNode;
        VkSamplerMap.erase(object_handle);
    } else {
        log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT ) 0, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
            "Unable to remove obj 0x%" PRIxLEAST64 ". Was it created? Has it already been destroyed?",
           object_handle);
    }
}

// CODEGEN : file ../vk-layer-generate.py line #893
static VkBool32 set_sampler_status(VkDevice dispatchable_object, VkSampler object, VkDebugReportObjectTypeEXT objType, ObjectStatusFlags status_flag)
{
    if (object != VK_NULL_HANDLE) {
        uint64_t object_handle = (uint64_t)(object);
        if (VkSamplerMap.find(object_handle) != VkSamplerMap.end()) {
            OBJTRACK_NODE* pNode = VkSamplerMap[object_handle];
            pNode->status |= status_flag;
        }
        else {
            // If we do not find it print an error
            return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT ) 0, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
                "Unable to set status for non-existent object 0x%" PRIxLEAST64 " of %s type",
                object_handle, string_VkDebugReportObjectTypeEXT(objType));
        }
    }
    return VK_FALSE;
}

// CODEGEN : file ../vk-layer-generate.py line #915
static VkBool32 validate_sampler_status(
VkDevice dispatchable_object, VkSampler object,
    VkDebugReportObjectTypeEXT     objType,
    ObjectStatusFlags   status_mask,
    ObjectStatusFlags   status_flag,
    VkFlags             msg_flags,
    OBJECT_TRACK_ERROR  error_code,
    const char         *fail_msg)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkSamplerMap.find(object_handle) != VkSamplerMap.end()) {
        OBJTRACK_NODE* pNode = VkSamplerMap[object_handle];
        if ((pNode->status & status_mask) != status_flag) {
            log_msg(mdd(dispatchable_object), msg_flags, pNode->objType, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
                "OBJECT VALIDATION WARNING: %s object 0x%" PRIxLEAST64 ": %s", string_VkDebugReportObjectTypeEXT(objType),
                 object_handle, fail_msg);
            return VK_FALSE;
        }
        return VK_TRUE;
    }
    else {
        // If we do not find it print an error
        log_msg(mdd(dispatchable_object), msg_flags, (VkDebugReportObjectTypeEXT) 0, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
            "Unable to obtain status for non-existent object 0x%" PRIxLEAST64 " of %s type",
            object_handle, string_VkDebugReportObjectTypeEXT(objType));
        return VK_FALSE;
    }
}

// CODEGEN : file ../vk-layer-generate.py line #948
static VkBool32 reset_sampler_status(VkDevice dispatchable_object, VkSampler object, VkDebugReportObjectTypeEXT objType, ObjectStatusFlags status_flag)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkSamplerMap.find(object_handle) != VkSamplerMap.end()) {
        OBJTRACK_NODE* pNode = VkSamplerMap[object_handle];
        pNode->status &= ~status_flag;
    }
    else {
        // If we do not find it print an error
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
            "Unable to reset status for non-existent object 0x%" PRIxLEAST64 " of %s type",
            object_handle, string_VkDebugReportObjectTypeEXT(objType));
    }
    return VK_FALSE;
}

// CODEGEN : file ../vk-layer-generate.py line #844
static void create_descriptor_pool(VkDevice dispatchable_object, VkDescriptorPool vkObj, VkDebugReportObjectTypeEXT objType)
{
    log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, objType,(uint64_t)(vkObj), __LINE__, OBJTRACK_NONE, "OBJTRACK",
        "OBJ[%llu] : CREATE %s object 0x%" PRIxLEAST64 , object_track_index++, string_VkDebugReportObjectTypeEXT(objType),
        (uint64_t)(vkObj));

    OBJTRACK_NODE* pNewObjNode = new OBJTRACK_NODE;
    pNewObjNode->objType = objType;
    pNewObjNode->status  = OBJSTATUS_NONE;
    pNewObjNode->vkObj  = (uint64_t)(vkObj);
    VkDescriptorPoolMap[(uint64_t)vkObj] = pNewObjNode;
    uint32_t objIndex = objTypeToIndex(objType);
    numObjs[objIndex]++;
    numTotalObjs++;
}

// CODEGEN : file ../vk-layer-generate.py line #866
static void destroy_descriptor_pool(VkDevice dispatchable_object, VkDescriptorPool object)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkDescriptorPoolMap.find(object_handle) != VkDescriptorPoolMap.end()) {
        OBJTRACK_NODE* pNode = VkDescriptorPoolMap[(uint64_t)object];
        uint32_t objIndex = objTypeToIndex(pNode->objType);
        assert(numTotalObjs > 0);
        numTotalObjs--;
        assert(numObjs[objIndex] > 0);
        numObjs[objIndex]--;
        log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, pNode->objType, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
           "OBJ_STAT Destroy %s obj 0x%" PRIxLEAST64 " (%" PRIu64 " total objs remain & %" PRIu64 " %s objs).",
            string_VkDebugReportObjectTypeEXT(pNode->objType), (uint64_t)(object), numTotalObjs, numObjs[objIndex],
            string_VkDebugReportObjectTypeEXT(pNode->objType));
        delete pNode;
        VkDescriptorPoolMap.erase(object_handle);
    } else {
        log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT ) 0, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
            "Unable to remove obj 0x%" PRIxLEAST64 ". Was it created? Has it already been destroyed?",
           object_handle);
    }
}

// CODEGEN : file ../vk-layer-generate.py line #893
static VkBool32 set_descriptor_pool_status(VkDevice dispatchable_object, VkDescriptorPool object, VkDebugReportObjectTypeEXT objType, ObjectStatusFlags status_flag)
{
    if (object != VK_NULL_HANDLE) {
        uint64_t object_handle = (uint64_t)(object);
        if (VkDescriptorPoolMap.find(object_handle) != VkDescriptorPoolMap.end()) {
            OBJTRACK_NODE* pNode = VkDescriptorPoolMap[object_handle];
            pNode->status |= status_flag;
        }
        else {
            // If we do not find it print an error
            return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT ) 0, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
                "Unable to set status for non-existent object 0x%" PRIxLEAST64 " of %s type",
                object_handle, string_VkDebugReportObjectTypeEXT(objType));
        }
    }
    return VK_FALSE;
}

// CODEGEN : file ../vk-layer-generate.py line #915
static VkBool32 validate_descriptor_pool_status(
VkDevice dispatchable_object, VkDescriptorPool object,
    VkDebugReportObjectTypeEXT     objType,
    ObjectStatusFlags   status_mask,
    ObjectStatusFlags   status_flag,
    VkFlags             msg_flags,
    OBJECT_TRACK_ERROR  error_code,
    const char         *fail_msg)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkDescriptorPoolMap.find(object_handle) != VkDescriptorPoolMap.end()) {
        OBJTRACK_NODE* pNode = VkDescriptorPoolMap[object_handle];
        if ((pNode->status & status_mask) != status_flag) {
            log_msg(mdd(dispatchable_object), msg_flags, pNode->objType, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
                "OBJECT VALIDATION WARNING: %s object 0x%" PRIxLEAST64 ": %s", string_VkDebugReportObjectTypeEXT(objType),
                 object_handle, fail_msg);
            return VK_FALSE;
        }
        return VK_TRUE;
    }
    else {
        // If we do not find it print an error
        log_msg(mdd(dispatchable_object), msg_flags, (VkDebugReportObjectTypeEXT) 0, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
            "Unable to obtain status for non-existent object 0x%" PRIxLEAST64 " of %s type",
            object_handle, string_VkDebugReportObjectTypeEXT(objType));
        return VK_FALSE;
    }
}

// CODEGEN : file ../vk-layer-generate.py line #948
static VkBool32 reset_descriptor_pool_status(VkDevice dispatchable_object, VkDescriptorPool object, VkDebugReportObjectTypeEXT objType, ObjectStatusFlags status_flag)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkDescriptorPoolMap.find(object_handle) != VkDescriptorPoolMap.end()) {
        OBJTRACK_NODE* pNode = VkDescriptorPoolMap[object_handle];
        pNode->status &= ~status_flag;
    }
    else {
        // If we do not find it print an error
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
            "Unable to reset status for non-existent object 0x%" PRIxLEAST64 " of %s type",
            object_handle, string_VkDebugReportObjectTypeEXT(objType));
    }
    return VK_FALSE;
}

// CODEGEN : file ../vk-layer-generate.py line #844
static void create_descriptor_set(VkDevice dispatchable_object, VkDescriptorSet vkObj, VkDebugReportObjectTypeEXT objType)
{
    log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, objType,(uint64_t)(vkObj), __LINE__, OBJTRACK_NONE, "OBJTRACK",
        "OBJ[%llu] : CREATE %s object 0x%" PRIxLEAST64 , object_track_index++, string_VkDebugReportObjectTypeEXT(objType),
        (uint64_t)(vkObj));

    OBJTRACK_NODE* pNewObjNode = new OBJTRACK_NODE;
    pNewObjNode->objType = objType;
    pNewObjNode->status  = OBJSTATUS_NONE;
    pNewObjNode->vkObj  = (uint64_t)(vkObj);
    VkDescriptorSetMap[(uint64_t)vkObj] = pNewObjNode;
    uint32_t objIndex = objTypeToIndex(objType);
    numObjs[objIndex]++;
    numTotalObjs++;
}

// CODEGEN : file ../vk-layer-generate.py line #866
static void destroy_descriptor_set(VkDevice dispatchable_object, VkDescriptorSet object)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkDescriptorSetMap.find(object_handle) != VkDescriptorSetMap.end()) {
        OBJTRACK_NODE* pNode = VkDescriptorSetMap[(uint64_t)object];
        uint32_t objIndex = objTypeToIndex(pNode->objType);
        assert(numTotalObjs > 0);
        numTotalObjs--;
        assert(numObjs[objIndex] > 0);
        numObjs[objIndex]--;
        log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, pNode->objType, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
           "OBJ_STAT Destroy %s obj 0x%" PRIxLEAST64 " (%" PRIu64 " total objs remain & %" PRIu64 " %s objs).",
            string_VkDebugReportObjectTypeEXT(pNode->objType), (uint64_t)(object), numTotalObjs, numObjs[objIndex],
            string_VkDebugReportObjectTypeEXT(pNode->objType));
        delete pNode;
        VkDescriptorSetMap.erase(object_handle);
    } else {
        log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT ) 0, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
            "Unable to remove obj 0x%" PRIxLEAST64 ". Was it created? Has it already been destroyed?",
           object_handle);
    }
}

// CODEGEN : file ../vk-layer-generate.py line #893
static VkBool32 set_descriptor_set_status(VkDevice dispatchable_object, VkDescriptorSet object, VkDebugReportObjectTypeEXT objType, ObjectStatusFlags status_flag)
{
    if (object != VK_NULL_HANDLE) {
        uint64_t object_handle = (uint64_t)(object);
        if (VkDescriptorSetMap.find(object_handle) != VkDescriptorSetMap.end()) {
            OBJTRACK_NODE* pNode = VkDescriptorSetMap[object_handle];
            pNode->status |= status_flag;
        }
        else {
            // If we do not find it print an error
            return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT ) 0, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
                "Unable to set status for non-existent object 0x%" PRIxLEAST64 " of %s type",
                object_handle, string_VkDebugReportObjectTypeEXT(objType));
        }
    }
    return VK_FALSE;
}

// CODEGEN : file ../vk-layer-generate.py line #915
static VkBool32 validate_descriptor_set_status(
VkDevice dispatchable_object, VkDescriptorSet object,
    VkDebugReportObjectTypeEXT     objType,
    ObjectStatusFlags   status_mask,
    ObjectStatusFlags   status_flag,
    VkFlags             msg_flags,
    OBJECT_TRACK_ERROR  error_code,
    const char         *fail_msg)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkDescriptorSetMap.find(object_handle) != VkDescriptorSetMap.end()) {
        OBJTRACK_NODE* pNode = VkDescriptorSetMap[object_handle];
        if ((pNode->status & status_mask) != status_flag) {
            log_msg(mdd(dispatchable_object), msg_flags, pNode->objType, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
                "OBJECT VALIDATION WARNING: %s object 0x%" PRIxLEAST64 ": %s", string_VkDebugReportObjectTypeEXT(objType),
                 object_handle, fail_msg);
            return VK_FALSE;
        }
        return VK_TRUE;
    }
    else {
        // If we do not find it print an error
        log_msg(mdd(dispatchable_object), msg_flags, (VkDebugReportObjectTypeEXT) 0, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
            "Unable to obtain status for non-existent object 0x%" PRIxLEAST64 " of %s type",
            object_handle, string_VkDebugReportObjectTypeEXT(objType));
        return VK_FALSE;
    }
}

// CODEGEN : file ../vk-layer-generate.py line #948
static VkBool32 reset_descriptor_set_status(VkDevice dispatchable_object, VkDescriptorSet object, VkDebugReportObjectTypeEXT objType, ObjectStatusFlags status_flag)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkDescriptorSetMap.find(object_handle) != VkDescriptorSetMap.end()) {
        OBJTRACK_NODE* pNode = VkDescriptorSetMap[object_handle];
        pNode->status &= ~status_flag;
    }
    else {
        // If we do not find it print an error
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
            "Unable to reset status for non-existent object 0x%" PRIxLEAST64 " of %s type",
            object_handle, string_VkDebugReportObjectTypeEXT(objType));
    }
    return VK_FALSE;
}

// CODEGEN : file ../vk-layer-generate.py line #844
static void create_render_pass(VkDevice dispatchable_object, VkRenderPass vkObj, VkDebugReportObjectTypeEXT objType)
{
    log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, objType,(uint64_t)(vkObj), __LINE__, OBJTRACK_NONE, "OBJTRACK",
        "OBJ[%llu] : CREATE %s object 0x%" PRIxLEAST64 , object_track_index++, string_VkDebugReportObjectTypeEXT(objType),
        (uint64_t)(vkObj));

    OBJTRACK_NODE* pNewObjNode = new OBJTRACK_NODE;
    pNewObjNode->objType = objType;
    pNewObjNode->status  = OBJSTATUS_NONE;
    pNewObjNode->vkObj  = (uint64_t)(vkObj);
    VkRenderPassMap[(uint64_t)vkObj] = pNewObjNode;
    uint32_t objIndex = objTypeToIndex(objType);
    numObjs[objIndex]++;
    numTotalObjs++;
}

// CODEGEN : file ../vk-layer-generate.py line #866
static void destroy_render_pass(VkDevice dispatchable_object, VkRenderPass object)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkRenderPassMap.find(object_handle) != VkRenderPassMap.end()) {
        OBJTRACK_NODE* pNode = VkRenderPassMap[(uint64_t)object];
        uint32_t objIndex = objTypeToIndex(pNode->objType);
        assert(numTotalObjs > 0);
        numTotalObjs--;
        assert(numObjs[objIndex] > 0);
        numObjs[objIndex]--;
        log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, pNode->objType, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
           "OBJ_STAT Destroy %s obj 0x%" PRIxLEAST64 " (%" PRIu64 " total objs remain & %" PRIu64 " %s objs).",
            string_VkDebugReportObjectTypeEXT(pNode->objType), (uint64_t)(object), numTotalObjs, numObjs[objIndex],
            string_VkDebugReportObjectTypeEXT(pNode->objType));
        delete pNode;
        VkRenderPassMap.erase(object_handle);
    } else {
        log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT ) 0, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
            "Unable to remove obj 0x%" PRIxLEAST64 ". Was it created? Has it already been destroyed?",
           object_handle);
    }
}

// CODEGEN : file ../vk-layer-generate.py line #893
static VkBool32 set_render_pass_status(VkDevice dispatchable_object, VkRenderPass object, VkDebugReportObjectTypeEXT objType, ObjectStatusFlags status_flag)
{
    if (object != VK_NULL_HANDLE) {
        uint64_t object_handle = (uint64_t)(object);
        if (VkRenderPassMap.find(object_handle) != VkRenderPassMap.end()) {
            OBJTRACK_NODE* pNode = VkRenderPassMap[object_handle];
            pNode->status |= status_flag;
        }
        else {
            // If we do not find it print an error
            return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT ) 0, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
                "Unable to set status for non-existent object 0x%" PRIxLEAST64 " of %s type",
                object_handle, string_VkDebugReportObjectTypeEXT(objType));
        }
    }
    return VK_FALSE;
}

// CODEGEN : file ../vk-layer-generate.py line #915
static VkBool32 validate_render_pass_status(
VkDevice dispatchable_object, VkRenderPass object,
    VkDebugReportObjectTypeEXT     objType,
    ObjectStatusFlags   status_mask,
    ObjectStatusFlags   status_flag,
    VkFlags             msg_flags,
    OBJECT_TRACK_ERROR  error_code,
    const char         *fail_msg)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkRenderPassMap.find(object_handle) != VkRenderPassMap.end()) {
        OBJTRACK_NODE* pNode = VkRenderPassMap[object_handle];
        if ((pNode->status & status_mask) != status_flag) {
            log_msg(mdd(dispatchable_object), msg_flags, pNode->objType, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
                "OBJECT VALIDATION WARNING: %s object 0x%" PRIxLEAST64 ": %s", string_VkDebugReportObjectTypeEXT(objType),
                 object_handle, fail_msg);
            return VK_FALSE;
        }
        return VK_TRUE;
    }
    else {
        // If we do not find it print an error
        log_msg(mdd(dispatchable_object), msg_flags, (VkDebugReportObjectTypeEXT) 0, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
            "Unable to obtain status for non-existent object 0x%" PRIxLEAST64 " of %s type",
            object_handle, string_VkDebugReportObjectTypeEXT(objType));
        return VK_FALSE;
    }
}

// CODEGEN : file ../vk-layer-generate.py line #948
static VkBool32 reset_render_pass_status(VkDevice dispatchable_object, VkRenderPass object, VkDebugReportObjectTypeEXT objType, ObjectStatusFlags status_flag)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkRenderPassMap.find(object_handle) != VkRenderPassMap.end()) {
        OBJTRACK_NODE* pNode = VkRenderPassMap[object_handle];
        pNode->status &= ~status_flag;
    }
    else {
        // If we do not find it print an error
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
            "Unable to reset status for non-existent object 0x%" PRIxLEAST64 " of %s type",
            object_handle, string_VkDebugReportObjectTypeEXT(objType));
    }
    return VK_FALSE;
}

// CODEGEN : file ../vk-layer-generate.py line #844
static void create_framebuffer(VkDevice dispatchable_object, VkFramebuffer vkObj, VkDebugReportObjectTypeEXT objType)
{
    log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, objType,(uint64_t)(vkObj), __LINE__, OBJTRACK_NONE, "OBJTRACK",
        "OBJ[%llu] : CREATE %s object 0x%" PRIxLEAST64 , object_track_index++, string_VkDebugReportObjectTypeEXT(objType),
        (uint64_t)(vkObj));

    OBJTRACK_NODE* pNewObjNode = new OBJTRACK_NODE;
    pNewObjNode->objType = objType;
    pNewObjNode->status  = OBJSTATUS_NONE;
    pNewObjNode->vkObj  = (uint64_t)(vkObj);
    VkFramebufferMap[(uint64_t)vkObj] = pNewObjNode;
    uint32_t objIndex = objTypeToIndex(objType);
    numObjs[objIndex]++;
    numTotalObjs++;
}

// CODEGEN : file ../vk-layer-generate.py line #866
static void destroy_framebuffer(VkDevice dispatchable_object, VkFramebuffer object)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkFramebufferMap.find(object_handle) != VkFramebufferMap.end()) {
        OBJTRACK_NODE* pNode = VkFramebufferMap[(uint64_t)object];
        uint32_t objIndex = objTypeToIndex(pNode->objType);
        assert(numTotalObjs > 0);
        numTotalObjs--;
        assert(numObjs[objIndex] > 0);
        numObjs[objIndex]--;
        log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, pNode->objType, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
           "OBJ_STAT Destroy %s obj 0x%" PRIxLEAST64 " (%" PRIu64 " total objs remain & %" PRIu64 " %s objs).",
            string_VkDebugReportObjectTypeEXT(pNode->objType), (uint64_t)(object), numTotalObjs, numObjs[objIndex],
            string_VkDebugReportObjectTypeEXT(pNode->objType));
        delete pNode;
        VkFramebufferMap.erase(object_handle);
    } else {
        log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT ) 0, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
            "Unable to remove obj 0x%" PRIxLEAST64 ". Was it created? Has it already been destroyed?",
           object_handle);
    }
}

// CODEGEN : file ../vk-layer-generate.py line #893
static VkBool32 set_framebuffer_status(VkDevice dispatchable_object, VkFramebuffer object, VkDebugReportObjectTypeEXT objType, ObjectStatusFlags status_flag)
{
    if (object != VK_NULL_HANDLE) {
        uint64_t object_handle = (uint64_t)(object);
        if (VkFramebufferMap.find(object_handle) != VkFramebufferMap.end()) {
            OBJTRACK_NODE* pNode = VkFramebufferMap[object_handle];
            pNode->status |= status_flag;
        }
        else {
            // If we do not find it print an error
            return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT ) 0, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
                "Unable to set status for non-existent object 0x%" PRIxLEAST64 " of %s type",
                object_handle, string_VkDebugReportObjectTypeEXT(objType));
        }
    }
    return VK_FALSE;
}

// CODEGEN : file ../vk-layer-generate.py line #915
static VkBool32 validate_framebuffer_status(
VkDevice dispatchable_object, VkFramebuffer object,
    VkDebugReportObjectTypeEXT     objType,
    ObjectStatusFlags   status_mask,
    ObjectStatusFlags   status_flag,
    VkFlags             msg_flags,
    OBJECT_TRACK_ERROR  error_code,
    const char         *fail_msg)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkFramebufferMap.find(object_handle) != VkFramebufferMap.end()) {
        OBJTRACK_NODE* pNode = VkFramebufferMap[object_handle];
        if ((pNode->status & status_mask) != status_flag) {
            log_msg(mdd(dispatchable_object), msg_flags, pNode->objType, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
                "OBJECT VALIDATION WARNING: %s object 0x%" PRIxLEAST64 ": %s", string_VkDebugReportObjectTypeEXT(objType),
                 object_handle, fail_msg);
            return VK_FALSE;
        }
        return VK_TRUE;
    }
    else {
        // If we do not find it print an error
        log_msg(mdd(dispatchable_object), msg_flags, (VkDebugReportObjectTypeEXT) 0, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
            "Unable to obtain status for non-existent object 0x%" PRIxLEAST64 " of %s type",
            object_handle, string_VkDebugReportObjectTypeEXT(objType));
        return VK_FALSE;
    }
}

// CODEGEN : file ../vk-layer-generate.py line #948
static VkBool32 reset_framebuffer_status(VkDevice dispatchable_object, VkFramebuffer object, VkDebugReportObjectTypeEXT objType, ObjectStatusFlags status_flag)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkFramebufferMap.find(object_handle) != VkFramebufferMap.end()) {
        OBJTRACK_NODE* pNode = VkFramebufferMap[object_handle];
        pNode->status &= ~status_flag;
    }
    else {
        // If we do not find it print an error
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
            "Unable to reset status for non-existent object 0x%" PRIxLEAST64 " of %s type",
            object_handle, string_VkDebugReportObjectTypeEXT(objType));
    }
    return VK_FALSE;
}

// CODEGEN : file ../vk-layer-generate.py line #844
static void create_swapchain_khr(VkDevice dispatchable_object, VkSwapchainKHR vkObj, VkDebugReportObjectTypeEXT objType)
{
    log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, objType,(uint64_t)(vkObj), __LINE__, OBJTRACK_NONE, "OBJTRACK",
        "OBJ[%llu] : CREATE %s object 0x%" PRIxLEAST64 , object_track_index++, string_VkDebugReportObjectTypeEXT(objType),
        (uint64_t)(vkObj));

    OBJTRACK_NODE* pNewObjNode = new OBJTRACK_NODE;
    pNewObjNode->objType = objType;
    pNewObjNode->status  = OBJSTATUS_NONE;
    pNewObjNode->vkObj  = (uint64_t)(vkObj);
    VkSwapchainKHRMap[(uint64_t)vkObj] = pNewObjNode;
    uint32_t objIndex = objTypeToIndex(objType);
    numObjs[objIndex]++;
    numTotalObjs++;
}

// CODEGEN : file ../vk-layer-generate.py line #866
static void destroy_swapchain_khr(VkDevice dispatchable_object, VkSwapchainKHR object)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkSwapchainKHRMap.find(object_handle) != VkSwapchainKHRMap.end()) {
        OBJTRACK_NODE* pNode = VkSwapchainKHRMap[(uint64_t)object];
        uint32_t objIndex = objTypeToIndex(pNode->objType);
        assert(numTotalObjs > 0);
        numTotalObjs--;
        assert(numObjs[objIndex] > 0);
        numObjs[objIndex]--;
        log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, pNode->objType, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
           "OBJ_STAT Destroy %s obj 0x%" PRIxLEAST64 " (%" PRIu64 " total objs remain & %" PRIu64 " %s objs).",
            string_VkDebugReportObjectTypeEXT(pNode->objType), (uint64_t)(object), numTotalObjs, numObjs[objIndex],
            string_VkDebugReportObjectTypeEXT(pNode->objType));
        delete pNode;
        VkSwapchainKHRMap.erase(object_handle);
    } else {
        log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT ) 0, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
            "Unable to remove obj 0x%" PRIxLEAST64 ". Was it created? Has it already been destroyed?",
           object_handle);
    }
}

// CODEGEN : file ../vk-layer-generate.py line #893
static VkBool32 set_swapchain_khr_status(VkDevice dispatchable_object, VkSwapchainKHR object, VkDebugReportObjectTypeEXT objType, ObjectStatusFlags status_flag)
{
    if (object != VK_NULL_HANDLE) {
        uint64_t object_handle = (uint64_t)(object);
        if (VkSwapchainKHRMap.find(object_handle) != VkSwapchainKHRMap.end()) {
            OBJTRACK_NODE* pNode = VkSwapchainKHRMap[object_handle];
            pNode->status |= status_flag;
        }
        else {
            // If we do not find it print an error
            return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT ) 0, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
                "Unable to set status for non-existent object 0x%" PRIxLEAST64 " of %s type",
                object_handle, string_VkDebugReportObjectTypeEXT(objType));
        }
    }
    return VK_FALSE;
}

// CODEGEN : file ../vk-layer-generate.py line #915
static VkBool32 validate_swapchain_khr_status(
VkDevice dispatchable_object, VkSwapchainKHR object,
    VkDebugReportObjectTypeEXT     objType,
    ObjectStatusFlags   status_mask,
    ObjectStatusFlags   status_flag,
    VkFlags             msg_flags,
    OBJECT_TRACK_ERROR  error_code,
    const char         *fail_msg)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkSwapchainKHRMap.find(object_handle) != VkSwapchainKHRMap.end()) {
        OBJTRACK_NODE* pNode = VkSwapchainKHRMap[object_handle];
        if ((pNode->status & status_mask) != status_flag) {
            log_msg(mdd(dispatchable_object), msg_flags, pNode->objType, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
                "OBJECT VALIDATION WARNING: %s object 0x%" PRIxLEAST64 ": %s", string_VkDebugReportObjectTypeEXT(objType),
                 object_handle, fail_msg);
            return VK_FALSE;
        }
        return VK_TRUE;
    }
    else {
        // If we do not find it print an error
        log_msg(mdd(dispatchable_object), msg_flags, (VkDebugReportObjectTypeEXT) 0, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
            "Unable to obtain status for non-existent object 0x%" PRIxLEAST64 " of %s type",
            object_handle, string_VkDebugReportObjectTypeEXT(objType));
        return VK_FALSE;
    }
}

// CODEGEN : file ../vk-layer-generate.py line #948
static VkBool32 reset_swapchain_khr_status(VkDevice dispatchable_object, VkSwapchainKHR object, VkDebugReportObjectTypeEXT objType, ObjectStatusFlags status_flag)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkSwapchainKHRMap.find(object_handle) != VkSwapchainKHRMap.end()) {
        OBJTRACK_NODE* pNode = VkSwapchainKHRMap[object_handle];
        pNode->status &= ~status_flag;
    }
    else {
        // If we do not find it print an error
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
            "Unable to reset status for non-existent object 0x%" PRIxLEAST64 " of %s type",
            object_handle, string_VkDebugReportObjectTypeEXT(objType));
    }
    return VK_FALSE;
}

// CODEGEN : file ../vk-layer-generate.py line #844
static void create_surface_khr(VkDevice dispatchable_object, VkSurfaceKHR vkObj, VkDebugReportObjectTypeEXT objType)
{
    log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, objType,(uint64_t)(vkObj), __LINE__, OBJTRACK_NONE, "OBJTRACK",
        "OBJ[%llu] : CREATE %s object 0x%" PRIxLEAST64 , object_track_index++, string_VkDebugReportObjectTypeEXT(objType),
        (uint64_t)(vkObj));

    OBJTRACK_NODE* pNewObjNode = new OBJTRACK_NODE;
    pNewObjNode->objType = objType;
    pNewObjNode->status  = OBJSTATUS_NONE;
    pNewObjNode->vkObj  = (uint64_t)(vkObj);
    VkSurfaceKHRMap[(uint64_t)vkObj] = pNewObjNode;
    uint32_t objIndex = objTypeToIndex(objType);
    numObjs[objIndex]++;
    numTotalObjs++;
}

// CODEGEN : file ../vk-layer-generate.py line #866
static void destroy_surface_khr(VkDevice dispatchable_object, VkSurfaceKHR object)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkSurfaceKHRMap.find(object_handle) != VkSurfaceKHRMap.end()) {
        OBJTRACK_NODE* pNode = VkSurfaceKHRMap[(uint64_t)object];
        uint32_t objIndex = objTypeToIndex(pNode->objType);
        assert(numTotalObjs > 0);
        numTotalObjs--;
        assert(numObjs[objIndex] > 0);
        numObjs[objIndex]--;
        log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, pNode->objType, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
           "OBJ_STAT Destroy %s obj 0x%" PRIxLEAST64 " (%" PRIu64 " total objs remain & %" PRIu64 " %s objs).",
            string_VkDebugReportObjectTypeEXT(pNode->objType), (uint64_t)(object), numTotalObjs, numObjs[objIndex],
            string_VkDebugReportObjectTypeEXT(pNode->objType));
        delete pNode;
        VkSurfaceKHRMap.erase(object_handle);
    } else {
        log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT ) 0, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
            "Unable to remove obj 0x%" PRIxLEAST64 ". Was it created? Has it already been destroyed?",
           object_handle);
    }
}

// CODEGEN : file ../vk-layer-generate.py line #893
static VkBool32 set_surface_khr_status(VkDevice dispatchable_object, VkSurfaceKHR object, VkDebugReportObjectTypeEXT objType, ObjectStatusFlags status_flag)
{
    if (object != VK_NULL_HANDLE) {
        uint64_t object_handle = (uint64_t)(object);
        if (VkSurfaceKHRMap.find(object_handle) != VkSurfaceKHRMap.end()) {
            OBJTRACK_NODE* pNode = VkSurfaceKHRMap[object_handle];
            pNode->status |= status_flag;
        }
        else {
            // If we do not find it print an error
            return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT ) 0, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
                "Unable to set status for non-existent object 0x%" PRIxLEAST64 " of %s type",
                object_handle, string_VkDebugReportObjectTypeEXT(objType));
        }
    }
    return VK_FALSE;
}

// CODEGEN : file ../vk-layer-generate.py line #915
static VkBool32 validate_surface_khr_status(
VkDevice dispatchable_object, VkSurfaceKHR object,
    VkDebugReportObjectTypeEXT     objType,
    ObjectStatusFlags   status_mask,
    ObjectStatusFlags   status_flag,
    VkFlags             msg_flags,
    OBJECT_TRACK_ERROR  error_code,
    const char         *fail_msg)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkSurfaceKHRMap.find(object_handle) != VkSurfaceKHRMap.end()) {
        OBJTRACK_NODE* pNode = VkSurfaceKHRMap[object_handle];
        if ((pNode->status & status_mask) != status_flag) {
            log_msg(mdd(dispatchable_object), msg_flags, pNode->objType, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
                "OBJECT VALIDATION WARNING: %s object 0x%" PRIxLEAST64 ": %s", string_VkDebugReportObjectTypeEXT(objType),
                 object_handle, fail_msg);
            return VK_FALSE;
        }
        return VK_TRUE;
    }
    else {
        // If we do not find it print an error
        log_msg(mdd(dispatchable_object), msg_flags, (VkDebugReportObjectTypeEXT) 0, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
            "Unable to obtain status for non-existent object 0x%" PRIxLEAST64 " of %s type",
            object_handle, string_VkDebugReportObjectTypeEXT(objType));
        return VK_FALSE;
    }
}

// CODEGEN : file ../vk-layer-generate.py line #948
static VkBool32 reset_surface_khr_status(VkDevice dispatchable_object, VkSurfaceKHR object, VkDebugReportObjectTypeEXT objType, ObjectStatusFlags status_flag)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkSurfaceKHRMap.find(object_handle) != VkSurfaceKHRMap.end()) {
        OBJTRACK_NODE* pNode = VkSurfaceKHRMap[object_handle];
        pNode->status &= ~status_flag;
    }
    else {
        // If we do not find it print an error
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
            "Unable to reset status for non-existent object 0x%" PRIxLEAST64 " of %s type",
            object_handle, string_VkDebugReportObjectTypeEXT(objType));
    }
    return VK_FALSE;
}

// CODEGEN : file ../vk-layer-generate.py line #844
static void create_debug_report_callback_ext(VkDevice dispatchable_object, VkDebugReportCallbackEXT vkObj, VkDebugReportObjectTypeEXT objType)
{
    log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, objType,(uint64_t)(vkObj), __LINE__, OBJTRACK_NONE, "OBJTRACK",
        "OBJ[%llu] : CREATE %s object 0x%" PRIxLEAST64 , object_track_index++, string_VkDebugReportObjectTypeEXT(objType),
        (uint64_t)(vkObj));

    OBJTRACK_NODE* pNewObjNode = new OBJTRACK_NODE;
    pNewObjNode->objType = objType;
    pNewObjNode->status  = OBJSTATUS_NONE;
    pNewObjNode->vkObj  = (uint64_t)(vkObj);
    VkDebugReportCallbackEXTMap[(uint64_t)vkObj] = pNewObjNode;
    uint32_t objIndex = objTypeToIndex(objType);
    numObjs[objIndex]++;
    numTotalObjs++;
}

// CODEGEN : file ../vk-layer-generate.py line #866
static void destroy_debug_report_callback_ext(VkDevice dispatchable_object, VkDebugReportCallbackEXT object)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkDebugReportCallbackEXTMap.find(object_handle) != VkDebugReportCallbackEXTMap.end()) {
        OBJTRACK_NODE* pNode = VkDebugReportCallbackEXTMap[(uint64_t)object];
        uint32_t objIndex = objTypeToIndex(pNode->objType);
        assert(numTotalObjs > 0);
        numTotalObjs--;
        assert(numObjs[objIndex] > 0);
        numObjs[objIndex]--;
        log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, pNode->objType, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
           "OBJ_STAT Destroy %s obj 0x%" PRIxLEAST64 " (%" PRIu64 " total objs remain & %" PRIu64 " %s objs).",
            string_VkDebugReportObjectTypeEXT(pNode->objType), (uint64_t)(object), numTotalObjs, numObjs[objIndex],
            string_VkDebugReportObjectTypeEXT(pNode->objType));
        delete pNode;
        VkDebugReportCallbackEXTMap.erase(object_handle);
    } else {
        log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT ) 0, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
            "Unable to remove obj 0x%" PRIxLEAST64 ". Was it created? Has it already been destroyed?",
           object_handle);
    }
}

// CODEGEN : file ../vk-layer-generate.py line #893
static VkBool32 set_debug_report_callback_ext_status(VkDevice dispatchable_object, VkDebugReportCallbackEXT object, VkDebugReportObjectTypeEXT objType, ObjectStatusFlags status_flag)
{
    if (object != VK_NULL_HANDLE) {
        uint64_t object_handle = (uint64_t)(object);
        if (VkDebugReportCallbackEXTMap.find(object_handle) != VkDebugReportCallbackEXTMap.end()) {
            OBJTRACK_NODE* pNode = VkDebugReportCallbackEXTMap[object_handle];
            pNode->status |= status_flag;
        }
        else {
            // If we do not find it print an error
            return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT ) 0, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
                "Unable to set status for non-existent object 0x%" PRIxLEAST64 " of %s type",
                object_handle, string_VkDebugReportObjectTypeEXT(objType));
        }
    }
    return VK_FALSE;
}

// CODEGEN : file ../vk-layer-generate.py line #915
static VkBool32 validate_debug_report_callback_ext_status(
VkDevice dispatchable_object, VkDebugReportCallbackEXT object,
    VkDebugReportObjectTypeEXT     objType,
    ObjectStatusFlags   status_mask,
    ObjectStatusFlags   status_flag,
    VkFlags             msg_flags,
    OBJECT_TRACK_ERROR  error_code,
    const char         *fail_msg)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkDebugReportCallbackEXTMap.find(object_handle) != VkDebugReportCallbackEXTMap.end()) {
        OBJTRACK_NODE* pNode = VkDebugReportCallbackEXTMap[object_handle];
        if ((pNode->status & status_mask) != status_flag) {
            log_msg(mdd(dispatchable_object), msg_flags, pNode->objType, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
                "OBJECT VALIDATION WARNING: %s object 0x%" PRIxLEAST64 ": %s", string_VkDebugReportObjectTypeEXT(objType),
                 object_handle, fail_msg);
            return VK_FALSE;
        }
        return VK_TRUE;
    }
    else {
        // If we do not find it print an error
        log_msg(mdd(dispatchable_object), msg_flags, (VkDebugReportObjectTypeEXT) 0, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
            "Unable to obtain status for non-existent object 0x%" PRIxLEAST64 " of %s type",
            object_handle, string_VkDebugReportObjectTypeEXT(objType));
        return VK_FALSE;
    }
}

// CODEGEN : file ../vk-layer-generate.py line #948
static VkBool32 reset_debug_report_callback_ext_status(VkDevice dispatchable_object, VkDebugReportCallbackEXT object, VkDebugReportObjectTypeEXT objType, ObjectStatusFlags status_flag)
{
    uint64_t object_handle = (uint64_t)(object);
    if (VkDebugReportCallbackEXTMap.find(object_handle) != VkDebugReportCallbackEXTMap.end()) {
        OBJTRACK_NODE* pNode = VkDebugReportCallbackEXTMap[object_handle];
        pNode->status &= ~status_flag;
    }
    else {
        // If we do not find it print an error
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
            "Unable to reset status for non-existent object 0x%" PRIxLEAST64 " of %s type",
            object_handle, string_VkDebugReportObjectTypeEXT(objType));
    }
    return VK_FALSE;
}

// CODEGEN : file ../vk-layer-generate.py line #968
//{'VkPhysicalDevice': set(['VkDevice', 'VkSurfaceKHR']), 'VkQueue': set(['VkDeviceMemory', 'VkSemaphore', 'VkFence', 'VkImage', 'VkBuffer', 'VkCommandBuffer', 'VkSwapchainKHR']), 'VkInstance': set(['VkPhysicalDevice', 'VkSurfaceKHR']), 'VkDevice': set(['VkBuffer', 'VkDescriptorPool', 'VkQueue', 'VkDescriptorSet', 'VkSurfaceKHR', 'VkDescriptorSetLayout', 'VkPipelineCache', 'VkCommandBuffer', 'VkRenderPass', 'VkDeviceMemory', 'VkSemaphore', 'VkFence', 'VkPipelineLayout', 'VkImage', 'VkShaderModule', 'VkSampler', 'VkQueryPool', 'VkImageView', 'VkFramebuffer', 'VkEvent', 'VkBufferView', 'VkSwapchainKHR', 'VkPipeline', 'VkCommandPool']), 'VkCommandBuffer': set(['VkBuffer', 'VkPipelineLayout', 'VkImage', 'VkQueryPool', 'VkDescriptorSet', 'VkFramebuffer', 'VkEvent', 'VkCommandBuffer', 'VkRenderPass', 'VkPipeline'])}
// CODEGEN : file ../vk-layer-generate.py line #978
static VkBool32 validate_physical_device(VkPhysicalDevice dispatchable_object, VkPhysicalDevice object, VkDebugReportObjectTypeEXT objType, bool null_allowed)
{
    if (null_allowed && (object == VK_NULL_HANDLE))
        return VK_FALSE;
    if (VkPhysicalDeviceMap.find((uint64_t)object) == VkPhysicalDeviceMap.end()) {
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, (uint64_t)(object), __LINE__, OBJTRACK_INVALID_OBJECT, "OBJTRACK",
            "Invalid VkPhysicalDevice Object 0x%" PRIx64 ,(uint64_t)(object));
    }
    return VK_FALSE;
}

// CODEGEN : file ../vk-layer-generate.py line #995
static VkBool32 validate_device(VkPhysicalDevice dispatchable_object, VkDevice object, VkDebugReportObjectTypeEXT objType, bool null_allowed)
{
    if (null_allowed && (object == VK_NULL_HANDLE))
        return VK_FALSE;
    if (VkDeviceMap.find((uint64_t)object) == VkDeviceMap.end()) {
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, (uint64_t)(object), __LINE__, OBJTRACK_INVALID_OBJECT, "OBJTRACK",
            "Invalid VkDevice Object 0x%" PRIx64, (uint64_t)(object));
    }
    return VK_FALSE;
}
// CODEGEN : file ../vk-layer-generate.py line #995
static VkBool32 validate_surface_khr(VkPhysicalDevice dispatchable_object, VkSurfaceKHR object, VkDebugReportObjectTypeEXT objType, bool null_allowed)
{
    if (null_allowed && (object == VK_NULL_HANDLE))
        return VK_FALSE;
    if (VkSurfaceKHRMap.find((uint64_t)object) == VkSurfaceKHRMap.end()) {
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, (uint64_t)(object), __LINE__, OBJTRACK_INVALID_OBJECT, "OBJTRACK",
            "Invalid VkSurfaceKHR Object 0x%" PRIx64, (uint64_t)(object));
    }
    return VK_FALSE;
}

// CODEGEN : file ../vk-layer-generate.py line #978
static VkBool32 validate_queue(VkQueue dispatchable_object, VkQueue object, VkDebugReportObjectTypeEXT objType, bool null_allowed)
{
    if (null_allowed && (object == VK_NULL_HANDLE))
        return VK_FALSE;
    if (VkQueueMap.find((uint64_t)object) == VkQueueMap.end()) {
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, (uint64_t)(object), __LINE__, OBJTRACK_INVALID_OBJECT, "OBJTRACK",
            "Invalid VkQueue Object 0x%" PRIx64 ,(uint64_t)(object));
    }
    return VK_FALSE;
}

// CODEGEN : file ../vk-layer-generate.py line #995
static VkBool32 validate_device_memory(VkQueue dispatchable_object, VkDeviceMemory object, VkDebugReportObjectTypeEXT objType, bool null_allowed)
{
    if (null_allowed && (object == VK_NULL_HANDLE))
        return VK_FALSE;
    if (VkDeviceMemoryMap.find((uint64_t)object) == VkDeviceMemoryMap.end()) {
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, (uint64_t)(object), __LINE__, OBJTRACK_INVALID_OBJECT, "OBJTRACK",
            "Invalid VkDeviceMemory Object 0x%" PRIx64, (uint64_t)(object));
    }
    return VK_FALSE;
}
// CODEGEN : file ../vk-layer-generate.py line #995
static VkBool32 validate_semaphore(VkQueue dispatchable_object, VkSemaphore object, VkDebugReportObjectTypeEXT objType, bool null_allowed)
{
    if (null_allowed && (object == VK_NULL_HANDLE))
        return VK_FALSE;
    if (VkSemaphoreMap.find((uint64_t)object) == VkSemaphoreMap.end()) {
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, (uint64_t)(object), __LINE__, OBJTRACK_INVALID_OBJECT, "OBJTRACK",
            "Invalid VkSemaphore Object 0x%" PRIx64, (uint64_t)(object));
    }
    return VK_FALSE;
}
// CODEGEN : file ../vk-layer-generate.py line #995
static VkBool32 validate_fence(VkQueue dispatchable_object, VkFence object, VkDebugReportObjectTypeEXT objType, bool null_allowed)
{
    if (null_allowed && (object == VK_NULL_HANDLE))
        return VK_FALSE;
    if (VkFenceMap.find((uint64_t)object) == VkFenceMap.end()) {
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, (uint64_t)(object), __LINE__, OBJTRACK_INVALID_OBJECT, "OBJTRACK",
            "Invalid VkFence Object 0x%" PRIx64, (uint64_t)(object));
    }
    return VK_FALSE;
}
// CODEGEN : file ../vk-layer-generate.py line #995
static VkBool32 validate_image(VkQueue dispatchable_object, VkImage object, VkDebugReportObjectTypeEXT objType, bool null_allowed)
{
    if (null_allowed && (object == VK_NULL_HANDLE))
        return VK_FALSE;
    // We need to validate normal image objects and those from the swapchain
    if ((VkImageMap.find((uint64_t)object) == VkImageMap.end()) &&
        (swapchainImageMap.find((uint64_t)object) == swapchainImageMap.end())) {
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, (uint64_t)(object), __LINE__, OBJTRACK_INVALID_OBJECT, "OBJTRACK",
            "Invalid VkImage Object 0x%" PRIx64, (uint64_t)(object));
    }
    return VK_FALSE;
}
// CODEGEN : file ../vk-layer-generate.py line #995
static VkBool32 validate_buffer(VkQueue dispatchable_object, VkBuffer object, VkDebugReportObjectTypeEXT objType, bool null_allowed)
{
    if (null_allowed && (object == VK_NULL_HANDLE))
        return VK_FALSE;
    if (VkBufferMap.find((uint64_t)object) == VkBufferMap.end()) {
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, (uint64_t)(object), __LINE__, OBJTRACK_INVALID_OBJECT, "OBJTRACK",
            "Invalid VkBuffer Object 0x%" PRIx64, (uint64_t)(object));
    }
    return VK_FALSE;
}
// CODEGEN : file ../vk-layer-generate.py line #995
static VkBool32 validate_command_buffer(VkQueue dispatchable_object, VkCommandBuffer object, VkDebugReportObjectTypeEXT objType, bool null_allowed)
{
    if (null_allowed && (object == VK_NULL_HANDLE))
        return VK_FALSE;
    if (VkCommandBufferMap.find((uint64_t)object) == VkCommandBufferMap.end()) {
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, (uint64_t)(object), __LINE__, OBJTRACK_INVALID_OBJECT, "OBJTRACK",
            "Invalid VkCommandBuffer Object 0x%" PRIx64, (uint64_t)(object));
    }
    return VK_FALSE;
}
// CODEGEN : file ../vk-layer-generate.py line #995
static VkBool32 validate_swapchain_khr(VkQueue dispatchable_object, VkSwapchainKHR object, VkDebugReportObjectTypeEXT objType, bool null_allowed)
{
    if (null_allowed && (object == VK_NULL_HANDLE))
        return VK_FALSE;
    if (VkSwapchainKHRMap.find((uint64_t)object) == VkSwapchainKHRMap.end()) {
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, (uint64_t)(object), __LINE__, OBJTRACK_INVALID_OBJECT, "OBJTRACK",
            "Invalid VkSwapchainKHR Object 0x%" PRIx64, (uint64_t)(object));
    }
    return VK_FALSE;
}

// CODEGEN : file ../vk-layer-generate.py line #978
static VkBool32 validate_instance(VkInstance dispatchable_object, VkInstance object, VkDebugReportObjectTypeEXT objType, bool null_allowed)
{
    if (null_allowed && (object == VK_NULL_HANDLE))
        return VK_FALSE;
    if (VkInstanceMap.find((uint64_t)object) == VkInstanceMap.end()) {
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, (uint64_t)(object), __LINE__, OBJTRACK_INVALID_OBJECT, "OBJTRACK",
            "Invalid VkInstance Object 0x%" PRIx64 ,(uint64_t)(object));
    }
    return VK_FALSE;
}

// CODEGEN : file ../vk-layer-generate.py line #995
static VkBool32 validate_physical_device(VkInstance dispatchable_object, VkPhysicalDevice object, VkDebugReportObjectTypeEXT objType, bool null_allowed)
{
    if (null_allowed && (object == VK_NULL_HANDLE))
        return VK_FALSE;
    if (VkPhysicalDeviceMap.find((uint64_t)object) == VkPhysicalDeviceMap.end()) {
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, (uint64_t)(object), __LINE__, OBJTRACK_INVALID_OBJECT, "OBJTRACK",
            "Invalid VkPhysicalDevice Object 0x%" PRIx64, (uint64_t)(object));
    }
    return VK_FALSE;
}
// CODEGEN : file ../vk-layer-generate.py line #995
static VkBool32 validate_surface_khr(VkInstance dispatchable_object, VkSurfaceKHR object, VkDebugReportObjectTypeEXT objType, bool null_allowed)
{
    if (null_allowed && (object == VK_NULL_HANDLE))
        return VK_FALSE;
    if (VkSurfaceKHRMap.find((uint64_t)object) == VkSurfaceKHRMap.end()) {
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, (uint64_t)(object), __LINE__, OBJTRACK_INVALID_OBJECT, "OBJTRACK",
            "Invalid VkSurfaceKHR Object 0x%" PRIx64, (uint64_t)(object));
    }
    return VK_FALSE;
}

// CODEGEN : file ../vk-layer-generate.py line #978
static VkBool32 validate_device(VkDevice dispatchable_object, VkDevice object, VkDebugReportObjectTypeEXT objType, bool null_allowed)
{
    if (null_allowed && (object == VK_NULL_HANDLE))
        return VK_FALSE;
    if (VkDeviceMap.find((uint64_t)object) == VkDeviceMap.end()) {
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, (uint64_t)(object), __LINE__, OBJTRACK_INVALID_OBJECT, "OBJTRACK",
            "Invalid VkDevice Object 0x%" PRIx64 ,(uint64_t)(object));
    }
    return VK_FALSE;
}

// CODEGEN : file ../vk-layer-generate.py line #995
static VkBool32 validate_buffer(VkDevice dispatchable_object, VkBuffer object, VkDebugReportObjectTypeEXT objType, bool null_allowed)
{
    if (null_allowed && (object == VK_NULL_HANDLE))
        return VK_FALSE;
    if (VkBufferMap.find((uint64_t)object) == VkBufferMap.end()) {
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, (uint64_t)(object), __LINE__, OBJTRACK_INVALID_OBJECT, "OBJTRACK",
            "Invalid VkBuffer Object 0x%" PRIx64, (uint64_t)(object));
    }
    return VK_FALSE;
}
// CODEGEN : file ../vk-layer-generate.py line #995
static VkBool32 validate_descriptor_pool(VkDevice dispatchable_object, VkDescriptorPool object, VkDebugReportObjectTypeEXT objType, bool null_allowed)
{
    if (null_allowed && (object == VK_NULL_HANDLE))
        return VK_FALSE;
    if (VkDescriptorPoolMap.find((uint64_t)object) == VkDescriptorPoolMap.end()) {
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, (uint64_t)(object), __LINE__, OBJTRACK_INVALID_OBJECT, "OBJTRACK",
            "Invalid VkDescriptorPool Object 0x%" PRIx64, (uint64_t)(object));
    }
    return VK_FALSE;
}
// CODEGEN : file ../vk-layer-generate.py line #995
static VkBool32 validate_queue(VkDevice dispatchable_object, VkQueue object, VkDebugReportObjectTypeEXT objType, bool null_allowed)
{
    if (null_allowed && (object == VK_NULL_HANDLE))
        return VK_FALSE;
    if (VkQueueMap.find((uint64_t)object) == VkQueueMap.end()) {
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, (uint64_t)(object), __LINE__, OBJTRACK_INVALID_OBJECT, "OBJTRACK",
            "Invalid VkQueue Object 0x%" PRIx64, (uint64_t)(object));
    }
    return VK_FALSE;
}
// CODEGEN : file ../vk-layer-generate.py line #995
static VkBool32 validate_descriptor_set(VkDevice dispatchable_object, VkDescriptorSet object, VkDebugReportObjectTypeEXT objType, bool null_allowed)
{
    if (null_allowed && (object == VK_NULL_HANDLE))
        return VK_FALSE;
    if (VkDescriptorSetMap.find((uint64_t)object) == VkDescriptorSetMap.end()) {
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, (uint64_t)(object), __LINE__, OBJTRACK_INVALID_OBJECT, "OBJTRACK",
            "Invalid VkDescriptorSet Object 0x%" PRIx64, (uint64_t)(object));
    }
    return VK_FALSE;
}
// CODEGEN : file ../vk-layer-generate.py line #995
static VkBool32 validate_surface_khr(VkDevice dispatchable_object, VkSurfaceKHR object, VkDebugReportObjectTypeEXT objType, bool null_allowed)
{
    if (null_allowed && (object == VK_NULL_HANDLE))
        return VK_FALSE;
    if (VkSurfaceKHRMap.find((uint64_t)object) == VkSurfaceKHRMap.end()) {
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, (uint64_t)(object), __LINE__, OBJTRACK_INVALID_OBJECT, "OBJTRACK",
            "Invalid VkSurfaceKHR Object 0x%" PRIx64, (uint64_t)(object));
    }
    return VK_FALSE;
}
// CODEGEN : file ../vk-layer-generate.py line #995
static VkBool32 validate_descriptor_set_layout(VkDevice dispatchable_object, VkDescriptorSetLayout object, VkDebugReportObjectTypeEXT objType, bool null_allowed)
{
    if (null_allowed && (object == VK_NULL_HANDLE))
        return VK_FALSE;
    if (VkDescriptorSetLayoutMap.find((uint64_t)object) == VkDescriptorSetLayoutMap.end()) {
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, (uint64_t)(object), __LINE__, OBJTRACK_INVALID_OBJECT, "OBJTRACK",
            "Invalid VkDescriptorSetLayout Object 0x%" PRIx64, (uint64_t)(object));
    }
    return VK_FALSE;
}
// CODEGEN : file ../vk-layer-generate.py line #995
static VkBool32 validate_pipeline_cache(VkDevice dispatchable_object, VkPipelineCache object, VkDebugReportObjectTypeEXT objType, bool null_allowed)
{
    if (null_allowed && (object == VK_NULL_HANDLE))
        return VK_FALSE;
    if (VkPipelineCacheMap.find((uint64_t)object) == VkPipelineCacheMap.end()) {
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, (uint64_t)(object), __LINE__, OBJTRACK_INVALID_OBJECT, "OBJTRACK",
            "Invalid VkPipelineCache Object 0x%" PRIx64, (uint64_t)(object));
    }
    return VK_FALSE;
}
// CODEGEN : file ../vk-layer-generate.py line #995
static VkBool32 validate_command_buffer(VkDevice dispatchable_object, VkCommandBuffer object, VkDebugReportObjectTypeEXT objType, bool null_allowed)
{
    if (null_allowed && (object == VK_NULL_HANDLE))
        return VK_FALSE;
    if (VkCommandBufferMap.find((uint64_t)object) == VkCommandBufferMap.end()) {
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, (uint64_t)(object), __LINE__, OBJTRACK_INVALID_OBJECT, "OBJTRACK",
            "Invalid VkCommandBuffer Object 0x%" PRIx64, (uint64_t)(object));
    }
    return VK_FALSE;
}
// CODEGEN : file ../vk-layer-generate.py line #995
static VkBool32 validate_render_pass(VkDevice dispatchable_object, VkRenderPass object, VkDebugReportObjectTypeEXT objType, bool null_allowed)
{
    if (null_allowed && (object == VK_NULL_HANDLE))
        return VK_FALSE;
    if (VkRenderPassMap.find((uint64_t)object) == VkRenderPassMap.end()) {
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, (uint64_t)(object), __LINE__, OBJTRACK_INVALID_OBJECT, "OBJTRACK",
            "Invalid VkRenderPass Object 0x%" PRIx64, (uint64_t)(object));
    }
    return VK_FALSE;
}
// CODEGEN : file ../vk-layer-generate.py line #995
static VkBool32 validate_device_memory(VkDevice dispatchable_object, VkDeviceMemory object, VkDebugReportObjectTypeEXT objType, bool null_allowed)
{
    if (null_allowed && (object == VK_NULL_HANDLE))
        return VK_FALSE;
    if (VkDeviceMemoryMap.find((uint64_t)object) == VkDeviceMemoryMap.end()) {
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, (uint64_t)(object), __LINE__, OBJTRACK_INVALID_OBJECT, "OBJTRACK",
            "Invalid VkDeviceMemory Object 0x%" PRIx64, (uint64_t)(object));
    }
    return VK_FALSE;
}
// CODEGEN : file ../vk-layer-generate.py line #995
static VkBool32 validate_semaphore(VkDevice dispatchable_object, VkSemaphore object, VkDebugReportObjectTypeEXT objType, bool null_allowed)
{
    if (null_allowed && (object == VK_NULL_HANDLE))
        return VK_FALSE;
    if (VkSemaphoreMap.find((uint64_t)object) == VkSemaphoreMap.end()) {
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, (uint64_t)(object), __LINE__, OBJTRACK_INVALID_OBJECT, "OBJTRACK",
            "Invalid VkSemaphore Object 0x%" PRIx64, (uint64_t)(object));
    }
    return VK_FALSE;
}
// CODEGEN : file ../vk-layer-generate.py line #995
static VkBool32 validate_fence(VkDevice dispatchable_object, VkFence object, VkDebugReportObjectTypeEXT objType, bool null_allowed)
{
    if (null_allowed && (object == VK_NULL_HANDLE))
        return VK_FALSE;
    if (VkFenceMap.find((uint64_t)object) == VkFenceMap.end()) {
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, (uint64_t)(object), __LINE__, OBJTRACK_INVALID_OBJECT, "OBJTRACK",
            "Invalid VkFence Object 0x%" PRIx64, (uint64_t)(object));
    }
    return VK_FALSE;
}
// CODEGEN : file ../vk-layer-generate.py line #995
static VkBool32 validate_pipeline_layout(VkDevice dispatchable_object, VkPipelineLayout object, VkDebugReportObjectTypeEXT objType, bool null_allowed)
{
    if (null_allowed && (object == VK_NULL_HANDLE))
        return VK_FALSE;
    if (VkPipelineLayoutMap.find((uint64_t)object) == VkPipelineLayoutMap.end()) {
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, (uint64_t)(object), __LINE__, OBJTRACK_INVALID_OBJECT, "OBJTRACK",
            "Invalid VkPipelineLayout Object 0x%" PRIx64, (uint64_t)(object));
    }
    return VK_FALSE;
}
// CODEGEN : file ../vk-layer-generate.py line #995
static VkBool32 validate_image(VkDevice dispatchable_object, VkImage object, VkDebugReportObjectTypeEXT objType, bool null_allowed)
{
    if (null_allowed && (object == VK_NULL_HANDLE))
        return VK_FALSE;
    // We need to validate normal image objects and those from the swapchain
    if ((VkImageMap.find((uint64_t)object) == VkImageMap.end()) &&
        (swapchainImageMap.find((uint64_t)object) == swapchainImageMap.end())) {
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, (uint64_t)(object), __LINE__, OBJTRACK_INVALID_OBJECT, "OBJTRACK",
            "Invalid VkImage Object 0x%" PRIx64, (uint64_t)(object));
    }
    return VK_FALSE;
}
// CODEGEN : file ../vk-layer-generate.py line #995
static VkBool32 validate_shader_module(VkDevice dispatchable_object, VkShaderModule object, VkDebugReportObjectTypeEXT objType, bool null_allowed)
{
    if (null_allowed && (object == VK_NULL_HANDLE))
        return VK_FALSE;
    if (VkShaderModuleMap.find((uint64_t)object) == VkShaderModuleMap.end()) {
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, (uint64_t)(object), __LINE__, OBJTRACK_INVALID_OBJECT, "OBJTRACK",
            "Invalid VkShaderModule Object 0x%" PRIx64, (uint64_t)(object));
    }
    return VK_FALSE;
}
// CODEGEN : file ../vk-layer-generate.py line #995
static VkBool32 validate_sampler(VkDevice dispatchable_object, VkSampler object, VkDebugReportObjectTypeEXT objType, bool null_allowed)
{
    if (null_allowed && (object == VK_NULL_HANDLE))
        return VK_FALSE;
    if (VkSamplerMap.find((uint64_t)object) == VkSamplerMap.end()) {
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, (uint64_t)(object), __LINE__, OBJTRACK_INVALID_OBJECT, "OBJTRACK",
            "Invalid VkSampler Object 0x%" PRIx64, (uint64_t)(object));
    }
    return VK_FALSE;
}
// CODEGEN : file ../vk-layer-generate.py line #995
static VkBool32 validate_query_pool(VkDevice dispatchable_object, VkQueryPool object, VkDebugReportObjectTypeEXT objType, bool null_allowed)
{
    if (null_allowed && (object == VK_NULL_HANDLE))
        return VK_FALSE;
    if (VkQueryPoolMap.find((uint64_t)object) == VkQueryPoolMap.end()) {
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, (uint64_t)(object), __LINE__, OBJTRACK_INVALID_OBJECT, "OBJTRACK",
            "Invalid VkQueryPool Object 0x%" PRIx64, (uint64_t)(object));
    }
    return VK_FALSE;
}
// CODEGEN : file ../vk-layer-generate.py line #995
static VkBool32 validate_image_view(VkDevice dispatchable_object, VkImageView object, VkDebugReportObjectTypeEXT objType, bool null_allowed)
{
    if (null_allowed && (object == VK_NULL_HANDLE))
        return VK_FALSE;
    if (VkImageViewMap.find((uint64_t)object) == VkImageViewMap.end()) {
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, (uint64_t)(object), __LINE__, OBJTRACK_INVALID_OBJECT, "OBJTRACK",
            "Invalid VkImageView Object 0x%" PRIx64, (uint64_t)(object));
    }
    return VK_FALSE;
}
// CODEGEN : file ../vk-layer-generate.py line #995
static VkBool32 validate_framebuffer(VkDevice dispatchable_object, VkFramebuffer object, VkDebugReportObjectTypeEXT objType, bool null_allowed)
{
    if (null_allowed && (object == VK_NULL_HANDLE))
        return VK_FALSE;
    if (VkFramebufferMap.find((uint64_t)object) == VkFramebufferMap.end()) {
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, (uint64_t)(object), __LINE__, OBJTRACK_INVALID_OBJECT, "OBJTRACK",
            "Invalid VkFramebuffer Object 0x%" PRIx64, (uint64_t)(object));
    }
    return VK_FALSE;
}
// CODEGEN : file ../vk-layer-generate.py line #995
static VkBool32 validate_event(VkDevice dispatchable_object, VkEvent object, VkDebugReportObjectTypeEXT objType, bool null_allowed)
{
    if (null_allowed && (object == VK_NULL_HANDLE))
        return VK_FALSE;
    if (VkEventMap.find((uint64_t)object) == VkEventMap.end()) {
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, (uint64_t)(object), __LINE__, OBJTRACK_INVALID_OBJECT, "OBJTRACK",
            "Invalid VkEvent Object 0x%" PRIx64, (uint64_t)(object));
    }
    return VK_FALSE;
}
// CODEGEN : file ../vk-layer-generate.py line #995
static VkBool32 validate_buffer_view(VkDevice dispatchable_object, VkBufferView object, VkDebugReportObjectTypeEXT objType, bool null_allowed)
{
    if (null_allowed && (object == VK_NULL_HANDLE))
        return VK_FALSE;
    if (VkBufferViewMap.find((uint64_t)object) == VkBufferViewMap.end()) {
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, (uint64_t)(object), __LINE__, OBJTRACK_INVALID_OBJECT, "OBJTRACK",
            "Invalid VkBufferView Object 0x%" PRIx64, (uint64_t)(object));
    }
    return VK_FALSE;
}
// CODEGEN : file ../vk-layer-generate.py line #995
static VkBool32 validate_swapchain_khr(VkDevice dispatchable_object, VkSwapchainKHR object, VkDebugReportObjectTypeEXT objType, bool null_allowed)
{
    if (null_allowed && (object == VK_NULL_HANDLE))
        return VK_FALSE;
    if (VkSwapchainKHRMap.find((uint64_t)object) == VkSwapchainKHRMap.end()) {
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, (uint64_t)(object), __LINE__, OBJTRACK_INVALID_OBJECT, "OBJTRACK",
            "Invalid VkSwapchainKHR Object 0x%" PRIx64, (uint64_t)(object));
    }
    return VK_FALSE;
}
// CODEGEN : file ../vk-layer-generate.py line #995
static VkBool32 validate_pipeline(VkDevice dispatchable_object, VkPipeline object, VkDebugReportObjectTypeEXT objType, bool null_allowed)
{
    if (null_allowed && (object == VK_NULL_HANDLE))
        return VK_FALSE;
    if (VkPipelineMap.find((uint64_t)object) == VkPipelineMap.end()) {
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, (uint64_t)(object), __LINE__, OBJTRACK_INVALID_OBJECT, "OBJTRACK",
            "Invalid VkPipeline Object 0x%" PRIx64, (uint64_t)(object));
    }
    return VK_FALSE;
}
// CODEGEN : file ../vk-layer-generate.py line #995
static VkBool32 validate_command_pool(VkDevice dispatchable_object, VkCommandPool object, VkDebugReportObjectTypeEXT objType, bool null_allowed)
{
    if (null_allowed && (object == VK_NULL_HANDLE))
        return VK_FALSE;
    if (VkCommandPoolMap.find((uint64_t)object) == VkCommandPoolMap.end()) {
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, (uint64_t)(object), __LINE__, OBJTRACK_INVALID_OBJECT, "OBJTRACK",
            "Invalid VkCommandPool Object 0x%" PRIx64, (uint64_t)(object));
    }
    return VK_FALSE;
}

// CODEGEN : file ../vk-layer-generate.py line #978
static VkBool32 validate_command_buffer(VkCommandBuffer dispatchable_object, VkCommandBuffer object, VkDebugReportObjectTypeEXT objType, bool null_allowed)
{
    if (null_allowed && (object == VK_NULL_HANDLE))
        return VK_FALSE;
    if (VkCommandBufferMap.find((uint64_t)object) == VkCommandBufferMap.end()) {
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, (uint64_t)(object), __LINE__, OBJTRACK_INVALID_OBJECT, "OBJTRACK",
            "Invalid VkCommandBuffer Object 0x%" PRIx64 ,(uint64_t)(object));
    }
    return VK_FALSE;
}

// CODEGEN : file ../vk-layer-generate.py line #995
static VkBool32 validate_buffer(VkCommandBuffer dispatchable_object, VkBuffer object, VkDebugReportObjectTypeEXT objType, bool null_allowed)
{
    if (null_allowed && (object == VK_NULL_HANDLE))
        return VK_FALSE;
    if (VkBufferMap.find((uint64_t)object) == VkBufferMap.end()) {
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, (uint64_t)(object), __LINE__, OBJTRACK_INVALID_OBJECT, "OBJTRACK",
            "Invalid VkBuffer Object 0x%" PRIx64, (uint64_t)(object));
    }
    return VK_FALSE;
}
// CODEGEN : file ../vk-layer-generate.py line #995
static VkBool32 validate_pipeline_layout(VkCommandBuffer dispatchable_object, VkPipelineLayout object, VkDebugReportObjectTypeEXT objType, bool null_allowed)
{
    if (null_allowed && (object == VK_NULL_HANDLE))
        return VK_FALSE;
    if (VkPipelineLayoutMap.find((uint64_t)object) == VkPipelineLayoutMap.end()) {
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, (uint64_t)(object), __LINE__, OBJTRACK_INVALID_OBJECT, "OBJTRACK",
            "Invalid VkPipelineLayout Object 0x%" PRIx64, (uint64_t)(object));
    }
    return VK_FALSE;
}
// CODEGEN : file ../vk-layer-generate.py line #995
static VkBool32 validate_image(VkCommandBuffer dispatchable_object, VkImage object, VkDebugReportObjectTypeEXT objType, bool null_allowed)
{
    if (null_allowed && (object == VK_NULL_HANDLE))
        return VK_FALSE;
    // We need to validate normal image objects and those from the swapchain
    if ((VkImageMap.find((uint64_t)object) == VkImageMap.end()) &&
        (swapchainImageMap.find((uint64_t)object) == swapchainImageMap.end())) {
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, (uint64_t)(object), __LINE__, OBJTRACK_INVALID_OBJECT, "OBJTRACK",
            "Invalid VkImage Object 0x%" PRIx64, (uint64_t)(object));
    }
    return VK_FALSE;
}
// CODEGEN : file ../vk-layer-generate.py line #995
static VkBool32 validate_query_pool(VkCommandBuffer dispatchable_object, VkQueryPool object, VkDebugReportObjectTypeEXT objType, bool null_allowed)
{
    if (null_allowed && (object == VK_NULL_HANDLE))
        return VK_FALSE;
    if (VkQueryPoolMap.find((uint64_t)object) == VkQueryPoolMap.end()) {
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, (uint64_t)(object), __LINE__, OBJTRACK_INVALID_OBJECT, "OBJTRACK",
            "Invalid VkQueryPool Object 0x%" PRIx64, (uint64_t)(object));
    }
    return VK_FALSE;
}
// CODEGEN : file ../vk-layer-generate.py line #995
static VkBool32 validate_descriptor_set(VkCommandBuffer dispatchable_object, VkDescriptorSet object, VkDebugReportObjectTypeEXT objType, bool null_allowed)
{
    if (null_allowed && (object == VK_NULL_HANDLE))
        return VK_FALSE;
    if (VkDescriptorSetMap.find((uint64_t)object) == VkDescriptorSetMap.end()) {
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, (uint64_t)(object), __LINE__, OBJTRACK_INVALID_OBJECT, "OBJTRACK",
            "Invalid VkDescriptorSet Object 0x%" PRIx64, (uint64_t)(object));
    }
    return VK_FALSE;
}
// CODEGEN : file ../vk-layer-generate.py line #995
static VkBool32 validate_framebuffer(VkCommandBuffer dispatchable_object, VkFramebuffer object, VkDebugReportObjectTypeEXT objType, bool null_allowed)
{
    if (null_allowed && (object == VK_NULL_HANDLE))
        return VK_FALSE;
    if (VkFramebufferMap.find((uint64_t)object) == VkFramebufferMap.end()) {
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, (uint64_t)(object), __LINE__, OBJTRACK_INVALID_OBJECT, "OBJTRACK",
            "Invalid VkFramebuffer Object 0x%" PRIx64, (uint64_t)(object));
    }
    return VK_FALSE;
}
// CODEGEN : file ../vk-layer-generate.py line #995
static VkBool32 validate_event(VkCommandBuffer dispatchable_object, VkEvent object, VkDebugReportObjectTypeEXT objType, bool null_allowed)
{
    if (null_allowed && (object == VK_NULL_HANDLE))
        return VK_FALSE;
    if (VkEventMap.find((uint64_t)object) == VkEventMap.end()) {
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, (uint64_t)(object), __LINE__, OBJTRACK_INVALID_OBJECT, "OBJTRACK",
            "Invalid VkEvent Object 0x%" PRIx64, (uint64_t)(object));
    }
    return VK_FALSE;
}
// CODEGEN : file ../vk-layer-generate.py line #995
static VkBool32 validate_render_pass(VkCommandBuffer dispatchable_object, VkRenderPass object, VkDebugReportObjectTypeEXT objType, bool null_allowed)
{
    if (null_allowed && (object == VK_NULL_HANDLE))
        return VK_FALSE;
    if (VkRenderPassMap.find((uint64_t)object) == VkRenderPassMap.end()) {
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, (uint64_t)(object), __LINE__, OBJTRACK_INVALID_OBJECT, "OBJTRACK",
            "Invalid VkRenderPass Object 0x%" PRIx64, (uint64_t)(object));
    }
    return VK_FALSE;
}
// CODEGEN : file ../vk-layer-generate.py line #995
static VkBool32 validate_pipeline(VkCommandBuffer dispatchable_object, VkPipeline object, VkDebugReportObjectTypeEXT objType, bool null_allowed)
{
    if (null_allowed && (object == VK_NULL_HANDLE))
        return VK_FALSE;
    if (VkPipelineMap.find((uint64_t)object) == VkPipelineMap.end()) {
        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, (uint64_t)(object), __LINE__, OBJTRACK_INVALID_OBJECT, "OBJTRACK",
            "Invalid VkPipeline Object 0x%" PRIx64, (uint64_t)(object));
    }
    return VK_FALSE;
}



// CODEGEN : file ../vk-layer-generate.py line #1017
VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyInstance(
VkInstance instance,
const VkAllocationCallbacks* pAllocator)
{
    loader_platform_thread_lock_mutex(&objLock);
    validate_instance(instance, instance, VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT, false);

    destroy_instance(instance, instance);
    // Report any remaining objects in LL
    for (auto it = VkDeviceMap.begin(); it != VkDeviceMap.end(); ++it) {
        OBJTRACK_NODE* pNode = it->second;
        log_msg(mid(instance), VK_DEBUG_REPORT_ERROR_BIT_EXT, pNode->objType, pNode->vkObj, __LINE__, OBJTRACK_OBJECT_LEAK, "OBJTRACK",
                "OBJ ERROR : %s object 0x%" PRIxLEAST64 " has not been destroyed.", string_VkDebugReportObjectTypeEXT(pNode->objType),
                pNode->vkObj);
    }
    VkDeviceMap.clear();

    for (auto it = VkSemaphoreMap.begin(); it != VkSemaphoreMap.end(); ++it) {
        OBJTRACK_NODE* pNode = it->second;
        log_msg(mid(instance), VK_DEBUG_REPORT_ERROR_BIT_EXT, pNode->objType, pNode->vkObj, __LINE__, OBJTRACK_OBJECT_LEAK, "OBJTRACK",
                "OBJ ERROR : %s object 0x%" PRIxLEAST64 " has not been destroyed.", string_VkDebugReportObjectTypeEXT(pNode->objType),
                pNode->vkObj);
    }
    VkSemaphoreMap.clear();

    for (auto it = VkCommandBufferMap.begin(); it != VkCommandBufferMap.end(); ++it) {
        OBJTRACK_NODE* pNode = it->second;
        log_msg(mid(instance), VK_DEBUG_REPORT_ERROR_BIT_EXT, pNode->objType, pNode->vkObj, __LINE__, OBJTRACK_OBJECT_LEAK, "OBJTRACK",
                "OBJ ERROR : %s object 0x%" PRIxLEAST64 " has not been destroyed.", string_VkDebugReportObjectTypeEXT(pNode->objType),
                pNode->vkObj);
    }
    VkCommandBufferMap.clear();

    for (auto it = VkFenceMap.begin(); it != VkFenceMap.end(); ++it) {
        OBJTRACK_NODE* pNode = it->second;
        log_msg(mid(instance), VK_DEBUG_REPORT_ERROR_BIT_EXT, pNode->objType, pNode->vkObj, __LINE__, OBJTRACK_OBJECT_LEAK, "OBJTRACK",
                "OBJ ERROR : %s object 0x%" PRIxLEAST64 " has not been destroyed.", string_VkDebugReportObjectTypeEXT(pNode->objType),
                pNode->vkObj);
    }
    VkFenceMap.clear();

    for (auto it = VkDeviceMemoryMap.begin(); it != VkDeviceMemoryMap.end(); ++it) {
        OBJTRACK_NODE* pNode = it->second;
        log_msg(mid(instance), VK_DEBUG_REPORT_ERROR_BIT_EXT, pNode->objType, pNode->vkObj, __LINE__, OBJTRACK_OBJECT_LEAK, "OBJTRACK",
                "OBJ ERROR : %s object 0x%" PRIxLEAST64 " has not been destroyed.", string_VkDebugReportObjectTypeEXT(pNode->objType),
                pNode->vkObj);
    }
    VkDeviceMemoryMap.clear();

    for (auto it = VkBufferMap.begin(); it != VkBufferMap.end(); ++it) {
        OBJTRACK_NODE* pNode = it->second;
        log_msg(mid(instance), VK_DEBUG_REPORT_ERROR_BIT_EXT, pNode->objType, pNode->vkObj, __LINE__, OBJTRACK_OBJECT_LEAK, "OBJTRACK",
                "OBJ ERROR : %s object 0x%" PRIxLEAST64 " has not been destroyed.", string_VkDebugReportObjectTypeEXT(pNode->objType),
                pNode->vkObj);
    }
    VkBufferMap.clear();

    for (auto it = VkImageMap.begin(); it != VkImageMap.end(); ++it) {
        OBJTRACK_NODE* pNode = it->second;
        log_msg(mid(instance), VK_DEBUG_REPORT_ERROR_BIT_EXT, pNode->objType, pNode->vkObj, __LINE__, OBJTRACK_OBJECT_LEAK, "OBJTRACK",
                "OBJ ERROR : %s object 0x%" PRIxLEAST64 " has not been destroyed.", string_VkDebugReportObjectTypeEXT(pNode->objType),
                pNode->vkObj);
    }
    VkImageMap.clear();

    for (auto it = VkEventMap.begin(); it != VkEventMap.end(); ++it) {
        OBJTRACK_NODE* pNode = it->second;
        log_msg(mid(instance), VK_DEBUG_REPORT_ERROR_BIT_EXT, pNode->objType, pNode->vkObj, __LINE__, OBJTRACK_OBJECT_LEAK, "OBJTRACK",
                "OBJ ERROR : %s object 0x%" PRIxLEAST64 " has not been destroyed.", string_VkDebugReportObjectTypeEXT(pNode->objType),
                pNode->vkObj);
    }
    VkEventMap.clear();

    for (auto it = VkQueryPoolMap.begin(); it != VkQueryPoolMap.end(); ++it) {
        OBJTRACK_NODE* pNode = it->second;
        log_msg(mid(instance), VK_DEBUG_REPORT_ERROR_BIT_EXT, pNode->objType, pNode->vkObj, __LINE__, OBJTRACK_OBJECT_LEAK, "OBJTRACK",
                "OBJ ERROR : %s object 0x%" PRIxLEAST64 " has not been destroyed.", string_VkDebugReportObjectTypeEXT(pNode->objType),
                pNode->vkObj);
    }
    VkQueryPoolMap.clear();

    for (auto it = VkBufferViewMap.begin(); it != VkBufferViewMap.end(); ++it) {
        OBJTRACK_NODE* pNode = it->second;
        log_msg(mid(instance), VK_DEBUG_REPORT_ERROR_BIT_EXT, pNode->objType, pNode->vkObj, __LINE__, OBJTRACK_OBJECT_LEAK, "OBJTRACK",
                "OBJ ERROR : %s object 0x%" PRIxLEAST64 " has not been destroyed.", string_VkDebugReportObjectTypeEXT(pNode->objType),
                pNode->vkObj);
    }
    VkBufferViewMap.clear();

    for (auto it = VkImageViewMap.begin(); it != VkImageViewMap.end(); ++it) {
        OBJTRACK_NODE* pNode = it->second;
        log_msg(mid(instance), VK_DEBUG_REPORT_ERROR_BIT_EXT, pNode->objType, pNode->vkObj, __LINE__, OBJTRACK_OBJECT_LEAK, "OBJTRACK",
                "OBJ ERROR : %s object 0x%" PRIxLEAST64 " has not been destroyed.", string_VkDebugReportObjectTypeEXT(pNode->objType),
                pNode->vkObj);
    }
    VkImageViewMap.clear();

    for (auto it = VkShaderModuleMap.begin(); it != VkShaderModuleMap.end(); ++it) {
        OBJTRACK_NODE* pNode = it->second;
        log_msg(mid(instance), VK_DEBUG_REPORT_ERROR_BIT_EXT, pNode->objType, pNode->vkObj, __LINE__, OBJTRACK_OBJECT_LEAK, "OBJTRACK",
                "OBJ ERROR : %s object 0x%" PRIxLEAST64 " has not been destroyed.", string_VkDebugReportObjectTypeEXT(pNode->objType),
                pNode->vkObj);
    }
    VkShaderModuleMap.clear();

    for (auto it = VkPipelineCacheMap.begin(); it != VkPipelineCacheMap.end(); ++it) {
        OBJTRACK_NODE* pNode = it->second;
        log_msg(mid(instance), VK_DEBUG_REPORT_ERROR_BIT_EXT, pNode->objType, pNode->vkObj, __LINE__, OBJTRACK_OBJECT_LEAK, "OBJTRACK",
                "OBJ ERROR : %s object 0x%" PRIxLEAST64 " has not been destroyed.", string_VkDebugReportObjectTypeEXT(pNode->objType),
                pNode->vkObj);
    }
    VkPipelineCacheMap.clear();

    for (auto it = VkPipelineLayoutMap.begin(); it != VkPipelineLayoutMap.end(); ++it) {
        OBJTRACK_NODE* pNode = it->second;
        log_msg(mid(instance), VK_DEBUG_REPORT_ERROR_BIT_EXT, pNode->objType, pNode->vkObj, __LINE__, OBJTRACK_OBJECT_LEAK, "OBJTRACK",
                "OBJ ERROR : %s object 0x%" PRIxLEAST64 " has not been destroyed.", string_VkDebugReportObjectTypeEXT(pNode->objType),
                pNode->vkObj);
    }
    VkPipelineLayoutMap.clear();

    for (auto it = VkRenderPassMap.begin(); it != VkRenderPassMap.end(); ++it) {
        OBJTRACK_NODE* pNode = it->second;
        log_msg(mid(instance), VK_DEBUG_REPORT_ERROR_BIT_EXT, pNode->objType, pNode->vkObj, __LINE__, OBJTRACK_OBJECT_LEAK, "OBJTRACK",
                "OBJ ERROR : %s object 0x%" PRIxLEAST64 " has not been destroyed.", string_VkDebugReportObjectTypeEXT(pNode->objType),
                pNode->vkObj);
    }
    VkRenderPassMap.clear();

    for (auto it = VkPipelineMap.begin(); it != VkPipelineMap.end(); ++it) {
        OBJTRACK_NODE* pNode = it->second;
        log_msg(mid(instance), VK_DEBUG_REPORT_ERROR_BIT_EXT, pNode->objType, pNode->vkObj, __LINE__, OBJTRACK_OBJECT_LEAK, "OBJTRACK",
                "OBJ ERROR : %s object 0x%" PRIxLEAST64 " has not been destroyed.", string_VkDebugReportObjectTypeEXT(pNode->objType),
                pNode->vkObj);
    }
    VkPipelineMap.clear();

    for (auto it = VkDescriptorSetLayoutMap.begin(); it != VkDescriptorSetLayoutMap.end(); ++it) {
        OBJTRACK_NODE* pNode = it->second;
        log_msg(mid(instance), VK_DEBUG_REPORT_ERROR_BIT_EXT, pNode->objType, pNode->vkObj, __LINE__, OBJTRACK_OBJECT_LEAK, "OBJTRACK",
                "OBJ ERROR : %s object 0x%" PRIxLEAST64 " has not been destroyed.", string_VkDebugReportObjectTypeEXT(pNode->objType),
                pNode->vkObj);
    }
    VkDescriptorSetLayoutMap.clear();

    for (auto it = VkSamplerMap.begin(); it != VkSamplerMap.end(); ++it) {
        OBJTRACK_NODE* pNode = it->second;
        log_msg(mid(instance), VK_DEBUG_REPORT_ERROR_BIT_EXT, pNode->objType, pNode->vkObj, __LINE__, OBJTRACK_OBJECT_LEAK, "OBJTRACK",
                "OBJ ERROR : %s object 0x%" PRIxLEAST64 " has not been destroyed.", string_VkDebugReportObjectTypeEXT(pNode->objType),
                pNode->vkObj);
    }
    VkSamplerMap.clear();

    for (auto it = VkDescriptorPoolMap.begin(); it != VkDescriptorPoolMap.end(); ++it) {
        OBJTRACK_NODE* pNode = it->second;
        log_msg(mid(instance), VK_DEBUG_REPORT_ERROR_BIT_EXT, pNode->objType, pNode->vkObj, __LINE__, OBJTRACK_OBJECT_LEAK, "OBJTRACK",
                "OBJ ERROR : %s object 0x%" PRIxLEAST64 " has not been destroyed.", string_VkDebugReportObjectTypeEXT(pNode->objType),
                pNode->vkObj);
    }
    VkDescriptorPoolMap.clear();

    for (auto it = VkDescriptorSetMap.begin(); it != VkDescriptorSetMap.end(); ++it) {
        OBJTRACK_NODE* pNode = it->second;
        log_msg(mid(instance), VK_DEBUG_REPORT_ERROR_BIT_EXT, pNode->objType, pNode->vkObj, __LINE__, OBJTRACK_OBJECT_LEAK, "OBJTRACK",
                "OBJ ERROR : %s object 0x%" PRIxLEAST64 " has not been destroyed.", string_VkDebugReportObjectTypeEXT(pNode->objType),
                pNode->vkObj);
    }
    VkDescriptorSetMap.clear();

    for (auto it = VkFramebufferMap.begin(); it != VkFramebufferMap.end(); ++it) {
        OBJTRACK_NODE* pNode = it->second;
        log_msg(mid(instance), VK_DEBUG_REPORT_ERROR_BIT_EXT, pNode->objType, pNode->vkObj, __LINE__, OBJTRACK_OBJECT_LEAK, "OBJTRACK",
                "OBJ ERROR : %s object 0x%" PRIxLEAST64 " has not been destroyed.", string_VkDebugReportObjectTypeEXT(pNode->objType),
                pNode->vkObj);
    }
    VkFramebufferMap.clear();

    for (auto it = VkCommandPoolMap.begin(); it != VkCommandPoolMap.end(); ++it) {
        OBJTRACK_NODE* pNode = it->second;
        log_msg(mid(instance), VK_DEBUG_REPORT_ERROR_BIT_EXT, pNode->objType, pNode->vkObj, __LINE__, OBJTRACK_OBJECT_LEAK, "OBJTRACK",
                "OBJ ERROR : %s object 0x%" PRIxLEAST64 " has not been destroyed.", string_VkDebugReportObjectTypeEXT(pNode->objType),
                pNode->vkObj);
    }
    VkCommandPoolMap.clear();

    dispatch_key key = get_dispatch_key(instance);
    VkLayerInstanceDispatchTable *pInstanceTable = get_dispatch_table(object_tracker_instance_table_map, instance);
    pInstanceTable->DestroyInstance(instance, pAllocator);

    // Clean up logging callback, if any
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    if (my_data->logging_callback) {
        layer_destroy_msg_callback(my_data->report_data, my_data->logging_callback, pAllocator);
    }

    layer_debug_report_destroy_instance(mid(instance));
    layer_data_map.erase(pInstanceTable);

    instanceExtMap.erase(pInstanceTable);
    loader_platform_thread_unlock_mutex(&objLock);
    object_tracker_instance_table_map.erase(key);
    if (object_tracker_instance_table_map.empty()) {
        // Release mutex when destroying last instance.
        loader_platform_thread_delete_mutex(&objLock);
        objLockInitialized = 0;
    }
}


// CODEGEN : file ../vk-layer-generate.py line #1066
VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyDevice(
VkDevice device,
const VkAllocationCallbacks* pAllocator)
{
    loader_platform_thread_lock_mutex(&objLock);
    validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);

    destroy_device(device, device);
    // Report any remaining objects in LL
    for (auto it = VkSemaphoreMap.begin(); it != VkSemaphoreMap.end(); ++it) {
        OBJTRACK_NODE* pNode = it->second;
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, pNode->objType, pNode->vkObj, __LINE__, OBJTRACK_OBJECT_LEAK, "OBJTRACK",
                "OBJ ERROR : %s object 0x%" PRIxLEAST64 " has not been destroyed.", string_VkDebugReportObjectTypeEXT(pNode->objType),
                pNode->vkObj);
    }
    VkSemaphoreMap.clear();

    for (auto it = VkFenceMap.begin(); it != VkFenceMap.end(); ++it) {
        OBJTRACK_NODE* pNode = it->second;
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, pNode->objType, pNode->vkObj, __LINE__, OBJTRACK_OBJECT_LEAK, "OBJTRACK",
                "OBJ ERROR : %s object 0x%" PRIxLEAST64 " has not been destroyed.", string_VkDebugReportObjectTypeEXT(pNode->objType),
                pNode->vkObj);
    }
    VkFenceMap.clear();

    for (auto it = VkDeviceMemoryMap.begin(); it != VkDeviceMemoryMap.end(); ++it) {
        OBJTRACK_NODE* pNode = it->second;
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, pNode->objType, pNode->vkObj, __LINE__, OBJTRACK_OBJECT_LEAK, "OBJTRACK",
                "OBJ ERROR : %s object 0x%" PRIxLEAST64 " has not been destroyed.", string_VkDebugReportObjectTypeEXT(pNode->objType),
                pNode->vkObj);
    }
    VkDeviceMemoryMap.clear();

    for (auto it = VkBufferMap.begin(); it != VkBufferMap.end(); ++it) {
        OBJTRACK_NODE* pNode = it->second;
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, pNode->objType, pNode->vkObj, __LINE__, OBJTRACK_OBJECT_LEAK, "OBJTRACK",
                "OBJ ERROR : %s object 0x%" PRIxLEAST64 " has not been destroyed.", string_VkDebugReportObjectTypeEXT(pNode->objType),
                pNode->vkObj);
    }
    VkBufferMap.clear();

    for (auto it = VkImageMap.begin(); it != VkImageMap.end(); ++it) {
        OBJTRACK_NODE* pNode = it->second;
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, pNode->objType, pNode->vkObj, __LINE__, OBJTRACK_OBJECT_LEAK, "OBJTRACK",
                "OBJ ERROR : %s object 0x%" PRIxLEAST64 " has not been destroyed.", string_VkDebugReportObjectTypeEXT(pNode->objType),
                pNode->vkObj);
    }
    VkImageMap.clear();

    for (auto it = VkEventMap.begin(); it != VkEventMap.end(); ++it) {
        OBJTRACK_NODE* pNode = it->second;
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, pNode->objType, pNode->vkObj, __LINE__, OBJTRACK_OBJECT_LEAK, "OBJTRACK",
                "OBJ ERROR : %s object 0x%" PRIxLEAST64 " has not been destroyed.", string_VkDebugReportObjectTypeEXT(pNode->objType),
                pNode->vkObj);
    }
    VkEventMap.clear();

    for (auto it = VkQueryPoolMap.begin(); it != VkQueryPoolMap.end(); ++it) {
        OBJTRACK_NODE* pNode = it->second;
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, pNode->objType, pNode->vkObj, __LINE__, OBJTRACK_OBJECT_LEAK, "OBJTRACK",
                "OBJ ERROR : %s object 0x%" PRIxLEAST64 " has not been destroyed.", string_VkDebugReportObjectTypeEXT(pNode->objType),
                pNode->vkObj);
    }
    VkQueryPoolMap.clear();

    for (auto it = VkBufferViewMap.begin(); it != VkBufferViewMap.end(); ++it) {
        OBJTRACK_NODE* pNode = it->second;
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, pNode->objType, pNode->vkObj, __LINE__, OBJTRACK_OBJECT_LEAK, "OBJTRACK",
                "OBJ ERROR : %s object 0x%" PRIxLEAST64 " has not been destroyed.", string_VkDebugReportObjectTypeEXT(pNode->objType),
                pNode->vkObj);
    }
    VkBufferViewMap.clear();

    for (auto it = VkImageViewMap.begin(); it != VkImageViewMap.end(); ++it) {
        OBJTRACK_NODE* pNode = it->second;
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, pNode->objType, pNode->vkObj, __LINE__, OBJTRACK_OBJECT_LEAK, "OBJTRACK",
                "OBJ ERROR : %s object 0x%" PRIxLEAST64 " has not been destroyed.", string_VkDebugReportObjectTypeEXT(pNode->objType),
                pNode->vkObj);
    }
    VkImageViewMap.clear();

    for (auto it = VkShaderModuleMap.begin(); it != VkShaderModuleMap.end(); ++it) {
        OBJTRACK_NODE* pNode = it->second;
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, pNode->objType, pNode->vkObj, __LINE__, OBJTRACK_OBJECT_LEAK, "OBJTRACK",
                "OBJ ERROR : %s object 0x%" PRIxLEAST64 " has not been destroyed.", string_VkDebugReportObjectTypeEXT(pNode->objType),
                pNode->vkObj);
    }
    VkShaderModuleMap.clear();

    for (auto it = VkPipelineCacheMap.begin(); it != VkPipelineCacheMap.end(); ++it) {
        OBJTRACK_NODE* pNode = it->second;
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, pNode->objType, pNode->vkObj, __LINE__, OBJTRACK_OBJECT_LEAK, "OBJTRACK",
                "OBJ ERROR : %s object 0x%" PRIxLEAST64 " has not been destroyed.", string_VkDebugReportObjectTypeEXT(pNode->objType),
                pNode->vkObj);
    }
    VkPipelineCacheMap.clear();

    for (auto it = VkPipelineLayoutMap.begin(); it != VkPipelineLayoutMap.end(); ++it) {
        OBJTRACK_NODE* pNode = it->second;
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, pNode->objType, pNode->vkObj, __LINE__, OBJTRACK_OBJECT_LEAK, "OBJTRACK",
                "OBJ ERROR : %s object 0x%" PRIxLEAST64 " has not been destroyed.", string_VkDebugReportObjectTypeEXT(pNode->objType),
                pNode->vkObj);
    }
    VkPipelineLayoutMap.clear();

    for (auto it = VkRenderPassMap.begin(); it != VkRenderPassMap.end(); ++it) {
        OBJTRACK_NODE* pNode = it->second;
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, pNode->objType, pNode->vkObj, __LINE__, OBJTRACK_OBJECT_LEAK, "OBJTRACK",
                "OBJ ERROR : %s object 0x%" PRIxLEAST64 " has not been destroyed.", string_VkDebugReportObjectTypeEXT(pNode->objType),
                pNode->vkObj);
    }
    VkRenderPassMap.clear();

    for (auto it = VkPipelineMap.begin(); it != VkPipelineMap.end(); ++it) {
        OBJTRACK_NODE* pNode = it->second;
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, pNode->objType, pNode->vkObj, __LINE__, OBJTRACK_OBJECT_LEAK, "OBJTRACK",
                "OBJ ERROR : %s object 0x%" PRIxLEAST64 " has not been destroyed.", string_VkDebugReportObjectTypeEXT(pNode->objType),
                pNode->vkObj);
    }
    VkPipelineMap.clear();

    for (auto it = VkDescriptorSetLayoutMap.begin(); it != VkDescriptorSetLayoutMap.end(); ++it) {
        OBJTRACK_NODE* pNode = it->second;
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, pNode->objType, pNode->vkObj, __LINE__, OBJTRACK_OBJECT_LEAK, "OBJTRACK",
                "OBJ ERROR : %s object 0x%" PRIxLEAST64 " has not been destroyed.", string_VkDebugReportObjectTypeEXT(pNode->objType),
                pNode->vkObj);
    }
    VkDescriptorSetLayoutMap.clear();

    for (auto it = VkSamplerMap.begin(); it != VkSamplerMap.end(); ++it) {
        OBJTRACK_NODE* pNode = it->second;
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, pNode->objType, pNode->vkObj, __LINE__, OBJTRACK_OBJECT_LEAK, "OBJTRACK",
                "OBJ ERROR : %s object 0x%" PRIxLEAST64 " has not been destroyed.", string_VkDebugReportObjectTypeEXT(pNode->objType),
                pNode->vkObj);
    }
    VkSamplerMap.clear();

    for (auto it = VkDescriptorPoolMap.begin(); it != VkDescriptorPoolMap.end(); ++it) {
        OBJTRACK_NODE* pNode = it->second;
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, pNode->objType, pNode->vkObj, __LINE__, OBJTRACK_OBJECT_LEAK, "OBJTRACK",
                "OBJ ERROR : %s object 0x%" PRIxLEAST64 " has not been destroyed.", string_VkDebugReportObjectTypeEXT(pNode->objType),
                pNode->vkObj);
    }
    VkDescriptorPoolMap.clear();

    for (auto it = VkFramebufferMap.begin(); it != VkFramebufferMap.end(); ++it) {
        OBJTRACK_NODE* pNode = it->second;
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, pNode->objType, pNode->vkObj, __LINE__, OBJTRACK_OBJECT_LEAK, "OBJTRACK",
                "OBJ ERROR : %s object 0x%" PRIxLEAST64 " has not been destroyed.", string_VkDebugReportObjectTypeEXT(pNode->objType),
                pNode->vkObj);
    }
    VkFramebufferMap.clear();

    for (auto it = VkCommandPoolMap.begin(); it != VkCommandPoolMap.end(); ++it) {
        OBJTRACK_NODE* pNode = it->second;
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, pNode->objType, pNode->vkObj, __LINE__, OBJTRACK_OBJECT_LEAK, "OBJTRACK",
                "OBJ ERROR : %s object 0x%" PRIxLEAST64 " has not been destroyed.", string_VkDebugReportObjectTypeEXT(pNode->objType),
                pNode->vkObj);
    }
    VkCommandPoolMap.clear();

    // Clean up Queue's MemRef Linked Lists
    destroyQueueMemRefLists();

    loader_platform_thread_unlock_mutex(&objLock);

    dispatch_key key = get_dispatch_key(device);
    VkLayerDispatchTable *pDisp = get_dispatch_table(object_tracker_device_table_map, device);
    pDisp->DestroyDevice(device, pAllocator);
    object_tracker_device_table_map.erase(key);

}


// CODEGEN : file ../vk-layer-generate.py line #1310
VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateInstance(const VkInstanceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkInstance* pInstance)
{
    return explicit_CreateInstance(pCreateInfo, pAllocator, pInstance);
}



// CODEGEN : file ../vk-layer-generate.py line #1310
VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumeratePhysicalDevices(VkInstance instance, uint32_t* pPhysicalDeviceCount, VkPhysicalDevice* pPhysicalDevices)
{
    return explicit_EnumeratePhysicalDevices(instance, pPhysicalDeviceCount, pPhysicalDevices);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceFeatures(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures* pFeatures)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'physicalDevice': 'VkPhysicalDevice'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_physical_device(physicalDevice, physicalDevice, VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    get_dispatch_table(object_tracker_instance_table_map, physicalDevice)->GetPhysicalDeviceFeatures(physicalDevice, pFeatures);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkFormatProperties* pFormatProperties)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'physicalDevice': 'VkPhysicalDevice'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_physical_device(physicalDevice, physicalDevice, VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    get_dispatch_table(object_tracker_instance_table_map, physicalDevice)->GetPhysicalDeviceFormatProperties(physicalDevice, format, pFormatProperties);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceImageFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type, VkImageTiling tiling, VkImageUsageFlags usage, VkImageCreateFlags flags, VkImageFormatProperties* pImageFormatProperties)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'physicalDevice': 'VkPhysicalDevice'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_physical_device(physicalDevice, physicalDevice, VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = get_dispatch_table(object_tracker_instance_table_map, physicalDevice)->GetPhysicalDeviceImageFormatProperties(physicalDevice, format, type, tiling, usage, flags, pImageFormatProperties);
    return result;
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceProperties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties* pProperties)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'physicalDevice': 'VkPhysicalDevice'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_physical_device(physicalDevice, physicalDevice, VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    get_dispatch_table(object_tracker_instance_table_map, physicalDevice)->GetPhysicalDeviceProperties(physicalDevice, pProperties);
}

// CODEGEN : file ../vk-layer-generate.py line #1310
VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice physicalDevice, uint32_t* pQueueFamilyPropertyCount, VkQueueFamilyProperties* pQueueFamilyProperties)
{
    return explicit_GetPhysicalDeviceQueueFamilyProperties(physicalDevice, pQueueFamilyPropertyCount, pQueueFamilyProperties);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceMemoryProperties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceMemoryProperties* pMemoryProperties)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'physicalDevice': 'VkPhysicalDevice'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_physical_device(physicalDevice, physicalDevice, VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    get_dispatch_table(object_tracker_instance_table_map, physicalDevice)->GetPhysicalDeviceMemoryProperties(physicalDevice, pMemoryProperties);
}

// CODEGEN : file ../vk-layer-generate.py line #1310
VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDevice* pDevice)
{
    return explicit_CreateDevice(physicalDevice, pCreateInfo, pAllocator, pDevice);
}



// CODEGEN : file ../vk-layer-generate.py line #361

static const VkExtensionProperties instance_extensions[] = {
    {
        VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
        VK_EXT_DEBUG_REPORT_SPEC_VERSION
    }
};
VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceExtensionProperties(const char *pLayerName, uint32_t *pCount,  VkExtensionProperties* pProperties)
{
    return util_GetExtensionProperties(1, instance_extensions, pCount, pProperties);
}

// CODEGEN : file ../vk-layer-generate.py line #384
static const VkLayerProperties globalLayerProps[] = {
    {
        "VK_LAYER_LUNARG_object_tracker",
        VK_API_VERSION, // specVersion
        1, // implementationVersion
        "LunarG Validation Layer"
    }
};

// CODEGEN : file ../vk-layer-generate.py line #400

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceLayerProperties(uint32_t *pCount,  VkLayerProperties* pProperties)
{
    return util_GetLayerProperties(ARRAY_SIZE(globalLayerProps), globalLayerProps, pCount, pProperties);
}

// CODEGEN : file ../vk-layer-generate.py line #410
static const VkLayerProperties deviceLayerProps[] = {
    {
        "VK_LAYER_LUNARG_object_tracker",
        VK_API_VERSION, // specVersion
        1, // implementationVersion
        "LunarG Validation Layer"
    }
};
VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceLayerProperties(VkPhysicalDevice physicalDevice, uint32_t *pCount, VkLayerProperties* pProperties)
{
    return util_GetLayerProperties(ARRAY_SIZE(deviceLayerProps), deviceLayerProps, pCount, pProperties);
}


// CODEGEN : file ../vk-layer-generate.py line #1310
VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetDeviceQueue(VkDevice device, uint32_t queueFamilyIndex, uint32_t queueIndex, VkQueue* pQueue)
{
    return explicit_GetDeviceQueue(device, queueFamilyIndex, queueIndex, pQueue);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkQueueSubmit(VkQueue queue, uint32_t submitCount, const VkSubmitInfo* pSubmits, VkFence fence)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'queue': 'VkQueue', 'pSubmits[submitCount]': {'pWaitSemaphores[waitSemaphoreCount]': 'VkSemaphore', 'pSignalSemaphores[signalSemaphoreCount]': 'VkSemaphore', 'pCommandBuffers[commandBufferCount]': 'VkCommandBuffer'}, 'fence': 'VkFence'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_fence(queue, fence, VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT, true);
    if (pSubmits) {
// CODEGEN : file ../vk-layer-generate.py line #1149
        for (uint32_t idx0=0; idx0<submitCount; ++idx0) {
            if (pSubmits[idx0].pCommandBuffers) {
                for (uint32_t idx1=0; idx1<pSubmits[idx0].commandBufferCount; ++idx1) {
// CODEGEN : file ../vk-layer-generate.py line #1185
                    skipCall |= validate_command_buffer(queue, pSubmits[idx0].pCommandBuffers[idx1], VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false);
                }
            }
            if (pSubmits[idx0].pSignalSemaphores) {
                for (uint32_t idx2=0; idx2<pSubmits[idx0].signalSemaphoreCount; ++idx2) {
// CODEGEN : file ../vk-layer-generate.py line #1185
                    skipCall |= validate_semaphore(queue, pSubmits[idx0].pSignalSemaphores[idx2], VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT, false);
                }
            }
            if (pSubmits[idx0].pWaitSemaphores) {
                for (uint32_t idx3=0; idx3<pSubmits[idx0].waitSemaphoreCount; ++idx3) {
// CODEGEN : file ../vk-layer-generate.py line #1185
                    skipCall |= validate_semaphore(queue, pSubmits[idx0].pWaitSemaphores[idx3], VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT, false);
                }
            }
        }
    }
    if (queue) {
// CODEGEN : file ../vk-layer-generate.py line #1185
        skipCall |= validate_queue(queue, queue, VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT, false);
    }
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = get_dispatch_table(object_tracker_device_table_map, queue)->QueueSubmit(queue, submitCount, pSubmits, fence);
    return result;
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkQueueWaitIdle(VkQueue queue)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'queue': 'VkQueue'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_queue(queue, queue, VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = get_dispatch_table(object_tracker_device_table_map, queue)->QueueWaitIdle(queue);
    return result;
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkDeviceWaitIdle(VkDevice device)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'device': 'VkDevice'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = get_dispatch_table(object_tracker_device_table_map, device)->DeviceWaitIdle(device);
    return result;
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkAllocateMemory(VkDevice device, const VkMemoryAllocateInfo* pAllocateInfo, const VkAllocationCallbacks* pAllocator, VkDeviceMemory* pMemory)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'device': 'VkDevice'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = get_dispatch_table(object_tracker_device_table_map, device)->AllocateMemory(device, pAllocateInfo, pAllocator, pMemory);
    loader_platform_thread_lock_mutex(&objLock);
    if (result == VK_SUCCESS) {
        create_device_memory(device, *pMemory, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT);
    }
    loader_platform_thread_unlock_mutex(&objLock);
    return result;
}

// CODEGEN : file ../vk-layer-generate.py line #1310
VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkFreeMemory(VkDevice device, VkDeviceMemory memory, const VkAllocationCallbacks* pAllocator)
{
    return explicit_FreeMemory(device, memory, pAllocator);
}

// CODEGEN : file ../vk-layer-generate.py line #1310
VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkMapMemory(VkDevice device, VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags, void** ppData)
{
    return explicit_MapMemory(device, memory, offset, size, flags, ppData);
}

// CODEGEN : file ../vk-layer-generate.py line #1310
VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkUnmapMemory(VkDevice device, VkDeviceMemory memory)
{
    return explicit_UnmapMemory(device, memory);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkFlushMappedMemoryRanges(VkDevice device, uint32_t memoryRangeCount, const VkMappedMemoryRange* pMemoryRanges)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'device': 'VkDevice', 'pMemoryRanges[memoryRangeCount]': {'memory': 'VkDeviceMemory'}}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
    if (pMemoryRanges) {
// CODEGEN : file ../vk-layer-generate.py line #1149
        for (uint32_t idx0=0; idx0<memoryRangeCount; ++idx0) {
            if (pMemoryRanges[idx0].memory) {
// CODEGEN : file ../vk-layer-generate.py line #1185
                skipCall |= validate_device_memory(device, pMemoryRanges[idx0].memory, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, false);
            }
        }
    }
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = get_dispatch_table(object_tracker_device_table_map, device)->FlushMappedMemoryRanges(device, memoryRangeCount, pMemoryRanges);
    return result;
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkInvalidateMappedMemoryRanges(VkDevice device, uint32_t memoryRangeCount, const VkMappedMemoryRange* pMemoryRanges)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'device': 'VkDevice', 'pMemoryRanges[memoryRangeCount]': {'memory': 'VkDeviceMemory'}}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
    if (pMemoryRanges) {
// CODEGEN : file ../vk-layer-generate.py line #1149
        for (uint32_t idx0=0; idx0<memoryRangeCount; ++idx0) {
            if (pMemoryRanges[idx0].memory) {
// CODEGEN : file ../vk-layer-generate.py line #1185
                skipCall |= validate_device_memory(device, pMemoryRanges[idx0].memory, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, false);
            }
        }
    }
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = get_dispatch_table(object_tracker_device_table_map, device)->InvalidateMappedMemoryRanges(device, memoryRangeCount, pMemoryRanges);
    return result;
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetDeviceMemoryCommitment(VkDevice device, VkDeviceMemory memory, VkDeviceSize* pCommittedMemoryInBytes)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'device': 'VkDevice', 'memory': 'VkDeviceMemory'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_device_memory(device, memory, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    get_dispatch_table(object_tracker_device_table_map, device)->GetDeviceMemoryCommitment(device, memory, pCommittedMemoryInBytes);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkBindBufferMemory(VkDevice device, VkBuffer buffer, VkDeviceMemory memory, VkDeviceSize memoryOffset)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'device': 'VkDevice', 'buffer': 'VkBuffer', 'memory': 'VkDeviceMemory'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_buffer(device, buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_device_memory(device, memory, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = get_dispatch_table(object_tracker_device_table_map, device)->BindBufferMemory(device, buffer, memory, memoryOffset);
    return result;
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkBindImageMemory(VkDevice device, VkImage image, VkDeviceMemory memory, VkDeviceSize memoryOffset)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'device': 'VkDevice', 'image': 'VkImage', 'memory': 'VkDeviceMemory'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_image(device, image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_device_memory(device, memory, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = get_dispatch_table(object_tracker_device_table_map, device)->BindImageMemory(device, image, memory, memoryOffset);
    return result;
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetBufferMemoryRequirements(VkDevice device, VkBuffer buffer, VkMemoryRequirements* pMemoryRequirements)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'device': 'VkDevice', 'buffer': 'VkBuffer'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_buffer(device, buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    get_dispatch_table(object_tracker_device_table_map, device)->GetBufferMemoryRequirements(device, buffer, pMemoryRequirements);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetImageMemoryRequirements(VkDevice device, VkImage image, VkMemoryRequirements* pMemoryRequirements)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'device': 'VkDevice', 'image': 'VkImage'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_image(device, image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    get_dispatch_table(object_tracker_device_table_map, device)->GetImageMemoryRequirements(device, image, pMemoryRequirements);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetImageSparseMemoryRequirements(VkDevice device, VkImage image, uint32_t* pSparseMemoryRequirementCount, VkSparseImageMemoryRequirements* pSparseMemoryRequirements)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'device': 'VkDevice', 'image': 'VkImage'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_image(device, image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    get_dispatch_table(object_tracker_device_table_map, device)->GetImageSparseMemoryRequirements(device, image, pSparseMemoryRequirementCount, pSparseMemoryRequirements);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceSparseImageFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type, VkSampleCountFlagBits samples, VkImageUsageFlags usage, VkImageTiling tiling, uint32_t* pPropertyCount, VkSparseImageFormatProperties* pProperties)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'physicalDevice': 'VkPhysicalDevice'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_physical_device(physicalDevice, physicalDevice, VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    get_dispatch_table(object_tracker_instance_table_map, physicalDevice)->GetPhysicalDeviceSparseImageFormatProperties(physicalDevice, format, type, samples, usage, tiling, pPropertyCount, pProperties);
}

// CODEGEN : file ../vk-layer-generate.py line #1310
VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkQueueBindSparse(VkQueue queue, uint32_t bindInfoCount, const VkBindSparseInfo* pBindInfo, VkFence fence)
{
    return explicit_QueueBindSparse(queue, bindInfoCount, pBindInfo, fence);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateFence(VkDevice device, const VkFenceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkFence* pFence)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'device': 'VkDevice'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = get_dispatch_table(object_tracker_device_table_map, device)->CreateFence(device, pCreateInfo, pAllocator, pFence);
    loader_platform_thread_lock_mutex(&objLock);
    if (result == VK_SUCCESS) {
        create_fence(device, *pFence, VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT);
    }
    loader_platform_thread_unlock_mutex(&objLock);
    return result;
}

// CODEGEN : file ../vk-layer-generate.py line #1310


// CODEGEN : file ../vk-layer-generate.py line #1344


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyFence(VkDevice device, VkFence fence, const VkAllocationCallbacks* pAllocator)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'device': 'VkDevice', 'fence': 'VkFence'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_fence(device, fence, VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    loader_platform_thread_lock_mutex(&objLock);
    destroy_fence(device, fence);
    loader_platform_thread_unlock_mutex(&objLock);
    get_dispatch_table(object_tracker_device_table_map, device)->DestroyFence(device, fence, pAllocator);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkResetFences(VkDevice device, uint32_t fenceCount, const VkFence* pFences)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'device': 'VkDevice', 'pFences[fenceCount]': 'VkFence'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
    if (pFences) {
        for (uint32_t idx0=0; idx0<fenceCount; ++idx0) {
// CODEGEN : file ../vk-layer-generate.py line #1185
            skipCall |= validate_fence(device, pFences[idx0], VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT, false);
        }
    }
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = get_dispatch_table(object_tracker_device_table_map, device)->ResetFences(device, fenceCount, pFences);
    return result;
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkGetFenceStatus(VkDevice device, VkFence fence)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'device': 'VkDevice', 'fence': 'VkFence'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_fence(device, fence, VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = get_dispatch_table(object_tracker_device_table_map, device)->GetFenceStatus(device, fence);
    return result;
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkWaitForFences(VkDevice device, uint32_t fenceCount, const VkFence* pFences, VkBool32 waitAll, uint64_t timeout)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'device': 'VkDevice', 'pFences[fenceCount]': 'VkFence'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
    if (pFences) {
        for (uint32_t idx0=0; idx0<fenceCount; ++idx0) {
// CODEGEN : file ../vk-layer-generate.py line #1185
            skipCall |= validate_fence(device, pFences[idx0], VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT, false);
        }
    }
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = get_dispatch_table(object_tracker_device_table_map, device)->WaitForFences(device, fenceCount, pFences, waitAll, timeout);
    return result;
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateSemaphore(VkDevice device, const VkSemaphoreCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSemaphore* pSemaphore)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'device': 'VkDevice'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = get_dispatch_table(object_tracker_device_table_map, device)->CreateSemaphore(device, pCreateInfo, pAllocator, pSemaphore);
    loader_platform_thread_lock_mutex(&objLock);
    if (result == VK_SUCCESS) {
        create_semaphore(device, *pSemaphore, VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT);
    }
    loader_platform_thread_unlock_mutex(&objLock);
    return result;
}

// CODEGEN : file ../vk-layer-generate.py line #1310


// CODEGEN : file ../vk-layer-generate.py line #1344


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroySemaphore(VkDevice device, VkSemaphore semaphore, const VkAllocationCallbacks* pAllocator)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'device': 'VkDevice', 'semaphore': 'VkSemaphore'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_semaphore(device, semaphore, VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    loader_platform_thread_lock_mutex(&objLock);
    destroy_semaphore(device, semaphore);
    loader_platform_thread_unlock_mutex(&objLock);
    get_dispatch_table(object_tracker_device_table_map, device)->DestroySemaphore(device, semaphore, pAllocator);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateEvent(VkDevice device, const VkEventCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkEvent* pEvent)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'device': 'VkDevice'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = get_dispatch_table(object_tracker_device_table_map, device)->CreateEvent(device, pCreateInfo, pAllocator, pEvent);
    loader_platform_thread_lock_mutex(&objLock);
    if (result == VK_SUCCESS) {
        create_event(device, *pEvent, VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT);
    }
    loader_platform_thread_unlock_mutex(&objLock);
    return result;
}

// CODEGEN : file ../vk-layer-generate.py line #1310


// CODEGEN : file ../vk-layer-generate.py line #1344


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyEvent(VkDevice device, VkEvent event, const VkAllocationCallbacks* pAllocator)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'device': 'VkDevice', 'event': 'VkEvent'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_event(device, event, VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    loader_platform_thread_lock_mutex(&objLock);
    destroy_event(device, event);
    loader_platform_thread_unlock_mutex(&objLock);
    get_dispatch_table(object_tracker_device_table_map, device)->DestroyEvent(device, event, pAllocator);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkGetEventStatus(VkDevice device, VkEvent event)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'device': 'VkDevice', 'event': 'VkEvent'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_event(device, event, VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = get_dispatch_table(object_tracker_device_table_map, device)->GetEventStatus(device, event);
    return result;
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkSetEvent(VkDevice device, VkEvent event)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'device': 'VkDevice', 'event': 'VkEvent'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_event(device, event, VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = get_dispatch_table(object_tracker_device_table_map, device)->SetEvent(device, event);
    return result;
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkResetEvent(VkDevice device, VkEvent event)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'device': 'VkDevice', 'event': 'VkEvent'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_event(device, event, VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = get_dispatch_table(object_tracker_device_table_map, device)->ResetEvent(device, event);
    return result;
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateQueryPool(VkDevice device, const VkQueryPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkQueryPool* pQueryPool)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'device': 'VkDevice'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = get_dispatch_table(object_tracker_device_table_map, device)->CreateQueryPool(device, pCreateInfo, pAllocator, pQueryPool);
    loader_platform_thread_lock_mutex(&objLock);
    if (result == VK_SUCCESS) {
        create_query_pool(device, *pQueryPool, VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT);
    }
    loader_platform_thread_unlock_mutex(&objLock);
    return result;
}

// CODEGEN : file ../vk-layer-generate.py line #1310


// CODEGEN : file ../vk-layer-generate.py line #1344


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyQueryPool(VkDevice device, VkQueryPool queryPool, const VkAllocationCallbacks* pAllocator)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'device': 'VkDevice', 'queryPool': 'VkQueryPool'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_query_pool(device, queryPool, VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    loader_platform_thread_lock_mutex(&objLock);
    destroy_query_pool(device, queryPool);
    loader_platform_thread_unlock_mutex(&objLock);
    get_dispatch_table(object_tracker_device_table_map, device)->DestroyQueryPool(device, queryPool, pAllocator);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkGetQueryPoolResults(VkDevice device, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount, size_t dataSize, void* pData, VkDeviceSize stride, VkQueryResultFlags flags)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'device': 'VkDevice', 'queryPool': 'VkQueryPool'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_query_pool(device, queryPool, VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = get_dispatch_table(object_tracker_device_table_map, device)->GetQueryPoolResults(device, queryPool, firstQuery, queryCount, dataSize, pData, stride, flags);
    return result;
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateBuffer(VkDevice device, const VkBufferCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkBuffer* pBuffer)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'device': 'VkDevice'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = get_dispatch_table(object_tracker_device_table_map, device)->CreateBuffer(device, pCreateInfo, pAllocator, pBuffer);
    loader_platform_thread_lock_mutex(&objLock);
    if (result == VK_SUCCESS) {
        create_buffer(device, *pBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT);
    }
    loader_platform_thread_unlock_mutex(&objLock);
    return result;
}

// CODEGEN : file ../vk-layer-generate.py line #1310


// CODEGEN : file ../vk-layer-generate.py line #1344


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyBuffer(VkDevice device, VkBuffer buffer, const VkAllocationCallbacks* pAllocator)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'device': 'VkDevice', 'buffer': 'VkBuffer'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_buffer(device, buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    loader_platform_thread_lock_mutex(&objLock);
    destroy_buffer(device, buffer);
    loader_platform_thread_unlock_mutex(&objLock);
    get_dispatch_table(object_tracker_device_table_map, device)->DestroyBuffer(device, buffer, pAllocator);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateBufferView(VkDevice device, const VkBufferViewCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkBufferView* pView)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'device': 'VkDevice', 'pCreateInfo': {'buffer': 'VkBuffer'}}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
    if (pCreateInfo) {
// CODEGEN : file ../vk-layer-generate.py line #1193
        skipCall |= validate_buffer(device, pCreateInfo->buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, false);
    }
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = get_dispatch_table(object_tracker_device_table_map, device)->CreateBufferView(device, pCreateInfo, pAllocator, pView);
    loader_platform_thread_lock_mutex(&objLock);
    if (result == VK_SUCCESS) {
        create_buffer_view(device, *pView, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_VIEW_EXT);
    }
    loader_platform_thread_unlock_mutex(&objLock);
    return result;
}

// CODEGEN : file ../vk-layer-generate.py line #1310


// CODEGEN : file ../vk-layer-generate.py line #1344


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyBufferView(VkDevice device, VkBufferView bufferView, const VkAllocationCallbacks* pAllocator)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'device': 'VkDevice', 'bufferView': 'VkBufferView'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_buffer_view(device, bufferView, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_VIEW_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    loader_platform_thread_lock_mutex(&objLock);
    destroy_buffer_view(device, bufferView);
    loader_platform_thread_unlock_mutex(&objLock);
    get_dispatch_table(object_tracker_device_table_map, device)->DestroyBufferView(device, bufferView, pAllocator);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateImage(VkDevice device, const VkImageCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkImage* pImage)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'device': 'VkDevice'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = get_dispatch_table(object_tracker_device_table_map, device)->CreateImage(device, pCreateInfo, pAllocator, pImage);
    loader_platform_thread_lock_mutex(&objLock);
    if (result == VK_SUCCESS) {
        create_image(device, *pImage, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT);
    }
    loader_platform_thread_unlock_mutex(&objLock);
    return result;
}

// CODEGEN : file ../vk-layer-generate.py line #1310


// CODEGEN : file ../vk-layer-generate.py line #1344


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyImage(VkDevice device, VkImage image, const VkAllocationCallbacks* pAllocator)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'device': 'VkDevice', 'image': 'VkImage'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_image(device, image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    loader_platform_thread_lock_mutex(&objLock);
    destroy_image(device, image);
    loader_platform_thread_unlock_mutex(&objLock);
    get_dispatch_table(object_tracker_device_table_map, device)->DestroyImage(device, image, pAllocator);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetImageSubresourceLayout(VkDevice device, VkImage image, const VkImageSubresource* pSubresource, VkSubresourceLayout* pLayout)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'device': 'VkDevice', 'image': 'VkImage'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_image(device, image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    get_dispatch_table(object_tracker_device_table_map, device)->GetImageSubresourceLayout(device, image, pSubresource, pLayout);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateImageView(VkDevice device, const VkImageViewCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkImageView* pView)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'device': 'VkDevice', 'pCreateInfo': {'image': 'VkImage'}}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
    if (pCreateInfo) {
// CODEGEN : file ../vk-layer-generate.py line #1193
        skipCall |= validate_image(device, pCreateInfo->image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, false);
    }
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = get_dispatch_table(object_tracker_device_table_map, device)->CreateImageView(device, pCreateInfo, pAllocator, pView);
    loader_platform_thread_lock_mutex(&objLock);
    if (result == VK_SUCCESS) {
        create_image_view(device, *pView, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT);
    }
    loader_platform_thread_unlock_mutex(&objLock);
    return result;
}

// CODEGEN : file ../vk-layer-generate.py line #1310


// CODEGEN : file ../vk-layer-generate.py line #1344


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyImageView(VkDevice device, VkImageView imageView, const VkAllocationCallbacks* pAllocator)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'device': 'VkDevice', 'imageView': 'VkImageView'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_image_view(device, imageView, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    loader_platform_thread_lock_mutex(&objLock);
    destroy_image_view(device, imageView);
    loader_platform_thread_unlock_mutex(&objLock);
    get_dispatch_table(object_tracker_device_table_map, device)->DestroyImageView(device, imageView, pAllocator);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateShaderModule(VkDevice device, const VkShaderModuleCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkShaderModule* pShaderModule)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'device': 'VkDevice'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = get_dispatch_table(object_tracker_device_table_map, device)->CreateShaderModule(device, pCreateInfo, pAllocator, pShaderModule);
    loader_platform_thread_lock_mutex(&objLock);
    if (result == VK_SUCCESS) {
        create_shader_module(device, *pShaderModule, VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT);
    }
    loader_platform_thread_unlock_mutex(&objLock);
    return result;
}

// CODEGEN : file ../vk-layer-generate.py line #1310


// CODEGEN : file ../vk-layer-generate.py line #1344


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyShaderModule(VkDevice device, VkShaderModule shaderModule, const VkAllocationCallbacks* pAllocator)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'device': 'VkDevice', 'shaderModule': 'VkShaderModule'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_shader_module(device, shaderModule, VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    loader_platform_thread_lock_mutex(&objLock);
    destroy_shader_module(device, shaderModule);
    loader_platform_thread_unlock_mutex(&objLock);
    get_dispatch_table(object_tracker_device_table_map, device)->DestroyShaderModule(device, shaderModule, pAllocator);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreatePipelineCache(VkDevice device, const VkPipelineCacheCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkPipelineCache* pPipelineCache)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'device': 'VkDevice'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = get_dispatch_table(object_tracker_device_table_map, device)->CreatePipelineCache(device, pCreateInfo, pAllocator, pPipelineCache);
    loader_platform_thread_lock_mutex(&objLock);
    if (result == VK_SUCCESS) {
        create_pipeline_cache(device, *pPipelineCache, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_CACHE_EXT);
    }
    loader_platform_thread_unlock_mutex(&objLock);
    return result;
}

// CODEGEN : file ../vk-layer-generate.py line #1310


// CODEGEN : file ../vk-layer-generate.py line #1344


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyPipelineCache(VkDevice device, VkPipelineCache pipelineCache, const VkAllocationCallbacks* pAllocator)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'device': 'VkDevice', 'pipelineCache': 'VkPipelineCache'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_pipeline_cache(device, pipelineCache, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_CACHE_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    loader_platform_thread_lock_mutex(&objLock);
    destroy_pipeline_cache(device, pipelineCache);
    loader_platform_thread_unlock_mutex(&objLock);
    get_dispatch_table(object_tracker_device_table_map, device)->DestroyPipelineCache(device, pipelineCache, pAllocator);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkGetPipelineCacheData(VkDevice device, VkPipelineCache pipelineCache, size_t* pDataSize, void* pData)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'device': 'VkDevice', 'pipelineCache': 'VkPipelineCache'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_pipeline_cache(device, pipelineCache, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_CACHE_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = get_dispatch_table(object_tracker_device_table_map, device)->GetPipelineCacheData(device, pipelineCache, pDataSize, pData);
    return result;
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkMergePipelineCaches(VkDevice device, VkPipelineCache dstCache, uint32_t srcCacheCount, const VkPipelineCache* pSrcCaches)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'device': 'VkDevice', 'dstCache': 'VkPipelineCache', 'pSrcCaches[srcCacheCount]': 'VkPipelineCache'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_pipeline_cache(device, dstCache, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_CACHE_EXT, false);
    if (pSrcCaches) {
        for (uint32_t idx0=0; idx0<srcCacheCount; ++idx0) {
// CODEGEN : file ../vk-layer-generate.py line #1185
            skipCall |= validate_pipeline_cache(device, pSrcCaches[idx0], VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_CACHE_EXT, false);
        }
    }
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = get_dispatch_table(object_tracker_device_table_map, device)->MergePipelineCaches(device, dstCache, srcCacheCount, pSrcCaches);
    return result;
}

// CODEGEN : file ../vk-layer-generate.py line #1310
VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateGraphicsPipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkGraphicsPipelineCreateInfo* pCreateInfos, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines)
{
    return explicit_CreateGraphicsPipelines(device, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
}

// CODEGEN : file ../vk-layer-generate.py line #1310
VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateComputePipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkComputePipelineCreateInfo* pCreateInfos, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines)
{
    return explicit_CreateComputePipelines(device, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


// CODEGEN : file ../vk-layer-generate.py line #1344


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyPipeline(VkDevice device, VkPipeline pipeline, const VkAllocationCallbacks* pAllocator)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'device': 'VkDevice', 'pipeline': 'VkPipeline'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_pipeline(device, pipeline, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    loader_platform_thread_lock_mutex(&objLock);
    destroy_pipeline(device, pipeline);
    loader_platform_thread_unlock_mutex(&objLock);
    get_dispatch_table(object_tracker_device_table_map, device)->DestroyPipeline(device, pipeline, pAllocator);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreatePipelineLayout(VkDevice device, const VkPipelineLayoutCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkPipelineLayout* pPipelineLayout)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'device': 'VkDevice', 'pCreateInfo': {'pSetLayouts[setLayoutCount]': 'VkDescriptorSetLayout'}}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
    if (pCreateInfo) {
        if (pCreateInfo->pSetLayouts) {
            for (uint32_t idx0=0; idx0<pCreateInfo->setLayoutCount; ++idx0) {
// CODEGEN : file ../vk-layer-generate.py line #1185
                skipCall |= validate_descriptor_set_layout(device, pCreateInfo->pSetLayouts[idx0], VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT, false);
            }
        }
    }
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = get_dispatch_table(object_tracker_device_table_map, device)->CreatePipelineLayout(device, pCreateInfo, pAllocator, pPipelineLayout);
    loader_platform_thread_lock_mutex(&objLock);
    if (result == VK_SUCCESS) {
        create_pipeline_layout(device, *pPipelineLayout, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT);
    }
    loader_platform_thread_unlock_mutex(&objLock);
    return result;
}

// CODEGEN : file ../vk-layer-generate.py line #1310


// CODEGEN : file ../vk-layer-generate.py line #1344


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyPipelineLayout(VkDevice device, VkPipelineLayout pipelineLayout, const VkAllocationCallbacks* pAllocator)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'device': 'VkDevice', 'pipelineLayout': 'VkPipelineLayout'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_pipeline_layout(device, pipelineLayout, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    loader_platform_thread_lock_mutex(&objLock);
    destroy_pipeline_layout(device, pipelineLayout);
    loader_platform_thread_unlock_mutex(&objLock);
    get_dispatch_table(object_tracker_device_table_map, device)->DestroyPipelineLayout(device, pipelineLayout, pAllocator);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateSampler(VkDevice device, const VkSamplerCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSampler* pSampler)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'device': 'VkDevice'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = get_dispatch_table(object_tracker_device_table_map, device)->CreateSampler(device, pCreateInfo, pAllocator, pSampler);
    loader_platform_thread_lock_mutex(&objLock);
    if (result == VK_SUCCESS) {
        create_sampler(device, *pSampler, VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT);
    }
    loader_platform_thread_unlock_mutex(&objLock);
    return result;
}

// CODEGEN : file ../vk-layer-generate.py line #1310


// CODEGEN : file ../vk-layer-generate.py line #1344


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroySampler(VkDevice device, VkSampler sampler, const VkAllocationCallbacks* pAllocator)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'device': 'VkDevice', 'sampler': 'VkSampler'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_sampler(device, sampler, VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    loader_platform_thread_lock_mutex(&objLock);
    destroy_sampler(device, sampler);
    loader_platform_thread_unlock_mutex(&objLock);
    get_dispatch_table(object_tracker_device_table_map, device)->DestroySampler(device, sampler, pAllocator);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateDescriptorSetLayout(VkDevice device, const VkDescriptorSetLayoutCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorSetLayout* pSetLayout)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'device': 'VkDevice', 'pCreateInfo': {'pBindings[bindingCount]': {'pImmutableSamplers[descriptorCount]': 'VkSampler'}}}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
    if (pCreateInfo) {
        if (pCreateInfo->pBindings) {
// CODEGEN : file ../vk-layer-generate.py line #1149
            for (uint32_t idx0=0; idx0<pCreateInfo->bindingCount; ++idx0) {
                if (pCreateInfo->pBindings[idx0].pImmutableSamplers) {
                    for (uint32_t idx1=0; idx1<pCreateInfo->pBindings[idx0].descriptorCount; ++idx1) {
// CODEGEN : file ../vk-layer-generate.py line #1185
                        skipCall |= validate_sampler(device, pCreateInfo->pBindings[idx0].pImmutableSamplers[idx1], VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT, false);
                    }
                }
            }
        }
    }
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = get_dispatch_table(object_tracker_device_table_map, device)->CreateDescriptorSetLayout(device, pCreateInfo, pAllocator, pSetLayout);
    loader_platform_thread_lock_mutex(&objLock);
    if (result == VK_SUCCESS) {
        create_descriptor_set_layout(device, *pSetLayout, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT);
    }
    loader_platform_thread_unlock_mutex(&objLock);
    return result;
}

// CODEGEN : file ../vk-layer-generate.py line #1310


// CODEGEN : file ../vk-layer-generate.py line #1344


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout descriptorSetLayout, const VkAllocationCallbacks* pAllocator)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'device': 'VkDevice', 'descriptorSetLayout': 'VkDescriptorSetLayout'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_descriptor_set_layout(device, descriptorSetLayout, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    loader_platform_thread_lock_mutex(&objLock);
    destroy_descriptor_set_layout(device, descriptorSetLayout);
    loader_platform_thread_unlock_mutex(&objLock);
    get_dispatch_table(object_tracker_device_table_map, device)->DestroyDescriptorSetLayout(device, descriptorSetLayout, pAllocator);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateDescriptorPool(VkDevice device, const VkDescriptorPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorPool* pDescriptorPool)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'device': 'VkDevice'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = get_dispatch_table(object_tracker_device_table_map, device)->CreateDescriptorPool(device, pCreateInfo, pAllocator, pDescriptorPool);
    loader_platform_thread_lock_mutex(&objLock);
    if (result == VK_SUCCESS) {
        create_descriptor_pool(device, *pDescriptorPool, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT);
    }
    loader_platform_thread_unlock_mutex(&objLock);
    return result;
}

// CODEGEN : file ../vk-layer-generate.py line #1310
VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool, const VkAllocationCallbacks* pAllocator)
{
    return explicit_DestroyDescriptorPool(device, descriptorPool, pAllocator);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkResetDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool, VkDescriptorPoolResetFlags flags)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'device': 'VkDevice', 'descriptorPool': 'VkDescriptorPool'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_descriptor_pool(device, descriptorPool, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = get_dispatch_table(object_tracker_device_table_map, device)->ResetDescriptorPool(device, descriptorPool, flags);
    return result;
}

// CODEGEN : file ../vk-layer-generate.py line #1310
VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkAllocateDescriptorSets(VkDevice device, const VkDescriptorSetAllocateInfo* pAllocateInfo, VkDescriptorSet* pDescriptorSets)
{
    return explicit_AllocateDescriptorSets(device, pAllocateInfo, pDescriptorSets);
}

// CODEGEN : file ../vk-layer-generate.py line #1310
VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkFreeDescriptorSets(VkDevice device, VkDescriptorPool descriptorPool, uint32_t descriptorSetCount, const VkDescriptorSet* pDescriptorSets)
{
    return explicit_FreeDescriptorSets(device, descriptorPool, descriptorSetCount, pDescriptorSets);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkUpdateDescriptorSets(VkDevice device, uint32_t descriptorWriteCount, const VkWriteDescriptorSet* pDescriptorWrites, uint32_t descriptorCopyCount, const VkCopyDescriptorSet* pDescriptorCopies)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'device': 'VkDevice', 'pDescriptorCopies[descriptorCopyCount]': {'srcSet': 'VkDescriptorSet', 'dstSet': 'VkDescriptorSet'}, 'pDescriptorWrites[descriptorWriteCount]': {'pImageInfo[descriptorCount]': {'imageView': 'VkImageView', 'sampler': 'VkSampler'}, 'pBufferInfo[descriptorCount]': {'buffer': 'VkBuffer'}, 'pTexelBufferView[descriptorCount]': 'VkBufferView', 'dstSet': 'VkDescriptorSet'}}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
    if (pDescriptorCopies) {
// CODEGEN : file ../vk-layer-generate.py line #1149
        for (uint32_t idx0=0; idx0<descriptorCopyCount; ++idx0) {
            if (pDescriptorCopies[idx0].dstSet) {
// CODEGEN : file ../vk-layer-generate.py line #1185
                skipCall |= validate_descriptor_set(device, pDescriptorCopies[idx0].dstSet, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT, false);
            }
            if (pDescriptorCopies[idx0].srcSet) {
// CODEGEN : file ../vk-layer-generate.py line #1185
                skipCall |= validate_descriptor_set(device, pDescriptorCopies[idx0].srcSet, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT, false);
            }
        }
    }
    if (pDescriptorWrites) {
// CODEGEN : file ../vk-layer-generate.py line #1149
        for (uint32_t idx1=0; idx1<descriptorWriteCount; ++idx1) {
            if (pDescriptorWrites[idx1].dstSet) {
// CODEGEN : file ../vk-layer-generate.py line #1185
                skipCall |= validate_descriptor_set(device, pDescriptorWrites[idx1].dstSet, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT, false);
            }
            if ((pDescriptorWrites[idx1].descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)         ||
                (pDescriptorWrites[idx1].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)         ||
                (pDescriptorWrites[idx1].descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC) ||
                (pDescriptorWrites[idx1].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC)   ) {
// CODEGEN : file ../vk-layer-generate.py line #1149
                for (uint32_t idx2=0; idx2<pDescriptorWrites[idx1].descriptorCount; ++idx2) {
                    if (pDescriptorWrites[idx1].pBufferInfo[idx2].buffer) {
// CODEGEN : file ../vk-layer-generate.py line #1185
                        skipCall |= validate_buffer(device, pDescriptorWrites[idx1].pBufferInfo[idx2].buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, false);
                    }
                }
            }
            if ((pDescriptorWrites[idx1].descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER)                ||
                (pDescriptorWrites[idx1].descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) ||
                (pDescriptorWrites[idx1].descriptorType == VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT)       ||
                (pDescriptorWrites[idx1].descriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)          ||
                (pDescriptorWrites[idx1].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)            ) {
// CODEGEN : file ../vk-layer-generate.py line #1149
                for (uint32_t idx3=0; idx3<pDescriptorWrites[idx1].descriptorCount; ++idx3) {
                    if (pDescriptorWrites[idx1].pImageInfo[idx3].imageView) {
// CODEGEN : file ../vk-layer-generate.py line #1185
                        skipCall |= validate_image_view(device, pDescriptorWrites[idx1].pImageInfo[idx3].imageView, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT, false);
                    }
                    if (pDescriptorWrites[idx1].pImageInfo[idx3].sampler) {
// CODEGEN : file ../vk-layer-generate.py line #1185
                        skipCall |= validate_sampler(device, pDescriptorWrites[idx1].pImageInfo[idx3].sampler, VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT, false);
                    }
                }
            }
            if ((pDescriptorWrites[idx1].descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER) ||
                (pDescriptorWrites[idx1].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER)   ) {
                for (uint32_t idx4=0; idx4<pDescriptorWrites[idx1].descriptorCount; ++idx4) {
// CODEGEN : file ../vk-layer-generate.py line #1185
                    skipCall |= validate_buffer_view(device, pDescriptorWrites[idx1].pTexelBufferView[idx4], VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_VIEW_EXT, true);
                }
            }
        }
    }
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    get_dispatch_table(object_tracker_device_table_map, device)->UpdateDescriptorSets(device, descriptorWriteCount, pDescriptorWrites, descriptorCopyCount, pDescriptorCopies);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateFramebuffer(VkDevice device, const VkFramebufferCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkFramebuffer* pFramebuffer)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'device': 'VkDevice', 'pCreateInfo': {'renderPass': 'VkRenderPass', 'pAttachments[attachmentCount]': 'VkImageView'}}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
    if (pCreateInfo) {
        if (pCreateInfo->pAttachments) {
            for (uint32_t idx0=0; idx0<pCreateInfo->attachmentCount; ++idx0) {
// CODEGEN : file ../vk-layer-generate.py line #1185
                skipCall |= validate_image_view(device, pCreateInfo->pAttachments[idx0], VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT, false);
            }
        }
        if (pCreateInfo->renderPass) {
// CODEGEN : file ../vk-layer-generate.py line #1185
            skipCall |= validate_render_pass(device, pCreateInfo->renderPass, VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT, false);
        }
    }
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = get_dispatch_table(object_tracker_device_table_map, device)->CreateFramebuffer(device, pCreateInfo, pAllocator, pFramebuffer);
    loader_platform_thread_lock_mutex(&objLock);
    if (result == VK_SUCCESS) {
        create_framebuffer(device, *pFramebuffer, VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT);
    }
    loader_platform_thread_unlock_mutex(&objLock);
    return result;
}

// CODEGEN : file ../vk-layer-generate.py line #1310


// CODEGEN : file ../vk-layer-generate.py line #1344


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyFramebuffer(VkDevice device, VkFramebuffer framebuffer, const VkAllocationCallbacks* pAllocator)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'device': 'VkDevice', 'framebuffer': 'VkFramebuffer'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_framebuffer(device, framebuffer, VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    loader_platform_thread_lock_mutex(&objLock);
    destroy_framebuffer(device, framebuffer);
    loader_platform_thread_unlock_mutex(&objLock);
    get_dispatch_table(object_tracker_device_table_map, device)->DestroyFramebuffer(device, framebuffer, pAllocator);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateRenderPass(VkDevice device, const VkRenderPassCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkRenderPass* pRenderPass)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'device': 'VkDevice'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = get_dispatch_table(object_tracker_device_table_map, device)->CreateRenderPass(device, pCreateInfo, pAllocator, pRenderPass);
    loader_platform_thread_lock_mutex(&objLock);
    if (result == VK_SUCCESS) {
        create_render_pass(device, *pRenderPass, VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT);
    }
    loader_platform_thread_unlock_mutex(&objLock);
    return result;
}

// CODEGEN : file ../vk-layer-generate.py line #1310


// CODEGEN : file ../vk-layer-generate.py line #1344


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyRenderPass(VkDevice device, VkRenderPass renderPass, const VkAllocationCallbacks* pAllocator)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'device': 'VkDevice', 'renderPass': 'VkRenderPass'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_render_pass(device, renderPass, VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    loader_platform_thread_lock_mutex(&objLock);
    destroy_render_pass(device, renderPass);
    loader_platform_thread_unlock_mutex(&objLock);
    get_dispatch_table(object_tracker_device_table_map, device)->DestroyRenderPass(device, renderPass, pAllocator);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetRenderAreaGranularity(VkDevice device, VkRenderPass renderPass, VkExtent2D* pGranularity)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'device': 'VkDevice', 'renderPass': 'VkRenderPass'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_render_pass(device, renderPass, VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    get_dispatch_table(object_tracker_device_table_map, device)->GetRenderAreaGranularity(device, renderPass, pGranularity);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateCommandPool(VkDevice device, const VkCommandPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkCommandPool* pCommandPool)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'device': 'VkDevice'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = get_dispatch_table(object_tracker_device_table_map, device)->CreateCommandPool(device, pCreateInfo, pAllocator, pCommandPool);
    loader_platform_thread_lock_mutex(&objLock);
    if (result == VK_SUCCESS) {
        create_command_pool(device, *pCommandPool, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_POOL_EXT);
    }
    loader_platform_thread_unlock_mutex(&objLock);
    return result;
}

// CODEGEN : file ../vk-layer-generate.py line #1310
VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyCommandPool(VkDevice device, VkCommandPool commandPool, const VkAllocationCallbacks* pAllocator)
{
    return explicit_DestroyCommandPool(device, commandPool, pAllocator);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkResetCommandPool(VkDevice device, VkCommandPool commandPool, VkCommandPoolResetFlags flags)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'device': 'VkDevice', 'commandPool': 'VkCommandPool'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_command_pool(device, commandPool, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_POOL_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = get_dispatch_table(object_tracker_device_table_map, device)->ResetCommandPool(device, commandPool, flags);
    return result;
}

// CODEGEN : file ../vk-layer-generate.py line #1310
VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkAllocateCommandBuffers(VkDevice device, const VkCommandBufferAllocateInfo* pAllocateInfo, VkCommandBuffer* pCommandBuffers)
{
    return explicit_AllocateCommandBuffers(device, pAllocateInfo, pCommandBuffers);
}

// CODEGEN : file ../vk-layer-generate.py line #1310
VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkFreeCommandBuffers(VkDevice device, VkCommandPool commandPool, uint32_t commandBufferCount, const VkCommandBuffer* pCommandBuffers)
{
    return explicit_FreeCommandBuffers(device, commandPool, commandBufferCount, pCommandBuffers);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkBeginCommandBuffer(VkCommandBuffer commandBuffer, const VkCommandBufferBeginInfo* pBeginInfo)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'commandBuffer': 'VkCommandBuffer', 'pBeginInfo': {'pInheritanceInfo': {'renderPass': 'VkRenderPass', 'framebuffer': 'VkFramebuffer'}}}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_command_buffer(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false);
    if (pBeginInfo) {
        OBJTRACK_NODE* pNode = VkCommandBufferMap[(uint64_t)commandBuffer];
        if ((pBeginInfo->pInheritanceInfo) && (pNode->status & OBJSTATUS_COMMAND_BUFFER_SECONDARY)) {
// CODEGEN : file ../vk-layer-generate.py line #1193
            skipCall |= validate_framebuffer(commandBuffer, pBeginInfo->pInheritanceInfo->framebuffer, VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT, true);
// CODEGEN : file ../vk-layer-generate.py line #1193
            skipCall |= validate_render_pass(commandBuffer, pBeginInfo->pInheritanceInfo->renderPass, VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT, true);
        }
    }
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = get_dispatch_table(object_tracker_device_table_map, commandBuffer)->BeginCommandBuffer(commandBuffer, pBeginInfo);
    return result;
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEndCommandBuffer(VkCommandBuffer commandBuffer)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'commandBuffer': 'VkCommandBuffer'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_command_buffer(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = get_dispatch_table(object_tracker_device_table_map, commandBuffer)->EndCommandBuffer(commandBuffer);
    return result;
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkResetCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferResetFlags flags)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'commandBuffer': 'VkCommandBuffer'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_command_buffer(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = get_dispatch_table(object_tracker_device_table_map, commandBuffer)->ResetCommandBuffer(commandBuffer, flags);
    return result;
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdBindPipeline(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'commandBuffer': 'VkCommandBuffer', 'pipeline': 'VkPipeline'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_command_buffer(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_pipeline(commandBuffer, pipeline, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    get_dispatch_table(object_tracker_device_table_map, commandBuffer)->CmdBindPipeline(commandBuffer, pipelineBindPoint, pipeline);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetViewport(VkCommandBuffer commandBuffer, uint32_t firstViewport, uint32_t viewportCount, const VkViewport* pViewports)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'commandBuffer': 'VkCommandBuffer'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_command_buffer(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    get_dispatch_table(object_tracker_device_table_map, commandBuffer)->CmdSetViewport(commandBuffer, firstViewport, viewportCount, pViewports);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetScissor(VkCommandBuffer commandBuffer, uint32_t firstScissor, uint32_t scissorCount, const VkRect2D* pScissors)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'commandBuffer': 'VkCommandBuffer'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_command_buffer(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    get_dispatch_table(object_tracker_device_table_map, commandBuffer)->CmdSetScissor(commandBuffer, firstScissor, scissorCount, pScissors);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetLineWidth(VkCommandBuffer commandBuffer, float lineWidth)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'commandBuffer': 'VkCommandBuffer'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_command_buffer(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    get_dispatch_table(object_tracker_device_table_map, commandBuffer)->CmdSetLineWidth(commandBuffer, lineWidth);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetDepthBias(VkCommandBuffer commandBuffer, float depthBiasConstantFactor, float depthBiasClamp, float depthBiasSlopeFactor)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'commandBuffer': 'VkCommandBuffer'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_command_buffer(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    get_dispatch_table(object_tracker_device_table_map, commandBuffer)->CmdSetDepthBias(commandBuffer, depthBiasConstantFactor, depthBiasClamp, depthBiasSlopeFactor);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetBlendConstants(VkCommandBuffer commandBuffer, const float blendConstants[4])
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'commandBuffer': 'VkCommandBuffer'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_command_buffer(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    get_dispatch_table(object_tracker_device_table_map, commandBuffer)->CmdSetBlendConstants(commandBuffer, blendConstants);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetDepthBounds(VkCommandBuffer commandBuffer, float minDepthBounds, float maxDepthBounds)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'commandBuffer': 'VkCommandBuffer'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_command_buffer(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    get_dispatch_table(object_tracker_device_table_map, commandBuffer)->CmdSetDepthBounds(commandBuffer, minDepthBounds, maxDepthBounds);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetStencilCompareMask(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t compareMask)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'commandBuffer': 'VkCommandBuffer'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_command_buffer(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    get_dispatch_table(object_tracker_device_table_map, commandBuffer)->CmdSetStencilCompareMask(commandBuffer, faceMask, compareMask);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetStencilWriteMask(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t writeMask)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'commandBuffer': 'VkCommandBuffer'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_command_buffer(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    get_dispatch_table(object_tracker_device_table_map, commandBuffer)->CmdSetStencilWriteMask(commandBuffer, faceMask, writeMask);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetStencilReference(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t reference)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'commandBuffer': 'VkCommandBuffer'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_command_buffer(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    get_dispatch_table(object_tracker_device_table_map, commandBuffer)->CmdSetStencilReference(commandBuffer, faceMask, reference);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdBindDescriptorSets(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t firstSet, uint32_t descriptorSetCount, const VkDescriptorSet* pDescriptorSets, uint32_t dynamicOffsetCount, const uint32_t* pDynamicOffsets)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'commandBuffer': 'VkCommandBuffer', 'pDescriptorSets[descriptorSetCount]': 'VkDescriptorSet', 'layout': 'VkPipelineLayout'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_command_buffer(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_pipeline_layout(commandBuffer, layout, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT, false);
    if (pDescriptorSets) {
        for (uint32_t idx0=0; idx0<descriptorSetCount; ++idx0) {
// CODEGEN : file ../vk-layer-generate.py line #1185
            skipCall |= validate_descriptor_set(commandBuffer, pDescriptorSets[idx0], VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT, false);
        }
    }
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    get_dispatch_table(object_tracker_device_table_map, commandBuffer)->CmdBindDescriptorSets(commandBuffer, pipelineBindPoint, layout, firstSet, descriptorSetCount, pDescriptorSets, dynamicOffsetCount, pDynamicOffsets);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdBindIndexBuffer(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'commandBuffer': 'VkCommandBuffer', 'buffer': 'VkBuffer'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_buffer(commandBuffer, buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_command_buffer(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    get_dispatch_table(object_tracker_device_table_map, commandBuffer)->CmdBindIndexBuffer(commandBuffer, buffer, offset, indexType);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdBindVertexBuffers(VkCommandBuffer commandBuffer, uint32_t firstBinding, uint32_t bindingCount, const VkBuffer* pBuffers, const VkDeviceSize* pOffsets)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'commandBuffer': 'VkCommandBuffer', 'pBuffers[bindingCount]': 'VkBuffer'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_command_buffer(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false);
    if (pBuffers) {
        for (uint32_t idx0=0; idx0<bindingCount; ++idx0) {
// CODEGEN : file ../vk-layer-generate.py line #1185
            skipCall |= validate_buffer(commandBuffer, pBuffers[idx0], VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, false);
        }
    }
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    get_dispatch_table(object_tracker_device_table_map, commandBuffer)->CmdBindVertexBuffers(commandBuffer, firstBinding, bindingCount, pBuffers, pOffsets);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdDraw(VkCommandBuffer commandBuffer, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'commandBuffer': 'VkCommandBuffer'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_command_buffer(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    get_dispatch_table(object_tracker_device_table_map, commandBuffer)->CmdDraw(commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdDrawIndexed(VkCommandBuffer commandBuffer, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'commandBuffer': 'VkCommandBuffer'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_command_buffer(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    get_dispatch_table(object_tracker_device_table_map, commandBuffer)->CmdDrawIndexed(commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdDrawIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'commandBuffer': 'VkCommandBuffer', 'buffer': 'VkBuffer'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_buffer(commandBuffer, buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_command_buffer(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    get_dispatch_table(object_tracker_device_table_map, commandBuffer)->CmdDrawIndirect(commandBuffer, buffer, offset, drawCount, stride);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdDrawIndexedIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'commandBuffer': 'VkCommandBuffer', 'buffer': 'VkBuffer'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_buffer(commandBuffer, buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_command_buffer(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    get_dispatch_table(object_tracker_device_table_map, commandBuffer)->CmdDrawIndexedIndirect(commandBuffer, buffer, offset, drawCount, stride);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdDispatch(VkCommandBuffer commandBuffer, uint32_t x, uint32_t y, uint32_t z)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'commandBuffer': 'VkCommandBuffer'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_command_buffer(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    get_dispatch_table(object_tracker_device_table_map, commandBuffer)->CmdDispatch(commandBuffer, x, y, z);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdDispatchIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'commandBuffer': 'VkCommandBuffer', 'buffer': 'VkBuffer'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_buffer(commandBuffer, buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_command_buffer(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    get_dispatch_table(object_tracker_device_table_map, commandBuffer)->CmdDispatchIndirect(commandBuffer, buffer, offset);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdCopyBuffer(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferCopy* pRegions)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'commandBuffer': 'VkCommandBuffer', 'srcBuffer': 'VkBuffer', 'dstBuffer': 'VkBuffer'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_command_buffer(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_buffer(commandBuffer, dstBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_buffer(commandBuffer, srcBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    get_dispatch_table(object_tracker_device_table_map, commandBuffer)->CmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, regionCount, pRegions);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdCopyImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageCopy* pRegions)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'commandBuffer': 'VkCommandBuffer', 'srcImage': 'VkImage', 'dstImage': 'VkImage'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_command_buffer(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_image(commandBuffer, dstImage, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_image(commandBuffer, srcImage, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    get_dispatch_table(object_tracker_device_table_map, commandBuffer)->CmdCopyImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdBlitImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageBlit* pRegions, VkFilter filter)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'commandBuffer': 'VkCommandBuffer', 'srcImage': 'VkImage', 'dstImage': 'VkImage'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_command_buffer(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_image(commandBuffer, dstImage, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_image(commandBuffer, srcImage, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    get_dispatch_table(object_tracker_device_table_map, commandBuffer)->CmdBlitImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions, filter);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdCopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkBufferImageCopy* pRegions)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'commandBuffer': 'VkCommandBuffer', 'srcBuffer': 'VkBuffer', 'dstImage': 'VkImage'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_command_buffer(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_image(commandBuffer, dstImage, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_buffer(commandBuffer, srcBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    get_dispatch_table(object_tracker_device_table_map, commandBuffer)->CmdCopyBufferToImage(commandBuffer, srcBuffer, dstImage, dstImageLayout, regionCount, pRegions);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdCopyImageToBuffer(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferImageCopy* pRegions)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'commandBuffer': 'VkCommandBuffer', 'srcImage': 'VkImage', 'dstBuffer': 'VkBuffer'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_command_buffer(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_buffer(commandBuffer, dstBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_image(commandBuffer, srcImage, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    get_dispatch_table(object_tracker_device_table_map, commandBuffer)->CmdCopyImageToBuffer(commandBuffer, srcImage, srcImageLayout, dstBuffer, regionCount, pRegions);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdUpdateBuffer(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize dataSize, const uint32_t* pData)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'commandBuffer': 'VkCommandBuffer', 'dstBuffer': 'VkBuffer'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_command_buffer(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_buffer(commandBuffer, dstBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    get_dispatch_table(object_tracker_device_table_map, commandBuffer)->CmdUpdateBuffer(commandBuffer, dstBuffer, dstOffset, dataSize, pData);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdFillBuffer(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize size, uint32_t data)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'commandBuffer': 'VkCommandBuffer', 'dstBuffer': 'VkBuffer'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_command_buffer(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_buffer(commandBuffer, dstBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    get_dispatch_table(object_tracker_device_table_map, commandBuffer)->CmdFillBuffer(commandBuffer, dstBuffer, dstOffset, size, data);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdClearColorImage(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout, const VkClearColorValue* pColor, uint32_t rangeCount, const VkImageSubresourceRange* pRanges)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'commandBuffer': 'VkCommandBuffer', 'image': 'VkImage'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_command_buffer(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_image(commandBuffer, image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    get_dispatch_table(object_tracker_device_table_map, commandBuffer)->CmdClearColorImage(commandBuffer, image, imageLayout, pColor, rangeCount, pRanges);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdClearDepthStencilImage(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout, const VkClearDepthStencilValue* pDepthStencil, uint32_t rangeCount, const VkImageSubresourceRange* pRanges)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'commandBuffer': 'VkCommandBuffer', 'image': 'VkImage'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_command_buffer(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_image(commandBuffer, image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    get_dispatch_table(object_tracker_device_table_map, commandBuffer)->CmdClearDepthStencilImage(commandBuffer, image, imageLayout, pDepthStencil, rangeCount, pRanges);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdClearAttachments(VkCommandBuffer commandBuffer, uint32_t attachmentCount, const VkClearAttachment* pAttachments, uint32_t rectCount, const VkClearRect* pRects)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'commandBuffer': 'VkCommandBuffer'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_command_buffer(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    get_dispatch_table(object_tracker_device_table_map, commandBuffer)->CmdClearAttachments(commandBuffer, attachmentCount, pAttachments, rectCount, pRects);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdResolveImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageResolve* pRegions)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'commandBuffer': 'VkCommandBuffer', 'srcImage': 'VkImage', 'dstImage': 'VkImage'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_command_buffer(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_image(commandBuffer, dstImage, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_image(commandBuffer, srcImage, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    get_dispatch_table(object_tracker_device_table_map, commandBuffer)->CmdResolveImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetEvent(VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags stageMask)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'commandBuffer': 'VkCommandBuffer', 'event': 'VkEvent'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_command_buffer(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_event(commandBuffer, event, VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    get_dispatch_table(object_tracker_device_table_map, commandBuffer)->CmdSetEvent(commandBuffer, event, stageMask);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdResetEvent(VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags stageMask)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'commandBuffer': 'VkCommandBuffer', 'event': 'VkEvent'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_command_buffer(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_event(commandBuffer, event, VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    get_dispatch_table(object_tracker_device_table_map, commandBuffer)->CmdResetEvent(commandBuffer, event, stageMask);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdWaitEvents(VkCommandBuffer commandBuffer, uint32_t eventCount, const VkEvent* pEvents, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, uint32_t memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers, uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier* pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier* pImageMemoryBarriers)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'commandBuffer': 'VkCommandBuffer', 'pImageMemoryBarriers[imageMemoryBarrierCount]': {'image': 'VkImage'}, 'pBufferMemoryBarriers[bufferMemoryBarrierCount]': {'buffer': 'VkBuffer'}, 'pEvents[eventCount]': 'VkEvent'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_command_buffer(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false);
    if (pBufferMemoryBarriers) {
// CODEGEN : file ../vk-layer-generate.py line #1149
        for (uint32_t idx0=0; idx0<bufferMemoryBarrierCount; ++idx0) {
            if (pBufferMemoryBarriers[idx0].buffer) {
// CODEGEN : file ../vk-layer-generate.py line #1185
                skipCall |= validate_buffer(commandBuffer, pBufferMemoryBarriers[idx0].buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, false);
            }
        }
    }
    if (pEvents) {
        for (uint32_t idx1=0; idx1<eventCount; ++idx1) {
// CODEGEN : file ../vk-layer-generate.py line #1185
            skipCall |= validate_event(commandBuffer, pEvents[idx1], VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT, false);
        }
    }
    if (pImageMemoryBarriers) {
// CODEGEN : file ../vk-layer-generate.py line #1149
        for (uint32_t idx2=0; idx2<imageMemoryBarrierCount; ++idx2) {
            if (pImageMemoryBarriers[idx2].image) {
// CODEGEN : file ../vk-layer-generate.py line #1185
                skipCall |= validate_image(commandBuffer, pImageMemoryBarriers[idx2].image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, false);
            }
        }
    }
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    get_dispatch_table(object_tracker_device_table_map, commandBuffer)->CmdWaitEvents(commandBuffer, eventCount, pEvents, srcStageMask, dstStageMask, memoryBarrierCount, pMemoryBarriers, bufferMemoryBarrierCount, pBufferMemoryBarriers, imageMemoryBarrierCount, pImageMemoryBarriers);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdPipelineBarrier(VkCommandBuffer commandBuffer, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags, uint32_t memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers, uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier* pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier* pImageMemoryBarriers)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'commandBuffer': 'VkCommandBuffer', 'pImageMemoryBarriers[imageMemoryBarrierCount]': {'image': 'VkImage'}, 'pBufferMemoryBarriers[bufferMemoryBarrierCount]': {'buffer': 'VkBuffer'}}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_command_buffer(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false);
    if (pBufferMemoryBarriers) {
// CODEGEN : file ../vk-layer-generate.py line #1149
        for (uint32_t idx0=0; idx0<bufferMemoryBarrierCount; ++idx0) {
            if (pBufferMemoryBarriers[idx0].buffer) {
// CODEGEN : file ../vk-layer-generate.py line #1185
                skipCall |= validate_buffer(commandBuffer, pBufferMemoryBarriers[idx0].buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, false);
            }
        }
    }
    if (pImageMemoryBarriers) {
// CODEGEN : file ../vk-layer-generate.py line #1149
        for (uint32_t idx1=0; idx1<imageMemoryBarrierCount; ++idx1) {
            if (pImageMemoryBarriers[idx1].image) {
// CODEGEN : file ../vk-layer-generate.py line #1185
                skipCall |= validate_image(commandBuffer, pImageMemoryBarriers[idx1].image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, false);
            }
        }
    }
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    get_dispatch_table(object_tracker_device_table_map, commandBuffer)->CmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask, dependencyFlags, memoryBarrierCount, pMemoryBarriers, bufferMemoryBarrierCount, pBufferMemoryBarriers, imageMemoryBarrierCount, pImageMemoryBarriers);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdBeginQuery(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t query, VkQueryControlFlags flags)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'commandBuffer': 'VkCommandBuffer', 'queryPool': 'VkQueryPool'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_command_buffer(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_query_pool(commandBuffer, queryPool, VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    get_dispatch_table(object_tracker_device_table_map, commandBuffer)->CmdBeginQuery(commandBuffer, queryPool, query, flags);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdEndQuery(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t query)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'commandBuffer': 'VkCommandBuffer', 'queryPool': 'VkQueryPool'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_command_buffer(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_query_pool(commandBuffer, queryPool, VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    get_dispatch_table(object_tracker_device_table_map, commandBuffer)->CmdEndQuery(commandBuffer, queryPool, query);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdResetQueryPool(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'commandBuffer': 'VkCommandBuffer', 'queryPool': 'VkQueryPool'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_command_buffer(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_query_pool(commandBuffer, queryPool, VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    get_dispatch_table(object_tracker_device_table_map, commandBuffer)->CmdResetQueryPool(commandBuffer, queryPool, firstQuery, queryCount);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdWriteTimestamp(VkCommandBuffer commandBuffer, VkPipelineStageFlagBits pipelineStage, VkQueryPool queryPool, uint32_t query)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'commandBuffer': 'VkCommandBuffer', 'queryPool': 'VkQueryPool'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_command_buffer(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_query_pool(commandBuffer, queryPool, VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    get_dispatch_table(object_tracker_device_table_map, commandBuffer)->CmdWriteTimestamp(commandBuffer, pipelineStage, queryPool, query);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdCopyQueryPoolResults(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize stride, VkQueryResultFlags flags)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'commandBuffer': 'VkCommandBuffer', 'dstBuffer': 'VkBuffer', 'queryPool': 'VkQueryPool'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_command_buffer(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_buffer(commandBuffer, dstBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_query_pool(commandBuffer, queryPool, VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    get_dispatch_table(object_tracker_device_table_map, commandBuffer)->CmdCopyQueryPoolResults(commandBuffer, queryPool, firstQuery, queryCount, dstBuffer, dstOffset, stride, flags);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdPushConstants(VkCommandBuffer commandBuffer, VkPipelineLayout layout, VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size, const void* pValues)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'commandBuffer': 'VkCommandBuffer', 'layout': 'VkPipelineLayout'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_command_buffer(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_pipeline_layout(commandBuffer, layout, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    get_dispatch_table(object_tracker_device_table_map, commandBuffer)->CmdPushConstants(commandBuffer, layout, stageFlags, offset, size, pValues);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdBeginRenderPass(VkCommandBuffer commandBuffer, const VkRenderPassBeginInfo* pRenderPassBegin, VkSubpassContents contents)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'commandBuffer': 'VkCommandBuffer', 'pRenderPassBegin': {'renderPass': 'VkRenderPass', 'framebuffer': 'VkFramebuffer'}}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_command_buffer(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false);
    if (pRenderPassBegin) {
// CODEGEN : file ../vk-layer-generate.py line #1193
        skipCall |= validate_framebuffer(commandBuffer, pRenderPassBegin->framebuffer, VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
        skipCall |= validate_render_pass(commandBuffer, pRenderPassBegin->renderPass, VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT, false);
    }
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    get_dispatch_table(object_tracker_device_table_map, commandBuffer)->CmdBeginRenderPass(commandBuffer, pRenderPassBegin, contents);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdNextSubpass(VkCommandBuffer commandBuffer, VkSubpassContents contents)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'commandBuffer': 'VkCommandBuffer'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_command_buffer(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    get_dispatch_table(object_tracker_device_table_map, commandBuffer)->CmdNextSubpass(commandBuffer, contents);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdEndRenderPass(VkCommandBuffer commandBuffer)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'commandBuffer': 'VkCommandBuffer'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_command_buffer(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    get_dispatch_table(object_tracker_device_table_map, commandBuffer)->CmdEndRenderPass(commandBuffer);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdExecuteCommands(VkCommandBuffer commandBuffer, uint32_t commandBufferCount, const VkCommandBuffer* pCommandBuffers)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'commandBuffer': 'VkCommandBuffer', 'pCommandBuffers[commandBufferCount]': 'VkCommandBuffer'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_command_buffer(commandBuffer, commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false);
    if (pCommandBuffers) {
        for (uint32_t idx0=0; idx0<commandBufferCount; ++idx0) {
// CODEGEN : file ../vk-layer-generate.py line #1185
            skipCall |= validate_command_buffer(commandBuffer, pCommandBuffers[idx0], VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, false);
        }
    }
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    get_dispatch_table(object_tracker_device_table_map, commandBuffer)->CmdExecuteCommands(commandBuffer, commandBufferCount, pCommandBuffers);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


// CODEGEN : file ../vk-layer-generate.py line #1344


VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroySurfaceKHR(VkInstance instance, VkSurfaceKHR surface, const VkAllocationCallbacks* pAllocator)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'instance': 'VkInstance', 'surface': 'VkSurfaceKHR'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_instance(instance, instance, VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_surface_khr(instance, surface, VK_DEBUG_REPORT_OBJECT_TYPE_SURFACE_KHR_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return;
    loader_platform_thread_lock_mutex(&objLock);
    destroy_surface_khr(instance, surface);
    loader_platform_thread_unlock_mutex(&objLock);
    get_dispatch_table(object_tracker_instance_table_map, instance)->DestroySurfaceKHR(instance, surface, pAllocator);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceSurfaceSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, VkSurfaceKHR surface, VkBool32* pSupported)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'surface': 'VkSurfaceKHR', 'physicalDevice': 'VkPhysicalDevice'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_physical_device(physicalDevice, physicalDevice, VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_surface_khr(physicalDevice, surface, VK_DEBUG_REPORT_OBJECT_TYPE_SURFACE_KHR_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = get_dispatch_table(object_tracker_instance_table_map, physicalDevice)->GetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queueFamilyIndex, surface, pSupported);
    return result;
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkSurfaceCapabilitiesKHR* pSurfaceCapabilities)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'surface': 'VkSurfaceKHR', 'physicalDevice': 'VkPhysicalDevice'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_physical_device(physicalDevice, physicalDevice, VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_surface_khr(physicalDevice, surface, VK_DEBUG_REPORT_OBJECT_TYPE_SURFACE_KHR_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = get_dispatch_table(object_tracker_instance_table_map, physicalDevice)->GetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, pSurfaceCapabilities);
    return result;
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceSurfaceFormatsKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t* pSurfaceFormatCount, VkSurfaceFormatKHR* pSurfaceFormats)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'surface': 'VkSurfaceKHR', 'physicalDevice': 'VkPhysicalDevice'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_physical_device(physicalDevice, physicalDevice, VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_surface_khr(physicalDevice, surface, VK_DEBUG_REPORT_OBJECT_TYPE_SURFACE_KHR_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = get_dispatch_table(object_tracker_instance_table_map, physicalDevice)->GetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, pSurfaceFormatCount, pSurfaceFormats);
    return result;
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceSurfacePresentModesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t* pPresentModeCount, VkPresentModeKHR* pPresentModes)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'surface': 'VkSurfaceKHR', 'physicalDevice': 'VkPhysicalDevice'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_physical_device(physicalDevice, physicalDevice, VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_surface_khr(physicalDevice, surface, VK_DEBUG_REPORT_OBJECT_TYPE_SURFACE_KHR_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = get_dispatch_table(object_tracker_instance_table_map, physicalDevice)->GetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, pPresentModeCount, pPresentModes);
    return result;
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateSwapchainKHR(VkDevice device, const VkSwapchainCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSwapchainKHR* pSwapchain)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'device': 'VkDevice', 'pCreateInfo': {'oldSwapchain': 'VkSwapchainKHR', 'surface': 'VkSurfaceKHR'}}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
    if (pCreateInfo) {
// CODEGEN : file ../vk-layer-generate.py line #1193
        skipCall |= validate_swapchain_khr(device, pCreateInfo->oldSwapchain, VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT, true);
// CODEGEN : file ../vk-layer-generate.py line #1193
        skipCall |= validate_surface_khr(device, pCreateInfo->surface, VK_DEBUG_REPORT_OBJECT_TYPE_SURFACE_KHR_EXT, false);
    }
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = get_dispatch_table(object_tracker_device_table_map, device)->CreateSwapchainKHR(device, pCreateInfo, pAllocator, pSwapchain);
    loader_platform_thread_lock_mutex(&objLock);
    if (result == VK_SUCCESS) {
        create_swapchain_khr(device, *pSwapchain, VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT);
    }
    loader_platform_thread_unlock_mutex(&objLock);
    return result;
}

// CODEGEN : file ../vk-layer-generate.py line #1310
VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroySwapchainKHR(VkDevice device, VkSwapchainKHR swapchain, const VkAllocationCallbacks* pAllocator)
{
    return explicit_DestroySwapchainKHR(device, swapchain, pAllocator);
}

// CODEGEN : file ../vk-layer-generate.py line #1310
VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkGetSwapchainImagesKHR(VkDevice device, VkSwapchainKHR swapchain, uint32_t* pSwapchainImageCount, VkImage* pSwapchainImages)
{
    return explicit_GetSwapchainImagesKHR(device, swapchain, pSwapchainImageCount, pSwapchainImages);
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkAcquireNextImageKHR(VkDevice device, VkSwapchainKHR swapchain, uint64_t timeout, VkSemaphore semaphore, VkFence fence, uint32_t* pImageIndex)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'device': 'VkDevice', 'swapchain': 'VkSwapchainKHR', 'fence': 'VkFence', 'semaphore': 'VkSemaphore'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_fence(device, fence, VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT, true);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_semaphore(device, semaphore, VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT, true);
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_swapchain_khr(device, swapchain, VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = get_dispatch_table(object_tracker_device_table_map, device)->AcquireNextImageKHR(device, swapchain, timeout, semaphore, fence, pImageIndex);
    return result;
}

// CODEGEN : file ../vk-layer-generate.py line #1310


VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkQueuePresentKHR(VkQueue queue, const VkPresentInfoKHR* pPresentInfo)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'queue': 'VkQueue', 'pPresentInfo': {'pWaitSemaphores[waitSemaphoreCount]': 'VkSemaphore', 'pSwapchains[swapchainCount]': 'VkSwapchainKHR'}}
    if (pPresentInfo) {
        if (pPresentInfo->pSwapchains) {
            for (uint32_t idx0=0; idx0<pPresentInfo->swapchainCount; ++idx0) {
// CODEGEN : file ../vk-layer-generate.py line #1185
                skipCall |= validate_swapchain_khr(queue, pPresentInfo->pSwapchains[idx0], VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT, false);
            }
        }
        if (pPresentInfo->pWaitSemaphores) {
            for (uint32_t idx1=0; idx1<pPresentInfo->waitSemaphoreCount; ++idx1) {
// CODEGEN : file ../vk-layer-generate.py line #1185
                skipCall |= validate_semaphore(queue, pPresentInfo->pWaitSemaphores[idx1], VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT, false);
            }
        }
    }
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_queue(queue, queue, VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = get_dispatch_table(object_tracker_device_table_map, queue)->QueuePresentKHR(queue, pPresentInfo);
    return result;
}

// CODEGEN : file ../vk-layer-generate.py line #1310


#ifdef VK_USE_PLATFORM_XCB_KHR

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateXcbSurfaceKHR(VkInstance instance, const VkXcbSurfaceCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'instance': 'VkInstance'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_instance(instance, instance, VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = get_dispatch_table(object_tracker_instance_table_map, instance)->CreateXcbSurfaceKHR(instance, pCreateInfo, pAllocator, pSurface);
    loader_platform_thread_lock_mutex(&objLock);
    if (result == VK_SUCCESS) {
        create_surface_khr(instance, *pSurface, VK_DEBUG_REPORT_OBJECT_TYPE_SURFACE_KHR_EXT);
    }
    loader_platform_thread_unlock_mutex(&objLock);
    return result;
}

#endif  // VK_USE_PLATFORM_XCB_KHR

// CODEGEN : file ../vk-layer-generate.py line #1310


#ifdef VK_USE_PLATFORM_XCB_KHR

VK_LAYER_EXPORT VKAPI_ATTR VkBool32 VKAPI_CALL vkGetPhysicalDeviceXcbPresentationSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, xcb_connection_t* connection, xcb_visualid_t visual_id)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'physicalDevice': 'VkPhysicalDevice'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_physical_device(physicalDevice, physicalDevice, VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    VkBool32 result = get_dispatch_table(object_tracker_instance_table_map, physicalDevice)->GetPhysicalDeviceXcbPresentationSupportKHR(physicalDevice, queueFamilyIndex, connection, visual_id);
    return result;
}

#endif  // VK_USE_PLATFORM_XCB_KHR

// CODEGEN : file ../vk-layer-generate.py line #1310


#ifdef VK_USE_PLATFORM_ANDROID_KHR

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateAndroidSurfaceKHR(VkInstance instance, const VkAndroidSurfaceCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
// objects to validate: {'instance': 'VkInstance'}
// CODEGEN : file ../vk-layer-generate.py line #1193
    skipCall |= validate_instance(instance, instance, VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = get_dispatch_table(object_tracker_instance_table_map, instance)->CreateAndroidSurfaceKHR(instance, pCreateInfo, pAllocator, pSurface);
    loader_platform_thread_lock_mutex(&objLock);
    if (result == VK_SUCCESS) {
        create_surface_khr(instance, *pSurface, VK_DEBUG_REPORT_OBJECT_TYPE_SURFACE_KHR_EXT);
    }
    loader_platform_thread_unlock_mutex(&objLock);
    return result;
}

#endif  // VK_USE_PLATFORM_ANDROID_KHR

// CODEGEN : file ../vk-layer-generate.py line #474
static inline PFN_vkVoidFunction layer_intercept_proc(const char *name)
{
    if (!name || name[0] != 'v' || name[1] != 'k')
        return NULL;

    name += 2;
    if (!strcmp(name, "CreateInstance"))
        return (PFN_vkVoidFunction) vkCreateInstance;
    if (!strcmp(name, "DestroyInstance"))
        return (PFN_vkVoidFunction) vkDestroyInstance;
    if (!strcmp(name, "EnumeratePhysicalDevices"))
        return (PFN_vkVoidFunction) vkEnumeratePhysicalDevices;
    if (!strcmp(name, "GetPhysicalDeviceFeatures"))
        return (PFN_vkVoidFunction) vkGetPhysicalDeviceFeatures;
    if (!strcmp(name, "GetPhysicalDeviceFormatProperties"))
        return (PFN_vkVoidFunction) vkGetPhysicalDeviceFormatProperties;
    if (!strcmp(name, "GetPhysicalDeviceImageFormatProperties"))
        return (PFN_vkVoidFunction) vkGetPhysicalDeviceImageFormatProperties;
    if (!strcmp(name, "GetPhysicalDeviceProperties"))
        return (PFN_vkVoidFunction) vkGetPhysicalDeviceProperties;
    if (!strcmp(name, "GetPhysicalDeviceQueueFamilyProperties"))
        return (PFN_vkVoidFunction) vkGetPhysicalDeviceQueueFamilyProperties;
    if (!strcmp(name, "GetPhysicalDeviceMemoryProperties"))
        return (PFN_vkVoidFunction) vkGetPhysicalDeviceMemoryProperties;
    if (!strcmp(name, "CreateDevice"))
        return (PFN_vkVoidFunction) vkCreateDevice;
    if (!strcmp(name, "DestroyDevice"))
        return (PFN_vkVoidFunction) vkDestroyDevice;
    if (!strcmp(name, "EnumerateInstanceExtensionProperties"))
        return (PFN_vkVoidFunction) vkEnumerateInstanceExtensionProperties;
    if (!strcmp(name, "EnumerateInstanceLayerProperties"))
        return (PFN_vkVoidFunction) vkEnumerateInstanceLayerProperties;
    if (!strcmp(name, "EnumerateDeviceLayerProperties"))
        return (PFN_vkVoidFunction) vkEnumerateDeviceLayerProperties;
    if (!strcmp(name, "GetDeviceQueue"))
        return (PFN_vkVoidFunction) vkGetDeviceQueue;
    if (!strcmp(name, "QueueSubmit"))
        return (PFN_vkVoidFunction) vkQueueSubmit;
    if (!strcmp(name, "QueueWaitIdle"))
        return (PFN_vkVoidFunction) vkQueueWaitIdle;
    if (!strcmp(name, "DeviceWaitIdle"))
        return (PFN_vkVoidFunction) vkDeviceWaitIdle;
    if (!strcmp(name, "AllocateMemory"))
        return (PFN_vkVoidFunction) vkAllocateMemory;
    if (!strcmp(name, "FreeMemory"))
        return (PFN_vkVoidFunction) vkFreeMemory;
    if (!strcmp(name, "MapMemory"))
        return (PFN_vkVoidFunction) vkMapMemory;
    if (!strcmp(name, "UnmapMemory"))
        return (PFN_vkVoidFunction) vkUnmapMemory;
    if (!strcmp(name, "FlushMappedMemoryRanges"))
        return (PFN_vkVoidFunction) vkFlushMappedMemoryRanges;
    if (!strcmp(name, "InvalidateMappedMemoryRanges"))
        return (PFN_vkVoidFunction) vkInvalidateMappedMemoryRanges;
    if (!strcmp(name, "GetDeviceMemoryCommitment"))
        return (PFN_vkVoidFunction) vkGetDeviceMemoryCommitment;
    if (!strcmp(name, "BindBufferMemory"))
        return (PFN_vkVoidFunction) vkBindBufferMemory;
    if (!strcmp(name, "BindImageMemory"))
        return (PFN_vkVoidFunction) vkBindImageMemory;
    if (!strcmp(name, "GetBufferMemoryRequirements"))
        return (PFN_vkVoidFunction) vkGetBufferMemoryRequirements;
    if (!strcmp(name, "GetImageMemoryRequirements"))
        return (PFN_vkVoidFunction) vkGetImageMemoryRequirements;
    if (!strcmp(name, "GetImageSparseMemoryRequirements"))
        return (PFN_vkVoidFunction) vkGetImageSparseMemoryRequirements;
    if (!strcmp(name, "GetPhysicalDeviceSparseImageFormatProperties"))
        return (PFN_vkVoidFunction) vkGetPhysicalDeviceSparseImageFormatProperties;
    if (!strcmp(name, "QueueBindSparse"))
        return (PFN_vkVoidFunction) vkQueueBindSparse;
    if (!strcmp(name, "CreateFence"))
        return (PFN_vkVoidFunction) vkCreateFence;
    if (!strcmp(name, "DestroyFence"))
        return (PFN_vkVoidFunction) vkDestroyFence;
    if (!strcmp(name, "ResetFences"))
        return (PFN_vkVoidFunction) vkResetFences;
    if (!strcmp(name, "GetFenceStatus"))
        return (PFN_vkVoidFunction) vkGetFenceStatus;
    if (!strcmp(name, "WaitForFences"))
        return (PFN_vkVoidFunction) vkWaitForFences;
    if (!strcmp(name, "CreateSemaphore"))
        return (PFN_vkVoidFunction) vkCreateSemaphore;
    if (!strcmp(name, "DestroySemaphore"))
        return (PFN_vkVoidFunction) vkDestroySemaphore;
    if (!strcmp(name, "CreateEvent"))
        return (PFN_vkVoidFunction) vkCreateEvent;
    if (!strcmp(name, "DestroyEvent"))
        return (PFN_vkVoidFunction) vkDestroyEvent;
    if (!strcmp(name, "GetEventStatus"))
        return (PFN_vkVoidFunction) vkGetEventStatus;
    if (!strcmp(name, "SetEvent"))
        return (PFN_vkVoidFunction) vkSetEvent;
    if (!strcmp(name, "ResetEvent"))
        return (PFN_vkVoidFunction) vkResetEvent;
    if (!strcmp(name, "CreateQueryPool"))
        return (PFN_vkVoidFunction) vkCreateQueryPool;
    if (!strcmp(name, "DestroyQueryPool"))
        return (PFN_vkVoidFunction) vkDestroyQueryPool;
    if (!strcmp(name, "GetQueryPoolResults"))
        return (PFN_vkVoidFunction) vkGetQueryPoolResults;
    if (!strcmp(name, "CreateBuffer"))
        return (PFN_vkVoidFunction) vkCreateBuffer;
    if (!strcmp(name, "DestroyBuffer"))
        return (PFN_vkVoidFunction) vkDestroyBuffer;
    if (!strcmp(name, "CreateBufferView"))
        return (PFN_vkVoidFunction) vkCreateBufferView;
    if (!strcmp(name, "DestroyBufferView"))
        return (PFN_vkVoidFunction) vkDestroyBufferView;
    if (!strcmp(name, "CreateImage"))
        return (PFN_vkVoidFunction) vkCreateImage;
    if (!strcmp(name, "DestroyImage"))
        return (PFN_vkVoidFunction) vkDestroyImage;
    if (!strcmp(name, "GetImageSubresourceLayout"))
        return (PFN_vkVoidFunction) vkGetImageSubresourceLayout;
    if (!strcmp(name, "CreateImageView"))
        return (PFN_vkVoidFunction) vkCreateImageView;
    if (!strcmp(name, "DestroyImageView"))
        return (PFN_vkVoidFunction) vkDestroyImageView;
    if (!strcmp(name, "CreateShaderModule"))
        return (PFN_vkVoidFunction) vkCreateShaderModule;
    if (!strcmp(name, "DestroyShaderModule"))
        return (PFN_vkVoidFunction) vkDestroyShaderModule;
    if (!strcmp(name, "CreatePipelineCache"))
        return (PFN_vkVoidFunction) vkCreatePipelineCache;
    if (!strcmp(name, "DestroyPipelineCache"))
        return (PFN_vkVoidFunction) vkDestroyPipelineCache;
    if (!strcmp(name, "GetPipelineCacheData"))
        return (PFN_vkVoidFunction) vkGetPipelineCacheData;
    if (!strcmp(name, "MergePipelineCaches"))
        return (PFN_vkVoidFunction) vkMergePipelineCaches;
    if (!strcmp(name, "CreateGraphicsPipelines"))
        return (PFN_vkVoidFunction) vkCreateGraphicsPipelines;
    if (!strcmp(name, "CreateComputePipelines"))
        return (PFN_vkVoidFunction) vkCreateComputePipelines;
    if (!strcmp(name, "DestroyPipeline"))
        return (PFN_vkVoidFunction) vkDestroyPipeline;
    if (!strcmp(name, "CreatePipelineLayout"))
        return (PFN_vkVoidFunction) vkCreatePipelineLayout;
    if (!strcmp(name, "DestroyPipelineLayout"))
        return (PFN_vkVoidFunction) vkDestroyPipelineLayout;
    if (!strcmp(name, "CreateSampler"))
        return (PFN_vkVoidFunction) vkCreateSampler;
    if (!strcmp(name, "DestroySampler"))
        return (PFN_vkVoidFunction) vkDestroySampler;
    if (!strcmp(name, "CreateDescriptorSetLayout"))
        return (PFN_vkVoidFunction) vkCreateDescriptorSetLayout;
    if (!strcmp(name, "DestroyDescriptorSetLayout"))
        return (PFN_vkVoidFunction) vkDestroyDescriptorSetLayout;
    if (!strcmp(name, "CreateDescriptorPool"))
        return (PFN_vkVoidFunction) vkCreateDescriptorPool;
    if (!strcmp(name, "DestroyDescriptorPool"))
        return (PFN_vkVoidFunction) vkDestroyDescriptorPool;
    if (!strcmp(name, "ResetDescriptorPool"))
        return (PFN_vkVoidFunction) vkResetDescriptorPool;
    if (!strcmp(name, "AllocateDescriptorSets"))
        return (PFN_vkVoidFunction) vkAllocateDescriptorSets;
    if (!strcmp(name, "FreeDescriptorSets"))
        return (PFN_vkVoidFunction) vkFreeDescriptorSets;
    if (!strcmp(name, "UpdateDescriptorSets"))
        return (PFN_vkVoidFunction) vkUpdateDescriptorSets;
    if (!strcmp(name, "CreateFramebuffer"))
        return (PFN_vkVoidFunction) vkCreateFramebuffer;
    if (!strcmp(name, "DestroyFramebuffer"))
        return (PFN_vkVoidFunction) vkDestroyFramebuffer;
    if (!strcmp(name, "CreateRenderPass"))
        return (PFN_vkVoidFunction) vkCreateRenderPass;
    if (!strcmp(name, "DestroyRenderPass"))
        return (PFN_vkVoidFunction) vkDestroyRenderPass;
    if (!strcmp(name, "GetRenderAreaGranularity"))
        return (PFN_vkVoidFunction) vkGetRenderAreaGranularity;
    if (!strcmp(name, "CreateCommandPool"))
        return (PFN_vkVoidFunction) vkCreateCommandPool;
    if (!strcmp(name, "DestroyCommandPool"))
        return (PFN_vkVoidFunction) vkDestroyCommandPool;
    if (!strcmp(name, "ResetCommandPool"))
        return (PFN_vkVoidFunction) vkResetCommandPool;
    if (!strcmp(name, "AllocateCommandBuffers"))
        return (PFN_vkVoidFunction) vkAllocateCommandBuffers;
    if (!strcmp(name, "FreeCommandBuffers"))
        return (PFN_vkVoidFunction) vkFreeCommandBuffers;
    if (!strcmp(name, "BeginCommandBuffer"))
        return (PFN_vkVoidFunction) vkBeginCommandBuffer;
    if (!strcmp(name, "EndCommandBuffer"))
        return (PFN_vkVoidFunction) vkEndCommandBuffer;
    if (!strcmp(name, "ResetCommandBuffer"))
        return (PFN_vkVoidFunction) vkResetCommandBuffer;
    if (!strcmp(name, "CmdBindPipeline"))
        return (PFN_vkVoidFunction) vkCmdBindPipeline;
    if (!strcmp(name, "CmdSetViewport"))
        return (PFN_vkVoidFunction) vkCmdSetViewport;
    if (!strcmp(name, "CmdSetScissor"))
        return (PFN_vkVoidFunction) vkCmdSetScissor;
    if (!strcmp(name, "CmdSetLineWidth"))
        return (PFN_vkVoidFunction) vkCmdSetLineWidth;
    if (!strcmp(name, "CmdSetDepthBias"))
        return (PFN_vkVoidFunction) vkCmdSetDepthBias;
    if (!strcmp(name, "CmdSetBlendConstants"))
        return (PFN_vkVoidFunction) vkCmdSetBlendConstants;
    if (!strcmp(name, "CmdSetDepthBounds"))
        return (PFN_vkVoidFunction) vkCmdSetDepthBounds;
    if (!strcmp(name, "CmdSetStencilCompareMask"))
        return (PFN_vkVoidFunction) vkCmdSetStencilCompareMask;
    if (!strcmp(name, "CmdSetStencilWriteMask"))
        return (PFN_vkVoidFunction) vkCmdSetStencilWriteMask;
    if (!strcmp(name, "CmdSetStencilReference"))
        return (PFN_vkVoidFunction) vkCmdSetStencilReference;
    if (!strcmp(name, "CmdBindDescriptorSets"))
        return (PFN_vkVoidFunction) vkCmdBindDescriptorSets;
    if (!strcmp(name, "CmdBindIndexBuffer"))
        return (PFN_vkVoidFunction) vkCmdBindIndexBuffer;
    if (!strcmp(name, "CmdBindVertexBuffers"))
        return (PFN_vkVoidFunction) vkCmdBindVertexBuffers;
    if (!strcmp(name, "CmdDraw"))
        return (PFN_vkVoidFunction) vkCmdDraw;
    if (!strcmp(name, "CmdDrawIndexed"))
        return (PFN_vkVoidFunction) vkCmdDrawIndexed;
    if (!strcmp(name, "CmdDrawIndirect"))
        return (PFN_vkVoidFunction) vkCmdDrawIndirect;
    if (!strcmp(name, "CmdDrawIndexedIndirect"))
        return (PFN_vkVoidFunction) vkCmdDrawIndexedIndirect;
    if (!strcmp(name, "CmdDispatch"))
        return (PFN_vkVoidFunction) vkCmdDispatch;
    if (!strcmp(name, "CmdDispatchIndirect"))
        return (PFN_vkVoidFunction) vkCmdDispatchIndirect;
    if (!strcmp(name, "CmdCopyBuffer"))
        return (PFN_vkVoidFunction) vkCmdCopyBuffer;
    if (!strcmp(name, "CmdCopyImage"))
        return (PFN_vkVoidFunction) vkCmdCopyImage;
    if (!strcmp(name, "CmdBlitImage"))
        return (PFN_vkVoidFunction) vkCmdBlitImage;
    if (!strcmp(name, "CmdCopyBufferToImage"))
        return (PFN_vkVoidFunction) vkCmdCopyBufferToImage;
    if (!strcmp(name, "CmdCopyImageToBuffer"))
        return (PFN_vkVoidFunction) vkCmdCopyImageToBuffer;
    if (!strcmp(name, "CmdUpdateBuffer"))
        return (PFN_vkVoidFunction) vkCmdUpdateBuffer;
    if (!strcmp(name, "CmdFillBuffer"))
        return (PFN_vkVoidFunction) vkCmdFillBuffer;
    if (!strcmp(name, "CmdClearColorImage"))
        return (PFN_vkVoidFunction) vkCmdClearColorImage;
    if (!strcmp(name, "CmdClearDepthStencilImage"))
        return (PFN_vkVoidFunction) vkCmdClearDepthStencilImage;
    if (!strcmp(name, "CmdClearAttachments"))
        return (PFN_vkVoidFunction) vkCmdClearAttachments;
    if (!strcmp(name, "CmdResolveImage"))
        return (PFN_vkVoidFunction) vkCmdResolveImage;
    if (!strcmp(name, "CmdSetEvent"))
        return (PFN_vkVoidFunction) vkCmdSetEvent;
    if (!strcmp(name, "CmdResetEvent"))
        return (PFN_vkVoidFunction) vkCmdResetEvent;
    if (!strcmp(name, "CmdWaitEvents"))
        return (PFN_vkVoidFunction) vkCmdWaitEvents;
    if (!strcmp(name, "CmdPipelineBarrier"))
        return (PFN_vkVoidFunction) vkCmdPipelineBarrier;
    if (!strcmp(name, "CmdBeginQuery"))
        return (PFN_vkVoidFunction) vkCmdBeginQuery;
    if (!strcmp(name, "CmdEndQuery"))
        return (PFN_vkVoidFunction) vkCmdEndQuery;
    if (!strcmp(name, "CmdResetQueryPool"))
        return (PFN_vkVoidFunction) vkCmdResetQueryPool;
    if (!strcmp(name, "CmdWriteTimestamp"))
        return (PFN_vkVoidFunction) vkCmdWriteTimestamp;
    if (!strcmp(name, "CmdCopyQueryPoolResults"))
        return (PFN_vkVoidFunction) vkCmdCopyQueryPoolResults;
    if (!strcmp(name, "CmdPushConstants"))
        return (PFN_vkVoidFunction) vkCmdPushConstants;
    if (!strcmp(name, "CmdBeginRenderPass"))
        return (PFN_vkVoidFunction) vkCmdBeginRenderPass;
    if (!strcmp(name, "CmdNextSubpass"))
        return (PFN_vkVoidFunction) vkCmdNextSubpass;
    if (!strcmp(name, "CmdEndRenderPass"))
        return (PFN_vkVoidFunction) vkCmdEndRenderPass;
    if (!strcmp(name, "CmdExecuteCommands"))
        return (PFN_vkVoidFunction) vkCmdExecuteCommands;

    return NULL;
}
static inline PFN_vkVoidFunction layer_intercept_instance_proc(const char *name)
{
    if (!name || name[0] != 'v' || name[1] != 'k')
        return NULL;

    name += 2;
    if (!strcmp(name, "DestroyInstance"))
        return (PFN_vkVoidFunction) vkDestroyInstance;
    if (!strcmp(name, "EnumeratePhysicalDevices"))
        return (PFN_vkVoidFunction) vkEnumeratePhysicalDevices;
    if (!strcmp(name, "GetPhysicalDeviceFeatures"))
        return (PFN_vkVoidFunction) vkGetPhysicalDeviceFeatures;
    if (!strcmp(name, "GetPhysicalDeviceFormatProperties"))
        return (PFN_vkVoidFunction) vkGetPhysicalDeviceFormatProperties;
    if (!strcmp(name, "GetPhysicalDeviceImageFormatProperties"))
        return (PFN_vkVoidFunction) vkGetPhysicalDeviceImageFormatProperties;
    if (!strcmp(name, "GetPhysicalDeviceProperties"))
        return (PFN_vkVoidFunction) vkGetPhysicalDeviceProperties;
    if (!strcmp(name, "GetPhysicalDeviceQueueFamilyProperties"))
        return (PFN_vkVoidFunction) vkGetPhysicalDeviceQueueFamilyProperties;
    if (!strcmp(name, "GetPhysicalDeviceMemoryProperties"))
        return (PFN_vkVoidFunction) vkGetPhysicalDeviceMemoryProperties;
    if (!strcmp(name, "EnumerateInstanceExtensionProperties"))
        return (PFN_vkVoidFunction) vkEnumerateInstanceExtensionProperties;
    if (!strcmp(name, "EnumerateInstanceLayerProperties"))
        return (PFN_vkVoidFunction) vkEnumerateInstanceLayerProperties;
    if (!strcmp(name, "EnumerateDeviceLayerProperties"))
        return (PFN_vkVoidFunction) vkEnumerateDeviceLayerProperties;
    if (!strcmp(name, "GetPhysicalDeviceSparseImageFormatProperties"))
        return (PFN_vkVoidFunction) vkGetPhysicalDeviceSparseImageFormatProperties;

    return NULL;
}

// CODEGEN : file ../vk-layer-generate.py line #514
// CODEGEN : file ../vk-layer-generate.py line #299
VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugReportCallbackEXT(
        VkInstance                                   instance,
        const VkDebugReportCallbackCreateInfoEXT*    pCreateInfo,
        const VkAllocationCallbacks*                 pAllocator,
        VkDebugReportCallbackEXT*                    pCallback)
{
    VkLayerInstanceDispatchTable *pInstanceTable = get_dispatch_table(object_tracker_instance_table_map, instance);
    VkResult result = pInstanceTable->CreateDebugReportCallbackEXT(instance, pCreateInfo, pAllocator, pCallback);
    if (VK_SUCCESS == result) {
        layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
        result = layer_create_msg_callback(my_data->report_data,
                                           pCreateInfo,
                                           pAllocator,
                                           pCallback);
    }
    return result;
}
// CODEGEN : file ../vk-layer-generate.py line #330
VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT msgCallback, const VkAllocationCallbacks *pAllocator)
{
    VkLayerInstanceDispatchTable *pInstanceTable = get_dispatch_table(object_tracker_instance_table_map, instance);
    pInstanceTable->DestroyDebugReportCallbackEXT(instance, msgCallback, pAllocator);
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    layer_destroy_msg_callback(my_data->report_data, msgCallback, pAllocator);
}
// CODEGEN : file ../vk-layer-generate.py line #346
VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDebugReportMessageEXT(VkInstance instance, VkDebugReportFlagsEXT    flags, VkDebugReportObjectTypeEXT objType, uint64_t object, size_t location, int32_t msgCode, const char *pLayerPrefix, const char *pMsg)
{
    VkLayerInstanceDispatchTable *pInstanceTable = get_dispatch_table(object_tracker_instance_table_map, instance);
    pInstanceTable->DebugReportMessageEXT(instance, flags, objType, object, location, msgCode, pLayerPrefix, pMsg);
}

VK_LAYER_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr(VkDevice device, const char* funcName)
{
    PFN_vkVoidFunction addr;
    if (!strcmp("vkGetDeviceProcAddr", funcName)) {
        return (PFN_vkVoidFunction) vkGetDeviceProcAddr;
    }

    addr = layer_intercept_proc(funcName);
    if (addr)
        return addr;
    if (device == VK_NULL_HANDLE) {
        return NULL;
    }

// CODEGEN : file ../vk-layer-generate.py line #539
    layer_data *my_device_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    if (my_device_data->wsi_enabled) {
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


    if (get_dispatch_table(object_tracker_device_table_map, device)->GetDeviceProcAddr == NULL)
        return NULL;
    return get_dispatch_table(object_tracker_device_table_map, device)->GetDeviceProcAddr(device, funcName);
}

VK_LAYER_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance instance, const char* funcName)
{
    PFN_vkVoidFunction addr;
    if (!strcmp(funcName, "vkGetInstanceProcAddr"))
        return (PFN_vkVoidFunction) vkGetInstanceProcAddr;
    if (!strcmp(funcName, "vkCreateInstance"))
        return (PFN_vkVoidFunction) vkCreateInstance;
    if (!strcmp(funcName, "vkCreateDevice"))
        return (PFN_vkVoidFunction) vkCreateDevice;
    addr = layer_intercept_instance_proc(funcName);
    if (addr) {
        return addr;    }
    if (instance == VK_NULL_HANDLE) {
        return NULL;
    }

    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    addr = debug_report_get_instance_proc_addr(my_data->report_data, funcName);
    if (addr) {
        return addr;
    }

    VkLayerInstanceDispatchTable* pTable = get_dispatch_table(object_tracker_instance_table_map, instance);
    if (instanceExtMap.size() != 0 && instanceExtMap[pTable].wsi_enabled)
    {
        if (!strcmp("vkGetPhysicalDeviceSurfaceSupportKHR", funcName))
            return reinterpret_cast<PFN_vkVoidFunction>(vkGetPhysicalDeviceSurfaceSupportKHR);
        if (!strcmp("vkGetPhysicalDeviceSurfaceCapabilitiesKHR", funcName))
            return reinterpret_cast<PFN_vkVoidFunction>(vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
        if (!strcmp("vkGetPhysicalDeviceSurfaceFormatsKHR", funcName))
            return reinterpret_cast<PFN_vkVoidFunction>(vkGetPhysicalDeviceSurfaceFormatsKHR);
        if (!strcmp("vkGetPhysicalDeviceSurfacePresentModesKHR", funcName))
            return reinterpret_cast<PFN_vkVoidFunction>(vkGetPhysicalDeviceSurfacePresentModesKHR);
#ifdef VK_USE_PLATFORM_XCB_KHR
        if (!strcmp("vkCreateXcbSurfaceKHR", funcName))
            return reinterpret_cast<PFN_vkVoidFunction>(vkCreateXcbSurfaceKHR);
#endif  // VK_USE_PLATFORM_XCB_KHR
#ifdef VK_USE_PLATFORM_ANDROID_KHR
        if (!strcmp("vkCreateAndroidSurfaceKHR", funcName))
            return reinterpret_cast<PFN_vkVoidFunction>(vkCreateAndroidSurfaceKHR);
#endif  // VK_USE_PLATFORM_ANDROID_KHR
#ifdef VK_USE_PLATFORM_XCB_KHR
        if (!strcmp("vkGetPhysicalDeviceXcbPresentationSupportKHR", funcName))
            return reinterpret_cast<PFN_vkVoidFunction>(vkGetPhysicalDeviceXcbPresentationSupportKHR);
#endif  // VK_USE_PLATFORM_XCB_KHR
    }

    if (get_dispatch_table(object_tracker_instance_table_map, instance)->GetInstanceProcAddr == NULL) {
        return NULL;
    }
    return get_dispatch_table(object_tracker_instance_table_map, instance)->GetInstanceProcAddr(instance, funcName);
}

