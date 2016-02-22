/* Copyright (c) 2015-2016 The Khronos Group Inc.
 * Copyright (c) 2015-2016 Valve Corporation
 * Copyright (c) 2015-2016 LunarG, Inc.
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
 * Author: Mike Stroyan <mike@LunarG.com>
 */

#ifndef THREADING_H
#define THREADING_H
#include <vector>
#include "vk_layer_config.h"
#include "vk_layer_logging.h"

#if defined(__LP64__) || defined(_WIN64) || defined(__x86_64__) || defined(_M_X64) || defined(__ia64) || defined (_M_IA64) || defined(__aarch64__) || defined(__powerpc64__)
// If pointers are 64-bit, then there can be separate counters for each
// NONDISPATCHABLE_HANDLE type.  Otherwise they are all typedef uint64_t.
#define DISTINCT_NONDISPATCHABLE_HANDLES
#endif

// Draw State ERROR codes
typedef enum _THREADING_CHECKER_ERROR
{
    THREADING_CHECKER_NONE,                             // Used for INFO & other non-error messages
    THREADING_CHECKER_MULTIPLE_THREADS,                 // Object used simultaneously by multiple threads
    THREADING_CHECKER_SINGLE_THREAD_REUSE,              // Object used simultaneously by recursion in single thread
} THREADING_CHECKER_ERROR;

struct object_use_data {
    loader_platform_thread_id thread;
    int reader_count;
    int writer_count;
};

struct layer_data;

static int threadingLockInitialized = 0;
static loader_platform_thread_mutex threadingLock;
static loader_platform_thread_cond threadingCond;

template <typename T> class counter {
    public:
    const char *typeName;
    VkDebugReportObjectTypeEXT objectType;
    std::unordered_map<T, object_use_data> uses;
    void startWrite(debug_report_data *report_data, T object)
    {
        VkBool32 skipCall = VK_FALSE;
        loader_platform_thread_id tid = loader_platform_get_thread_id();
        loader_platform_thread_lock_mutex(&threadingLock);
        if (uses.find(object) == uses.end()) {
            // There is no current use of the object.  Record writer thread.
            struct object_use_data *use_data = &uses[object];
            use_data->reader_count = 0;
            use_data->writer_count = 1;
            use_data->thread = tid;
        } else {
            struct object_use_data *use_data = &uses[object];
            if (use_data->reader_count == 0) {
                // There are no readers.  Two writers just collided.
                if (use_data->thread != tid) {
                    skipCall |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, objectType, (uint64_t)(object),
                        /*location*/ 0, THREADING_CHECKER_MULTIPLE_THREADS, "THREADING",
                        "THREADING ERROR : object of type %s is simultaneously used in thread %ld and thread %ld",
                        typeName, use_data->thread, tid);
                    if (skipCall) {
                        // Wait for thread-safe access to object instead of skipping call.
                        while (uses.find(object) != uses.end()) {
                            loader_platform_thread_cond_wait(&threadingCond, &threadingLock);
                        }
                        // There is now no current use of the object.  Record writer thread.
                        struct object_use_data *use_data = &uses[object];
                        use_data->thread = tid ;
                        use_data->reader_count = 0;
                        use_data->writer_count = 1;
                    } else {
                        // Continue with an unsafe use of the object.
                        use_data->thread = tid ;
                        use_data->writer_count += 1;
                    }
                } else {
                    // This is either safe multiple use in one call, or recursive use.
                    // There is no way to make recursion safe.  Just forge ahead.
                    use_data->writer_count += 1;
                }
            } else {
                // There are readers.  This writer collided with them.
                if (use_data->thread != tid) {
                    skipCall |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, objectType, (uint64_t)(object),
                        /*location*/ 0, THREADING_CHECKER_MULTIPLE_THREADS, "THREADING",
                        "THREADING ERROR : object of type %s is simultaneously used in thread %ld and thread %ld",
                        typeName, use_data->thread, tid);
                    if (skipCall) {
                        // Wait for thread-safe access to object instead of skipping call.
                        while (uses.find(object) != uses.end()) {
                            loader_platform_thread_cond_wait(&threadingCond, &threadingLock);
                        }
                        // There is now no current use of the object.  Record writer thread.
                        struct object_use_data *use_data = &uses[object];
                        use_data->thread = tid ;
                        use_data->reader_count = 0;
                        use_data->writer_count = 1;
                    } else {
                        // Continue with an unsafe use of the object.
                        use_data->thread = tid ;
                        use_data->writer_count += 1;
                    }
                } else {
                    // This is either safe multiple use in one call, or recursive use.
                    // There is no way to make recursion safe.  Just forge ahead.
                    use_data->writer_count += 1;
                }
            }
        }
        loader_platform_thread_unlock_mutex(&threadingLock);
    }

    void finishWrite(T object)
    {
        // Object is no longer in use
        loader_platform_thread_lock_mutex(&threadingLock);
        uses[object].writer_count -= 1;
        if ((uses[object].reader_count == 0) && (uses[object].writer_count == 0)) {
            uses.erase(object);
        }
        // Notify any waiting threads that this object may be safe to use
        loader_platform_thread_cond_broadcast(&threadingCond);
        loader_platform_thread_unlock_mutex(&threadingLock);
    }

    void startRead(debug_report_data *report_data, T object) {
        VkBool32 skipCall = VK_FALSE;
        loader_platform_thread_id tid = loader_platform_get_thread_id();
        loader_platform_thread_lock_mutex(&threadingLock);
        if (uses.find(object) == uses.end()) {
            // There is no current use of the object.  Record reader count
            struct object_use_data *use_data = &uses[object];
            use_data->reader_count = 1;
            use_data->writer_count = 0;
            use_data->thread = tid;
        } else if (uses[object].writer_count > 0 && uses[object].thread != tid) {
            // There is a writer of the object.
            skipCall |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, objectType, (uint64_t)(object),
                /*location*/ 0, THREADING_CHECKER_MULTIPLE_THREADS, "THREADING",
                "THREADING ERROR : object of type %s is simultaneously used in thread %ld and thread %ld",
                typeName, uses[object].thread, tid);
            if (skipCall) {
                // Wait for thread-safe access to object instead of skipping call.
                while (uses.find(object) != uses.end()) {
                    loader_platform_thread_cond_wait(&threadingCond, &threadingLock);
                }
                // There is no current use of the object.  Record reader count
                struct object_use_data *use_data = &uses[object];
                use_data->reader_count = 1;
                use_data->writer_count = 0;
                use_data->thread = tid;
            } else {
                uses[object].reader_count += 1;
            }
        } else {
            // There are other readers of the object.  Increase reader count
            uses[object].reader_count += 1;
        }
        loader_platform_thread_unlock_mutex(&threadingLock);
    }
    void finishRead(T object) {
        loader_platform_thread_lock_mutex(&threadingLock);
        uses[object].reader_count -= 1;
        if ((uses[object].reader_count == 0) && (uses[object].writer_count == 0)) {
            uses.erase(object);
        }
        // Notify and waiting threads that this object may be safe to use
        loader_platform_thread_cond_broadcast(&threadingCond);
        loader_platform_thread_unlock_mutex(&threadingLock);
    }
    counter(const char *name = "",
            VkDebugReportObjectTypeEXT type=VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT) {
        typeName = name;
        objectType=type;
    }
};

struct layer_data {
    debug_report_data *report_data;
    std::vector<VkDebugReportCallbackEXT> logging_callback;
    VkLayerDispatchTable* device_dispatch_table;
    VkLayerInstanceDispatchTable* instance_dispatch_table;
    counter<VkCommandBuffer> c_VkCommandBuffer;
    counter<VkDevice> c_VkDevice;
    counter<VkInstance> c_VkInstance;
    counter<VkQueue> c_VkQueue;
#ifdef DISTINCT_NONDISPATCHABLE_HANDLES
    counter<VkBuffer> c_VkBuffer;
    counter<VkBufferView> c_VkBufferView;
    counter<VkCommandPool> c_VkCommandPool;
    counter<VkDescriptorPool> c_VkDescriptorPool;
    counter<VkDescriptorSet> c_VkDescriptorSet;
    counter<VkDescriptorSetLayout> c_VkDescriptorSetLayout;
    counter<VkDeviceMemory> c_VkDeviceMemory;
    counter<VkEvent> c_VkEvent;
    counter<VkFence> c_VkFence;
    counter<VkFramebuffer> c_VkFramebuffer;
    counter<VkImage> c_VkImage;
    counter<VkImageView> c_VkImageView;
    counter<VkPipeline> c_VkPipeline;
    counter<VkPipelineCache> c_VkPipelineCache;
    counter<VkPipelineLayout> c_VkPipelineLayout;
    counter<VkQueryPool> c_VkQueryPool;
    counter<VkRenderPass> c_VkRenderPass;
    counter<VkSampler> c_VkSampler;
    counter<VkSemaphore> c_VkSemaphore;
    counter<VkShaderModule> c_VkShaderModule;
    counter<VkDebugReportCallbackEXT> c_VkDebugReportCallbackEXT;
#else // DISTINCT_NONDISPATCHABLE_HANDLES
    counter<uint64_t> c_uint64_t;
#endif // DISTINCT_NONDISPATCHABLE_HANDLES
    layer_data():
        report_data(nullptr),
        c_VkCommandBuffer("VkCommandBuffer", VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT),
        c_VkDevice("VkDevice", VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT),
        c_VkInstance("VkInstance", VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT),
        c_VkQueue("VkQueue", VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT),
#ifdef DISTINCT_NONDISPATCHABLE_HANDLES
        c_VkBuffer("VkBuffer", VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT),
        c_VkBufferView("VkBufferView", VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_VIEW_EXT),
        c_VkCommandPool("VkCommandPool", VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_POOL_EXT),
        c_VkDescriptorPool("VkDescriptorPool", VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT),
        c_VkDescriptorSet("VkDescriptorSet", VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT),
        c_VkDescriptorSetLayout("VkDescriptorSetLayout", VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT),
        c_VkDeviceMemory("VkDeviceMemory", VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT),
        c_VkEvent("VkEvent", VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT),
        c_VkFence("VkFence", VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT),
        c_VkFramebuffer("VkFramebuffer", VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT),
        c_VkImage("VkImage", VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT),
        c_VkImageView("VkImageView", VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT),
        c_VkPipeline("VkPipeline", VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT),
        c_VkPipelineCache("VkPipelineCache", VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_CACHE_EXT),
        c_VkPipelineLayout("VkPipelineLayout", VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT),
        c_VkQueryPool("VkQueryPool", VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT),
        c_VkRenderPass("VkRenderPass", VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT),
        c_VkSampler("VkSampler", VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT),
        c_VkSemaphore("VkSemaphore", VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT),
        c_VkShaderModule("VkShaderModule", VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT),
        c_VkDebugReportCallbackEXT("VkDebugReportCallbackEXT", VK_DEBUG_REPORT_OBJECT_TYPE_DEBUG_REPORT_EXT)
#else // DISTINCT_NONDISPATCHABLE_HANDLES
        c_uint64_t("NON_DISPATCHABLE_HANDLE", VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT)
#endif // DISTINCT_NONDISPATCHABLE_HANDLES
    {};
};

#define WRAPPER(type) \
static void startWriteObject(struct layer_data *my_data, type object){my_data->c_##type.startWrite(my_data->report_data, object);}\
static void finishWriteObject(struct layer_data *my_data, type object){my_data->c_##type.finishWrite(object);}\
static void startReadObject(struct layer_data *my_data, type object){my_data->c_##type.startRead(my_data->report_data, object);}\
static void finishReadObject(struct layer_data *my_data, type object){my_data->c_##type.finishRead(object);}

WRAPPER(VkDevice)
WRAPPER(VkInstance)
WRAPPER(VkQueue)
#ifdef DISTINCT_NONDISPATCHABLE_HANDLES
WRAPPER(VkBuffer)
WRAPPER(VkBufferView)
WRAPPER(VkCommandPool)
WRAPPER(VkDescriptorPool)
WRAPPER(VkDescriptorSet)
WRAPPER(VkDescriptorSetLayout)
WRAPPER(VkDeviceMemory)
WRAPPER(VkEvent)
WRAPPER(VkFence)
WRAPPER(VkFramebuffer)
WRAPPER(VkImage)
WRAPPER(VkImageView)
WRAPPER(VkPipeline)
WRAPPER(VkPipelineCache)
WRAPPER(VkPipelineLayout)
WRAPPER(VkQueryPool)
WRAPPER(VkRenderPass)
WRAPPER(VkSampler)
WRAPPER(VkSemaphore)
WRAPPER(VkShaderModule)
WRAPPER(VkDebugReportCallbackEXT)
#else // DISTINCT_NONDISPATCHABLE_HANDLES
WRAPPER(uint64_t)
#endif // DISTINCT_NONDISPATCHABLE_HANDLES

static std::unordered_map<void*, layer_data *> layer_data_map;
static std::unordered_map<VkCommandBuffer, VkCommandPool> command_pool_map;

// VkCommandBuffer needs check for implicit use of command pool
static void startWriteObject(struct layer_data *my_data, VkCommandBuffer object, bool lockPool=true)
{
    if (lockPool) {
        loader_platform_thread_lock_mutex(&threadingLock);
        VkCommandPool pool = command_pool_map[object];
        loader_platform_thread_unlock_mutex(&threadingLock);
        startWriteObject(my_data, pool);
    }
    my_data->c_VkCommandBuffer.startWrite(my_data->report_data, object);
}
static void finishWriteObject(struct layer_data *my_data, VkCommandBuffer object, bool lockPool=true)
{
    my_data->c_VkCommandBuffer.finishWrite(object);
    if (lockPool) {
        loader_platform_thread_lock_mutex(&threadingLock);
        VkCommandPool pool = command_pool_map[object];
        loader_platform_thread_unlock_mutex(&threadingLock);
        finishWriteObject(my_data, pool);
    }
}
static void startReadObject(struct layer_data *my_data, VkCommandBuffer object)
{
    loader_platform_thread_lock_mutex(&threadingLock);
    VkCommandPool pool = command_pool_map[object];
    loader_platform_thread_unlock_mutex(&threadingLock);
    startReadObject(my_data, pool);
    my_data->c_VkCommandBuffer.startRead(my_data->report_data, object);
}
static void finishReadObject(struct layer_data *my_data, VkCommandBuffer object)
{
    my_data->c_VkCommandBuffer.finishRead(object);
    loader_platform_thread_lock_mutex(&threadingLock);
    VkCommandPool pool = command_pool_map[object];
    loader_platform_thread_unlock_mutex(&threadingLock);
    finishReadObject(my_data, pool);
}
#endif // THREADING_H
