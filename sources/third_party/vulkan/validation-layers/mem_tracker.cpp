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
 * Author: Cody Northrop <cody@lunarg.com>
 * Author: Jon Ashburn <jon@lunarg.com>
 * Author: Mark Lobodzinski <mark@lunarg.com>
 * Author: Tobin Ehlis <tobin@lunarg.com>
 */

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <functional>
#include <list>
#include <map>
#include <vector>
using namespace std;

#include "vk_loader_platform.h"
#include "vk_dispatch_table_helper.h"
#include "vk_struct_string_helper_cpp.h"
#include "mem_tracker.h"
#include "vk_layer_config.h"
#include "vk_layer_extension_utils.h"
#include "vk_layer_table.h"
#include "vk_layer_data.h"
#include "vk_layer_logging.h"

// WSI Image Objects bypass usual Image Object creation methods.  A special Memory
// Object value will be used to identify them internally.
static const VkDeviceMemory MEMTRACKER_SWAP_CHAIN_IMAGE_KEY = (VkDeviceMemory)(-1);

struct layer_data {
    debug_report_data                 *report_data;
    std::vector<VkDebugReportCallbackEXT>      logging_callback;
    VkLayerDispatchTable              *device_dispatch_table;
    VkLayerInstanceDispatchTable      *instance_dispatch_table;
    VkBool32                           wsi_enabled;
    uint64_t                           currentFenceId;
    VkPhysicalDeviceProperties         properties;
    unordered_map<VkDeviceMemory, vector<MEMORY_RANGE>>          bufferRanges, imageRanges;
    // Maps for tracking key structs related to MemTracker state
    unordered_map<VkCommandBuffer,     MT_CB_INFO>               cbMap;
    unordered_map<VkCommandPool,       MT_CMD_POOL_INFO>         commandPoolMap;
    unordered_map<VkDeviceMemory,      MT_MEM_OBJ_INFO>          memObjMap;
    unordered_map<VkFence,             MT_FENCE_INFO>            fenceMap;
    unordered_map<VkQueue,             MT_QUEUE_INFO>            queueMap;
    unordered_map<VkSwapchainKHR,      MT_SWAP_CHAIN_INFO*>      swapchainMap;
    unordered_map<VkSemaphore,         MtSemaphoreState>         semaphoreMap;
    unordered_map<VkFramebuffer,       MT_FB_INFO>               fbMap;
    unordered_map<VkRenderPass,        MT_PASS_INFO>             passMap;
    unordered_map<VkImageView,         MT_IMAGE_VIEW_INFO>       imageViewMap;
    unordered_map<VkDescriptorSet,     MT_DESCRIPTOR_SET_INFO>   descriptorSetMap;
    // Images and Buffers are 2 objects that can have memory bound to them so they get special treatment
    unordered_map<uint64_t,            MT_OBJ_BINDING_INFO>      imageMap;
    unordered_map<uint64_t,            MT_OBJ_BINDING_INFO>      bufferMap;
    unordered_map<VkBufferView, VkBufferViewCreateInfo> bufferViewMap;

    layer_data() :
        report_data(nullptr),
        device_dispatch_table(nullptr),
        instance_dispatch_table(nullptr),
        wsi_enabled(VK_FALSE),
        currentFenceId(1)
    {};
};

static unordered_map<void *, layer_data *> layer_data_map;

static VkPhysicalDeviceMemoryProperties memProps;

static VkBool32 clear_cmd_buf_and_mem_references(layer_data* my_data, const VkCommandBuffer cb);

// TODO : This can be much smarter, using separate locks for separate global data
static int globalLockInitialized = 0;
static loader_platform_thread_mutex globalLock;

#define MAX_BINDING 0xFFFFFFFF

static MT_OBJ_BINDING_INFO*
 get_object_binding_info(
    layer_data      *my_data,
    uint64_t         handle,
    VkDebugReportObjectTypeEXT  type)
{
    MT_OBJ_BINDING_INFO* retValue = NULL;
    switch (type)
    {
        case VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT:
        {
            auto it = my_data->imageMap.find(handle);
            if (it != my_data->imageMap.end())
                return &(*it).second;
            break;
        }
        case VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT:
        {
            auto it = my_data->bufferMap.find(handle);
            if (it != my_data->bufferMap.end())
                return &(*it).second;
            break;
        }
        default:
            break;
    }
    return retValue;
}

template layer_data *get_my_data_ptr<layer_data>(
        void *data_key,
        std::unordered_map<void *, layer_data *> &data_map);

// Add new queue for this device to map container
static void
add_queue_info(
    layer_data    *my_data,
    const VkQueue  queue)
{
    MT_QUEUE_INFO* pInfo   = &my_data->queueMap[queue];
    pInfo->lastRetiredId   = 0;
    pInfo->lastSubmittedId = 0;
}

static void
delete_queue_info_list(
    layer_data* my_data)
{
    // Process queue list, cleaning up each entry before deleting
    my_data->queueMap.clear();
}

static void
add_swap_chain_info(
    layer_data                     *my_data,
    const VkSwapchainKHR            swapchain,
    const VkSwapchainCreateInfoKHR *pCI)
{
    MT_SWAP_CHAIN_INFO* pInfo = new MT_SWAP_CHAIN_INFO;
    memcpy(&pInfo->createInfo, pCI, sizeof(VkSwapchainCreateInfoKHR));
    my_data->swapchainMap[swapchain] = pInfo;
}

// Add new CBInfo for this cb to map container
static void
add_cmd_buf_info(
    layer_data            *my_data,
    VkCommandPool          commandPool,
    const VkCommandBuffer  cb)
{
    my_data->cbMap[cb].commandBuffer = cb;
    my_data->commandPoolMap[commandPool].pCommandBuffers.push_front(cb);
}

// Delete CBInfo from container and clear mem references to CB
static VkBool32
delete_cmd_buf_info(
    layer_data            *my_data,
    VkCommandPool          commandPool,
    const VkCommandBuffer  cb)
{
    VkBool32 result = VK_TRUE;
    result = clear_cmd_buf_and_mem_references(my_data, cb);
    // Delete the CBInfo info
    if (result != VK_TRUE) {
        my_data->commandPoolMap[commandPool].pCommandBuffers.remove(cb);
        my_data->cbMap.erase(cb);
    }
    return result;
}

// Return ptr to Info in CB map, or NULL if not found
static MT_CB_INFO*
get_cmd_buf_info(
    layer_data            *my_data,
    const VkCommandBuffer  cb)
{
    auto item = my_data->cbMap.find(cb);
    if (item != my_data->cbMap.end()) {
        return &(*item).second;
    } else {
        return NULL;
    }
}

static void
add_object_binding_info(
    layer_data            *my_data,
    const uint64_t         handle,
    const VkDebugReportObjectTypeEXT  type,
    const VkDeviceMemory   mem)
{
    switch (type)
    {
        // Buffers and images are unique as their CreateInfo is in container struct
        case VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT:
        {
            auto pCI = &my_data->bufferMap[handle];
            pCI->mem = mem;
            break;
        }
        case VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT:
        {
            auto pCI = &my_data->imageMap[handle];
            pCI->mem = mem;
            break;
        }
        default:
            break;
    }
}

static void
add_object_create_info(
    layer_data     *my_data,
    const uint64_t  handle,
    const           VkDebugReportObjectTypeEXT type,
    const void     *pCreateInfo)
{
    // TODO : For any CreateInfo struct that has ptrs, need to deep copy them and appropriately clean up on Destroy
    switch (type)
    {
        // Buffers and images are unique as their CreateInfo is in container struct
        case VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT:
        {
            auto pCI = &my_data->bufferMap[handle];
            memset(pCI, 0, sizeof(MT_OBJ_BINDING_INFO));
            memcpy(&pCI->create_info.buffer, pCreateInfo, sizeof(VkBufferCreateInfo));
            break;
        }
        case VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT:
        {
            auto pCI = &my_data->imageMap[handle];
            memset(pCI, 0, sizeof(MT_OBJ_BINDING_INFO));
            memcpy(&pCI->create_info.image, pCreateInfo, sizeof(VkImageCreateInfo));
            break;
        }
        // Swap Chain is very unique, use my_data->imageMap, but copy in
        // SwapChainCreatInfo's usage flags and set the mem value to a unique key. These is used by
        // vkCreateImageView and internal MemTracker routines to distinguish swap chain images
        case VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT:
        {
            auto pCI = &my_data->imageMap[handle];
            memset(pCI, 0, sizeof(MT_OBJ_BINDING_INFO));
            pCI->mem = MEMTRACKER_SWAP_CHAIN_IMAGE_KEY;
            pCI->valid = false;
            pCI->create_info.image.usage =
                const_cast<VkSwapchainCreateInfoKHR*>(static_cast<const VkSwapchainCreateInfoKHR *>(pCreateInfo))->imageUsage;
            break;
        }
        default:
            break;
    }
}

// Add a fence, creating one if necessary to our list of fences/fenceIds
static VkBool32
add_fence_info(
    layer_data *my_data,
    VkFence     fence,
    VkQueue     queue,
    uint64_t   *fenceId)
{
    VkBool32 skipCall = VK_FALSE;
    *fenceId = my_data->currentFenceId++;

    // If no fence, create an internal fence to track the submissions
    if (fence != VK_NULL_HANDLE) {
        my_data->fenceMap[fence].fenceId = *fenceId;
        my_data->fenceMap[fence].queue   = queue;
        // Validate that fence is in UNSIGNALED state
        VkFenceCreateInfo* pFenceCI = &(my_data->fenceMap[fence].createInfo);
        if (pFenceCI->flags & VK_FENCE_CREATE_SIGNALED_BIT) {
            skipCall = log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT, (uint64_t) fence, __LINE__, MEMTRACK_INVALID_FENCE_STATE, "MEM",
                           "Fence %#" PRIxLEAST64 " submitted in SIGNALED state.  Fences must be reset before being submitted", (uint64_t) fence);
        }
    } else {
        // TODO : Do we need to create an internal fence here for tracking purposes?
    }
    // Update most recently submitted fence and fenceId for Queue
    my_data->queueMap[queue].lastSubmittedId = *fenceId;
    return skipCall;
}

// Remove a fenceInfo from our list of fences/fenceIds
static void
delete_fence_info(
    layer_data *my_data,
    VkFence     fence)
{
    my_data->fenceMap.erase(fence);
}

// Record information when a fence is known to be signalled
static void
update_fence_tracking(
    layer_data *my_data,
    VkFence     fence)
{
    auto fence_item = my_data->fenceMap.find(fence);
    if (fence_item != my_data->fenceMap.end()) {
        MT_FENCE_INFO *pCurFenceInfo = &(*fence_item).second;
        VkQueue queue = pCurFenceInfo->queue;
        auto queue_item = my_data->queueMap.find(queue);
        if (queue_item != my_data->queueMap.end()) {
            MT_QUEUE_INFO *pQueueInfo = &(*queue_item).second;
            if (pQueueInfo->lastRetiredId < pCurFenceInfo->fenceId) {
                pQueueInfo->lastRetiredId = pCurFenceInfo->fenceId;
            }
        }
    }

    // Update fence state in fenceCreateInfo structure
    auto pFCI = &(my_data->fenceMap[fence].createInfo);
    pFCI->flags = static_cast<VkFenceCreateFlags>(pFCI->flags | VK_FENCE_CREATE_SIGNALED_BIT);
}

// Helper routine that updates the fence list for a specific queue to all-retired
static void
retire_queue_fences(
    layer_data *my_data,
    VkQueue     queue)
{
    MT_QUEUE_INFO *pQueueInfo = &my_data->queueMap[queue];
    // Set queue's lastRetired to lastSubmitted indicating all fences completed
    pQueueInfo->lastRetiredId = pQueueInfo->lastSubmittedId;
}

// Helper routine that updates all queues to all-retired
static void
retire_device_fences(
    layer_data *my_data,
    VkDevice    device)
{
    // Process each queue for device
    // TODO: Add multiple device support
    for (auto ii=my_data->queueMap.begin(); ii!=my_data->queueMap.end(); ++ii) {
        // Set queue's lastRetired to lastSubmitted indicating all fences completed
        MT_QUEUE_INFO *pQueueInfo = &(*ii).second;
        pQueueInfo->lastRetiredId = pQueueInfo->lastSubmittedId;
    }
}

// Helper function to validate correct usage bits set for buffers or images
//  Verify that (actual & desired) flags != 0 or,
//   if strict is true, verify that (actual & desired) flags == desired
//  In case of error, report it via dbg callbacks
static VkBool32
validate_usage_flags(
    layer_data      *my_data,
    void            *disp_obj,
    VkFlags          actual,
    VkFlags          desired,
    VkBool32         strict,
    uint64_t         obj_handle,
    VkDebugReportObjectTypeEXT  obj_type,
    char const      *ty_str,
    char const      *func_name,
    char const      *usage_str)
{
    VkBool32 correct_usage = VK_FALSE;
    VkBool32 skipCall      = VK_FALSE;
    if (strict)
        correct_usage = ((actual & desired) == desired);
    else
        correct_usage = ((actual & desired) != 0);
    if (!correct_usage) {
        skipCall = log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, obj_type, obj_handle, __LINE__, MEMTRACK_INVALID_USAGE_FLAG, "MEM",
                           "Invalid usage flag for %s %#" PRIxLEAST64 " used by %s. In this case, %s should have %s set during creation.",
                           ty_str, obj_handle, func_name, ty_str, usage_str);
    }
    return skipCall;
}

// Helper function to validate usage flags for images
// Pulls image info and then sends actual vs. desired usage off to helper above where
//  an error will be flagged if usage is not correct
static VkBool32
validate_image_usage_flags(
    layer_data *my_data,
    void       *disp_obj,
    VkImage     image,
    VkFlags     desired,
    VkBool32    strict,
    char const *func_name,
    char const *usage_string)
{
    VkBool32 skipCall = VK_FALSE;
    MT_OBJ_BINDING_INFO* pBindInfo = get_object_binding_info(my_data, (uint64_t)image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT);
    if (pBindInfo) {
        skipCall = validate_usage_flags(my_data, disp_obj, pBindInfo->create_info.image.usage, desired, strict,
                                      (uint64_t) image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, "image", func_name, usage_string);
    }
    return skipCall;
}

// Helper function to validate usage flags for buffers
// Pulls buffer info and then sends actual vs. desired usage off to helper above where
//  an error will be flagged if usage is not correct
static VkBool32
validate_buffer_usage_flags(
    layer_data  *my_data,
     void       *disp_obj,
     VkBuffer    buffer,
     VkFlags     desired,
     VkBool32    strict,
     char const *func_name,
     char const *usage_string)
{
    VkBool32 skipCall = VK_FALSE;
    MT_OBJ_BINDING_INFO* pBindInfo = get_object_binding_info(my_data, (uint64_t) buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT);
    if (pBindInfo) {
        skipCall = validate_usage_flags(my_data, disp_obj, pBindInfo->create_info.buffer.usage, desired, strict,
                                      (uint64_t) buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, "buffer", func_name, usage_string);
    }
    return skipCall;
}

// Return ptr to info in map container containing mem, or NULL if not found
//  Calls to this function should be wrapped in mutex
static MT_MEM_OBJ_INFO*
get_mem_obj_info(
    layer_data           *my_data,
    const VkDeviceMemory  mem)
{
    auto item = my_data->memObjMap.find(mem);
    if (item != my_data->memObjMap.end()) {
        return &(*item).second;
    } else {
        return NULL;
    }
}

static void
add_mem_obj_info(
    layer_data                 *my_data,
    void                       *object,
    const VkDeviceMemory        mem,
    const VkMemoryAllocateInfo *pAllocateInfo)
{
    assert(object != NULL);

    memcpy(&my_data->memObjMap[mem].allocInfo, pAllocateInfo, sizeof(VkMemoryAllocateInfo));
    // TODO:  Update for real hardware, actually process allocation info structures
    my_data->memObjMap[mem].allocInfo.pNext = NULL;
    my_data->memObjMap[mem].object          = object;
    my_data->memObjMap[mem].refCount        = 0;
    my_data->memObjMap[mem].mem             = mem;
    my_data->memObjMap[mem].memRange.offset = 0;
    my_data->memObjMap[mem].memRange.size   = 0;
    my_data->memObjMap[mem].pData           = 0;
    my_data->memObjMap[mem].pDriverData     = 0;
    my_data->memObjMap[mem].valid           = false;
}

static VkBool32 validate_memory_is_valid(layer_data *my_data, VkDeviceMemory mem, const char* functionName, VkImage image = VK_NULL_HANDLE) {
    if (mem == MEMTRACKER_SWAP_CHAIN_IMAGE_KEY) {
        MT_OBJ_BINDING_INFO* pBindInfo = get_object_binding_info(my_data, (uint64_t)(image), VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT);
        if (pBindInfo && !pBindInfo->valid) {
            return log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT,
                (uint64_t)(mem), __LINE__, MEMTRACK_INVALID_USAGE_FLAG, "MEM",
                "%s: Cannot read invalid swapchain image %" PRIx64 ", please fill the memory before using.", functionName, (uint64_t)(image));
        }
    }
    else {
        MT_MEM_OBJ_INFO *pMemObj = get_mem_obj_info(my_data, mem);
        if (pMemObj && !pMemObj->valid) {
            return log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT,
                (uint64_t)(mem), __LINE__, MEMTRACK_INVALID_USAGE_FLAG, "MEM",
                "%s: Cannot read invalid memory %" PRIx64 ", please fill the memory before using.", functionName, (uint64_t)(mem));
        }
    }
    return false;
}

static void set_memory_valid(layer_data *my_data, VkDeviceMemory mem, bool valid, VkImage image = VK_NULL_HANDLE) {
    if (mem == MEMTRACKER_SWAP_CHAIN_IMAGE_KEY) {
        MT_OBJ_BINDING_INFO* pBindInfo = get_object_binding_info(my_data, (uint64_t)(image), VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT);
        if (pBindInfo) {
            pBindInfo->valid = valid;
        }
    } else {
        MT_MEM_OBJ_INFO *pMemObj = get_mem_obj_info(my_data, mem);
        if (pMemObj) {
            pMemObj->valid = valid;
        }
    }
}

// Find CB Info and add mem reference to list container
// Find Mem Obj Info and add CB reference to list container
static VkBool32
update_cmd_buf_and_mem_references(
    layer_data            *my_data,
    const VkCommandBuffer  cb,
    const VkDeviceMemory   mem,
    const char            *apiName)
{
    VkBool32 skipCall = VK_FALSE;

    // Skip validation if this image was created through WSI
    if (mem != MEMTRACKER_SWAP_CHAIN_IMAGE_KEY) {

        // First update CB binding in MemObj mini CB list
        MT_MEM_OBJ_INFO* pMemInfo = get_mem_obj_info(my_data, mem);
        if (pMemInfo) {
            // Search for cmd buffer object in memory object's binding list
            VkBool32 found  = VK_FALSE;
            if (pMemInfo->pCommandBufferBindings.size() > 0) {
                for (list<VkCommandBuffer>::iterator it = pMemInfo->pCommandBufferBindings.begin(); it != pMemInfo->pCommandBufferBindings.end(); ++it) {
                    if ((*it) == cb) {
                        found = VK_TRUE;
                        break;
                    }
                }
            }
            // If not present, add to list
            if (found == VK_FALSE) {
                pMemInfo->pCommandBufferBindings.push_front(cb);
                pMemInfo->refCount++;
            }
            // Now update CBInfo's Mem reference list
            MT_CB_INFO* pCBInfo = get_cmd_buf_info(my_data, cb);
            // TODO: keep track of all destroyed CBs so we know if this is a stale or simply invalid object
            if (pCBInfo) {
                // Search for memory object in cmd buffer's reference list
                VkBool32 found  = VK_FALSE;
                if (pCBInfo->pMemObjList.size() > 0) {
                    for (auto it = pCBInfo->pMemObjList.begin(); it != pCBInfo->pMemObjList.end(); ++it) {
                        if ((*it) == mem) {
                            found = VK_TRUE;
                            break;
                        }
                    }
                }
                // If not present, add to list
                if (found == VK_FALSE) {
                    pCBInfo->pMemObjList.push_front(mem);
                }
            }
        }
    }
    return skipCall;
}

// Free bindings related to CB
static VkBool32
clear_cmd_buf_and_mem_references(
    layer_data            *my_data,
    const VkCommandBuffer  cb)
{
    VkBool32 skipCall = VK_FALSE;
    MT_CB_INFO* pCBInfo = get_cmd_buf_info(my_data, cb);

    if (pCBInfo && (pCBInfo->pMemObjList.size() > 0)) {
        list<VkDeviceMemory> mem_obj_list = pCBInfo->pMemObjList;
        for (list<VkDeviceMemory>::iterator it=mem_obj_list.begin(); it!=mem_obj_list.end(); ++it) {
            MT_MEM_OBJ_INFO* pInfo = get_mem_obj_info(my_data, *it);
            if (pInfo) {
                pInfo->pCommandBufferBindings.remove(cb);
                pInfo->refCount--;
            }
        }
        pCBInfo->pMemObjList.clear();
        pCBInfo->activeDescriptorSets.clear();
        pCBInfo->validate_functions.clear();
    }
    return skipCall;
}

// Delete the entire CB list
static VkBool32
delete_cmd_buf_info_list(
    layer_data* my_data)
{
    VkBool32 skipCall = VK_FALSE;
    for (unordered_map<VkCommandBuffer, MT_CB_INFO>::iterator ii=my_data->cbMap.begin(); ii!=my_data->cbMap.end(); ++ii) {
        skipCall |= clear_cmd_buf_and_mem_references(my_data, (*ii).first);
    }
    my_data->cbMap.clear();
    return skipCall;
}

// For given MemObjInfo, report Obj & CB bindings
static VkBool32
reportMemReferencesAndCleanUp(
    layer_data      *my_data,
    MT_MEM_OBJ_INFO *pMemObjInfo)
{
    VkBool32 skipCall = VK_FALSE;
    size_t cmdBufRefCount = pMemObjInfo->pCommandBufferBindings.size();
    size_t objRefCount    = pMemObjInfo->pObjBindings.size();

    if ((pMemObjInfo->pCommandBufferBindings.size()) != 0) {
        skipCall = log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, (uint64_t) pMemObjInfo->mem, __LINE__, MEMTRACK_FREED_MEM_REF, "MEM",
                       "Attempting to free memory object %#" PRIxLEAST64 " which still contains " PRINTF_SIZE_T_SPECIFIER " references",
                       (uint64_t) pMemObjInfo->mem, (cmdBufRefCount + objRefCount));
    }

    if (cmdBufRefCount > 0 && pMemObjInfo->pCommandBufferBindings.size() > 0) {
        for (list<VkCommandBuffer>::const_iterator it = pMemObjInfo->pCommandBufferBindings.begin(); it != pMemObjInfo->pCommandBufferBindings.end(); ++it) {
            // TODO : CommandBuffer should be source Obj here
            log_msg(my_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, (uint64_t)(*it), __LINE__, MEMTRACK_FREED_MEM_REF, "MEM",
                    "Command Buffer %p still has a reference to mem obj %#" PRIxLEAST64, (*it), (uint64_t) pMemObjInfo->mem);
        }
        // Clear the list of hanging references
        pMemObjInfo->pCommandBufferBindings.clear();
    }

    if (objRefCount > 0 && pMemObjInfo->pObjBindings.size() > 0) {
        for (auto it = pMemObjInfo->pObjBindings.begin(); it != pMemObjInfo->pObjBindings.end(); ++it) {
            log_msg(my_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, it->type, it->handle, __LINE__, MEMTRACK_FREED_MEM_REF, "MEM",
                    "VK Object %#" PRIxLEAST64 " still has a reference to mem obj %#" PRIxLEAST64, it->handle, (uint64_t) pMemObjInfo->mem);
        }
        // Clear the list of hanging references
        pMemObjInfo->pObjBindings.clear();
    }
    return skipCall;
}

static VkBool32
deleteMemObjInfo(
    layer_data     *my_data,
    void           *object,
    VkDeviceMemory  mem)
{
    VkBool32 skipCall = VK_FALSE;
    auto item = my_data->memObjMap.find(mem);
    if (item != my_data->memObjMap.end()) {
        my_data->memObjMap.erase(item);
    } else {
        skipCall = log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, (uint64_t) mem, __LINE__, MEMTRACK_INVALID_MEM_OBJ, "MEM",
                       "Request to delete memory object %#" PRIxLEAST64 " not present in memory Object Map", (uint64_t) mem);
    }
    return skipCall;
}

// Check if fence for given CB is completed
static VkBool32
checkCBCompleted(
    layer_data            *my_data,
    const VkCommandBuffer  cb,
    VkBool32              *complete)
{
    MT_CB_INFO *pCBInfo  = get_cmd_buf_info(my_data, cb);
    VkBool32    skipCall = VK_FALSE;
    *complete            = VK_TRUE;

    if (pCBInfo) {
        if (pCBInfo->lastSubmittedQueue != NULL) {
            VkQueue queue = pCBInfo->lastSubmittedQueue;
            MT_QUEUE_INFO *pQueueInfo = &my_data->queueMap[queue];
            if (pCBInfo->fenceId > pQueueInfo->lastRetiredId) {
                skipCall = log_msg(my_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, (uint64_t)cb, __LINE__,
                               MEMTRACK_NONE, "MEM", "fence %#" PRIxLEAST64 " for CB %p has not been checked for completion",
                               (uint64_t) pCBInfo->lastSubmittedFence, cb);
                *complete = VK_FALSE;
            }
        }
    }
    return skipCall;
}

static VkBool32
freeMemObjInfo(
    layer_data     *my_data,
    void*           object,
    VkDeviceMemory  mem,
    VkBool32        internal)
{
    VkBool32 skipCall = VK_FALSE;
    // Parse global list to find info w/ mem
    MT_MEM_OBJ_INFO* pInfo = get_mem_obj_info(my_data, mem);
    if (pInfo) {
        if (pInfo->allocInfo.allocationSize == 0 && !internal) {
            // TODO: Verify against Valid Use section
            skipCall = log_msg(my_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, (uint64_t) mem, __LINE__, MEMTRACK_INVALID_MEM_OBJ, "MEM",
                            "Attempting to free memory associated with a Persistent Image, %#" PRIxLEAST64 ", "
                            "this should not be explicitly freed\n", (uint64_t) mem);
        } else {
            // Clear any CB bindings for completed CBs
            //   TODO : Is there a better place to do this?

            VkBool32 commandBufferComplete = VK_FALSE;
            assert(pInfo->object != VK_NULL_HANDLE);
            list<VkCommandBuffer>::iterator it = pInfo->pCommandBufferBindings.begin();
            list<VkCommandBuffer>::iterator temp;
            while (pInfo->pCommandBufferBindings.size() > 0 && it != pInfo->pCommandBufferBindings.end()) {
                skipCall |= checkCBCompleted(my_data, *it, &commandBufferComplete);
                if (VK_TRUE == commandBufferComplete) {
                    temp = it;
                    ++temp;
                    skipCall |= clear_cmd_buf_and_mem_references(my_data, *it);
                    it = temp;
                } else {
                    ++it;
                }
            }

            // Now verify that no references to this mem obj remain and remove bindings
            if (0 != pInfo->refCount) {
                skipCall |= reportMemReferencesAndCleanUp(my_data, pInfo);
            }
            // Delete mem obj info
            skipCall |= deleteMemObjInfo(my_data, object, mem);
        }
    }
    return skipCall;
}

static const char*
object_type_to_string(
    VkDebugReportObjectTypeEXT type)
{
    switch (type)
    {
        case VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT:
           return "image";
           break;
        case VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT:
           return "buffer";
           break;
        case VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT:
           return "swapchain";
           break;
        default:
           return "unknown";
    }
}

// Remove object binding performs 3 tasks:
// 1. Remove ObjectInfo from MemObjInfo list container of obj bindings & free it
// 2. Decrement refCount for MemObjInfo
// 3. Clear mem binding for image/buffer by setting its handle to 0
// TODO : This only applied to Buffer, Image, and Swapchain objects now, how should it be updated/customized?
static VkBool32
clear_object_binding(
    layer_data      *my_data,
    void            *dispObj,
    uint64_t         handle,
    VkDebugReportObjectTypeEXT  type)
{
    // TODO : Need to customize images/buffers/swapchains to track mem binding and clear it here appropriately
    VkBool32 skipCall = VK_FALSE;
    MT_OBJ_BINDING_INFO* pObjBindInfo = get_object_binding_info(my_data, handle, type);
    if (pObjBindInfo) {
        MT_MEM_OBJ_INFO* pMemObjInfo = get_mem_obj_info(my_data, pObjBindInfo->mem);
        // TODO : Make sure this is a reasonable way to reset mem binding
        pObjBindInfo->mem = VK_NULL_HANDLE;
        if (pMemObjInfo) {
            // This obj is bound to a memory object. Remove the reference to this object in that memory object's list, decrement the memObj's refcount
            // and set the objects memory binding pointer to NULL.
            VkBool32 clearSucceeded = VK_FALSE;
            for (auto it = pMemObjInfo->pObjBindings.begin(); it != pMemObjInfo->pObjBindings.end(); ++it) {
                if ((it->handle == handle) && (it->type == type)) {
                    pMemObjInfo->refCount--;
                    pMemObjInfo->pObjBindings.erase(it);
                    clearSucceeded = VK_TRUE;
                    break;
                }
            }
            if (VK_FALSE == clearSucceeded ) {
                skipCall |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, type, handle, __LINE__, MEMTRACK_INVALID_OBJECT, "MEM",
                                "While trying to clear mem binding for %s obj %#" PRIxLEAST64 ", unable to find that object referenced by mem obj %#" PRIxLEAST64,
                                 object_type_to_string(type), handle, (uint64_t) pMemObjInfo->mem);
            }
        }
    }
    return skipCall;
}

// For NULL mem case, output warning
// Make sure given object is in global object map
//  IF a previous binding existed, output validation error
//  Otherwise, add reference from objectInfo to memoryInfo
//  Add reference off of objInfo
//  device is required for error logging, need a dispatchable
//  object for that.
static VkBool32
set_mem_binding(
    layer_data      *my_data,
    void            *dispatch_object,
    VkDeviceMemory   mem,
    uint64_t         handle,
    VkDebugReportObjectTypeEXT  type,
    const char      *apiName)
{
    VkBool32 skipCall = VK_FALSE;
    // Handle NULL case separately, just clear previous binding & decrement reference
    if (mem == VK_NULL_HANDLE) {
        // TODO: Verify against Valid Use section of spec.
        skipCall = log_msg(my_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, type, handle, __LINE__, MEMTRACK_INVALID_MEM_OBJ, "MEM",
                       "In %s, attempting to Bind Obj(%#" PRIxLEAST64 ") to NULL", apiName, handle);
    } else {
        MT_OBJ_BINDING_INFO* pObjBindInfo = get_object_binding_info(my_data, handle, type);
        if (!pObjBindInfo) {
            skipCall |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, type, handle, __LINE__, MEMTRACK_MISSING_MEM_BINDINGS, "MEM",
                            "In %s, attempting to update Binding of %s Obj(%#" PRIxLEAST64 ") that's not in global list()",
                            object_type_to_string(type), apiName, handle);
        } else {
            // non-null case so should have real mem obj
            MT_MEM_OBJ_INFO* pMemInfo = get_mem_obj_info(my_data, mem);
            if (pMemInfo) {
                // TODO : Need to track mem binding for obj and report conflict here
                MT_MEM_OBJ_INFO* pPrevBinding = get_mem_obj_info(my_data, pObjBindInfo->mem);
                if (pPrevBinding != NULL) {
                    skipCall |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, (uint64_t) mem, __LINE__, MEMTRACK_REBIND_OBJECT, "MEM",
                            "In %s, attempting to bind memory (%#" PRIxLEAST64 ") to object (%#" PRIxLEAST64 ") which has already been bound to mem object %#" PRIxLEAST64,
                            apiName, (uint64_t) mem, handle, (uint64_t) pPrevBinding->mem);
                }
                else {
                    MT_OBJ_HANDLE_TYPE oht;
                    oht.handle = handle;
                    oht.type = type;
                    pMemInfo->pObjBindings.push_front(oht);
                    pMemInfo->refCount++;
                    // For image objects, make sure default memory state is correctly set
                    // TODO : What's the best/correct way to handle this?
                    if (VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT == type) {
                        VkImageCreateInfo ici = pObjBindInfo->create_info.image;
                        if (ici.usage & (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)) {
                            // TODO::  More memory state transition stuff.
                        }
                    }
                    pObjBindInfo->mem = mem;
                }
            }
        }
    }
    return skipCall;
}

// For NULL mem case, clear any previous binding Else...
// Make sure given object is in its object map
//  IF a previous binding existed, update binding
//  Add reference from objectInfo to memoryInfo
//  Add reference off of object's binding info
// Return VK_TRUE if addition is successful, VK_FALSE otherwise
static VkBool32
set_sparse_mem_binding(
    layer_data      *my_data,
    void            *dispObject,
    VkDeviceMemory   mem,
    uint64_t         handle,
    VkDebugReportObjectTypeEXT  type,
    const char      *apiName)
{
    VkBool32 skipCall = VK_FALSE;
    // Handle NULL case separately, just clear previous binding & decrement reference
    if (mem == VK_NULL_HANDLE) {
        skipCall = clear_object_binding(my_data, dispObject, handle, type);
    } else {
        MT_OBJ_BINDING_INFO* pObjBindInfo = get_object_binding_info(my_data, handle, type);
        if (!pObjBindInfo) {
            skipCall |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, type, handle, __LINE__, MEMTRACK_MISSING_MEM_BINDINGS, "MEM",
                            "In %s, attempting to update Binding of Obj(%#" PRIxLEAST64 ") that's not in global list()", apiName, handle);
        }
        // non-null case so should have real mem obj
        MT_MEM_OBJ_INFO* pInfo = get_mem_obj_info(my_data, mem);
        if (pInfo) {
            // Search for object in memory object's binding list
            VkBool32 found  = VK_FALSE;
            if (pInfo->pObjBindings.size() > 0) {
                for (auto it = pInfo->pObjBindings.begin(); it != pInfo->pObjBindings.end(); ++it) {
                    if (((*it).handle == handle) && ((*it).type == type)) {
                        found = VK_TRUE;
                        break;
                    }
                }
            }
            // If not present, add to list
            if (found == VK_FALSE) {
                MT_OBJ_HANDLE_TYPE oht;
                oht.handle = handle;
                oht.type   = type;
                pInfo->pObjBindings.push_front(oht);
                pInfo->refCount++;
            }
            // Need to set mem binding for this object
            pObjBindInfo->mem = mem;
        }
    }
    return skipCall;
}

template <typename T> void
print_object_map_members(
    layer_data      *my_data,
    void            *dispObj,
    T const&         objectName,
    VkDebugReportObjectTypeEXT  objectType,
    const char      *objectStr)
{
    for (auto const& element : objectName) {
        log_msg(my_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, objectType, 0, __LINE__, MEMTRACK_NONE, "MEM",
            "    %s Object list contains %s Object %#" PRIxLEAST64 " ", objectStr, objectStr, element.first);
    }
}

// For given Object, get 'mem' obj that it's bound to or NULL if no binding
static VkBool32
get_mem_binding_from_object(
    layer_data            *my_data,
    void                  *dispObj,
    const uint64_t         handle,
    const VkDebugReportObjectTypeEXT  type,
    VkDeviceMemory        *mem)
{
    VkBool32 skipCall = VK_FALSE;
    *mem = VK_NULL_HANDLE;
    MT_OBJ_BINDING_INFO* pObjBindInfo = get_object_binding_info(my_data, handle, type);
    if (pObjBindInfo) {
        if (pObjBindInfo->mem) {
            *mem = pObjBindInfo->mem;
        } else {
            skipCall = log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, type, handle, __LINE__, MEMTRACK_MISSING_MEM_BINDINGS, "MEM",
                           "Trying to get mem binding for object %#" PRIxLEAST64 " but object has no mem binding", handle);
        }
    } else {
        skipCall = log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, type, handle, __LINE__, MEMTRACK_INVALID_OBJECT, "MEM",
                       "Trying to get mem binding for object %#" PRIxLEAST64 " but no such object in %s list",
                       handle, object_type_to_string(type));
    }
    return skipCall;
}

// Print details of MemObjInfo list
static void
print_mem_list(
    layer_data *my_data,
    void       *dispObj)
{
    MT_MEM_OBJ_INFO* pInfo = NULL;

    // Early out if info is not requested
    if (!(my_data->report_data->active_flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT)) {
        return;
    }

    // Just printing each msg individually for now, may want to package these into single large print
    log_msg(my_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, 0, __LINE__, MEMTRACK_NONE, "MEM",
            "Details of Memory Object list (of size " PRINTF_SIZE_T_SPECIFIER " elements)", my_data->memObjMap.size());
    log_msg(my_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, 0, __LINE__, MEMTRACK_NONE, "MEM",
            "=============================");

    if (my_data->memObjMap.size() <= 0)
        return;

    for (auto ii=my_data->memObjMap.begin(); ii!=my_data->memObjMap.end(); ++ii) {
        pInfo = &(*ii).second;

        log_msg(my_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, 0, __LINE__, MEMTRACK_NONE, "MEM",
            "    ===MemObjInfo at %p===", (void*)pInfo);
        log_msg(my_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, 0, __LINE__, MEMTRACK_NONE, "MEM",
                "    Mem object: %#" PRIxLEAST64, (uint64_t)(pInfo->mem));
        log_msg(my_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, 0, __LINE__, MEMTRACK_NONE, "MEM",
                "    Ref Count: %u", pInfo->refCount);
        if (0 != pInfo->allocInfo.allocationSize) {
            string pAllocInfoMsg = vk_print_vkmemoryallocateinfo(&pInfo->allocInfo, "MEM(INFO):         ");
            log_msg(my_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, 0, __LINE__, MEMTRACK_NONE, "MEM",
                    "    Mem Alloc info:\n%s", pAllocInfoMsg.c_str());
        } else {
            log_msg(my_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, 0, __LINE__, MEMTRACK_NONE, "MEM",
                    "    Mem Alloc info is NULL (alloc done by vkCreateSwapchainKHR())");
        }

        log_msg(my_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, 0, __LINE__, MEMTRACK_NONE, "MEM",
                "    VK OBJECT Binding list of size " PRINTF_SIZE_T_SPECIFIER " elements:", pInfo->pObjBindings.size());
        if (pInfo->pObjBindings.size() > 0) {
            for (list<MT_OBJ_HANDLE_TYPE>::iterator it = pInfo->pObjBindings.begin(); it != pInfo->pObjBindings.end(); ++it) {
                log_msg(my_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, 0, __LINE__, MEMTRACK_NONE, "MEM",
                        "       VK OBJECT %" PRIu64, it->handle);
            }
        }

        log_msg(my_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, 0, __LINE__, MEMTRACK_NONE, "MEM",
                "    VK Command Buffer (CB) binding list of size " PRINTF_SIZE_T_SPECIFIER " elements", pInfo->pCommandBufferBindings.size());
        if (pInfo->pCommandBufferBindings.size() > 0)
        {
            for (list<VkCommandBuffer>::iterator it = pInfo->pCommandBufferBindings.begin(); it != pInfo->pCommandBufferBindings.end(); ++it) {
                log_msg(my_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, 0, __LINE__, MEMTRACK_NONE, "MEM",
                        "      VK CB %p", (*it));
            }
        }
    }
}

static void
printCBList(
    layer_data *my_data,
    void       *dispObj)
{
    MT_CB_INFO* pCBInfo = NULL;

    // Early out if info is not requested
    if (!(my_data->report_data->active_flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT)) {
        return;
    }

    log_msg(my_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, 0, __LINE__, MEMTRACK_NONE, "MEM",
        "Details of CB list (of size " PRINTF_SIZE_T_SPECIFIER " elements)", my_data->cbMap.size());
    log_msg(my_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, 0, __LINE__, MEMTRACK_NONE, "MEM",
        "==================");

    if (my_data->cbMap.size() <= 0)
        return;

    for (auto ii=my_data->cbMap.begin(); ii!=my_data->cbMap.end(); ++ii) {
        pCBInfo = &(*ii).second;

        log_msg(my_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, 0, __LINE__, MEMTRACK_NONE, "MEM",
                "    CB Info (%p) has CB %p, fenceId %" PRIx64", and fence %#" PRIxLEAST64,
                (void*)pCBInfo, (void*)pCBInfo->commandBuffer, pCBInfo->fenceId,
                (uint64_t) pCBInfo->lastSubmittedFence);

        if (pCBInfo->pMemObjList.size() <= 0)
            continue;
        for (list<VkDeviceMemory>::iterator it = pCBInfo->pMemObjList.begin(); it != pCBInfo->pMemObjList.end(); ++it) {
            log_msg(my_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, 0, __LINE__, MEMTRACK_NONE, "MEM",
                    "      Mem obj %" PRIu64, (uint64_t)(*it));
        }
    }
}

static void
init_mem_tracker(
    layer_data *my_data,
    const VkAllocationCallbacks *pAllocator)
{
    uint32_t report_flags = 0;
    uint32_t debug_action = 0;
    FILE *log_output = NULL;
    const char *option_str;
    VkDebugReportCallbackEXT callback;
    // initialize MemTracker options
    report_flags = getLayerOptionFlags("MemTrackerReportFlags", 0);
    getLayerOptionEnum("MemTrackerDebugAction", (uint32_t *) &debug_action);

    if (debug_action & VK_DBG_LAYER_ACTION_LOG_MSG)
    {
        option_str = getLayerOption("MemTrackerLogFilename");
        log_output = getLayerLogOutput(option_str, "MemTracker");
        VkDebugReportCallbackCreateInfoEXT dbgInfo;
        memset(&dbgInfo, 0, sizeof(dbgInfo));
        dbgInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
        dbgInfo.pfnCallback = log_callback;
        dbgInfo.pUserData = log_output;
        dbgInfo.flags = report_flags;
        layer_create_msg_callback(my_data->report_data, &dbgInfo, pAllocator, &callback);
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

    // Zero out memory property data
    memset(&memProps, 0, sizeof(VkPhysicalDeviceMemoryProperties));
}

// hook DestroyInstance to remove tableInstanceMap entry
VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyInstance(
    VkInstance                   instance,
    const VkAllocationCallbacks *pAllocator)
{
    // Grab the key before the instance is destroyed.
    dispatch_key key = get_dispatch_key(instance);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerInstanceDispatchTable *pTable = my_data->instance_dispatch_table;
    pTable->DestroyInstance(instance, pAllocator);

    loader_platform_thread_lock_mutex(&globalLock);
    // Clean up logging callback, if any
    while (my_data->logging_callback.size() > 0) {
        VkDebugReportCallbackEXT callback = my_data->logging_callback.back();
        layer_destroy_msg_callback(my_data->report_data, callback, pAllocator);
        my_data->logging_callback.pop_back();
    }

    layer_debug_report_destroy_instance(my_data->report_data);
    delete my_data->instance_dispatch_table;
    layer_data_map.erase(key);
    loader_platform_thread_unlock_mutex(&globalLock);
    if (layer_data_map.empty()) {
        // Release mutex when destroying last instance
        loader_platform_thread_delete_mutex(&globalLock);
        globalLockInitialized = 0;
    }
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateInstance(
    const VkInstanceCreateInfo*                 pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkInstance*                                 pInstance)
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

    init_mem_tracker(my_data, pAllocator);

    return result;
}

static void
createDeviceRegisterExtensions(
    const VkDeviceCreateInfo *pCreateInfo,
    VkDevice                  device)
{
    layer_data *my_device_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkLayerDispatchTable *pDisp = my_device_data->device_dispatch_table;
    PFN_vkGetDeviceProcAddr gpa = pDisp->GetDeviceProcAddr;
    pDisp->CreateSwapchainKHR = (PFN_vkCreateSwapchainKHR) gpa(device, "vkCreateSwapchainKHR");
    pDisp->DestroySwapchainKHR = (PFN_vkDestroySwapchainKHR) gpa(device, "vkDestroySwapchainKHR");
    pDisp->GetSwapchainImagesKHR = (PFN_vkGetSwapchainImagesKHR) gpa(device, "vkGetSwapchainImagesKHR");
    pDisp->AcquireNextImageKHR = (PFN_vkAcquireNextImageKHR) gpa(device, "vkAcquireNextImageKHR");
    pDisp->QueuePresentKHR = (PFN_vkQueuePresentKHR) gpa(device, "vkQueuePresentKHR");
    my_device_data->wsi_enabled = VK_FALSE;
    for (uint32_t i = 0; i < pCreateInfo->enabledExtensionCount; i++) {
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0)
            my_device_data->wsi_enabled = true;
    }
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateDevice(
    VkPhysicalDevice             gpu,
    const VkDeviceCreateInfo    *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkDevice                    *pDevice)
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
    createDeviceRegisterExtensions(pCreateInfo, *pDevice);
    my_instance_data->instance_dispatch_table->GetPhysicalDeviceProperties(gpu, &my_device_data->properties);

    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyDevice(
    VkDevice                     device,
    const VkAllocationCallbacks *pAllocator)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_device_data = get_my_data_ptr(key, layer_data_map);
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&globalLock);
    log_msg(my_device_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, (uint64_t)device, __LINE__, MEMTRACK_NONE, "MEM",
        "Printing List details prior to vkDestroyDevice()");
    log_msg(my_device_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, (uint64_t)device, __LINE__, MEMTRACK_NONE, "MEM",
        "================================================");
    print_mem_list(my_device_data, device);
    printCBList(my_device_data, device);
    skipCall = delete_cmd_buf_info_list(my_device_data);
    // Report any memory leaks
    MT_MEM_OBJ_INFO* pInfo = NULL;
    if (my_device_data->memObjMap.size() > 0) {
        for (auto ii=my_device_data->memObjMap.begin(); ii!=my_device_data->memObjMap.end(); ++ii) {
            pInfo = &(*ii).second;
            if (pInfo->allocInfo.allocationSize != 0) {
                // Valid Usage: All child objects created on device must have been destroyed prior to destroying device
                skipCall |= log_msg(my_device_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, (uint64_t) pInfo->mem, __LINE__, MEMTRACK_MEMORY_LEAK, "MEM",
                                 "Mem Object %" PRIu64 " has not been freed. You should clean up this memory by calling "
                                 "vkFreeMemory(%" PRIu64 ") prior to vkDestroyDevice().", (uint64_t)(pInfo->mem), (uint64_t)(pInfo->mem));
            }
        }
    }
    // Queues persist until device is destroyed
    delete_queue_info_list(my_device_data);
    layer_debug_report_destroy_device(device);
    loader_platform_thread_unlock_mutex(&globalLock);

#if DISPATCH_MAP_DEBUG
    fprintf(stderr, "Device: %p, key: %p\n", device, key);
#endif
    VkLayerDispatchTable *pDisp  = my_device_data->device_dispatch_table;
    if (VK_FALSE == skipCall) {
        pDisp->DestroyDevice(device, pAllocator);
    }
    delete my_device_data->device_dispatch_table;
    layer_data_map.erase(key);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceMemoryProperties(
    VkPhysicalDevice                  physicalDevice,
    VkPhysicalDeviceMemoryProperties *pMemoryProperties)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
    VkLayerInstanceDispatchTable *pInstanceTable = my_data->instance_dispatch_table;
    pInstanceTable->GetPhysicalDeviceMemoryProperties(physicalDevice, pMemoryProperties);
    memcpy(&memProps, pMemoryProperties, sizeof(VkPhysicalDeviceMemoryProperties));
}

static const VkExtensionProperties instance_extensions[] = {
    {
        VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
        VK_EXT_DEBUG_REPORT_SPEC_VERSION
    }
};

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceExtensionProperties(
        const char            *pLayerName,
        uint32_t              *pCount,
        VkExtensionProperties *pProperties)
{
    return util_GetExtensionProperties(1, instance_extensions, pCount, pProperties);
}

static const VkLayerProperties mtGlobalLayers[] = {
    {
        "VK_LAYER_LUNARG_mem_tracker",
        VK_API_VERSION,
        1,
        "LunarG Validation Layer",
    }
};

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceLayerProperties(
        uint32_t          *pCount,
        VkLayerProperties *pProperties)
{
    return util_GetLayerProperties(ARRAY_SIZE(mtGlobalLayers),
                                   mtGlobalLayers,
                                   pCount, pProperties);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceExtensionProperties(
        VkPhysicalDevice                            physicalDevice,
        const char                                 *pLayerName,
        uint32_t                                   *pCount,
        VkExtensionProperties                      *pProperties)
{
    /* Mem tracker does not have any physical device extensions */
    if (pLayerName == NULL) {
        layer_data *my_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
        VkLayerInstanceDispatchTable *pInstanceTable = my_data->instance_dispatch_table;
        return pInstanceTable->EnumerateDeviceExtensionProperties(
            physicalDevice, NULL, pCount, pProperties);
    } else {
        return util_GetExtensionProperties(0, NULL, pCount, pProperties);
    }
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceLayerProperties(
        VkPhysicalDevice                            physicalDevice,
        uint32_t                                   *pCount,
        VkLayerProperties                          *pProperties)
{
    /* Mem tracker's physical device layers are the same as global */
    return util_GetLayerProperties(ARRAY_SIZE(mtGlobalLayers), mtGlobalLayers,
                                   pCount, pProperties);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetDeviceQueue(
    VkDevice  device,
    uint32_t  queueNodeIndex,
    uint32_t  queueIndex,
    VkQueue  *pQueue)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    my_data->device_dispatch_table->GetDeviceQueue(device, queueNodeIndex, queueIndex, pQueue);
    loader_platform_thread_lock_mutex(&globalLock);
    add_queue_info(my_data, *pQueue);
    loader_platform_thread_unlock_mutex(&globalLock);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkQueueSubmit(
    VkQueue             queue,
    uint32_t            submitCount,
    const VkSubmitInfo *pSubmits,
    VkFence             fence)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(queue), layer_data_map);
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;

    loader_platform_thread_lock_mutex(&globalLock);
    // TODO : Need to track fence and clear mem references when fence clears
    MT_CB_INFO* pCBInfo = NULL;
    uint64_t    fenceId = 0;
    VkBool32 skipCall = add_fence_info(my_data, fence, queue, &fenceId);

    print_mem_list(my_data, queue);
    printCBList(my_data, queue);
    for (uint32_t submit_idx = 0; submit_idx < submitCount; submit_idx++) {
        const VkSubmitInfo *submit = &pSubmits[submit_idx];
        for (uint32_t i = 0; i < submit->commandBufferCount; i++) {
            pCBInfo = get_cmd_buf_info(my_data, submit->pCommandBuffers[i]);
            if (pCBInfo) {
                pCBInfo->fenceId = fenceId;
                pCBInfo->lastSubmittedFence = fence;
                pCBInfo->lastSubmittedQueue = queue;
                for (auto& function : pCBInfo->validate_functions) {
                    skipCall |= function();
                }
            }
        }

        for (uint32_t i = 0; i < submit->waitSemaphoreCount; i++) {
            VkSemaphore sem = submit->pWaitSemaphores[i];

            if (my_data->semaphoreMap.find(sem) != my_data->semaphoreMap.end()) {
                if (my_data->semaphoreMap[sem] != MEMTRACK_SEMAPHORE_STATE_SIGNALLED) {
                    skipCall = log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT, (uint64_t) sem,
                            __LINE__, MEMTRACK_NONE, "SEMAPHORE",
                            "vkQueueSubmit: Semaphore must be in signaled state before passing to pWaitSemaphores");
                }
                my_data->semaphoreMap[sem] = MEMTRACK_SEMAPHORE_STATE_WAIT;
            }
        }
        for (uint32_t i = 0; i < submit->signalSemaphoreCount; i++) {
            VkSemaphore sem = submit->pSignalSemaphores[i];

            if (my_data->semaphoreMap.find(sem) != my_data->semaphoreMap.end()) {
                if (my_data->semaphoreMap[sem] != MEMTRACK_SEMAPHORE_STATE_UNSET) {
                    skipCall = log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT, (uint64_t) sem,
                            __LINE__, MEMTRACK_NONE, "SEMAPHORE",
                            "vkQueueSubmit: Semaphore must not be currently signaled or in a wait state");
                }
                my_data->semaphoreMap[sem] = MEMTRACK_SEMAPHORE_STATE_SIGNALLED;
            }
        }
    }

    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall) {
        result = my_data->device_dispatch_table->QueueSubmit(
            queue, submitCount, pSubmits, fence);
    }

    loader_platform_thread_lock_mutex(&globalLock);
    for (uint32_t submit_idx = 0; submit_idx < submitCount; submit_idx++) {
        const VkSubmitInfo *submit = &pSubmits[submit_idx];
        for (uint32_t i = 0; i < submit->waitSemaphoreCount; i++) {
            VkSemaphore sem = submit->pWaitSemaphores[i];

            if (my_data->semaphoreMap.find(sem) != my_data->semaphoreMap.end()) {
                my_data->semaphoreMap[sem] = MEMTRACK_SEMAPHORE_STATE_UNSET;
            }
        }
    }
    loader_platform_thread_unlock_mutex(&globalLock);

    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkAllocateMemory(
    VkDevice                     device,
    const VkMemoryAllocateInfo  *pAllocateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkDeviceMemory              *pMemory)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = my_data->device_dispatch_table->AllocateMemory(device, pAllocateInfo, pAllocator, pMemory);
    // TODO : Track allocations and overall size here
    loader_platform_thread_lock_mutex(&globalLock);
    add_mem_obj_info(my_data, device, *pMemory, pAllocateInfo);
    print_mem_list(my_data, device);
    loader_platform_thread_unlock_mutex(&globalLock);
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkFreeMemory(
    VkDevice                     device,
    VkDeviceMemory               mem,
    const VkAllocationCallbacks *pAllocator)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    my_data->bufferRanges.erase(mem);
    my_data->imageRanges.erase(mem);

    // From spec : A memory object is freed by calling vkFreeMemory() when it is no longer needed.
    // Before freeing a memory object, an application must ensure the memory object is no longer
    // in use by the device—for example by command buffers queued for execution. The memory need
    // not yet be unbound from all images and buffers, but any further use of those images or
    // buffers (on host or device) for anything other than destroying those objects will result in
    // undefined behavior.

    loader_platform_thread_lock_mutex(&globalLock);
    freeMemObjInfo(my_data, device, mem, VK_FALSE);
    print_mem_list(my_data, device);
    printCBList(my_data, device);
    loader_platform_thread_unlock_mutex(&globalLock);
    my_data->device_dispatch_table->FreeMemory(device, mem, pAllocator);
}

VkBool32
validateMemRange(
    layer_data     *my_data,
    VkDeviceMemory  mem,
    VkDeviceSize    offset,
    VkDeviceSize    size)
{
    VkBool32 skipCall = VK_FALSE;

    if (size == 0) {
        // TODO: a size of 0 is not listed as an invalid use in the spec, should it be?
        skipCall = log_msg(my_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, (uint64_t)mem, __LINE__,
            MEMTRACK_INVALID_MAP, "MEM", "VkMapMemory: Attempting to map memory range of size zero");
    }

    auto mem_element = my_data->memObjMap.find(mem);
    if (mem_element != my_data->memObjMap.end()) {
        // It is an application error to call VkMapMemory on an object that is already mapped
        if (mem_element->second.memRange.size != 0) {
            skipCall = log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, (uint64_t)mem, __LINE__,
                MEMTRACK_INVALID_MAP, "MEM", "VkMapMemory: Attempting to map memory on an already-mapped object %#" PRIxLEAST64, (uint64_t)mem);
        }

        // Validate that offset + size is within object's allocationSize
        if (size == VK_WHOLE_SIZE) {
            if (offset >= mem_element->second.allocInfo.allocationSize) {
                skipCall = log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, (uint64_t)mem, __LINE__,
                    MEMTRACK_INVALID_MAP, "MEM", "Mapping Memory from %" PRIu64 " to %" PRIu64 " with total array size %" PRIu64,
                    offset, mem_element->second.allocInfo.allocationSize, mem_element->second.allocInfo.allocationSize);
            }
        } else {
            if ((offset + size) > mem_element->second.allocInfo.allocationSize) {
                skipCall = log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, (uint64_t)mem, __LINE__,
                    MEMTRACK_INVALID_MAP, "MEM", "Mapping Memory from %" PRIu64 " to %" PRIu64 " with total array size %" PRIu64,
                    offset, size + offset, mem_element->second.allocInfo.allocationSize);
            }
        }
    }
    return skipCall;
}

void
storeMemRanges(
    layer_data     *my_data,
    VkDeviceMemory  mem,
    VkDeviceSize    offset,
    VkDeviceSize    size)
 {
    auto mem_element = my_data->memObjMap.find(mem);
    if (mem_element != my_data->memObjMap.end()) {
        MemRange new_range;
        new_range.offset = offset;
        new_range.size = size;
        mem_element->second.memRange = new_range;
    }
}

VkBool32 deleteMemRanges(
    layer_data     *my_data,
    VkDeviceMemory  mem)
{
    VkBool32 skipCall = VK_FALSE;
    auto mem_element = my_data->memObjMap.find(mem);
    if (mem_element != my_data->memObjMap.end()) {
        if (!mem_element->second.memRange.size) {
            // Valid Usage: memory must currently be mapped
            skipCall = log_msg(my_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, (uint64_t)mem, __LINE__, MEMTRACK_INVALID_MAP, "MEM",
                               "Unmapping Memory without memory being mapped: mem obj %#" PRIxLEAST64, (uint64_t)mem);
        }
        mem_element->second.memRange.size = 0;
        if (mem_element->second.pData) {
            free(mem_element->second.pData);
            mem_element->second.pData = 0;
        }
    }
    return skipCall;
}

static char NoncoherentMemoryFillValue = 0xb;

void
initializeAndTrackMemory(
    layer_data      *my_data,
    VkDeviceMemory   mem,
    VkDeviceSize     size,
    void           **ppData)
{
    auto mem_element = my_data->memObjMap.find(mem);
    if (mem_element != my_data->memObjMap.end()) {
        mem_element->second.pDriverData = *ppData;
        uint32_t index = mem_element->second.allocInfo.memoryTypeIndex;
        if (memProps.memoryTypes[index].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) {
            mem_element->second.pData = 0;
        } else {
            if (size == VK_WHOLE_SIZE) {
                size = mem_element->second.allocInfo.allocationSize;
            }
            size_t convSize = (size_t)(size);
            mem_element->second.pData = malloc(2 * convSize);
            memset(mem_element->second.pData, NoncoherentMemoryFillValue, 2 * convSize);
            *ppData = static_cast<char*>(mem_element->second.pData) + (convSize / 2);
        }
    }
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkMapMemory(
    VkDevice         device,
    VkDeviceMemory   mem,
    VkDeviceSize     offset,
    VkDeviceSize     size,
    VkFlags          flags,
    void           **ppData)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkBool32 skipCall   = VK_FALSE;
    VkResult result     = VK_ERROR_VALIDATION_FAILED_EXT;
    loader_platform_thread_lock_mutex(&globalLock);
    MT_MEM_OBJ_INFO *pMemObj = get_mem_obj_info(my_data, mem);
    if (pMemObj) {
        pMemObj->valid = true;
        if ((memProps.memoryTypes[pMemObj->allocInfo.memoryTypeIndex].propertyFlags &
             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == 0) {
            skipCall = log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT,
                (uint64_t) mem, __LINE__, MEMTRACK_INVALID_STATE, "MEM",
                "Mapping Memory without VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT set: mem obj %#" PRIxLEAST64, (uint64_t) mem);
        }
    }
    skipCall |= validateMemRange(my_data, mem, offset, size);
    storeMemRanges(my_data, mem, offset, size);
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall) {
        result = my_data->device_dispatch_table->MapMemory(device, mem, offset, size, flags, ppData);
        initializeAndTrackMemory(my_data, mem, size, ppData);
    }
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkUnmapMemory(
    VkDevice       device,
    VkDeviceMemory mem)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkBool32 skipCall   = VK_FALSE;

    loader_platform_thread_lock_mutex(&globalLock);
    skipCall |= deleteMemRanges(my_data, mem);
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall) {
        my_data->device_dispatch_table->UnmapMemory(device, mem);
    }
}

VkBool32
validateMemoryIsMapped(
    layer_data                *my_data,
    uint32_t                   memRangeCount,
    const VkMappedMemoryRange *pMemRanges)
{
    VkBool32 skipCall = VK_FALSE;
    for (uint32_t i = 0; i < memRangeCount; ++i) {
        auto mem_element = my_data->memObjMap.find(pMemRanges[i].memory);
        if (mem_element != my_data->memObjMap.end()) {
            if (mem_element->second.memRange.offset > pMemRanges[i].offset ||
                (mem_element->second.memRange.offset + mem_element->second.memRange.size) < (pMemRanges[i].offset + pMemRanges[i].size)) {
                skipCall |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, (uint64_t)pMemRanges[i].memory,
                                    __LINE__, MEMTRACK_INVALID_MAP, "MEM", "Memory must be mapped before it can be flushed or invalidated.");
            }
        }
    }
    return skipCall;
}

VkBool32
validateAndCopyNoncoherentMemoryToDriver(
    layer_data                *my_data,
    uint32_t                   memRangeCount,
    const VkMappedMemoryRange *pMemRanges)
{
    VkBool32 skipCall = VK_FALSE;
    for (uint32_t i = 0; i < memRangeCount; ++i) {
        auto mem_element = my_data->memObjMap.find(pMemRanges[i].memory);
        if (mem_element != my_data->memObjMap.end()) {
            if (mem_element->second.pData) {
                VkDeviceSize size      = mem_element->second.memRange.size;
                VkDeviceSize half_size = (size / 2);
                char* data = static_cast<char*>(mem_element->second.pData);
                for (auto j = 0; j < half_size; ++j) {
                    if (data[j] != NoncoherentMemoryFillValue) {
                        skipCall |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, (uint64_t)pMemRanges[i].memory,
                                            __LINE__, MEMTRACK_INVALID_MAP, "MEM", "Memory overflow was detected on mem obj %" PRIxLEAST64, (uint64_t)pMemRanges[i].memory);
                    }
                }
                for (auto j = size + half_size; j < 2 * size; ++j) {
                    if (data[j] != NoncoherentMemoryFillValue) {
                        skipCall |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, (uint64_t)pMemRanges[i].memory,
                                            __LINE__, MEMTRACK_INVALID_MAP, "MEM", "Memory overflow was detected on mem obj %" PRIxLEAST64, (uint64_t)pMemRanges[i].memory);
                    }
                }
                memcpy(mem_element->second.pDriverData, static_cast<void*>(data + (size_t)(half_size)), (size_t)(size));
            }
        }
    }
    return skipCall;
}

VK_LAYER_EXPORT VkResult VKAPI_CALL vkFlushMappedMemoryRanges(
    VkDevice                   device,
    uint32_t                   memRangeCount,
    const VkMappedMemoryRange *pMemRanges)
{
    VkResult    result    = VK_ERROR_VALIDATION_FAILED_EXT;
    VkBool32    skipCall  = VK_FALSE;
    layer_data *my_data   = get_my_data_ptr(get_dispatch_key(device), layer_data_map);

    loader_platform_thread_lock_mutex(&globalLock);
    skipCall  |= validateAndCopyNoncoherentMemoryToDriver(my_data, memRangeCount, pMemRanges);
    skipCall  |= validateMemoryIsMapped(my_data, memRangeCount, pMemRanges);
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall ) {
        result = my_data->device_dispatch_table->FlushMappedMemoryRanges(device, memRangeCount, pMemRanges);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI_CALL vkInvalidateMappedMemoryRanges(
    VkDevice                   device,
    uint32_t                   memRangeCount,
    const VkMappedMemoryRange *pMemRanges)
{
    VkResult    result    = VK_ERROR_VALIDATION_FAILED_EXT;
    VkBool32    skipCall = VK_FALSE;
    layer_data *my_data   = get_my_data_ptr(get_dispatch_key(device), layer_data_map);

    loader_platform_thread_lock_mutex(&globalLock);
    skipCall |= validateMemoryIsMapped(my_data, memRangeCount, pMemRanges);
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall) {
        result = my_data->device_dispatch_table->InvalidateMappedMemoryRanges(device, memRangeCount, pMemRanges);
    }
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyFence(
    VkDevice                     device,
    VkFence                      fence,
    const VkAllocationCallbacks *pAllocator)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    delete_fence_info(my_data, fence);
    auto item = my_data->fenceMap.find(fence);
    if (item != my_data->fenceMap.end()) {
        my_data->fenceMap.erase(item);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    my_data->device_dispatch_table->DestroyFence(device, fence, pAllocator);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyBuffer(
    VkDevice                     device,
    VkBuffer                     buffer,
    const VkAllocationCallbacks *pAllocator)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&globalLock);
    auto item = my_data->bufferMap.find((uint64_t)buffer);
    if (item != my_data->bufferMap.end()) {
        skipCall = clear_object_binding(my_data, device, (uint64_t)buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT);
        my_data->bufferMap.erase(item);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall) {
        my_data->device_dispatch_table->DestroyBuffer(device, buffer, pAllocator);
    }
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyImage(
    VkDevice                     device,
    VkImage                      image,
    const VkAllocationCallbacks *pAllocator)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&globalLock);
    auto item = my_data->imageMap.find((uint64_t)image);
    if (item != my_data->imageMap.end()) {
        skipCall = clear_object_binding(my_data, device, (uint64_t)image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT);
        my_data->imageMap.erase(item);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall) {
        my_data->device_dispatch_table->DestroyImage(device, image, pAllocator);
    }
}

VkBool32 print_memory_range_error(layer_data *my_data, const uint64_t object_handle, const uint64_t other_handle, VkDebugReportObjectTypeEXT object_type) {
    if (object_type == VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT) {
        return log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, object_type, object_handle, 0, MEMTRACK_INVALID_ALIASING, "MEM",
                       "Buffer %" PRIx64 " is alised with image %" PRIx64, object_handle, other_handle);
    } else {
        return log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, object_type, object_handle, 0, MEMTRACK_INVALID_ALIASING, "MEM",
                       "Image %" PRIx64 " is alised with buffer %" PRIx64, object_handle, other_handle);
    }
}

VkBool32 validate_memory_range(layer_data *my_data, const unordered_map<VkDeviceMemory, vector<MEMORY_RANGE>>& memory, const MEMORY_RANGE& new_range, VkDebugReportObjectTypeEXT object_type) {
    VkBool32 skip_call = false;
    if (!memory.count(new_range.memory)) return false;
    const vector<MEMORY_RANGE>& ranges = memory.at(new_range.memory);
    for (auto range : ranges) {
        if ((range.end & ~(my_data->properties.limits.bufferImageGranularity - 1)) < new_range.start) continue;
        if (range.start > (new_range.end & ~(my_data->properties.limits.bufferImageGranularity - 1))) continue;
        skip_call |= print_memory_range_error(my_data, new_range.handle, range.handle, object_type);
    }
    return skip_call;
}

VkBool32 validate_buffer_image_aliasing(
    layer_data *my_data,
    uint64_t handle,
    VkDeviceMemory mem,
    VkDeviceSize memoryOffset,
    VkMemoryRequirements memRequirements,
    unordered_map<VkDeviceMemory, vector<MEMORY_RANGE>>& ranges,
    const unordered_map<VkDeviceMemory, vector<MEMORY_RANGE>>& other_ranges,
    VkDebugReportObjectTypeEXT object_type)
{
    MEMORY_RANGE range;
    range.handle = handle;
    range.memory = mem;
    range.start = memoryOffset;
    range.end = memoryOffset + memRequirements.size - 1;
    ranges[mem].push_back(range);
    return validate_memory_range(my_data, other_ranges, range, object_type);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkBindBufferMemory(
    VkDevice       device,
    VkBuffer       buffer,
    VkDeviceMemory mem,
    VkDeviceSize   memoryOffset)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    loader_platform_thread_lock_mutex(&globalLock);
    // Track objects tied to memory
    uint64_t buffer_handle = (uint64_t)(buffer);
    VkBool32 skipCall = set_mem_binding(my_data, device, mem, buffer_handle, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, "vkBindBufferMemory");
    add_object_binding_info(my_data, buffer_handle, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, mem);
    {
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device, buffer, &memRequirements);
        skipCall |= validate_buffer_image_aliasing(my_data, buffer_handle, mem, memoryOffset, memRequirements, my_data->bufferRanges, my_data->imageRanges, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT);
    }
    print_mem_list(my_data, device);
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall) {
        result = my_data->device_dispatch_table->BindBufferMemory(device, buffer, mem, memoryOffset);
    }
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkBindImageMemory(
    VkDevice       device,
    VkImage        image,
    VkDeviceMemory mem,
    VkDeviceSize   memoryOffset)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    loader_platform_thread_lock_mutex(&globalLock);
    // Track objects tied to memory
    uint64_t image_handle = (uint64_t)(image);
    VkBool32 skipCall = set_mem_binding(my_data, device, mem, image_handle, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, "vkBindImageMemory");
    add_object_binding_info(my_data, image_handle, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, mem);
    {
        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device, image, &memRequirements);
        skipCall |= validate_buffer_image_aliasing(my_data, image_handle, mem, memoryOffset, memRequirements, my_data->imageRanges, my_data->bufferRanges, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT);
    }
    print_mem_list(my_data, device);
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall) {
        result = my_data->device_dispatch_table->BindImageMemory(device, image, mem, memoryOffset);
    }
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetBufferMemoryRequirements(
    VkDevice              device,
    VkBuffer              buffer,
    VkMemoryRequirements *pMemoryRequirements)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    // TODO : What to track here?
    //   Could potentially save returned mem requirements and validate values passed into BindBufferMemory
    my_data->device_dispatch_table->GetBufferMemoryRequirements(device, buffer, pMemoryRequirements);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetImageMemoryRequirements(
    VkDevice              device,
    VkImage               image,
    VkMemoryRequirements *pMemoryRequirements)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    // TODO : What to track here?
    //   Could potentially save returned mem requirements and validate values passed into BindImageMemory
    my_data->device_dispatch_table->GetImageMemoryRequirements(device, image, pMemoryRequirements);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkQueueBindSparse(
    VkQueue                 queue,
    uint32_t                bindInfoCount,
    const VkBindSparseInfo *pBindInfo,
    VkFence                 fence)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(queue), layer_data_map);
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    VkBool32 skipCall = VK_FALSE;

    loader_platform_thread_lock_mutex(&globalLock);

    for (uint32_t i = 0; i < bindInfoCount; i++) {
        // Track objects tied to memory
        for (uint32_t j = 0; j < pBindInfo[i].bufferBindCount; j++) {
            for (uint32_t k = 0; k < pBindInfo[i].pBufferBinds[j].bindCount; k++) {
                if (set_sparse_mem_binding(my_data, queue,
                            pBindInfo[i].pBufferBinds[j].pBinds[k].memory,
                            (uint64_t) pBindInfo[i].pBufferBinds[j].buffer,
                            VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, "vkQueueBindSparse"))
                    skipCall = VK_TRUE;
            }
        }
        for (uint32_t j = 0; j < pBindInfo[i].imageOpaqueBindCount; j++) {
            for (uint32_t k = 0; k < pBindInfo[i].pImageOpaqueBinds[j].bindCount; k++) {
                if (set_sparse_mem_binding(my_data, queue,
                            pBindInfo[i].pImageOpaqueBinds[j].pBinds[k].memory,
                            (uint64_t) pBindInfo[i].pImageOpaqueBinds[j].image,
                            VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, "vkQueueBindSparse"))
                    skipCall = VK_TRUE;
            }
        }
        for (uint32_t j = 0; j < pBindInfo[i].imageBindCount; j++) {
            for (uint32_t k = 0; k < pBindInfo[i].pImageBinds[j].bindCount; k++) {
                if (set_sparse_mem_binding(my_data, queue,
                            pBindInfo[i].pImageBinds[j].pBinds[k].memory,
                            (uint64_t) pBindInfo[i].pImageBinds[j].image,
                            VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, "vkQueueBindSparse"))
                    skipCall = VK_TRUE;
            }
        }
    }

    print_mem_list(my_data, queue);
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall) {
        result = my_data->device_dispatch_table->QueueBindSparse(queue, bindInfoCount, pBindInfo, fence);
    }
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateFence(
    VkDevice                     device,
    const VkFenceCreateInfo     *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkFence                     *pFence)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = my_data->device_dispatch_table->CreateFence(device, pCreateInfo, pAllocator, pFence);
    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        MT_FENCE_INFO* pFI = &my_data->fenceMap[*pFence];
        memset(pFI, 0, sizeof(MT_FENCE_INFO));
        memcpy(&(pFI->createInfo), pCreateInfo, sizeof(VkFenceCreateInfo));
        if (pCreateInfo->flags & VK_FENCE_CREATE_SIGNALED_BIT) {
            pFI->firstTimeFlag = VK_TRUE;
        }
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkResetFences(
    VkDevice       device,
    uint32_t       fenceCount,
    const VkFence *pFences)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result   = VK_ERROR_VALIDATION_FAILED_EXT;
    VkBool32 skipCall = VK_FALSE;

    loader_platform_thread_lock_mutex(&globalLock);
    // Reset fence state in fenceCreateInfo structure
    for (uint32_t i = 0; i < fenceCount; i++) {
        auto fence_item = my_data->fenceMap.find(pFences[i]);
        if (fence_item != my_data->fenceMap.end()) {
            // Validate fences in SIGNALED state
            if (!(fence_item->second.createInfo.flags & VK_FENCE_CREATE_SIGNALED_BIT)) {
                // TODO: I don't see a Valid Usage section for ResetFences. This behavior should be documented there.
                skipCall = log_msg(my_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT, (uint64_t) pFences[i], __LINE__, MEMTRACK_INVALID_FENCE_STATE, "MEM",
                        "Fence %#" PRIxLEAST64 " submitted to VkResetFences in UNSIGNALED STATE", (uint64_t) pFences[i]);
            }
            else {
                fence_item->second.createInfo.flags =
                    static_cast<VkFenceCreateFlags>(fence_item->second.createInfo.flags & ~VK_FENCE_CREATE_SIGNALED_BIT);
            }
        }
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall) {
        result = my_data->device_dispatch_table->ResetFences(device, fenceCount, pFences);
    }
    return result;
}

static inline VkBool32
verifyFenceStatus(
    VkDevice    device,
    VkFence     fence,
    const char *apiCall)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkBool32 skipCall = VK_FALSE;
    auto pFenceInfo = my_data->fenceMap.find(fence);
    if (pFenceInfo != my_data->fenceMap.end()) {
        if (pFenceInfo->second.firstTimeFlag != VK_TRUE) {
            if ((pFenceInfo->second.createInfo.flags & VK_FENCE_CREATE_SIGNALED_BIT) && pFenceInfo->second.firstTimeFlag != VK_TRUE) {
                skipCall |= log_msg(my_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT, (uint64_t) fence, __LINE__, MEMTRACK_INVALID_FENCE_STATE, "MEM",
                    "%s specified fence %#" PRIxLEAST64 " already in SIGNALED state.", apiCall, (uint64_t) fence);
            }
            if (!pFenceInfo->second.queue &&
                !pFenceInfo->second
                     .swapchain) { // Checking status of unsubmitted fence
                skipCall |= log_msg(
                    my_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT,
                    VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT,
                    reinterpret_cast<uint64_t &>(fence),
                    __LINE__, MEMTRACK_INVALID_FENCE_STATE, "MEM",
                    "%s called for fence %#" PRIxLEAST64
                    " which has not been submitted on a Queue or during "
                    "acquire next image.",
                    apiCall, reinterpret_cast<uint64_t &>(fence));
            }
        } else {
            pFenceInfo->second.firstTimeFlag = VK_FALSE;
        }
    }
    return skipCall;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkGetFenceStatus(
    VkDevice device,
    VkFence  fence)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    VkBool32 skipCall = verifyFenceStatus(device, fence, "vkGetFenceStatus");
    loader_platform_thread_unlock_mutex(&globalLock);
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = my_data->device_dispatch_table->GetFenceStatus(device, fence);
    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        update_fence_tracking(my_data, fence);
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkWaitForFences(
    VkDevice       device,
    uint32_t       fenceCount,
    const VkFence *pFences,
    VkBool32       waitAll,
    uint64_t       timeout)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkBool32 skipCall = VK_FALSE;
    // Verify fence status of submitted fences
    loader_platform_thread_lock_mutex(&globalLock);
    for(uint32_t i = 0; i < fenceCount; i++) {
        skipCall |= verifyFenceStatus(device, pFences[i], "vkWaitForFences");
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = my_data->device_dispatch_table->WaitForFences(device, fenceCount, pFences, waitAll, timeout);

    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        if (waitAll || fenceCount == 1) { // Clear all the fences
            for(uint32_t i = 0; i < fenceCount; i++) {
                update_fence_tracking(my_data, pFences[i]);
            }
        }
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkQueueWaitIdle(
    VkQueue queue)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(queue), layer_data_map);
    VkResult result = my_data->device_dispatch_table->QueueWaitIdle(queue);
    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        retire_queue_fences(my_data, queue);
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkDeviceWaitIdle(
    VkDevice device)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = my_data->device_dispatch_table->DeviceWaitIdle(device);
    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        retire_device_fences(my_data, device);
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateBuffer(
    VkDevice                     device,
    const VkBufferCreateInfo    *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkBuffer                    *pBuffer)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = my_data->device_dispatch_table->CreateBuffer(device, pCreateInfo, pAllocator, pBuffer);
    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        add_object_create_info(my_data, (uint64_t)*pBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, pCreateInfo);
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateImage(
    VkDevice                     device,
    const VkImageCreateInfo     *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkImage                     *pImage)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = my_data->device_dispatch_table->CreateImage(device, pCreateInfo, pAllocator, pImage);
    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        add_object_create_info(my_data, (uint64_t)*pImage, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, pCreateInfo);
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateImageView(
    VkDevice                     device,
    const VkImageViewCreateInfo *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkImageView                 *pView)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = my_data->device_dispatch_table->CreateImageView(device, pCreateInfo, pAllocator, pView);
    if (result == VK_SUCCESS) {
        loader_platform_thread_lock_mutex(&globalLock);
        my_data->imageViewMap[*pView].image = pCreateInfo->image;
        // Validate that img has correct usage flags set
        validate_image_usage_flags(my_data, device, pCreateInfo->image,
                    VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                    VK_FALSE, "vkCreateImageView()", "VK_IMAGE_USAGE_[SAMPLED|STORAGE|COLOR_ATTACHMENT]_BIT");
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateBufferView(
    VkDevice                      device,
    const VkBufferViewCreateInfo *pCreateInfo,
    const VkAllocationCallbacks  *pAllocator,
    VkBufferView                 *pView)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = my_data->device_dispatch_table->CreateBufferView(device, pCreateInfo, pAllocator, pView);
    if (result == VK_SUCCESS) {
        loader_platform_thread_lock_mutex(&globalLock);
        // In order to create a valid buffer view, the buffer must have been created with at least one of the
        // following flags:  UNIFORM_TEXEL_BUFFER_BIT or STORAGE_TEXEL_BUFFER_BIT
        validate_buffer_usage_flags(my_data, device, pCreateInfo->buffer,
                    VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT,
                    VK_FALSE, "vkCreateBufferView()", "VK_BUFFER_USAGE_[STORAGE|UNIFORM]_TEXEL_BUFFER_BIT");
        my_data->bufferViewMap[*pView] = *pCreateInfo;
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL
vkDestroyBufferView(VkDevice device, VkBufferView bufferView,
                    const VkAllocationCallbacks *pAllocator) {
    layer_data *my_data =
        get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    my_data->device_dispatch_table->DestroyBufferView(device, bufferView,
                                                      pAllocator);
    loader_platform_thread_lock_mutex(&globalLock);
    auto item = my_data->bufferViewMap.find(bufferView);
    if (item != my_data->bufferViewMap.end()) {
        my_data->bufferViewMap.erase(item);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkAllocateCommandBuffers(
    VkDevice                           device,
    const VkCommandBufferAllocateInfo *pCreateInfo,
    VkCommandBuffer                   *pCommandBuffer)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult    result  = my_data->device_dispatch_table->AllocateCommandBuffers(device, pCreateInfo, pCommandBuffer);

    loader_platform_thread_lock_mutex(&globalLock);
    if (VK_SUCCESS == result) {
        for (uint32_t i = 0; i < pCreateInfo->commandBufferCount; i++) {
            add_cmd_buf_info(my_data, pCreateInfo->commandPool, pCommandBuffer[i]);
        }
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    printCBList(my_data, device);
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkFreeCommandBuffers(
    VkDevice               device,
    VkCommandPool          commandPool,
    uint32_t               commandBufferCount,
    const VkCommandBuffer *pCommandBuffers)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);

    loader_platform_thread_lock_mutex(&globalLock);
    for (uint32_t i = 0; i < commandBufferCount; i++) {
        skipCall |= delete_cmd_buf_info(my_data, commandPool, pCommandBuffers[i]);
    }
    printCBList(my_data, device);
    loader_platform_thread_unlock_mutex(&globalLock);

    if (VK_FALSE == skipCall) {
        my_data->device_dispatch_table->FreeCommandBuffers(device, commandPool, commandBufferCount, pCommandBuffers);
    }
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateCommandPool(
    VkDevice                       device,
    const VkCommandPoolCreateInfo *pCreateInfo,
    const VkAllocationCallbacks   *pAllocator,
    VkCommandPool                 *pCommandPool)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = my_data->device_dispatch_table->CreateCommandPool(device, pCreateInfo, pAllocator, pCommandPool);

    loader_platform_thread_lock_mutex(&globalLock);

    // Add cmd pool to map
    my_data->commandPoolMap[*pCommandPool].createFlags = pCreateInfo->flags;
    loader_platform_thread_unlock_mutex(&globalLock);

    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyCommandPool(
    VkDevice                     device,
    VkCommandPool                commandPool,
    const VkAllocationCallbacks *pAllocator)
{
    VkBool32 commandBufferComplete = VK_FALSE;
    VkBool32 skipCall              = VK_FALSE;
    // Verify that command buffers in pool are complete (not in-flight)
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    for (auto it = my_data->commandPoolMap[commandPool].pCommandBuffers.begin();
              it != my_data->commandPoolMap[commandPool].pCommandBuffers.end(); it++) {
        commandBufferComplete = VK_FALSE;
        skipCall = checkCBCompleted(my_data, *it, &commandBufferComplete);
        if (VK_FALSE == commandBufferComplete) {
            skipCall |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, (uint64_t)(*it), __LINE__,
                                MEMTRACK_RESET_CB_WHILE_IN_FLIGHT, "MEM", "Destroying Command Pool 0x%" PRIxLEAST64 " before "
                                "its command buffer (0x%" PRIxLEAST64 ") has completed.", (uint64_t)(commandPool),
                                reinterpret_cast<uint64_t>(*it));
        }
    }
    loader_platform_thread_unlock_mutex(&globalLock);

    if (VK_FALSE == skipCall) {
        my_data->device_dispatch_table->DestroyCommandPool(device, commandPool, pAllocator);
    }

    loader_platform_thread_lock_mutex(&globalLock);
    auto item = my_data->commandPoolMap[commandPool].pCommandBuffers.begin();
    // Remove command buffers from command buffer map
    while (item != my_data->commandPoolMap[commandPool].pCommandBuffers.end()) {
        auto del_item = item++;
        delete_cmd_buf_info(my_data, commandPool, *del_item);
    }
    my_data->commandPoolMap.erase(commandPool);
    loader_platform_thread_unlock_mutex(&globalLock);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkResetCommandPool(
    VkDevice                device,
    VkCommandPool           commandPool,
    VkCommandPoolResetFlags flags)
{
    layer_data *my_data               = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkBool32    commandBufferComplete = VK_FALSE;
    VkBool32    skipCall              = VK_FALSE;
    VkResult    result                = VK_ERROR_VALIDATION_FAILED_EXT;

    loader_platform_thread_lock_mutex(&globalLock);
    auto it = my_data->commandPoolMap[commandPool].pCommandBuffers.begin();
    // Verify that CB's in pool are complete (not in-flight)
    while (it != my_data->commandPoolMap[commandPool].pCommandBuffers.end()) {
        skipCall = checkCBCompleted(my_data, (*it), &commandBufferComplete);
        if (VK_FALSE == commandBufferComplete) {
            skipCall |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, (uint64_t)(*it), __LINE__,
                                MEMTRACK_RESET_CB_WHILE_IN_FLIGHT, "MEM", "Resetting CB %p before it has completed. You must check CB "
                                "flag before calling vkResetCommandBuffer().", (*it));
        } else {
            // Clear memory references at this point.
            skipCall |= clear_cmd_buf_and_mem_references(my_data, (*it));
        }
        ++it;
    }
    loader_platform_thread_unlock_mutex(&globalLock);

    if (VK_FALSE == skipCall) {
        result = my_data->device_dispatch_table->ResetCommandPool(device, commandPool, flags);
    }

    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkBeginCommandBuffer(
    VkCommandBuffer                 commandBuffer,
    const VkCommandBufferBeginInfo *pBeginInfo)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    VkResult result            = VK_ERROR_VALIDATION_FAILED_EXT;
    VkBool32 skipCall          = VK_FALSE;
    VkBool32 commandBufferComplete = VK_FALSE;
    loader_platform_thread_lock_mutex(&globalLock);

    // This implicitly resets the Cmd Buffer so make sure any fence is done and then clear memory references
    skipCall = checkCBCompleted(my_data, commandBuffer, &commandBufferComplete);

    if (VK_FALSE == commandBufferComplete) {
        skipCall |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, (uint64_t)commandBuffer, __LINE__,
                        MEMTRACK_RESET_CB_WHILE_IN_FLIGHT, "MEM", "Calling vkBeginCommandBuffer() on active CB %p before it has completed. "
                        "You must check CB flag before this call.", commandBuffer);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall) {
        result = my_data->device_dispatch_table->BeginCommandBuffer(commandBuffer, pBeginInfo);
    }
    loader_platform_thread_lock_mutex(&globalLock);
    clear_cmd_buf_and_mem_references(my_data, commandBuffer);
    loader_platform_thread_unlock_mutex(&globalLock);
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEndCommandBuffer(
    VkCommandBuffer commandBuffer)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    // TODO : Anything to do here?
    VkResult result = my_data->device_dispatch_table->EndCommandBuffer(commandBuffer);
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkResetCommandBuffer(
    VkCommandBuffer           commandBuffer,
    VkCommandBufferResetFlags flags)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    VkResult result            = VK_ERROR_VALIDATION_FAILED_EXT;
    VkBool32 skipCall          = VK_FALSE;
    VkBool32 commandBufferComplete = VK_FALSE;
    loader_platform_thread_lock_mutex(&globalLock);

    // Verify that CB is complete (not in-flight)
    skipCall = checkCBCompleted(my_data, commandBuffer, &commandBufferComplete);
    if (VK_FALSE == commandBufferComplete) {
        skipCall |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, (uint64_t)commandBuffer, __LINE__,
                        MEMTRACK_RESET_CB_WHILE_IN_FLIGHT, "MEM", "Resetting CB %p before it has completed. You must check CB "
                        "flag before calling vkResetCommandBuffer().", commandBuffer);
    }
    // Clear memory references as this point.
    skipCall |= clear_cmd_buf_and_mem_references(my_data, commandBuffer);
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall) {
        result = my_data->device_dispatch_table->ResetCommandBuffer(commandBuffer, flags);
    }
    return result;
}

// TODO : For any vkCmdBind* calls that include an object which has mem bound to it,
//    need to account for that mem now having binding to given commandBuffer
VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdBindPipeline(
    VkCommandBuffer     commandBuffer,
    VkPipelineBindPoint pipelineBindPoint,
    VkPipeline          pipeline)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
#if 0 // FIXME: NEED TO FIX THE FOLLOWING CODE AND REMOVE THIS #if 0
    // TODO : If memory bound to pipeline, then need to tie that mem to commandBuffer
    if (getPipeline(pipeline)) {
        MT_CB_INFO *pCBInfo = get_cmd_buf_info(my_data, commandBuffer);
        if (pCBInfo) {
            pCBInfo->pipelines[pipelineBindPoint] = pipeline;
        }
    }
    else {
                "Attempt to bind Pipeline %p that doesn't exist!", (void*)pipeline);
        layerCbMsg(VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT, pipeline, __LINE__, MEMTRACK_INVALID_OBJECT, (char *) "DS", (char *) str);
    }
#endif
    my_data->device_dispatch_table->CmdBindPipeline(commandBuffer, pipelineBindPoint, pipeline);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdBindDescriptorSets(
    VkCommandBuffer        commandBuffer,
    VkPipelineBindPoint    pipelineBindPoint,
    VkPipelineLayout       layout,
    uint32_t               firstSet,
    uint32_t               setCount,
    const VkDescriptorSet *pDescriptorSets,
    uint32_t               dynamicOffsetCount,
    const uint32_t        *pDynamicOffsets)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    auto cb_data = my_data->cbMap.find(commandBuffer);
    if (cb_data != my_data->cbMap.end()) {
        std::vector<VkDescriptorSet>& activeDescriptorSets = cb_data->second.activeDescriptorSets;
        if (activeDescriptorSets.size() < (setCount + firstSet)) {
            activeDescriptorSets.resize(setCount + firstSet);
        }
        for (uint32_t i = 0; i < setCount; ++i) {
            activeDescriptorSets[i + firstSet] = pDescriptorSets[i];
        }
    }
    // TODO : Somewhere need to verify that all textures referenced by shaders in DS are in some type of *SHADER_READ* state
    my_data->device_dispatch_table->CmdBindDescriptorSets(
        commandBuffer, pipelineBindPoint, layout, firstSet, setCount, pDescriptorSets, dynamicOffsetCount, pDynamicOffsets);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdBindVertexBuffers(
    VkCommandBuffer     commandBuffer,
    uint32_t            firstBinding,
    uint32_t            bindingCount,
    const VkBuffer     *pBuffers,
    const VkDeviceSize *pOffsets)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    VkBool32 skip_call = false;
    loader_platform_thread_lock_mutex(&globalLock);
    for (uint32_t i = 0; i < bindingCount; ++i) {
        VkDeviceMemory mem;
        skip_call |= get_mem_binding_from_object(my_data, commandBuffer, (uint64_t)(pBuffers[i]),
            VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, &mem);
        auto cb_data = my_data->cbMap.find(commandBuffer);
        if (cb_data != my_data->cbMap.end()) {
            std::function<VkBool32()> function = [=]() { return validate_memory_is_valid(my_data, mem, "vkCmdBindVertexBuffers()"); };
            cb_data->second.validate_functions.push_back(function);
        }
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    // TODO : Somewhere need to verify that VBs have correct usage state flagged
    if (!skip_call)
        my_data->device_dispatch_table->CmdBindVertexBuffers(commandBuffer, firstBinding, bindingCount, pBuffers, pOffsets);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdBindIndexBuffer(
    VkCommandBuffer commandBuffer,
    VkBuffer        buffer,
    VkDeviceSize    offset,
    VkIndexType     indexType)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    VkDeviceMemory mem;
    loader_platform_thread_lock_mutex(&globalLock);
    VkBool32 skip_call = get_mem_binding_from_object(my_data, commandBuffer, (uint64_t)(buffer), VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, &mem);
    auto cb_data = my_data->cbMap.find(commandBuffer);
    if (cb_data != my_data->cbMap.end()) {
        std::function<VkBool32()> function = [=]() { return validate_memory_is_valid(my_data, mem, "vkCmdBindIndexBuffer()"); };
        cb_data->second.validate_functions.push_back(function);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    // TODO : Somewhere need to verify that IBs have correct usage state flagged
    if (!skip_call)
        my_data->device_dispatch_table->CmdBindIndexBuffer(commandBuffer, buffer, offset, indexType);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkUpdateDescriptorSets(
    VkDevice                                    device,
    uint32_t                                    descriptorWriteCount,
    const VkWriteDescriptorSet*                 pDescriptorWrites,
    uint32_t                                    descriptorCopyCount,
    const VkCopyDescriptorSet*                  pDescriptorCopies)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    uint32_t j = 0;
    for (uint32_t i = 0; i < descriptorWriteCount; ++i) {
        if (pDescriptorWrites[i].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE) {
            for (j = 0; j < pDescriptorWrites[i].descriptorCount; ++j) {
                my_data->descriptorSetMap[pDescriptorWrites[i].dstSet]
                    .images.push_back(
                        pDescriptorWrites[i].pImageInfo[j].imageView);
            }
        } else if (pDescriptorWrites[i].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER ) {
            for (j = 0; j < pDescriptorWrites[i].descriptorCount; ++j) {
                my_data->descriptorSetMap[pDescriptorWrites[i].dstSet]
                    .buffers.push_back(
                        my_data
                            ->bufferViewMap[pDescriptorWrites[i]
                                                .pTexelBufferView[j]]
                            .buffer);
            }
        } else if (pDescriptorWrites[i].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ||
                   pDescriptorWrites[i].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC) {
            for (j = 0; j < pDescriptorWrites[i].descriptorCount; ++j) {
                my_data->descriptorSetMap[pDescriptorWrites[i].dstSet]
                    .buffers.push_back(
                        pDescriptorWrites[i].pBufferInfo[j].buffer);
            }
        }
    }
    // TODO : Need to handle descriptor copies. Will wait on this until merge w/
    // draw_state
    my_data->device_dispatch_table->UpdateDescriptorSets(device, descriptorWriteCount, pDescriptorWrites, descriptorCopyCount, pDescriptorCopies);
}

bool markStoreImagesAndBuffersAsWritten(
    VkCommandBuffer commandBuffer)
{
    bool skip_call = false;
    loader_platform_thread_lock_mutex(&globalLock);
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    auto cb_data = my_data->cbMap.find(commandBuffer);
    if (cb_data == my_data->cbMap.end()) return skip_call;
    std::vector<VkDescriptorSet>& activeDescriptorSets = cb_data->second.activeDescriptorSets;
    for (auto descriptorSet : activeDescriptorSets) {
        auto ds_data = my_data->descriptorSetMap.find(descriptorSet);
        if (ds_data == my_data->descriptorSetMap.end()) continue;
        std::vector<VkImageView> images = ds_data->second.images;
        std::vector<VkBuffer> buffers = ds_data->second.buffers;
        for (auto imageView : images) {
            auto iv_data = my_data->imageViewMap.find(imageView);
            if (iv_data == my_data->imageViewMap.end()) continue;
            VkImage image = iv_data->second.image;
            VkDeviceMemory mem;
            skip_call |= get_mem_binding_from_object(my_data, commandBuffer, (uint64_t)image, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, &mem);
            std::function<VkBool32()> function = [=]() { set_memory_valid(my_data, mem, true, image); return VK_FALSE; };
            cb_data->second.validate_functions.push_back(function);
        }
        for (auto buffer : buffers) {
            VkDeviceMemory mem;
            skip_call |= get_mem_binding_from_object(my_data, commandBuffer, (uint64_t)buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, &mem);
            std::function<VkBool32()> function = [=]() { set_memory_valid(my_data, mem, true); return VK_FALSE; };
            cb_data->second.validate_functions.push_back(function);
        }
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    return skip_call;
}

VKAPI_ATTR void VKAPI_CALL vkCmdDraw(
    VkCommandBuffer                             commandBuffer,
    uint32_t                                    vertexCount,
    uint32_t                                    instanceCount,
    uint32_t                                    firstVertex,
    uint32_t                                    firstInstance)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    bool skip_call = markStoreImagesAndBuffersAsWritten(commandBuffer);
    if (!skip_call)
        my_data->device_dispatch_table->CmdDraw(commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
}

VKAPI_ATTR void VKAPI_CALL vkCmdDrawIndexed(
    VkCommandBuffer                             commandBuffer,
    uint32_t                                    indexCount,
    uint32_t                                    instanceCount,
    uint32_t                                    firstIndex,
    int32_t                                     vertexOffset,
    uint32_t                                    firstInstance)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    bool skip_call = markStoreImagesAndBuffersAsWritten(commandBuffer);
    if (!skip_call)
        my_data->device_dispatch_table->CmdDrawIndexed(commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdDrawIndirect(
    VkCommandBuffer commandBuffer,
     VkBuffer       buffer,
     VkDeviceSize   offset,
     uint32_t       count,
     uint32_t       stride)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    VkDeviceMemory mem;
    loader_platform_thread_lock_mutex(&globalLock);
    VkBool32 skipCall  = get_mem_binding_from_object(my_data, commandBuffer, (uint64_t)buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, &mem);
    skipCall          |= update_cmd_buf_and_mem_references(my_data, commandBuffer, mem, "vkCmdDrawIndirect");
    skipCall |= markStoreImagesAndBuffersAsWritten(commandBuffer);
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall) {
        my_data->device_dispatch_table->CmdDrawIndirect(commandBuffer, buffer, offset, count, stride);
    }
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdDrawIndexedIndirect(
    VkCommandBuffer commandBuffer,
    VkBuffer        buffer,
    VkDeviceSize    offset,
    uint32_t        count,
    uint32_t        stride)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    VkDeviceMemory mem;
    loader_platform_thread_lock_mutex(&globalLock);
    VkBool32 skipCall = get_mem_binding_from_object(my_data, commandBuffer, (uint64_t)buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, &mem);
    skipCall         |= update_cmd_buf_and_mem_references(my_data, commandBuffer, mem, "vkCmdDrawIndexedIndirect");
    skipCall |= markStoreImagesAndBuffersAsWritten(commandBuffer);
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall) {
        my_data->device_dispatch_table->CmdDrawIndexedIndirect(commandBuffer, buffer, offset, count, stride);
    }
}


VKAPI_ATTR void VKAPI_CALL vkCmdDispatch(
    VkCommandBuffer                             commandBuffer,
    uint32_t                                    x,
    uint32_t                                    y,
    uint32_t                                    z)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    bool skip_call = markStoreImagesAndBuffersAsWritten(commandBuffer);
    if (!skip_call)
        my_data->device_dispatch_table->CmdDispatch(commandBuffer, x, y, z);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdDispatchIndirect(
    VkCommandBuffer commandBuffer,
    VkBuffer        buffer,
    VkDeviceSize    offset)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    VkDeviceMemory mem;
    loader_platform_thread_lock_mutex(&globalLock);
    VkBool32 skipCall = get_mem_binding_from_object(my_data, commandBuffer, (uint64_t)buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, &mem);
    skipCall         |= update_cmd_buf_and_mem_references(my_data, commandBuffer, mem, "vkCmdDispatchIndirect");
    skipCall |= markStoreImagesAndBuffersAsWritten(commandBuffer);
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall) {
        my_data->device_dispatch_table->CmdDispatchIndirect(commandBuffer, buffer, offset);
    }
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdCopyBuffer(
    VkCommandBuffer     commandBuffer,
    VkBuffer            srcBuffer,
    VkBuffer            dstBuffer,
    uint32_t            regionCount,
    const VkBufferCopy *pRegions)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    VkDeviceMemory mem;
    VkBool32       skipCall = VK_FALSE;
    auto cb_data = my_data->cbMap.find(commandBuffer);
    loader_platform_thread_lock_mutex(&globalLock);
    skipCall  = get_mem_binding_from_object(my_data, commandBuffer, (uint64_t)srcBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, &mem);
    if (cb_data != my_data->cbMap.end()) {
        std::function<VkBool32()> function = [=]() { return validate_memory_is_valid(my_data, mem, "vkCmdCopyBuffer()"); };
        cb_data->second.validate_functions.push_back(function);
    }
    skipCall |= update_cmd_buf_and_mem_references(my_data, commandBuffer, mem, "vkCmdCopyBuffer");
    skipCall |= get_mem_binding_from_object(my_data, commandBuffer, (uint64_t)dstBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, &mem);
    if (cb_data != my_data->cbMap.end()) {
        std::function<VkBool32()> function = [=]() { set_memory_valid(my_data, mem, true); return VK_FALSE; };
        cb_data->second.validate_functions.push_back(function);
    }
    skipCall |= update_cmd_buf_and_mem_references(my_data, commandBuffer, mem, "vkCmdCopyBuffer");
    // Validate that SRC & DST buffers have correct usage flags set
    skipCall |= validate_buffer_usage_flags(my_data, commandBuffer, srcBuffer, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, true, "vkCmdCopyBuffer()", "VK_BUFFER_USAGE_TRANSFER_SRC_BIT");
    skipCall |= validate_buffer_usage_flags(my_data, commandBuffer, dstBuffer, VK_BUFFER_USAGE_TRANSFER_DST_BIT, true, "vkCmdCopyBuffer()", "VK_BUFFER_USAGE_TRANSFER_DST_BIT");
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall) {
        my_data->device_dispatch_table->CmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, regionCount, pRegions);
    }
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdCopyQueryPoolResults(
    VkCommandBuffer    commandBuffer,
    VkQueryPool        queryPool,
    uint32_t           firstQuery,
    uint32_t           queryCount,
    VkBuffer           dstBuffer,
    VkDeviceSize       dstOffset,
    VkDeviceSize       destStride,
    VkQueryResultFlags flags)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    VkDeviceMemory mem;
    VkBool32       skipCall = VK_FALSE;
    auto cb_data = my_data->cbMap.find(commandBuffer);
    loader_platform_thread_lock_mutex(&globalLock);
    skipCall |= get_mem_binding_from_object(my_data, commandBuffer, (uint64_t)dstBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, &mem);
    if (cb_data != my_data->cbMap.end()) {
        std::function<VkBool32()> function = [=]() { set_memory_valid(my_data, mem, true); return VK_FALSE; };
        cb_data->second.validate_functions.push_back(function);
    }
    skipCall |= update_cmd_buf_and_mem_references(my_data, commandBuffer, mem, "vkCmdCopyQueryPoolResults");
    // Validate that DST buffer has correct usage flags set
    skipCall |= validate_buffer_usage_flags(my_data, commandBuffer, dstBuffer, VK_BUFFER_USAGE_TRANSFER_DST_BIT, true, "vkCmdCopyQueryPoolResults()", "VK_BUFFER_USAGE_TRANSFER_DST_BIT");
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall) {
        my_data->device_dispatch_table->CmdCopyQueryPoolResults(commandBuffer, queryPool, firstQuery, queryCount, dstBuffer, dstOffset, destStride, flags);
    }
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdCopyImage(
    VkCommandBuffer    commandBuffer,
    VkImage            srcImage,
    VkImageLayout      srcImageLayout,
    VkImage            dstImage,
    VkImageLayout      dstImageLayout,
    uint32_t           regionCount,
    const VkImageCopy *pRegions)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    VkDeviceMemory mem;
    VkBool32       skipCall = VK_FALSE;
    auto cb_data = my_data->cbMap.find(commandBuffer);
    loader_platform_thread_lock_mutex(&globalLock);
    // Validate that src & dst images have correct usage flags set
    skipCall  = get_mem_binding_from_object(my_data, commandBuffer, (uint64_t)srcImage, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, &mem);
    if (cb_data != my_data->cbMap.end()) {
        std::function<VkBool32()> function = [=]() { return validate_memory_is_valid(my_data, mem, "vkCmdCopyImage()", srcImage); };
        cb_data->second.validate_functions.push_back(function);
    }
    skipCall |= update_cmd_buf_and_mem_references(my_data, commandBuffer, mem, "vkCmdCopyImage");
    skipCall |= get_mem_binding_from_object(my_data, commandBuffer, (uint64_t)dstImage, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, &mem);
    if (cb_data != my_data->cbMap.end()) {
        std::function<VkBool32()> function = [=]() { set_memory_valid(my_data, mem, true, dstImage); return VK_FALSE; };
        cb_data->second.validate_functions.push_back(function);
    }
    skipCall |= update_cmd_buf_and_mem_references(my_data, commandBuffer, mem, "vkCmdCopyImage");
    skipCall |= validate_image_usage_flags(my_data, commandBuffer, srcImage, VK_IMAGE_USAGE_TRANSFER_SRC_BIT, true, "vkCmdCopyImage()", "VK_IMAGE_USAGE_TRANSFER_SRC_BIT");
    skipCall |= validate_image_usage_flags(my_data, commandBuffer, dstImage, VK_IMAGE_USAGE_TRANSFER_DST_BIT, true, "vkCmdCopyImage()", "VK_IMAGE_USAGE_TRANSFER_DST_BIT");
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall) {
        my_data->device_dispatch_table->CmdCopyImage(
            commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions);
    }
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdBlitImage(
    VkCommandBuffer    commandBuffer,
    VkImage            srcImage,
    VkImageLayout      srcImageLayout,
    VkImage            dstImage,
    VkImageLayout      dstImageLayout,
    uint32_t           regionCount,
    const VkImageBlit *pRegions,
    VkFilter           filter)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    VkDeviceMemory mem;
    VkBool32       skipCall = VK_FALSE;
    auto cb_data = my_data->cbMap.find(commandBuffer);
    loader_platform_thread_lock_mutex(&globalLock);
    // Validate that src & dst images have correct usage flags set
    skipCall  = get_mem_binding_from_object(my_data, commandBuffer, (uint64_t)srcImage, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, &mem);
    if (cb_data != my_data->cbMap.end()) {
        std::function<VkBool32()> function = [=]() { return validate_memory_is_valid(my_data, mem, "vkCmdBlitImage()", srcImage); };
        cb_data->second.validate_functions.push_back(function);
    }
    skipCall |= update_cmd_buf_and_mem_references(my_data, commandBuffer, mem, "vkCmdBlitImage");
    skipCall |= get_mem_binding_from_object(my_data, commandBuffer, (uint64_t)dstImage, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, &mem);\
    if (cb_data != my_data->cbMap.end()) {
        std::function<VkBool32()> function = [=]() { set_memory_valid(my_data, mem, true, dstImage); return VK_FALSE; };
        cb_data->second.validate_functions.push_back(function);
    }
    skipCall |= update_cmd_buf_and_mem_references(my_data, commandBuffer, mem, "vkCmdBlitImage");
    skipCall |= validate_image_usage_flags(my_data, commandBuffer, srcImage, VK_IMAGE_USAGE_TRANSFER_SRC_BIT, true, "vkCmdBlitImage()", "VK_IMAGE_USAGE_TRANSFER_SRC_BIT");
    skipCall |= validate_image_usage_flags(my_data, commandBuffer, dstImage, VK_IMAGE_USAGE_TRANSFER_DST_BIT, true, "vkCmdBlitImage()", "VK_IMAGE_USAGE_TRANSFER_DST_BIT");
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall) {
        my_data->device_dispatch_table->CmdBlitImage(
            commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions, filter);
    }
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdCopyBufferToImage(
    VkCommandBuffer          commandBuffer,
    VkBuffer                 srcBuffer,
    VkImage                  dstImage,
    VkImageLayout            dstImageLayout,
    uint32_t                 regionCount,
    const VkBufferImageCopy *pRegions)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    VkDeviceMemory mem;
    VkBool32       skipCall = VK_FALSE;
    auto cb_data = my_data->cbMap.find(commandBuffer);
    loader_platform_thread_lock_mutex(&globalLock);
    skipCall  = get_mem_binding_from_object(my_data, commandBuffer, (uint64_t)dstImage, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, &mem);
    if (cb_data != my_data->cbMap.end()) {
        std::function<VkBool32()> function = [=]() { set_memory_valid(my_data, mem, true, dstImage); return VK_FALSE; };
        cb_data->second.validate_functions.push_back(function);
    }
    skipCall |= update_cmd_buf_and_mem_references(my_data, commandBuffer, mem, "vkCmdCopyBufferToImage");
    skipCall |= get_mem_binding_from_object(my_data, commandBuffer, (uint64_t)srcBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, &mem);
    if (cb_data != my_data->cbMap.end()) {
        std::function<VkBool32()> function = [=]() { return validate_memory_is_valid(my_data, mem, "vkCmdCopyBufferToImage()"); };
        cb_data->second.validate_functions.push_back(function);
    }
    skipCall |= update_cmd_buf_and_mem_references(my_data, commandBuffer, mem, "vkCmdCopyBufferToImage");
    // Validate that src buff & dst image have correct usage flags set
    skipCall |= validate_buffer_usage_flags(my_data, commandBuffer, srcBuffer, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, true, "vkCmdCopyBufferToImage()", "VK_BUFFER_USAGE_TRANSFER_SRC_BIT");
    skipCall |= validate_image_usage_flags(my_data, commandBuffer, dstImage, VK_IMAGE_USAGE_TRANSFER_DST_BIT, true, "vkCmdCopyBufferToImage()", "VK_IMAGE_USAGE_TRANSFER_DST_BIT");
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall) {
        my_data->device_dispatch_table->CmdCopyBufferToImage(
        commandBuffer, srcBuffer, dstImage, dstImageLayout, regionCount, pRegions);
    }
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdCopyImageToBuffer(
    VkCommandBuffer          commandBuffer,
    VkImage                  srcImage,
    VkImageLayout            srcImageLayout,
    VkBuffer                 dstBuffer,
    uint32_t                 regionCount,
    const VkBufferImageCopy *pRegions)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    VkDeviceMemory mem;
    VkBool32       skipCall = VK_FALSE;
    auto cb_data = my_data->cbMap.find(commandBuffer);
    loader_platform_thread_lock_mutex(&globalLock);
    skipCall  = get_mem_binding_from_object(my_data, commandBuffer, (uint64_t)srcImage, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, &mem);
    if (cb_data != my_data->cbMap.end()) {
        std::function<VkBool32()> function = [=]() { return validate_memory_is_valid(my_data, mem, "vkCmdCopyImageToBuffer()", srcImage); };
        cb_data->second.validate_functions.push_back(function);
    }
    skipCall |= update_cmd_buf_and_mem_references(my_data, commandBuffer, mem, "vkCmdCopyImageToBuffer");
    skipCall |= get_mem_binding_from_object(my_data, commandBuffer, (uint64_t)dstBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, &mem);
    if (cb_data != my_data->cbMap.end()) {
        std::function<VkBool32()> function = [=]() { set_memory_valid(my_data, mem, true); return VK_FALSE; };
        cb_data->second.validate_functions.push_back(function);
    }
    skipCall |= update_cmd_buf_and_mem_references(my_data, commandBuffer, mem, "vkCmdCopyImageToBuffer");
    // Validate that dst buff & src image have correct usage flags set
    skipCall |= validate_image_usage_flags(my_data, commandBuffer, srcImage, VK_IMAGE_USAGE_TRANSFER_SRC_BIT, true, "vkCmdCopyImageToBuffer()", "VK_IMAGE_USAGE_TRANSFER_SRC_BIT");
    skipCall |= validate_buffer_usage_flags(my_data, commandBuffer, dstBuffer, VK_BUFFER_USAGE_TRANSFER_DST_BIT, true, "vkCmdCopyImageToBuffer()", "VK_BUFFER_USAGE_TRANSFER_DST_BIT");
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall) {
        my_data->device_dispatch_table->CmdCopyImageToBuffer(
            commandBuffer, srcImage, srcImageLayout, dstBuffer, regionCount, pRegions);
    }
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdUpdateBuffer(
    VkCommandBuffer  commandBuffer,
    VkBuffer         dstBuffer,
    VkDeviceSize     dstOffset,
    VkDeviceSize     dataSize,
    const uint32_t  *pData)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    VkDeviceMemory mem;
    VkBool32       skipCall = VK_FALSE;
    auto cb_data = my_data->cbMap.find(commandBuffer);
    loader_platform_thread_lock_mutex(&globalLock);
    skipCall  = get_mem_binding_from_object(my_data, commandBuffer, (uint64_t)dstBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, &mem);
    if (cb_data != my_data->cbMap.end()) {
        std::function<VkBool32()> function = [=]() { set_memory_valid(my_data, mem, true); return VK_FALSE; };
        cb_data->second.validate_functions.push_back(function);
    }
    skipCall |= update_cmd_buf_and_mem_references(my_data, commandBuffer, mem, "vkCmdUpdateBuffer");
    // Validate that dst buff has correct usage flags set
    skipCall |= validate_buffer_usage_flags(my_data, commandBuffer, dstBuffer, VK_BUFFER_USAGE_TRANSFER_DST_BIT, true, "vkCmdUpdateBuffer()", "VK_BUFFER_USAGE_TRANSFER_DST_BIT");
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall) {
        my_data->device_dispatch_table->CmdUpdateBuffer(commandBuffer, dstBuffer, dstOffset, dataSize, pData);
    }
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdFillBuffer(
    VkCommandBuffer commandBuffer,
    VkBuffer        dstBuffer,
    VkDeviceSize    dstOffset,
    VkDeviceSize    size,
    uint32_t        data)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    VkDeviceMemory mem;
    VkBool32       skipCall = VK_FALSE;
    auto cb_data = my_data->cbMap.find(commandBuffer);
    loader_platform_thread_lock_mutex(&globalLock);
    skipCall  = get_mem_binding_from_object(my_data, commandBuffer, (uint64_t)dstBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, &mem);
    if (cb_data != my_data->cbMap.end()) {
        std::function<VkBool32()> function = [=]() { set_memory_valid(my_data, mem, true); return VK_FALSE; };
        cb_data->second.validate_functions.push_back(function);
    }
    skipCall |= update_cmd_buf_and_mem_references(my_data, commandBuffer, mem, "vkCmdFillBuffer");
    // Validate that dst buff has correct usage flags set
    skipCall |= validate_buffer_usage_flags(my_data, commandBuffer, dstBuffer, VK_BUFFER_USAGE_TRANSFER_DST_BIT, true, "vkCmdFillBuffer()", "VK_BUFFER_USAGE_TRANSFER_DST_BIT");
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall) {
        my_data->device_dispatch_table->CmdFillBuffer(commandBuffer, dstBuffer, dstOffset, size, data);
    }
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdClearColorImage(
    VkCommandBuffer                commandBuffer,
    VkImage                        image,
    VkImageLayout                  imageLayout,
    const VkClearColorValue       *pColor,
    uint32_t                       rangeCount,
    const VkImageSubresourceRange *pRanges)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    // TODO : Verify memory is in VK_IMAGE_STATE_CLEAR state
    VkDeviceMemory mem;
    VkBool32       skipCall = VK_FALSE;
    auto cb_data = my_data->cbMap.find(commandBuffer);
    loader_platform_thread_lock_mutex(&globalLock);
    skipCall  = get_mem_binding_from_object(my_data, commandBuffer, (uint64_t)image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, &mem);
    if (cb_data != my_data->cbMap.end()) {
        std::function<VkBool32()> function = [=]() { set_memory_valid(my_data, mem, true, image); return VK_FALSE; };
        cb_data->second.validate_functions.push_back(function);
    }
    skipCall |= update_cmd_buf_and_mem_references(my_data, commandBuffer, mem, "vkCmdClearColorImage");
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall) {
        my_data->device_dispatch_table->CmdClearColorImage(commandBuffer, image, imageLayout, pColor, rangeCount, pRanges);
    }
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdClearDepthStencilImage(
    VkCommandBuffer                 commandBuffer,
    VkImage                         image,
    VkImageLayout                   imageLayout,
    const VkClearDepthStencilValue *pDepthStencil,
    uint32_t                        rangeCount,
    const VkImageSubresourceRange  *pRanges)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    // TODO : Verify memory is in VK_IMAGE_STATE_CLEAR state
    VkDeviceMemory mem;
    VkBool32       skipCall = VK_FALSE;
    auto cb_data = my_data->cbMap.find(commandBuffer);
    loader_platform_thread_lock_mutex(&globalLock);
    skipCall  = get_mem_binding_from_object(my_data, commandBuffer, (uint64_t)image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, &mem);
    if (cb_data != my_data->cbMap.end()) {
        std::function<VkBool32()> function = [=]() { set_memory_valid(my_data, mem, true, image); return VK_FALSE; };
        cb_data->second.validate_functions.push_back(function);
    }
    skipCall |= update_cmd_buf_and_mem_references(my_data, commandBuffer, mem, "vkCmdClearDepthStencilImage");
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall) {
        my_data->device_dispatch_table->CmdClearDepthStencilImage(
            commandBuffer, image, imageLayout, pDepthStencil, rangeCount, pRanges);
    }
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdResolveImage(
    VkCommandBuffer       commandBuffer,
    VkImage               srcImage,
    VkImageLayout         srcImageLayout,
    VkImage               dstImage,
    VkImageLayout         dstImageLayout,
    uint32_t              regionCount,
    const VkImageResolve *pRegions)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    VkBool32 skipCall = VK_FALSE;
    auto cb_data = my_data->cbMap.find(commandBuffer);
    loader_platform_thread_lock_mutex(&globalLock);
    VkDeviceMemory mem;
    skipCall  = get_mem_binding_from_object(my_data, commandBuffer, (uint64_t)srcImage, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, &mem);
    if (cb_data != my_data->cbMap.end()) {
        std::function<VkBool32()> function = [=]() { return validate_memory_is_valid(my_data, mem, "vkCmdResolveImage()", srcImage); };
        cb_data->second.validate_functions.push_back(function);
    }
    skipCall |= update_cmd_buf_and_mem_references(my_data, commandBuffer, mem, "vkCmdResolveImage");
    skipCall |= get_mem_binding_from_object(my_data, commandBuffer, (uint64_t)dstImage, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, &mem);
    if (cb_data != my_data->cbMap.end()) {
        std::function<VkBool32()> function = [=]() { set_memory_valid(my_data, mem, true, dstImage); return VK_FALSE; };
        cb_data->second.validate_functions.push_back(function);
    }
    skipCall |= update_cmd_buf_and_mem_references(my_data, commandBuffer, mem, "vkCmdResolveImage");
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall) {
        my_data->device_dispatch_table->CmdResolveImage(
            commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions);
    }
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdBeginQuery(
    VkCommandBuffer commandBuffer,
    VkQueryPool     queryPool,
    uint32_t        slot,
    VkFlags         flags)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    my_data->device_dispatch_table->CmdBeginQuery(commandBuffer, queryPool, slot, flags);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdEndQuery(
    VkCommandBuffer commandBuffer,
    VkQueryPool     queryPool,
    uint32_t        slot)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    my_data->device_dispatch_table->CmdEndQuery(commandBuffer, queryPool, slot);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdResetQueryPool(
    VkCommandBuffer commandBuffer,
    VkQueryPool     queryPool,
    uint32_t        firstQuery,
    uint32_t        queryCount)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    my_data->device_dispatch_table->CmdResetQueryPool(commandBuffer, queryPool, firstQuery, queryCount);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugReportCallbackEXT(
        VkInstance                                      instance,
        const VkDebugReportCallbackCreateInfoEXT*       pCreateInfo,
        const VkAllocationCallbacks*                    pAllocator,
        VkDebugReportCallbackEXT*                       pMsgCallback)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    VkLayerInstanceDispatchTable *pTable = my_data->instance_dispatch_table;
    VkResult res = pTable->CreateDebugReportCallbackEXT(instance, pCreateInfo, pAllocator, pMsgCallback);
    if (res == VK_SUCCESS) {
        loader_platform_thread_lock_mutex(&globalLock);
        res = layer_create_msg_callback(my_data->report_data, pCreateInfo, pAllocator, pMsgCallback);
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return res;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyDebugReportCallbackEXT(
        VkInstance                                  instance,
        VkDebugReportCallbackEXT                 msgCallback,
        const VkAllocationCallbacks*                pAllocator)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    VkLayerInstanceDispatchTable *pTable = my_data->instance_dispatch_table;
    pTable->DestroyDebugReportCallbackEXT(instance, msgCallback, pAllocator);
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

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateSwapchainKHR(
    VkDevice                        device,
    const VkSwapchainCreateInfoKHR *pCreateInfo,
    const VkAllocationCallbacks    *pAllocator,
    VkSwapchainKHR                 *pSwapchain)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = my_data->device_dispatch_table->CreateSwapchainKHR(device, pCreateInfo, pAllocator, pSwapchain);

    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        add_swap_chain_info(my_data, *pSwapchain, pCreateInfo);
        loader_platform_thread_unlock_mutex(&globalLock);
    }

    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroySwapchainKHR(
    VkDevice                        device,
    VkSwapchainKHR                  swapchain,
    const VkAllocationCallbacks     *pAllocator)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&globalLock);
    if (my_data->swapchainMap.find(swapchain) != my_data->swapchainMap.end()) {
        MT_SWAP_CHAIN_INFO* pInfo = my_data->swapchainMap[swapchain];

        if (pInfo->images.size() > 0) {
            for (auto it = pInfo->images.begin(); it != pInfo->images.end(); it++) {
                skipCall = clear_object_binding(my_data, device, (uint64_t)*it, VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT);
                auto image_item = my_data->imageMap.find((uint64_t)*it);
                if (image_item != my_data->imageMap.end())
                    my_data->imageMap.erase(image_item);
            }
        }
        delete pInfo;
        my_data->swapchainMap.erase(swapchain);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall) {
        my_data->device_dispatch_table->DestroySwapchainKHR(device, swapchain, pAllocator);
    }
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkGetSwapchainImagesKHR(
    VkDevice        device,
    VkSwapchainKHR  swapchain,
    uint32_t       *pCount,
    VkImage        *pSwapchainImages)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = my_data->device_dispatch_table->GetSwapchainImagesKHR(device, swapchain, pCount, pSwapchainImages);

    loader_platform_thread_lock_mutex(&globalLock);
    if (result == VK_SUCCESS && pSwapchainImages != NULL) {
        const size_t count = *pCount;
        MT_SWAP_CHAIN_INFO *pInfo = my_data->swapchainMap[swapchain];

        if (pInfo->images.empty()) {
            pInfo->images.resize(count);
            memcpy(&pInfo->images[0], pSwapchainImages, sizeof(pInfo->images[0]) * count);

            if (pInfo->images.size() > 0) {
                for (std::vector<VkImage>::const_iterator it = pInfo->images.begin();
                     it != pInfo->images.end(); it++) {
                    // Add image object binding, then insert the new Mem Object and then bind it to created image
                    add_object_create_info(my_data, (uint64_t)*it, VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT, &pInfo->createInfo);
                }
            }
        } else {
            const size_t count = *pCount;
            MT_SWAP_CHAIN_INFO *pInfo = my_data->swapchainMap[swapchain];
            const VkBool32 mismatch = (pInfo->images.size() != count ||
                    memcmp(&pInfo->images[0], pSwapchainImages, sizeof(pInfo->images[0]) * count));

            if (mismatch) {
                // TODO: Verify against Valid Usage section of extension
                log_msg(my_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT, (uint64_t) swapchain, __LINE__, MEMTRACK_NONE, "SWAP_CHAIN",
                        "vkGetSwapchainInfoKHR(%" PRIu64 ", VK_SWAP_CHAIN_INFO_TYPE_PERSISTENT_IMAGES_KHR) returned mismatching data", (uint64_t)(swapchain));
            }
        }
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkAcquireNextImageKHR(
    VkDevice        device,
    VkSwapchainKHR  swapchain,
    uint64_t        timeout,
    VkSemaphore     semaphore,
    VkFence         fence,
    uint32_t       *pImageIndex)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result   = VK_ERROR_VALIDATION_FAILED_EXT;
    VkBool32 skipCall = VK_FALSE;

    loader_platform_thread_lock_mutex(&globalLock);
    if (my_data->semaphoreMap.find(semaphore) != my_data->semaphoreMap.end()) {
        if (my_data->semaphoreMap[semaphore] != MEMTRACK_SEMAPHORE_STATE_UNSET) {
            skipCall = log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT, (uint64_t)semaphore,
                               __LINE__, MEMTRACK_NONE, "SEMAPHORE",
                               "vkAcquireNextImageKHR: Semaphore must not be currently signaled or in a wait state");
        }
        my_data->semaphoreMap[semaphore] = MEMTRACK_SEMAPHORE_STATE_SIGNALLED;
    }
    auto fence_data = my_data->fenceMap.find(fence);
    if (fence_data != my_data->fenceMap.end()) {
        fence_data->second.swapchain = swapchain;
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall) {
        result = my_data->device_dispatch_table->AcquireNextImageKHR(device,
                                    swapchain, timeout, semaphore, fence, pImageIndex);
    }
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkQueuePresentKHR(
    VkQueue queue,
    const VkPresentInfoKHR* pPresentInfo)
{
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(queue), layer_data_map);
    VkBool32 skip_call = false;
    VkDeviceMemory mem;
    loader_platform_thread_lock_mutex(&globalLock);
    for (uint32_t i = 0; i < pPresentInfo->swapchainCount; ++i) {
        MT_SWAP_CHAIN_INFO *pInfo = my_data->swapchainMap[pPresentInfo->pSwapchains[i]];
        VkImage image = pInfo->images[pPresentInfo->pImageIndices[i]];
        skip_call |= get_mem_binding_from_object(my_data, queue, (uint64_t)(image), VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, &mem);
        skip_call |= validate_memory_is_valid(my_data, mem, "vkQueuePresentKHR()", image);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    if (!skip_call) {
        result = my_data->device_dispatch_table->QueuePresentKHR(queue, pPresentInfo);
    }

    loader_platform_thread_lock_mutex(&globalLock);
    for (uint32_t i = 0; i < pPresentInfo->waitSemaphoreCount; i++) {
        VkSemaphore sem = pPresentInfo->pWaitSemaphores[i];
        if (my_data->semaphoreMap.find(sem) != my_data->semaphoreMap.end()) {
            my_data->semaphoreMap[sem] = MEMTRACK_SEMAPHORE_STATE_UNSET;
        }
    }
    loader_platform_thread_unlock_mutex(&globalLock);

    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateSemaphore(
    VkDevice                     device,
    const VkSemaphoreCreateInfo *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkSemaphore                 *pSemaphore)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = my_data->device_dispatch_table->CreateSemaphore(device, pCreateInfo, pAllocator, pSemaphore);
    loader_platform_thread_lock_mutex(&globalLock);
    if (*pSemaphore != VK_NULL_HANDLE) {
        my_data->semaphoreMap[*pSemaphore] = MEMTRACK_SEMAPHORE_STATE_UNSET;
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroySemaphore(
    VkDevice                     device,
    VkSemaphore                  semaphore,
    const VkAllocationCallbacks *pAllocator)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    auto item = my_data->semaphoreMap.find(semaphore);
    if (item != my_data->semaphoreMap.end()) {
        my_data->semaphoreMap.erase(item);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    my_data->device_dispatch_table->DestroySemaphore(device, semaphore, pAllocator);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateFramebuffer(
    VkDevice device,
    const VkFramebufferCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkFramebuffer* pFramebuffer)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = my_data->device_dispatch_table->CreateFramebuffer(device, pCreateInfo, pAllocator, pFramebuffer);
    loader_platform_thread_lock_mutex(&globalLock);
    for (uint32_t i = 0; i < pCreateInfo->attachmentCount; ++i) {
        VkImageView view = pCreateInfo->pAttachments[i];
        auto view_data = my_data->imageViewMap.find(view);
        if (view_data == my_data->imageViewMap.end()) {
            continue;
        }
        MT_FB_ATTACHMENT_INFO fb_info;
        get_mem_binding_from_object(my_data, device, (uint64_t)(view_data->second.image), VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, &fb_info.mem);
        fb_info.image = view_data->second.image;
        my_data->fbMap[*pFramebuffer].attachments.push_back(fb_info);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    return result;
}

VKAPI_ATTR void VKAPI_CALL vkDestroyFramebuffer(
    VkDevice                                    device,
    VkFramebuffer                               framebuffer,
    const VkAllocationCallbacks*                pAllocator)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);

    loader_platform_thread_lock_mutex(&globalLock);
    auto item = my_data->fbMap.find(framebuffer);
    if (item != my_data->fbMap.end()) {
        my_data->fbMap.erase(framebuffer);
    }
    loader_platform_thread_unlock_mutex(&globalLock);

    my_data->device_dispatch_table->DestroyFramebuffer(device, framebuffer, pAllocator);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateRenderPass(
    VkDevice device,
    const VkRenderPassCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkRenderPass* pRenderPass)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = my_data->device_dispatch_table->CreateRenderPass(device, pCreateInfo, pAllocator, pRenderPass);
    loader_platform_thread_lock_mutex(&globalLock);
    for (uint32_t i = 0; i < pCreateInfo->attachmentCount; ++i) {
        VkAttachmentDescription desc = pCreateInfo->pAttachments[i];
        MT_PASS_ATTACHMENT_INFO pass_info;
        pass_info.load_op = desc.loadOp;
        pass_info.store_op = desc.storeOp;
        pass_info.attachment = i;
        my_data->passMap[*pRenderPass].attachments.push_back(pass_info);
    }
    //TODO: Maybe fill list and then copy instead of locking
    std::unordered_map<uint32_t, bool>& attachment_first_read = my_data->passMap[*pRenderPass].attachment_first_read;
    std::unordered_map<uint32_t, VkImageLayout>& attachment_first_layout = my_data->passMap[*pRenderPass].attachment_first_layout;
    for (uint32_t i = 0; i < pCreateInfo->subpassCount; ++i) {
        const VkSubpassDescription& subpass = pCreateInfo->pSubpasses[i];
        for (uint32_t j = 0; j < subpass.inputAttachmentCount; ++j) {
            uint32_t attachment = subpass.pInputAttachments[j].attachment;
            if (attachment_first_read.count(attachment)) continue;
            attachment_first_read.insert(std::make_pair(attachment, true));
            attachment_first_layout.insert(std::make_pair(attachment, subpass.pInputAttachments[j].layout));
        }
        for (uint32_t j = 0; j < subpass.colorAttachmentCount; ++j) {
            uint32_t attachment = subpass.pColorAttachments[j].attachment;
            if (attachment_first_read.count(attachment)) continue;
            attachment_first_read.insert(std::make_pair(attachment, false));
            attachment_first_layout.insert(std::make_pair(attachment, subpass.pColorAttachments[j].layout));
        }
        if (subpass.pDepthStencilAttachment && subpass.pDepthStencilAttachment->attachment != VK_ATTACHMENT_UNUSED) {
            uint32_t attachment = subpass.pDepthStencilAttachment->attachment;
            if (attachment_first_read.count(attachment)) continue;
            attachment_first_read.insert(std::make_pair(attachment, false));
            attachment_first_layout.insert(std::make_pair(attachment, subpass.pDepthStencilAttachment->layout));
        }
    }
    loader_platform_thread_unlock_mutex(&globalLock);

    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyRenderPass(
    VkDevice                     device,
    VkRenderPass                 renderPass,
    const VkAllocationCallbacks *pAllocator)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    my_data->device_dispatch_table->DestroyRenderPass(device, renderPass, pAllocator);

    loader_platform_thread_lock_mutex(&globalLock);
    my_data->passMap.erase(renderPass);
    loader_platform_thread_unlock_mutex(&globalLock);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdBeginRenderPass(
    VkCommandBuffer cmdBuffer,
    const VkRenderPassBeginInfo *pRenderPassBegin,
    VkSubpassContents contents)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    VkBool32 skip_call = false;
    if (pRenderPassBegin) {
        loader_platform_thread_lock_mutex(&globalLock);
        auto pass_data = my_data->passMap.find(pRenderPassBegin->renderPass);
        if (pass_data != my_data->passMap.end()) {
            MT_PASS_INFO& pass_info = pass_data->second;
            pass_info.fb = pRenderPassBegin->framebuffer;
            auto cb_data = my_data->cbMap.find(cmdBuffer);
            for (size_t i = 0; i < pass_info.attachments.size(); ++i) {
                MT_FB_ATTACHMENT_INFO& fb_info = my_data->fbMap[pass_info.fb].attachments[i];
                if (pass_info.attachments[i].load_op == VK_ATTACHMENT_LOAD_OP_CLEAR) {
                    if (cb_data != my_data->cbMap.end()) {
                        std::function<VkBool32()> function = [=]() { set_memory_valid(my_data, fb_info.mem, true, fb_info.image); return VK_FALSE; };
                        cb_data->second.validate_functions.push_back(function);
                    }
                    VkImageLayout& attachment_layout = pass_info.attachment_first_layout[pass_info.attachments[i].attachment];
                    if (attachment_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL ||
                        attachment_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
                        skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT,
                                             (uint64_t)(pRenderPassBegin->renderPass), __LINE__, MEMTRACK_INVALID_LAYOUT, "MEM",
                                             "Cannot clear attachment %d with invalid first layout %d.", pass_info.attachments[i].attachment, attachment_layout);
                    }
                } else if (pass_info.attachments[i].load_op == VK_ATTACHMENT_LOAD_OP_DONT_CARE) {
                    if (cb_data != my_data->cbMap.end()) {
                        std::function<VkBool32()> function = [=]() { set_memory_valid(my_data, fb_info.mem, false, fb_info.image); return VK_FALSE; };
                        cb_data->second.validate_functions.push_back(function);
                    }
                } else if (pass_info.attachments[i].load_op == VK_ATTACHMENT_LOAD_OP_LOAD) {
                    if (cb_data != my_data->cbMap.end()) {
                        std::function<VkBool32()> function = [=]() { return validate_memory_is_valid(my_data, fb_info.mem, "vkCmdBeginRenderPass()", fb_info.image); };
                        cb_data->second.validate_functions.push_back(function);
                    }
                }
                if (pass_info.attachment_first_read[pass_info.attachments[i].attachment]) {
                    if (cb_data != my_data->cbMap.end()) {
                        std::function<VkBool32()> function = [=]() { return validate_memory_is_valid(my_data, fb_info.mem, "vkCmdBeginRenderPass()", fb_info.image); };
                        cb_data->second.validate_functions.push_back(function);
                    }
                }
            }
            if (cb_data != my_data->cbMap.end()) {
                cb_data->second.pass = pRenderPassBegin->renderPass;
            }
        }
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    if (!skip_call)
        return my_data->device_dispatch_table->CmdBeginRenderPass(cmdBuffer, pRenderPassBegin, contents);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdEndRenderPass(
    VkCommandBuffer cmdBuffer)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    auto cb_data = my_data->cbMap.find(cmdBuffer);
    if (cb_data != my_data->cbMap.end()) {
        auto pass_data = my_data->passMap.find(cb_data->second.pass);
        if (pass_data != my_data->passMap.end()) {
            MT_PASS_INFO& pass_info = pass_data->second;
            for (size_t i = 0; i < pass_info.attachments.size(); ++i) {
                MT_FB_ATTACHMENT_INFO& fb_info = my_data->fbMap[pass_info.fb].attachments[i];
                if (pass_info.attachments[i].store_op == VK_ATTACHMENT_STORE_OP_STORE) {
                    if (cb_data != my_data->cbMap.end()) {
                        std::function<VkBool32()> function = [=]() { set_memory_valid(my_data, fb_info.mem, true, fb_info.image); return VK_FALSE; };
                        cb_data->second.validate_functions.push_back(function);
                    }
                } else if (pass_info.attachments[i].store_op == VK_ATTACHMENT_STORE_OP_DONT_CARE) {
                    if (cb_data != my_data->cbMap.end()) {
                        std::function<VkBool32()> function = [=]() { set_memory_valid(my_data, fb_info.mem, false, fb_info.image); return VK_FALSE; };
                        cb_data->second.validate_functions.push_back(function);
                    }
                }
            }
        }
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    my_data->device_dispatch_table->CmdEndRenderPass(cmdBuffer);
}

VK_LAYER_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr(
    VkDevice    dev,
    const char *funcName)
{
    if (!strcmp(funcName, "vkGetDeviceProcAddr"))
        return (PFN_vkVoidFunction) vkGetDeviceProcAddr;
    if (!strcmp(funcName, "vkDestroyDevice"))
        return (PFN_vkVoidFunction) vkDestroyDevice;
    if (!strcmp(funcName, "vkQueueSubmit"))
        return (PFN_vkVoidFunction) vkQueueSubmit;
    if (!strcmp(funcName, "vkAllocateMemory"))
        return (PFN_vkVoidFunction) vkAllocateMemory;
    if (!strcmp(funcName, "vkFreeMemory"))
        return (PFN_vkVoidFunction) vkFreeMemory;
    if (!strcmp(funcName, "vkMapMemory"))
        return (PFN_vkVoidFunction) vkMapMemory;
    if (!strcmp(funcName, "vkUnmapMemory"))
        return (PFN_vkVoidFunction) vkUnmapMemory;
    if (!strcmp(funcName, "vkFlushMappedMemoryRanges"))
        return (PFN_vkVoidFunction) vkFlushMappedMemoryRanges;
    if (!strcmp(funcName, "vkInvalidateMappedMemoryRanges"))
        return (PFN_vkVoidFunction) vkInvalidateMappedMemoryRanges;
    if (!strcmp(funcName, "vkDestroyFence"))
        return (PFN_vkVoidFunction) vkDestroyFence;
    if (!strcmp(funcName, "vkDestroyBuffer"))
        return (PFN_vkVoidFunction) vkDestroyBuffer;
    if (!strcmp(funcName, "vkDestroyBufferView"))
        return (PFN_vkVoidFunction)vkDestroyBufferView;
    if (!strcmp(funcName, "vkDestroyImage"))
        return (PFN_vkVoidFunction) vkDestroyImage;
    if (!strcmp(funcName, "vkBindBufferMemory"))
        return (PFN_vkVoidFunction) vkBindBufferMemory;
    if (!strcmp(funcName, "vkBindImageMemory"))
        return (PFN_vkVoidFunction) vkBindImageMemory;
    if (!strcmp(funcName, "vkGetBufferMemoryRequirements"))
        return (PFN_vkVoidFunction) vkGetBufferMemoryRequirements;
    if (!strcmp(funcName, "vkGetImageMemoryRequirements"))
        return (PFN_vkVoidFunction) vkGetImageMemoryRequirements;
    if (!strcmp(funcName, "vkQueueBindSparse"))
        return (PFN_vkVoidFunction) vkQueueBindSparse;
    if (!strcmp(funcName, "vkCreateFence"))
        return (PFN_vkVoidFunction) vkCreateFence;
    if (!strcmp(funcName, "vkGetFenceStatus"))
        return (PFN_vkVoidFunction) vkGetFenceStatus;
    if (!strcmp(funcName, "vkResetFences"))
        return (PFN_vkVoidFunction) vkResetFences;
    if (!strcmp(funcName, "vkWaitForFences"))
        return (PFN_vkVoidFunction) vkWaitForFences;
    if (!strcmp(funcName, "vkCreateSemaphore"))
        return (PFN_vkVoidFunction) vkCreateSemaphore;
    if (!strcmp(funcName, "vkDestroySemaphore"))
        return (PFN_vkVoidFunction) vkDestroySemaphore;
    if (!strcmp(funcName, "vkQueueWaitIdle"))
        return (PFN_vkVoidFunction) vkQueueWaitIdle;
    if (!strcmp(funcName, "vkDeviceWaitIdle"))
        return (PFN_vkVoidFunction) vkDeviceWaitIdle;
    if (!strcmp(funcName, "vkCreateBuffer"))
        return (PFN_vkVoidFunction) vkCreateBuffer;
    if (!strcmp(funcName, "vkCreateImage"))
        return (PFN_vkVoidFunction) vkCreateImage;
    if (!strcmp(funcName, "vkCreateImageView"))
        return (PFN_vkVoidFunction) vkCreateImageView;
    if (!strcmp(funcName, "vkCreateBufferView"))
        return (PFN_vkVoidFunction) vkCreateBufferView;
    if (!strcmp(funcName, "vkUpdateDescriptorSets"))
        return (PFN_vkVoidFunction) vkUpdateDescriptorSets;
    if (!strcmp(funcName, "vkAllocateCommandBuffers"))
        return (PFN_vkVoidFunction) vkAllocateCommandBuffers;
    if (!strcmp(funcName, "vkFreeCommandBuffers"))
        return (PFN_vkVoidFunction) vkFreeCommandBuffers;
    if (!strcmp(funcName, "vkCreateCommandPool"))
        return (PFN_vkVoidFunction) vkCreateCommandPool;
    if (!strcmp(funcName, "vkDestroyCommandPool"))
        return (PFN_vkVoidFunction) vkDestroyCommandPool;
    if (!strcmp(funcName, "vkResetCommandPool"))
        return (PFN_vkVoidFunction) vkResetCommandPool;
    if (!strcmp(funcName, "vkBeginCommandBuffer"))
        return (PFN_vkVoidFunction) vkBeginCommandBuffer;
    if (!strcmp(funcName, "vkEndCommandBuffer"))
        return (PFN_vkVoidFunction) vkEndCommandBuffer;
    if (!strcmp(funcName, "vkResetCommandBuffer"))
        return (PFN_vkVoidFunction) vkResetCommandBuffer;
    if (!strcmp(funcName, "vkCmdBindPipeline"))
        return (PFN_vkVoidFunction) vkCmdBindPipeline;
    if (!strcmp(funcName, "vkCmdBindDescriptorSets"))
        return (PFN_vkVoidFunction) vkCmdBindDescriptorSets;
    if (!strcmp(funcName, "vkCmdBindVertexBuffers"))
        return (PFN_vkVoidFunction) vkCmdBindVertexBuffers;
    if (!strcmp(funcName, "vkCmdBindIndexBuffer"))
        return (PFN_vkVoidFunction) vkCmdBindIndexBuffer;
    if (!strcmp(funcName, "vkCmdDraw"))
        return (PFN_vkVoidFunction) vkCmdDraw;
    if (!strcmp(funcName, "vkCmdDrawIndexed"))
        return (PFN_vkVoidFunction) vkCmdDrawIndexed;
    if (!strcmp(funcName, "vkCmdDrawIndirect"))
        return (PFN_vkVoidFunction) vkCmdDrawIndirect;
    if (!strcmp(funcName, "vkCmdDrawIndexedIndirect"))
        return (PFN_vkVoidFunction) vkCmdDrawIndexedIndirect;
    if (!strcmp(funcName, "vkCmdDispatch"))
        return (PFN_vkVoidFunction)vkCmdDispatch;
    if (!strcmp(funcName, "vkCmdDispatchIndirect"))
        return (PFN_vkVoidFunction)vkCmdDispatchIndirect;
    if (!strcmp(funcName, "vkCmdCopyBuffer"))
        return (PFN_vkVoidFunction)vkCmdCopyBuffer;
    if (!strcmp(funcName, "vkCmdCopyQueryPoolResults"))
        return (PFN_vkVoidFunction)vkCmdCopyQueryPoolResults;
    if (!strcmp(funcName, "vkCmdCopyImage"))
        return (PFN_vkVoidFunction) vkCmdCopyImage;
    if (!strcmp(funcName, "vkCmdCopyBufferToImage"))
        return (PFN_vkVoidFunction) vkCmdCopyBufferToImage;
    if (!strcmp(funcName, "vkCmdCopyImageToBuffer"))
        return (PFN_vkVoidFunction) vkCmdCopyImageToBuffer;
    if (!strcmp(funcName, "vkCmdUpdateBuffer"))
        return (PFN_vkVoidFunction) vkCmdUpdateBuffer;
    if (!strcmp(funcName, "vkCmdFillBuffer"))
        return (PFN_vkVoidFunction) vkCmdFillBuffer;
    if (!strcmp(funcName, "vkCmdClearColorImage"))
        return (PFN_vkVoidFunction) vkCmdClearColorImage;
    if (!strcmp(funcName, "vkCmdClearDepthStencilImage"))
        return (PFN_vkVoidFunction) vkCmdClearDepthStencilImage;
    if (!strcmp(funcName, "vkCmdResolveImage"))
        return (PFN_vkVoidFunction) vkCmdResolveImage;
    if (!strcmp(funcName, "vkCmdBeginQuery"))
        return (PFN_vkVoidFunction) vkCmdBeginQuery;
    if (!strcmp(funcName, "vkCmdEndQuery"))
        return (PFN_vkVoidFunction) vkCmdEndQuery;
    if (!strcmp(funcName, "vkCmdResetQueryPool"))
        return (PFN_vkVoidFunction) vkCmdResetQueryPool;
    if (!strcmp(funcName, "vkCreateRenderPass"))
        return (PFN_vkVoidFunction) vkCreateRenderPass;
    if (!strcmp(funcName, "vkDestroyRenderPass"))
        return (PFN_vkVoidFunction) vkDestroyRenderPass;
    if (!strcmp(funcName, "vkCmdBeginRenderPass"))
        return (PFN_vkVoidFunction) vkCmdBeginRenderPass;
    if (!strcmp(funcName, "vkCmdEndRenderPass"))
        return (PFN_vkVoidFunction) vkCmdEndRenderPass;
    if (!strcmp(funcName, "vkGetDeviceQueue"))
        return (PFN_vkVoidFunction) vkGetDeviceQueue;
    if (!strcmp(funcName, "vkCreateFramebuffer"))
        return (PFN_vkVoidFunction) vkCreateFramebuffer;
    if (!strcmp(funcName, "vkDestroyFramebuffer"))
        return (PFN_vkVoidFunction) vkDestroyFramebuffer;


    if (dev == NULL)
        return NULL;

    layer_data *my_data;
    my_data = get_my_data_ptr(get_dispatch_key(dev), layer_data_map);
    if (my_data->wsi_enabled)
    {
        if (!strcmp(funcName, "vkCreateSwapchainKHR"))
            return (PFN_vkVoidFunction) vkCreateSwapchainKHR;
        if (!strcmp(funcName, "vkDestroySwapchainKHR"))
            return (PFN_vkVoidFunction) vkDestroySwapchainKHR;
        if (!strcmp(funcName, "vkGetSwapchainImagesKHR"))
            return (PFN_vkVoidFunction) vkGetSwapchainImagesKHR;
        if (!strcmp(funcName, "vkAcquireNextImageKHR"))
            return (PFN_vkVoidFunction)vkAcquireNextImageKHR;
        if (!strcmp(funcName, "vkQueuePresentKHR"))
            return (PFN_vkVoidFunction)vkQueuePresentKHR;
    }

    VkLayerDispatchTable *pDisp = my_data->device_dispatch_table;
    if (pDisp->GetDeviceProcAddr == NULL)
        return NULL;
    return pDisp->GetDeviceProcAddr(dev, funcName);
}

VK_LAYER_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(
    VkInstance  instance,
    const char *funcName)
{
    PFN_vkVoidFunction fptr;

    if (!strcmp(funcName, "vkGetInstanceProcAddr"))
        return (PFN_vkVoidFunction) vkGetInstanceProcAddr;
    if (!strcmp(funcName, "vkGetDeviceProcAddr"))
        return (PFN_vkVoidFunction) vkGetDeviceProcAddr;
    if (!strcmp(funcName, "vkDestroyInstance"))
        return (PFN_vkVoidFunction) vkDestroyInstance;
    if (!strcmp(funcName, "vkCreateInstance"))
        return (PFN_vkVoidFunction) vkCreateInstance;
    if (!strcmp(funcName, "vkGetPhysicalDeviceMemoryProperties"))
        return (PFN_vkVoidFunction) vkGetPhysicalDeviceMemoryProperties;
    if (!strcmp(funcName, "vkCreateDevice"))
        return (PFN_vkVoidFunction) vkCreateDevice;
    if (!strcmp(funcName, "vkEnumerateInstanceLayerProperties"))
        return (PFN_vkVoidFunction) vkEnumerateInstanceLayerProperties;
    if (!strcmp(funcName, "vkEnumerateInstanceExtensionProperties"))
        return (PFN_vkVoidFunction) vkEnumerateInstanceExtensionProperties;
    if (!strcmp(funcName, "vkEnumerateDeviceLayerProperties"))
        return (PFN_vkVoidFunction) vkEnumerateDeviceLayerProperties;
    if (!strcmp(funcName, "vkEnumerateDeviceExtensionProperties"))
        return (PFN_vkVoidFunction) vkEnumerateDeviceExtensionProperties;

    if (instance == NULL) return NULL;

    layer_data *my_data;
    my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);

    fptr = debug_report_get_instance_proc_addr(my_data->report_data, funcName);
    if (fptr) return fptr;

    VkLayerInstanceDispatchTable* pTable = my_data->instance_dispatch_table;
    if (pTable->GetInstanceProcAddr == NULL)
        return NULL;
    return pTable->GetInstanceProcAddr(instance, funcName);
}
