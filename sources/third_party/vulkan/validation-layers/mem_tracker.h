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
 * Author: Tobin Ehlis <tobin@lunarg.com>
 * Author: Mark Lobodzinski <mark@lunarg.com>
 */

#pragma once
#include <vector>
#include <unordered_map>
#include "vulkan/vk_layer.h"

#ifdef __cplusplus
extern "C" {
#endif

// Mem Tracker ERROR codes
typedef enum _MEM_TRACK_ERROR
{
    MEMTRACK_NONE,                          // Used for INFO & other non-error messages
    MEMTRACK_INVALID_CB,                    // Cmd Buffer invalid
    MEMTRACK_INVALID_MEM_OBJ,               // Invalid Memory Object
    MEMTRACK_INVALID_ALIASING,              // Invalid Memory Aliasing
    MEMTRACK_INVALID_LAYOUT,                // Invalid Layout
    MEMTRACK_INTERNAL_ERROR,                // Bug in Mem Track Layer internal data structures
    MEMTRACK_FREED_MEM_REF,                 // MEM Obj freed while it still has obj and/or CB refs
    MEMTRACK_MEM_OBJ_CLEAR_EMPTY_BINDINGS,  // Clearing bindings on mem obj that doesn't have any bindings
    MEMTRACK_MISSING_MEM_BINDINGS,          // Trying to retrieve mem bindings, but none found (may be internal error)
    MEMTRACK_INVALID_OBJECT,                // Attempting to reference generic VK Object that is invalid
    MEMTRACK_MEMORY_BINDING_ERROR,          // Error during one of many calls that bind memory to object or CB
    MEMTRACK_MEMORY_LEAK,                   // Failure to call vkFreeMemory on Mem Obj prior to DestroyDevice
    MEMTRACK_INVALID_STATE,                 // Memory not in the correct state
    MEMTRACK_RESET_CB_WHILE_IN_FLIGHT,      // vkResetCommandBuffer() called on a CB that hasn't completed
    MEMTRACK_INVALID_FENCE_STATE,           // Invalid Fence State signaled or used
    MEMTRACK_REBIND_OBJECT,                 // Non-sparse object bindings are immutable
    MEMTRACK_INVALID_USAGE_FLAG,            // Usage flags specified at image/buffer create conflict w/ use of object
    MEMTRACK_INVALID_MAP,                   // Size flag specified at alloc is too small for mapping range
} MEM_TRACK_ERROR;

// MemTracker Semaphore states
typedef enum _MtSemaphoreState
{
    MEMTRACK_SEMAPHORE_STATE_UNSET,         // Semaphore is in an undefined state
    MEMTRACK_SEMAPHORE_STATE_SIGNALLED,     // Semaphore has is in signalled state
    MEMTRACK_SEMAPHORE_STATE_WAIT,          // Semaphore is in wait state
} MtSemaphoreState;

struct MemRange {
    VkDeviceSize offset;
    VkDeviceSize size;
};

/*
 * Data Structure overview
 *  There are 4 global STL(' maps
 *  cbMap -- map of command Buffer (CB) objects to MT_CB_INFO structures
 *    Each MT_CB_INFO struct has an stl list container with
 *    memory objects that are referenced by this CB
 *  memObjMap -- map of Memory Objects to MT_MEM_OBJ_INFO structures
 *    Each MT_MEM_OBJ_INFO has two stl list containers with:
 *      -- all CBs referencing this mem obj
 *      -- all VK Objects that are bound to this memory
 *  objectMap -- map of objects to MT_OBJ_INFO structures
 *
 * Algorithm overview
 * These are the primary events that should happen related to different objects
 * 1. Command buffers
 *   CREATION - Add object,structure to map
 *   CMD BIND - If mem associated, add mem reference to list container
 *   DESTROY  - Remove from map, decrement (and report) mem references
 * 2. Mem Objects
 *   CREATION - Add object,structure to map
 *   OBJ BIND - Add obj structure to list container for that mem node
 *   CMB BIND - If mem-related add CB structure to list container for that mem node
 *   DESTROY  - Flag as errors any remaining refs and remove from map
 * 3. Generic Objects
 *   MEM BIND - DESTROY any previous binding, Add obj node w/ ref to map, add obj ref to list container for that mem node
 *   DESTROY - If mem bound, remove reference list container for that memInfo, remove object ref from map
 */
// TODO : Is there a way to track when Cmd Buffer finishes & remove mem references at that point?
// TODO : Could potentially store a list of freed mem allocs to flag when they're incorrectly used

// Simple struct to hold handle and type of object so they can be uniquely identified and looked up in appropriate map
struct MT_OBJ_HANDLE_TYPE {
    uint64_t        handle;
    VkDebugReportObjectTypeEXT type;
};

// Data struct for tracking memory object
struct MT_MEM_OBJ_INFO {
    void*                       object;                 // Dispatchable object used to create this memory (device of swapchain)
    uint32_t                    refCount;               // Count of references (obj bindings or CB use)
    bool                        valid;                  // Stores if the memory has valid data or not
    VkDeviceMemory              mem;
    VkMemoryAllocateInfo        allocInfo;
    list<MT_OBJ_HANDLE_TYPE>    pObjBindings;           // list container of objects bound to this memory
    list<VkCommandBuffer>       pCommandBufferBindings; // list container of cmd buffers that reference this mem object
    MemRange                    memRange;
    void                       *pData, *pDriverData;
};

// This only applies to Buffers and Images, which can have memory bound to them
struct MT_OBJ_BINDING_INFO {
    VkDeviceMemory mem;
    bool valid; //If this is a swapchain image backing memory is not a MT_MEM_OBJ_INFO so store it here.
    union create_info {
        VkImageCreateInfo  image;
        VkBufferCreateInfo buffer;
    } create_info;
};

// Track all command buffers
typedef struct _MT_CB_INFO {
    VkCommandBufferAllocateInfo createInfo;
    VkPipeline                  pipelines[VK_PIPELINE_BIND_POINT_RANGE_SIZE];
    uint32_t                    attachmentCount;
    VkCommandBuffer             commandBuffer;
    uint64_t                    fenceId;
    VkFence                     lastSubmittedFence;
    VkQueue                     lastSubmittedQueue;
    VkRenderPass                pass;
    vector<VkDescriptorSet>     activeDescriptorSets;
    vector<std::function<VkBool32()> > validate_functions;
    // Order dependent, stl containers must be at end of struct
    list<VkDeviceMemory>        pMemObjList; // List container of Mem objs referenced by this CB
    // Constructor
    _MT_CB_INFO():createInfo{},pipelines{},attachmentCount(0),fenceId(0),lastSubmittedFence{},lastSubmittedQueue{} {};
} MT_CB_INFO;

// Track command pools and their command buffers
typedef struct _MT_CMD_POOL_INFO {
    VkCommandPoolCreateFlags    createFlags;
    list<VkCommandBuffer>       pCommandBuffers; // list container of cmd buffers allocated from this pool
} MT_CMD_POOL_INFO;

struct MT_IMAGE_VIEW_INFO {
    VkImage image;
};

struct MT_FB_ATTACHMENT_INFO {
    VkImage image;
    VkDeviceMemory mem;
};

struct MT_FB_INFO {
    std::vector<MT_FB_ATTACHMENT_INFO> attachments;
};

struct MT_PASS_ATTACHMENT_INFO {
   uint32_t attachment;
   VkAttachmentLoadOp load_op; 
   VkAttachmentStoreOp store_op;
};

struct MT_PASS_INFO {
    VkFramebuffer fb;
    std::vector<MT_PASS_ATTACHMENT_INFO> attachments;
    std::unordered_map<uint32_t, bool> attachment_first_read;
    std::unordered_map<uint32_t, VkImageLayout> attachment_first_layout;
};

// Associate fenceId with a fence object
struct MT_FENCE_INFO {
    uint64_t          fenceId;          // Sequence number for fence at last submit
    VkQueue           queue;            // Queue that this fence is submitted against or NULL
    VkSwapchainKHR
        swapchain; // Swapchain that this fence is submitted against or NULL
    VkBool32          firstTimeFlag;    // Fence was created in signaled state, avoid warnings for first use
    VkFenceCreateInfo createInfo;
};

// Track Queue information
struct MT_QUEUE_INFO {
    uint64_t                    lastRetiredId;
    uint64_t                    lastSubmittedId;
    list<VkCommandBuffer>       pQueueCommandBuffers;
    list<VkDeviceMemory>        pMemRefList;
};

struct MT_DESCRIPTOR_SET_INFO {
    std::vector<VkImageView> images;
    std::vector<VkBuffer> buffers;
};

// Track Swapchain Information
struct MT_SWAP_CHAIN_INFO {
    VkSwapchainCreateInfoKHR    createInfo;
    std::vector<VkImage>        images;
};

struct MEMORY_RANGE {
    uint64_t handle;
    VkDeviceMemory memory;
    VkDeviceSize start;
    VkDeviceSize end;
};

#ifdef __cplusplus
}
#endif
