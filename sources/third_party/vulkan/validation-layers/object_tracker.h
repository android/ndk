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
 * Author: Jon Ashburn <jon@lunarg.com>
 * Author: Mark Lobodzinski <mark@lunarg.com>
 * Author: Tobin Ehlis <tobin@lunarg.com>
 */

#include "vulkan/vk_layer.h"
#include "vk_layer_extension_utils.h"
#include "vk_enum_string_helper.h"
#include "vk_layer_table.h"

// Object Tracker ERROR codes
typedef enum _OBJECT_TRACK_ERROR
{
    OBJTRACK_NONE,                              // Used for INFO & other non-error messages
    OBJTRACK_UNKNOWN_OBJECT,                    // Updating uses of object that's not in global object list
    OBJTRACK_INTERNAL_ERROR,                    // Bug with data tracking within the layer
    OBJTRACK_DESTROY_OBJECT_FAILED,             // Couldn't find object to be destroyed
    OBJTRACK_OBJECT_LEAK,                       // OBJECT was not correctly freed/destroyed
    OBJTRACK_OBJCOUNT_MAX_EXCEEDED,             // Request for Object data in excess of max obj count
    OBJTRACK_INVALID_OBJECT,                    // Object used that has never been created
    OBJTRACK_DESCRIPTOR_POOL_MISMATCH,          // Descriptor Pools specified incorrectly
    OBJTRACK_COMMAND_POOL_MISMATCH,             // Command Pools specified incorrectly
} OBJECT_TRACK_ERROR;

// Object Status -- used to track state of individual objects
typedef VkFlags ObjectStatusFlags;
typedef enum _ObjectStatusFlagBits
{
    OBJSTATUS_NONE                              = 0x00000000, // No status is set
    OBJSTATUS_FENCE_IS_SUBMITTED                = 0x00000001, // Fence has been submitted
    OBJSTATUS_VIEWPORT_BOUND                    = 0x00000002, // Viewport state object has been bound
    OBJSTATUS_RASTER_BOUND                      = 0x00000004, // Viewport state object has been bound
    OBJSTATUS_COLOR_BLEND_BOUND                 = 0x00000008, // Viewport state object has been bound
    OBJSTATUS_DEPTH_STENCIL_BOUND               = 0x00000010, // Viewport state object has been bound
    OBJSTATUS_GPU_MEM_MAPPED                    = 0x00000020, // Memory object is currently mapped
    OBJSTATUS_COMMAND_BUFFER_SECONDARY          = 0x00000040, // Command Buffer is of type SECONDARY
} ObjectStatusFlagBits;

typedef struct _OBJTRACK_NODE {
    uint64_t                   vkObj;           // Object handle
    VkDebugReportObjectTypeEXT objType;         // Object type identifier
    ObjectStatusFlags          status;          // Object state
    uint64_t                   parentObj;       // Parent object
} OBJTRACK_NODE;

// prototype for extension functions
uint64_t objTrackGetObjectCount(VkDevice device);
uint64_t objTrackGetObjectsOfTypeCount(VkDevice, VkDebugReportObjectTypeEXT type);

// Func ptr typedefs
typedef uint64_t (*OBJ_TRACK_GET_OBJECT_COUNT)(VkDevice);
typedef uint64_t (*OBJ_TRACK_GET_OBJECTS_OF_TYPE_COUNT)(VkDevice, VkDebugReportObjectTypeEXT);

struct layer_data {
    debug_report_data *report_data;
    //TODO: put instance data here
    VkDebugReportCallbackEXT   logging_callback;
    bool wsi_enabled;
    bool objtrack_extensions_enabled;

    layer_data() :
        report_data(nullptr),
        logging_callback(VK_NULL_HANDLE),
        wsi_enabled(false),
        objtrack_extensions_enabled(false)
    {};
};

struct instExts {
    bool wsi_enabled;
};

static std::unordered_map<void *, struct instExts> instanceExtMap;
static std::unordered_map<void*, layer_data *> layer_data_map;
static device_table_map                        object_tracker_device_table_map;
static instance_table_map                      object_tracker_instance_table_map;

// We need additionally validate image usage using a separate map
// of swapchain-created images
static unordered_map<uint64_t, OBJTRACK_NODE*> swapchainImageMap;

static long long unsigned int object_track_index = 0;
static int objLockInitialized = 0;
static loader_platform_thread_mutex objLock;

// Objects stored in a global map w/ struct containing basic info
// unordered_map<const void*, OBJTRACK_NODE*> objMap;

#define NUM_OBJECT_TYPES (VK_DEBUG_REPORT_OBJECT_TYPE_DEBUG_REPORT_EXT+1)

static uint64_t                         numObjs[NUM_OBJECT_TYPES]     = {0};
static uint64_t                         numTotalObjs                  = 0;
static VkQueueFamilyProperties         *queueInfo                     = NULL;
static uint32_t                         queueCount                    = 0;

template layer_data *get_my_data_ptr<layer_data>(
        void *data_key, std::unordered_map<void *, layer_data *> &data_map);


//
// Internal Object Tracker Functions
//

static void createDeviceRegisterExtensions(const VkDeviceCreateInfo* pCreateInfo, VkDevice device)
{
    layer_data *my_device_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkLayerDispatchTable *pDisp = get_dispatch_table(object_tracker_device_table_map, device);
    PFN_vkGetDeviceProcAddr gpa = pDisp->GetDeviceProcAddr;
    pDisp->CreateSwapchainKHR = (PFN_vkCreateSwapchainKHR) gpa(device, "vkCreateSwapchainKHR");
    pDisp->DestroySwapchainKHR = (PFN_vkDestroySwapchainKHR) gpa(device, "vkDestroySwapchainKHR");
    pDisp->GetSwapchainImagesKHR = (PFN_vkGetSwapchainImagesKHR) gpa(device, "vkGetSwapchainImagesKHR");
    pDisp->AcquireNextImageKHR = (PFN_vkAcquireNextImageKHR) gpa(device, "vkAcquireNextImageKHR");
    pDisp->QueuePresentKHR = (PFN_vkQueuePresentKHR) gpa(device, "vkQueuePresentKHR");
    my_device_data->wsi_enabled = false;
    for (uint32_t i = 0; i < pCreateInfo->enabledExtensionCount; i++) {
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0)
            my_device_data->wsi_enabled = true;

        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], "OBJTRACK_EXTENSIONS") == 0)
            my_device_data->objtrack_extensions_enabled = true;
    }
}

static void createInstanceRegisterExtensions(const VkInstanceCreateInfo* pCreateInfo, VkInstance instance)
{
    uint32_t i;
    VkLayerInstanceDispatchTable *pDisp = get_dispatch_table(object_tracker_instance_table_map, instance);
    PFN_vkGetInstanceProcAddr gpa = pDisp->GetInstanceProcAddr;
    pDisp->GetPhysicalDeviceSurfaceSupportKHR = (PFN_vkGetPhysicalDeviceSurfaceSupportKHR) gpa(instance, "vkGetPhysicalDeviceSurfaceSupportKHR");
    pDisp->GetPhysicalDeviceSurfaceCapabilitiesKHR = (PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR) gpa(instance, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
    pDisp->GetPhysicalDeviceSurfaceFormatsKHR = (PFN_vkGetPhysicalDeviceSurfaceFormatsKHR) gpa(instance, "vkGetPhysicalDeviceSurfaceFormatsKHR");
    pDisp->GetPhysicalDeviceSurfacePresentModesKHR = (PFN_vkGetPhysicalDeviceSurfacePresentModesKHR) gpa(instance, "vkGetPhysicalDeviceSurfacePresentModesKHR");

#if VK_USE_PLATFORM_WIN32_KHR
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
#ifdef VK_USE_PLATFORM_MIR_KHR
    pDisp->CreateMirSurfaceKHR = (PFN_vkCreateMirSurfaceKHR) gpa(instance, "vkCreateMirSurfaceKHR");
    pDisp->GetPhysicalDeviceMirPresentationSupportKHR = (PFN_vkGetPhysicalDeviceMirPresentationSupportKHR) gpa(instance, "vkGetPhysicalDeviceMirPresentationSupportKHR");
#endif // VK_USE_PLATFORM_MIR_KHR
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
    pDisp->CreateWaylandSurfaceKHR = (PFN_vkCreateWaylandSurfaceKHR) gpa(instance, "vkCreateWaylandSurfaceKHR");
    pDisp->GetPhysicalDeviceWaylandPresentationSupportKHR = (PFN_vkGetPhysicalDeviceWaylandPresentationSupportKHR) gpa(instance, "vkGetPhysicalDeviceWaylandPresentationSupportKHR");
#endif //  VK_USE_PLATFORM_WAYLAND_KHR
#ifdef VK_USE_PLATFORM_ANDROID_KHR
    pDisp->CreateAndroidSurfaceKHR = (PFN_vkCreateAndroidSurfaceKHR) gpa(instance, "vkCreateAndroidSurfaceKHR");
#endif // VK_USE_PLATFORM_ANDROID_KHR

    instanceExtMap[pDisp].wsi_enabled = false;
    for (i = 0; i < pCreateInfo->enabledExtensionCount; i++) {
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_SURFACE_EXTENSION_NAME) == 0)
            instanceExtMap[pDisp].wsi_enabled = true;

    }
}

// Indicate device or instance dispatch table type
typedef enum _DispTableType
{
    DISP_TBL_TYPE_INSTANCE,
    DISP_TBL_TYPE_DEVICE,
} DispTableType;

debug_report_data *mdd(const void* object)
{
    dispatch_key key = get_dispatch_key(object);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    return my_data->report_data;
}

debug_report_data *mid(VkInstance object)
{
    dispatch_key key = get_dispatch_key(object);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    return my_data->report_data;
}

// For each Queue's doubly linked-list of mem refs
typedef struct _OT_MEM_INFO {
    VkDeviceMemory       mem;
    struct _OT_MEM_INFO *pNextMI;
    struct _OT_MEM_INFO *pPrevMI;

} OT_MEM_INFO;

// Track Queue information
typedef struct _OT_QUEUE_INFO {
    OT_MEM_INFO                     *pMemRefList;
    struct _OT_QUEUE_INFO           *pNextQI;
    uint32_t                         queueNodeIndex;
    VkQueue                          queue;
    uint32_t                         refCount;
} OT_QUEUE_INFO;

// Global list of QueueInfo structures, one per queue
static OT_QUEUE_INFO *g_pQueueInfo = NULL;

// Convert an object type enum to an object type array index
static uint32_t
objTypeToIndex(
    uint32_t objType)
{
    uint32_t index = objType;
    return index;
}

// Add new queue to head of global queue list
static void
addQueueInfo(
    uint32_t queueNodeIndex,
    VkQueue  queue)
{
    OT_QUEUE_INFO *pQueueInfo = new OT_QUEUE_INFO;

    if (pQueueInfo != NULL) {
        memset(pQueueInfo, 0, sizeof(OT_QUEUE_INFO));
        pQueueInfo->queue       = queue;
        pQueueInfo->queueNodeIndex = queueNodeIndex;
        pQueueInfo->pNextQI   = g_pQueueInfo;
        g_pQueueInfo          = pQueueInfo;
    }
    else {
        log_msg(mdd(queue), VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT, reinterpret_cast<uint64_t>(queue), __LINE__, OBJTRACK_INTERNAL_ERROR, "OBJTRACK",
            "ERROR:  VK_ERROR_OUT_OF_HOST_MEMORY -- could not allocate memory for Queue Information");
    }
}

// Destroy memRef lists and free all memory
static void
destroyQueueMemRefLists(void)
{
    OT_QUEUE_INFO *pQueueInfo    = g_pQueueInfo;
    OT_QUEUE_INFO *pDelQueueInfo = NULL;
    while (pQueueInfo != NULL) {
        OT_MEM_INFO *pMemInfo = pQueueInfo->pMemRefList;
        while (pMemInfo != NULL) {
            OT_MEM_INFO *pDelMemInfo = pMemInfo;
            pMemInfo = pMemInfo->pNextMI;
            delete pDelMemInfo;
        }
        pDelQueueInfo = pQueueInfo;
        pQueueInfo    = pQueueInfo->pNextQI;
        delete pDelQueueInfo;
    }
    g_pQueueInfo = pQueueInfo;
}

static void
setGpuQueueInfoState(
    uint32_t  count,
    void     *pData)
{
    queueCount = count;
    queueInfo  = (VkQueueFamilyProperties*)realloc((void*)queueInfo, count * sizeof(VkQueueFamilyProperties));
    if (queueInfo != NULL) {
        memcpy(queueInfo, pData, count * sizeof(VkQueueFamilyProperties));
    }
}

// Check Queue type flags for selected queue operations
static void
validateQueueFlags(
    VkQueue     queue,
    const char *function)
{
    OT_QUEUE_INFO *pQueueInfo = g_pQueueInfo;
    while ((pQueueInfo != NULL) && (pQueueInfo->queue != queue)) {
        pQueueInfo = pQueueInfo->pNextQI;
    }
    if (pQueueInfo != NULL) {
        if ((queueInfo != NULL) && (queueInfo[pQueueInfo->queueNodeIndex].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) == 0) {
            log_msg(mdd(queue), VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT, reinterpret_cast<uint64_t>(queue), __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
                "Attempting %s on a non-memory-management capable queue -- VK_QUEUE_SPARSE_BINDING_BIT not set", function);
        } else {
            log_msg(mdd(queue), VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT, reinterpret_cast<uint64_t>(queue), __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
                "Attempting %s on a possibly non-memory-management capable queue -- VK_QUEUE_SPARSE_BINDING_BIT not known", function);
        }
    }
}

/* TODO: Port to new type safety */
#if 0
// Check object status for selected flag state
static VkBool32
validate_status(
    VkObject            dispatchable_object,
    VkObject            vkObj,
    VkObjectType        objType,
    ObjectStatusFlags   status_mask,
    ObjectStatusFlags   status_flag,
    VkFlags             msg_flags,
    OBJECT_TRACK_ERROR  error_code,
    const char         *fail_msg)
{
    if (objMap.find(vkObj) != objMap.end()) {
        OBJTRACK_NODE* pNode = objMap[vkObj];
        if ((pNode->status & status_mask) != status_flag) {
            char str[1024];
            log_msg(mdd(dispatchable_object), msg_flags, pNode->objType, vkObj, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
                "OBJECT VALIDATION WARNING: %s object 0x%" PRIxLEAST64 ": %s", string_VkObjectType(objType),
                static_cast<uint64_t>(vkObj), fail_msg);
            return VK_FALSE;
        }
        return VK_TRUE;
    }
    else {
        // If we do not find it print an error
        log_msg(mdd(dispatchable_object), msg_flags, (VkObjectType) 0, vkObj, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
            "Unable to obtain status for non-existent object 0x%" PRIxLEAST64 " of %s type",
            static_cast<uint64_t>(vkObj), string_VkObjectType(objType));
        return VK_FALSE;
    }
}
#endif

#include "vk_dispatch_table_helper.h"
static void
initObjectTracker(
    layer_data *my_data,
    const VkAllocationCallbacks *pAllocator)
{
    uint32_t report_flags = 0;
    uint32_t debug_action = 0;
    FILE *log_output = NULL;
    const char *option_str;
    // initialize ObjectTracker options
    report_flags = getLayerOptionFlags("ObjectTrackerReportFlags", 0);
    getLayerOptionEnum("ObjectTrackerDebugAction", (uint32_t *) &debug_action);

    if (debug_action & VK_DBG_LAYER_ACTION_LOG_MSG)
    {
        option_str = getLayerOption("ObjectTrackerLogFilename");
        log_output = getLayerLogOutput(option_str, "ObjectTracker");
        VkDebugReportCallbackCreateInfoEXT dbgInfo;
        memset(&dbgInfo, 0, sizeof(dbgInfo));
        dbgInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
        dbgInfo.pfnCallback = log_callback;
        dbgInfo.pUserData = log_output;
        dbgInfo.flags = report_flags;
        layer_create_msg_callback(my_data->report_data, &dbgInfo, pAllocator, &my_data->logging_callback);
    }

    if (!objLockInitialized)
    {
        // TODO/TBD: Need to delete this mutex sometime.  How???  One
        // suggestion is to call this during vkCreateInstance(), and then we
        // can clean it up during vkDestroyInstance().  However, that requires
        // that the layer have per-instance locks.  We need to come back and
        // address this soon.
        loader_platform_thread_create_mutex(&objLock);
        objLockInitialized = 1;
    }
}

//
// Forward declares of generated routines
//

static void create_physical_device(VkInstance dispatchable_object, VkPhysicalDevice vkObj, VkDebugReportObjectTypeEXT objType);
static void create_instance(VkInstance dispatchable_object, VkInstance object, VkDebugReportObjectTypeEXT objType);
static void create_device(VkDevice dispatchable_object, VkDevice object, VkDebugReportObjectTypeEXT objType);
static void create_queue(VkDevice dispatchable_object, VkQueue vkObj, VkDebugReportObjectTypeEXT objType);
static VkBool32 validate_image(VkQueue dispatchable_object, VkImage object, VkDebugReportObjectTypeEXT objType, bool null_allowed);
static VkBool32 validate_instance(VkInstance dispatchable_object, VkInstance object, VkDebugReportObjectTypeEXT objType, bool null_allowed);
static VkBool32 validate_device(VkDevice dispatchable_object, VkDevice object, VkDebugReportObjectTypeEXT objType, bool null_allowed);
static VkBool32 validate_descriptor_pool(VkDevice dispatchable_object, VkDescriptorPool object, VkDebugReportObjectTypeEXT objType, bool null_allowed);
static VkBool32 validate_descriptor_set_layout(VkDevice dispatchable_object, VkDescriptorSetLayout object, VkDebugReportObjectTypeEXT objType, bool null_allowed);
static VkBool32 validate_command_pool(VkDevice dispatchable_object, VkCommandPool object, VkDebugReportObjectTypeEXT objType, bool null_allowed);
static VkBool32 validate_buffer(VkQueue dispatchable_object, VkBuffer object, VkDebugReportObjectTypeEXT objType, bool null_allowed);
static void create_pipeline(VkDevice dispatchable_object, VkPipeline vkObj, VkDebugReportObjectTypeEXT objType);
static VkBool32 validate_pipeline_cache(VkDevice dispatchable_object, VkPipelineCache object, VkDebugReportObjectTypeEXT objType, bool null_allowed);
static VkBool32 validate_render_pass(VkDevice dispatchable_object, VkRenderPass object, VkDebugReportObjectTypeEXT objType, bool null_allowed);
static VkBool32 validate_shader_module(VkDevice dispatchable_object, VkShaderModule object, VkDebugReportObjectTypeEXT objType, bool null_allowed);
static VkBool32 validate_pipeline_layout(VkDevice dispatchable_object, VkPipelineLayout object, VkDebugReportObjectTypeEXT objType, bool null_allowed);
static VkBool32 validate_pipeline(VkDevice dispatchable_object, VkPipeline object, VkDebugReportObjectTypeEXT objType, bool null_allowed);
static void destroy_command_pool(VkDevice dispatchable_object, VkCommandPool object);
static void destroy_command_buffer(VkCommandBuffer dispatchable_object, VkCommandBuffer object);
static void destroy_descriptor_pool(VkDevice dispatchable_object, VkDescriptorPool object);
static void destroy_descriptor_set(VkDevice dispatchable_object, VkDescriptorSet object);
static void destroy_device_memory(VkDevice dispatchable_object, VkDeviceMemory object);
static void destroy_swapchain_khr(VkDevice dispatchable_object, VkSwapchainKHR object);
static VkBool32 set_device_memory_status(VkDevice dispatchable_object, VkDeviceMemory object, VkDebugReportObjectTypeEXT objType, ObjectStatusFlags status_flag);
static VkBool32 reset_device_memory_status(VkDevice dispatchable_object, VkDeviceMemory object, VkDebugReportObjectTypeEXT objType, ObjectStatusFlags status_flag);
#if 0
static VkBool32 validate_status(VkDevice dispatchable_object, VkFence object, VkDebugReportObjectTypeEXT objType,
    ObjectStatusFlags status_mask, ObjectStatusFlags status_flag, VkFlags msg_flags, OBJECT_TRACK_ERROR  error_code,
    const char         *fail_msg);
#endif
extern unordered_map<uint64_t, OBJTRACK_NODE*> VkPhysicalDeviceMap;
extern unordered_map<uint64_t, OBJTRACK_NODE*> VkImageMap;
extern unordered_map<uint64_t, OBJTRACK_NODE*> VkQueueMap;
extern unordered_map<uint64_t, OBJTRACK_NODE*> VkDescriptorSetMap;
extern unordered_map<uint64_t, OBJTRACK_NODE*> VkBufferMap;
extern unordered_map<uint64_t, OBJTRACK_NODE*> VkFenceMap;
extern unordered_map<uint64_t, OBJTRACK_NODE*> VkSemaphoreMap;
extern unordered_map<uint64_t, OBJTRACK_NODE*> VkCommandPoolMap;
extern unordered_map<uint64_t, OBJTRACK_NODE*> VkCommandBufferMap;
extern unordered_map<uint64_t, OBJTRACK_NODE*> VkSwapchainKHRMap;
extern unordered_map<uint64_t, OBJTRACK_NODE*> VkSurfaceKHRMap;

static void create_physical_device(VkInstance dispatchable_object, VkPhysicalDevice vkObj, VkDebugReportObjectTypeEXT objType)
{
    log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, objType, reinterpret_cast<uint64_t>(vkObj), __LINE__, OBJTRACK_NONE, "OBJTRACK",
        "OBJ[%llu] : CREATE %s object 0x%" PRIxLEAST64 , object_track_index++, string_VkDebugReportObjectTypeEXT(objType),
        reinterpret_cast<uint64_t>(vkObj));

    OBJTRACK_NODE* pNewObjNode = new OBJTRACK_NODE;
    pNewObjNode->objType = objType;
    pNewObjNode->status  = OBJSTATUS_NONE;
    pNewObjNode->vkObj  = reinterpret_cast<uint64_t>(vkObj);
    VkPhysicalDeviceMap[reinterpret_cast<uint64_t>(vkObj)] = pNewObjNode;
    uint32_t objIndex = objTypeToIndex(objType);
    numObjs[objIndex]++;
    numTotalObjs++;
}

static void create_surface_khr(VkInstance dispatchable_object, VkSurfaceKHR vkObj, VkDebugReportObjectTypeEXT objType)
{
    // TODO: Add tracking of surface objects
    log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, objType, (uint64_t)(vkObj), __LINE__, OBJTRACK_NONE, "OBJTRACK",
        "OBJ[%llu] : CREATE %s object 0x%" PRIxLEAST64 , object_track_index++, string_VkDebugReportObjectTypeEXT(objType),
        (uint64_t)(vkObj));

    OBJTRACK_NODE* pNewObjNode = new OBJTRACK_NODE;
    pNewObjNode->objType = objType;
    pNewObjNode->status  = OBJSTATUS_NONE;
    pNewObjNode->vkObj   = (uint64_t)(vkObj);
    VkSurfaceKHRMap[(uint64_t)vkObj] = pNewObjNode;
    uint32_t objIndex = objTypeToIndex(objType);
    numObjs[objIndex]++;
    numTotalObjs++;
}

static void destroy_surface_khr(VkInstance dispatchable_object, VkSurfaceKHR object)
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

static void alloc_command_buffer(VkDevice device, VkCommandPool commandPool, VkCommandBuffer vkObj, VkDebugReportObjectTypeEXT objType, VkCommandBufferLevel level)
{
    log_msg(mdd(device), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, objType, reinterpret_cast<uint64_t>(vkObj), __LINE__, OBJTRACK_NONE, "OBJTRACK",
        "OBJ[%llu] : CREATE %s object 0x%" PRIxLEAST64 , object_track_index++, string_VkDebugReportObjectTypeEXT(objType),
        reinterpret_cast<uint64_t>(vkObj));

    OBJTRACK_NODE* pNewObjNode = new OBJTRACK_NODE;
    pNewObjNode->objType   = objType;
    pNewObjNode->vkObj     = reinterpret_cast<uint64_t>(vkObj);
    pNewObjNode->parentObj = (uint64_t) commandPool;
    if (level == VK_COMMAND_BUFFER_LEVEL_SECONDARY) {
        pNewObjNode->status = OBJSTATUS_COMMAND_BUFFER_SECONDARY;
    } else {
        pNewObjNode->status = OBJSTATUS_NONE;
    }
    VkCommandBufferMap[reinterpret_cast<uint64_t>(vkObj)] = pNewObjNode;
    uint32_t objIndex = objTypeToIndex(objType);
    numObjs[objIndex]++;
    numTotalObjs++;
}

static void free_command_buffer(VkDevice device, VkCommandPool commandPool, VkCommandBuffer commandBuffer)
{
    uint64_t object_handle = reinterpret_cast<uint64_t>(commandBuffer);
    if (VkCommandBufferMap.find(object_handle) != VkCommandBufferMap.end()) {
        OBJTRACK_NODE* pNode = VkCommandBufferMap[(uint64_t)commandBuffer];

       if (pNode->parentObj != (uint64_t)(commandPool)) {
           log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, pNode->objType, object_handle, __LINE__, OBJTRACK_COMMAND_POOL_MISMATCH, "OBJTRACK",
               "FreeCommandBuffers is attempting to free Command Buffer 0x%" PRIxLEAST64 " belonging to Command Pool 0x%" PRIxLEAST64 " from pool 0x%" PRIxLEAST64 ").",
               reinterpret_cast<uint64_t>(commandBuffer), pNode->parentObj, (uint64_t)(commandPool));
       } else {

            uint32_t objIndex = objTypeToIndex(pNode->objType);
            assert(numTotalObjs > 0);
            numTotalObjs--;
            assert(numObjs[objIndex] > 0);
            numObjs[objIndex]--;
            log_msg(mdd(device), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, pNode->objType, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
               "OBJ_STAT Destroy %s obj 0x%" PRIxLEAST64 " (%" PRIu64 " total objs remain & %" PRIu64 " %s objs).",
                string_VkDebugReportObjectTypeEXT(pNode->objType), reinterpret_cast<uint64_t>(commandBuffer), numTotalObjs, numObjs[objIndex],
                string_VkDebugReportObjectTypeEXT(pNode->objType));
            delete pNode;
            VkCommandBufferMap.erase(object_handle);
        }
    } else {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT) 0, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
            "Unable to remove obj 0x%" PRIxLEAST64 ". Was it created? Has it already been destroyed?",
           object_handle);
    }
}

static void alloc_descriptor_set(VkDevice device, VkDescriptorPool descriptorPool, VkDescriptorSet vkObj, VkDebugReportObjectTypeEXT objType)
{
    log_msg(mdd(device), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, objType, (uint64_t)(vkObj), __LINE__, OBJTRACK_NONE, "OBJTRACK",
        "OBJ[%llu] : CREATE %s object 0x%" PRIxLEAST64 , object_track_index++, string_VkDebugReportObjectTypeEXT(objType),
        (uint64_t)(vkObj));

    OBJTRACK_NODE* pNewObjNode = new OBJTRACK_NODE;
    pNewObjNode->objType   = objType;
    pNewObjNode->status    = OBJSTATUS_NONE;
    pNewObjNode->vkObj     = (uint64_t)(vkObj);
    pNewObjNode->parentObj = (uint64_t) descriptorPool;
    VkDescriptorSetMap[(uint64_t)vkObj] = pNewObjNode;
    uint32_t objIndex = objTypeToIndex(objType);
    numObjs[objIndex]++;
    numTotalObjs++;
}

static void free_descriptor_set(VkDevice device, VkDescriptorPool descriptorPool, VkDescriptorSet descriptorSet)
{
    uint64_t object_handle = (uint64_t)(descriptorSet);
    if (VkDescriptorSetMap.find(object_handle) != VkDescriptorSetMap.end()) {
        OBJTRACK_NODE* pNode = VkDescriptorSetMap[(uint64_t)descriptorSet];

        if (pNode->parentObj != (uint64_t)(descriptorPool)) {
            log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, pNode->objType, object_handle, __LINE__, OBJTRACK_DESCRIPTOR_POOL_MISMATCH, "OBJTRACK",
                "FreeDescriptorSets is attempting to free descriptorSet 0x%" PRIxLEAST64 " belonging to Descriptor Pool 0x%" PRIxLEAST64 " from pool 0x%" PRIxLEAST64 ").",
                (uint64_t)(descriptorSet), pNode->parentObj, (uint64_t)(descriptorPool));
        } else {
            uint32_t objIndex = objTypeToIndex(pNode->objType);
            assert(numTotalObjs > 0);
            numTotalObjs--;
            assert(numObjs[objIndex] > 0);
            numObjs[objIndex]--;
            log_msg(mdd(device), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, pNode->objType, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
               "OBJ_STAT Destroy %s obj 0x%" PRIxLEAST64 " (%" PRIu64 " total objs remain & %" PRIu64 " %s objs).",
                string_VkDebugReportObjectTypeEXT(pNode->objType), (uint64_t)(descriptorSet), numTotalObjs, numObjs[objIndex],
                string_VkDebugReportObjectTypeEXT(pNode->objType));
            delete pNode;
            VkDescriptorSetMap.erase(object_handle);
        }
    } else {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT) 0, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",
            "Unable to remove obj 0x%" PRIxLEAST64 ". Was it created? Has it already been destroyed?",
           object_handle);
    }
}

static void create_queue(VkDevice dispatchable_object, VkQueue vkObj, VkDebugReportObjectTypeEXT objType)
{
    log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, objType, reinterpret_cast<uint64_t>(vkObj), __LINE__, OBJTRACK_NONE, "OBJTRACK",
        "OBJ[%llu] : CREATE %s object 0x%" PRIxLEAST64 , object_track_index++, string_VkDebugReportObjectTypeEXT(objType),
        reinterpret_cast<uint64_t>(vkObj));

    OBJTRACK_NODE* pNewObjNode = new OBJTRACK_NODE;
    pNewObjNode->objType = objType;
    pNewObjNode->status  = OBJSTATUS_NONE;
    pNewObjNode->vkObj  = reinterpret_cast<uint64_t>(vkObj);
    VkQueueMap[reinterpret_cast<uint64_t>(vkObj)] = pNewObjNode;
    uint32_t objIndex = objTypeToIndex(objType);
    numObjs[objIndex]++;
    numTotalObjs++;
}
static void create_swapchain_image_obj(VkDevice dispatchable_object, VkImage vkObj, VkSwapchainKHR swapchain)
{
    log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, (uint64_t) vkObj, __LINE__, OBJTRACK_NONE, "OBJTRACK",
        "OBJ[%llu] : CREATE %s object 0x%" PRIxLEAST64 , object_track_index++, "SwapchainImage",
        (uint64_t)(vkObj));

    OBJTRACK_NODE* pNewObjNode             = new OBJTRACK_NODE;
    pNewObjNode->objType                   = VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT;
    pNewObjNode->status                    = OBJSTATUS_NONE;
    pNewObjNode->vkObj                     = (uint64_t) vkObj;
    pNewObjNode->parentObj                 = (uint64_t) swapchain;
    swapchainImageMap[(uint64_t)(vkObj)] = pNewObjNode;
}

//
// Non-auto-generated API functions called by generated code
//
VkResult
explicit_CreateInstance(
    const VkInstanceCreateInfo  *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkInstance                  *pInstance)
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
    initInstanceTable(*pInstance, fpGetInstanceProcAddr, object_tracker_instance_table_map);
    VkLayerInstanceDispatchTable *pInstanceTable = get_dispatch_table(object_tracker_instance_table_map, *pInstance);

    my_data->report_data = debug_report_create_instance(
                               pInstanceTable,
                               *pInstance,
                               pCreateInfo->enabledExtensionCount,
                               pCreateInfo->ppEnabledExtensionNames);

    initObjectTracker(my_data, pAllocator);
    createInstanceRegisterExtensions(pCreateInfo, *pInstance);

    create_instance(*pInstance, *pInstance, VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT);

    return result;
}

void
explicit_GetPhysicalDeviceQueueFamilyProperties(
    VkPhysicalDevice                 gpu,
    uint32_t*                        pCount,
    VkQueueFamilyProperties*         pProperties)
{
    get_dispatch_table(object_tracker_instance_table_map, gpu)->GetPhysicalDeviceQueueFamilyProperties(gpu, pCount, pProperties);

    loader_platform_thread_lock_mutex(&objLock);
    if (pProperties != NULL)
        setGpuQueueInfoState(*pCount, pProperties);
    loader_platform_thread_unlock_mutex(&objLock);
}

VkResult
explicit_CreateDevice(
    VkPhysicalDevice         gpu,
    const VkDeviceCreateInfo *pCreateInfo,
    const VkAllocationCallbacks   *pAllocator,
    VkDevice                 *pDevice)
{
    loader_platform_thread_lock_mutex(&objLock);
    VkLayerDeviceCreateInfo *chain_info = get_chain_info(pCreateInfo, VK_LAYER_LINK_INFO);

    assert(chain_info->u.pLayerInfo);
    PFN_vkGetInstanceProcAddr fpGetInstanceProcAddr = chain_info->u.pLayerInfo->pfnNextGetInstanceProcAddr;
    PFN_vkGetDeviceProcAddr fpGetDeviceProcAddr = chain_info->u.pLayerInfo->pfnNextGetDeviceProcAddr;
    PFN_vkCreateDevice fpCreateDevice = (PFN_vkCreateDevice) fpGetInstanceProcAddr(NULL, "vkCreateDevice");
    if (fpCreateDevice == NULL) {
        loader_platform_thread_unlock_mutex(&objLock);
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    // Advance the link info for the next element on the chain
    chain_info->u.pLayerInfo = chain_info->u.pLayerInfo->pNext;

    VkResult result = fpCreateDevice(gpu, pCreateInfo, pAllocator, pDevice);
    if (result != VK_SUCCESS) {
        loader_platform_thread_unlock_mutex(&objLock);
        return result;
    }

    layer_data *my_instance_data = get_my_data_ptr(get_dispatch_key(gpu), layer_data_map);
    layer_data *my_device_data = get_my_data_ptr(get_dispatch_key(*pDevice), layer_data_map);
    my_device_data->report_data = layer_debug_report_create_device(my_instance_data->report_data, *pDevice);

    initDeviceTable(*pDevice, fpGetDeviceProcAddr, object_tracker_device_table_map);

    createDeviceRegisterExtensions(pCreateInfo, *pDevice);

    create_device(*pDevice, *pDevice, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT);

    loader_platform_thread_unlock_mutex(&objLock);
    return result;
}

VkResult explicit_EnumeratePhysicalDevices(VkInstance instance, uint32_t* pPhysicalDeviceCount, VkPhysicalDevice* pPhysicalDevices)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
    skipCall |= validate_instance(instance, instance, VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = get_dispatch_table(object_tracker_instance_table_map, instance)->EnumeratePhysicalDevices(instance, pPhysicalDeviceCount, pPhysicalDevices);
    loader_platform_thread_lock_mutex(&objLock);
    if (result == VK_SUCCESS) {
        if (pPhysicalDevices) {
            for (uint32_t i = 0; i < *pPhysicalDeviceCount; i++) {
                create_physical_device(instance, pPhysicalDevices[i], VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT);
            }
        }
    }
    loader_platform_thread_unlock_mutex(&objLock);
    return result;
}

void
explicit_GetDeviceQueue(
    VkDevice  device,
    uint32_t  queueNodeIndex,
    uint32_t  queueIndex,
    VkQueue  *pQueue)
{
    loader_platform_thread_lock_mutex(&objLock);
    validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);

    get_dispatch_table(object_tracker_device_table_map, device)->GetDeviceQueue(device, queueNodeIndex, queueIndex, pQueue);

    loader_platform_thread_lock_mutex(&objLock);
    addQueueInfo(queueNodeIndex, *pQueue);
    create_queue(device, *pQueue, VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT);
    loader_platform_thread_unlock_mutex(&objLock);
}

VkResult
explicit_MapMemory(
    VkDevice         device,
    VkDeviceMemory   mem,
    VkDeviceSize     offset,
    VkDeviceSize     size,
    VkFlags          flags,
    void           **ppData)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
    skipCall |= set_device_memory_status(device, mem, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, OBJSTATUS_GPU_MEM_MAPPED);
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall == VK_TRUE)
        return VK_ERROR_VALIDATION_FAILED_EXT;

    VkResult result = get_dispatch_table(object_tracker_device_table_map, device)->MapMemory(device, mem, offset, size, flags, ppData);

    return result;
}

void
explicit_UnmapMemory(
    VkDevice       device,
    VkDeviceMemory mem)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
    skipCall |= reset_device_memory_status(device, mem, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, OBJSTATUS_GPU_MEM_MAPPED);
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall == VK_TRUE)
        return;

    get_dispatch_table(object_tracker_device_table_map, device)->UnmapMemory(device, mem);
}

VkResult
explicit_QueueBindSparse(
    VkQueue                       queue,
    uint32_t                                    bindInfoCount,
    const VkBindSparseInfo*                     pBindInfo,
    VkFence                                     fence)
{
    loader_platform_thread_lock_mutex(&objLock);
    validateQueueFlags(queue, "QueueBindSparse");

    for (uint32_t i = 0; i < bindInfoCount; i++) {
        for (uint32_t j = 0; j < pBindInfo[i].bufferBindCount; j++)
            validate_buffer(queue, pBindInfo[i].pBufferBinds[j].buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, false);
        for (uint32_t j = 0; j < pBindInfo[i].imageOpaqueBindCount; j++)
            validate_image(queue, pBindInfo[i].pImageOpaqueBinds[j].image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, false);
        for (uint32_t j = 0; j < pBindInfo[i].imageBindCount; j++)
            validate_image(queue, pBindInfo[i].pImageBinds[j].image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, false);
    }

    loader_platform_thread_unlock_mutex(&objLock);

    VkResult result = get_dispatch_table(object_tracker_device_table_map, queue)->QueueBindSparse(queue, bindInfoCount, pBindInfo, fence);
    return result;
}

VkResult
explicit_AllocateCommandBuffers(
    VkDevice                           device,
    const VkCommandBufferAllocateInfo *pAllocateInfo,
    VkCommandBuffer*                   pCommandBuffers)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
    skipCall |= validate_command_pool(device, pAllocateInfo->commandPool, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_POOL_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);

    if (skipCall) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }

    VkResult result = get_dispatch_table(object_tracker_device_table_map, device)->AllocateCommandBuffers(
        device, pAllocateInfo, pCommandBuffers);

    loader_platform_thread_lock_mutex(&objLock);
    for (uint32_t i = 0; i < pAllocateInfo->commandBufferCount; i++) {
        alloc_command_buffer(device, pAllocateInfo->commandPool, pCommandBuffers[i], VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, pAllocateInfo->level);
    }
    loader_platform_thread_unlock_mutex(&objLock);

    return result;
}

VkResult
explicit_AllocateDescriptorSets(
    VkDevice                           device,
    const VkDescriptorSetAllocateInfo *pAllocateInfo,
    VkDescriptorSet                   *pDescriptorSets)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
    skipCall |= validate_descriptor_pool(device, pAllocateInfo->descriptorPool, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT, false);
    for (uint32_t i = 0; i < pAllocateInfo->descriptorSetCount; i++) {
        skipCall |= validate_descriptor_set_layout(device, pAllocateInfo->pSetLayouts[i], VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT, false);
    }
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED_EXT;

    VkResult result = get_dispatch_table(object_tracker_device_table_map, device)->AllocateDescriptorSets(
        device, pAllocateInfo, pDescriptorSets);

    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&objLock);
        for (uint32_t i = 0; i < pAllocateInfo->descriptorSetCount; i++) {
            alloc_descriptor_set(device, pAllocateInfo->descriptorPool, pDescriptorSets[i], VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT);
        }
        loader_platform_thread_unlock_mutex(&objLock);
    }

    return result;
}

void
explicit_FreeCommandBuffers(
    VkDevice               device,
    VkCommandPool          commandPool,
    uint32_t               commandBufferCount,
    const VkCommandBuffer *pCommandBuffers)
{
    loader_platform_thread_lock_mutex(&objLock);
    validate_command_pool(device, commandPool, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_POOL_EXT, false);
    validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);

    get_dispatch_table(object_tracker_device_table_map, device)->FreeCommandBuffers(device,
        commandPool, commandBufferCount, pCommandBuffers);

    loader_platform_thread_lock_mutex(&objLock);
    for (uint32_t i = 0; i < commandBufferCount; i++)
    {
        free_command_buffer(device, commandPool, *pCommandBuffers);
        pCommandBuffers++;
    }
    loader_platform_thread_unlock_mutex(&objLock);
}

void
explicit_DestroySwapchainKHR(
    VkDevice                    device,
    VkSwapchainKHR              swapchain,
    const VkAllocationCallbacks *pAllocator)
{
    loader_platform_thread_lock_mutex(&objLock);
    // A swapchain's images are implicitly deleted when the swapchain is deleted.
    // Remove this swapchain's images from our map of such images.
    unordered_map<uint64_t, OBJTRACK_NODE*>::iterator itr = swapchainImageMap.begin();
    while (itr != swapchainImageMap.end()) {
        OBJTRACK_NODE* pNode = (*itr).second;
        if (pNode->parentObj == (uint64_t)(swapchain)) {
           swapchainImageMap.erase(itr++);
        } else {
           ++itr;
        }
    }
    destroy_swapchain_khr(device, swapchain);
    loader_platform_thread_unlock_mutex(&objLock);

    get_dispatch_table(object_tracker_device_table_map, device)->DestroySwapchainKHR(device, swapchain, pAllocator);
}

void
explicit_FreeMemory(
    VkDevice       device,
    VkDeviceMemory mem,
    const VkAllocationCallbacks* pAllocator)
{
    loader_platform_thread_lock_mutex(&objLock);
    validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);

    get_dispatch_table(object_tracker_device_table_map, device)->FreeMemory(device, mem, pAllocator);

    loader_platform_thread_lock_mutex(&objLock);
    destroy_device_memory(device, mem);
    loader_platform_thread_unlock_mutex(&objLock);
}

VkResult
explicit_FreeDescriptorSets(
    VkDevice               device,
    VkDescriptorPool       descriptorPool,
    uint32_t               count,
    const VkDescriptorSet *pDescriptorSets)
{
    loader_platform_thread_lock_mutex(&objLock);
    validate_descriptor_pool(device, descriptorPool, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT, false);
    validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    VkResult result = get_dispatch_table(object_tracker_device_table_map, device)->FreeDescriptorSets(device, descriptorPool, count, pDescriptorSets);

    loader_platform_thread_lock_mutex(&objLock);
    for (uint32_t i=0; i<count; i++)
    {
        free_descriptor_set(device, descriptorPool, *pDescriptorSets++);
    }
    loader_platform_thread_unlock_mutex(&objLock);
    return result;
}

void
explicit_DestroyDescriptorPool(
    VkDevice                     device,
    VkDescriptorPool             descriptorPool,
    const VkAllocationCallbacks *pAllocator)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
    skipCall |= validate_descriptor_pool(device, descriptorPool, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall) {
        return;
    }
    // A DescriptorPool's descriptor sets are implicitly deleted when the pool is deleted.
    // Remove this pool's descriptor sets from our descriptorSet map.
    loader_platform_thread_lock_mutex(&objLock);
    unordered_map<uint64_t, OBJTRACK_NODE*>::iterator itr = VkDescriptorSetMap.begin();
    while (itr != VkDescriptorSetMap.end()) {
        OBJTRACK_NODE* pNode = (*itr).second;
        auto del_itr = itr++;
        if (pNode->parentObj == (uint64_t)(descriptorPool)) {
            destroy_descriptor_set(device, (VkDescriptorSet)((*del_itr).first));
        }
    }
    destroy_descriptor_pool(device, descriptorPool);
    loader_platform_thread_unlock_mutex(&objLock);
    get_dispatch_table(object_tracker_device_table_map, device)->DestroyDescriptorPool(device, descriptorPool, pAllocator);
}

void
explicit_DestroyCommandPool(
    VkDevice                     device,
    VkCommandPool                commandPool,
    const VkAllocationCallbacks *pAllocator)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
    skipCall |= validate_command_pool(device, commandPool, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_POOL_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall) {
        return;
    }
    loader_platform_thread_lock_mutex(&objLock);
    // A CommandPool's command buffers are implicitly deleted when the pool is deleted.
    // Remove this pool's cmdBuffers from our cmd buffer map.
    unordered_map<uint64_t, OBJTRACK_NODE*>::iterator itr = VkCommandBufferMap.begin();
    unordered_map<uint64_t, OBJTRACK_NODE*>::iterator del_itr;
    while (itr != VkCommandBufferMap.end()) {
        OBJTRACK_NODE* pNode = (*itr).second;
        del_itr = itr++;
        if (pNode->parentObj == (uint64_t)(commandPool)) {
            destroy_command_buffer(reinterpret_cast<VkCommandBuffer>((*del_itr).first),
                                   reinterpret_cast<VkCommandBuffer>((*del_itr).first));
        }
    }
    destroy_command_pool(device, commandPool);
    loader_platform_thread_unlock_mutex(&objLock);
    get_dispatch_table(object_tracker_device_table_map, device)->DestroyCommandPool(device, commandPool, pAllocator);
}

VkResult
explicit_GetSwapchainImagesKHR(
    VkDevice        device,
    VkSwapchainKHR  swapchain,
    uint32_t       *pCount,
    VkImage        *pSwapchainImages)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED_EXT;

    VkResult result = get_dispatch_table(object_tracker_device_table_map, device)->GetSwapchainImagesKHR(device, swapchain, pCount, pSwapchainImages);

    if (pSwapchainImages != NULL) {
        loader_platform_thread_lock_mutex(&objLock);
        for (uint32_t i = 0; i < *pCount; i++) {
            create_swapchain_image_obj(device, pSwapchainImages[i], swapchain);
        }
        loader_platform_thread_unlock_mutex(&objLock);
    }
    return result;
}

// TODO: Add special case to codegen to cover validating all the pipelines instead of just the first
VkResult
explicit_CreateGraphicsPipelines(
    VkDevice                            device,
    VkPipelineCache                     pipelineCache,
    uint32_t                            createInfoCount,
    const VkGraphicsPipelineCreateInfo *pCreateInfos,
    const VkAllocationCallbacks        *pAllocator,
    VkPipeline                         *pPipelines)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
    if (pCreateInfos) {
        for (uint32_t idx0=0; idx0<createInfoCount; ++idx0) {
            if (pCreateInfos[idx0].basePipelineHandle) {
                skipCall |= validate_pipeline(device, pCreateInfos[idx0].basePipelineHandle, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT, true);
            }
            if (pCreateInfos[idx0].layout) {
                skipCall |= validate_pipeline_layout(device, pCreateInfos[idx0].layout, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT, false);
            }
            if (pCreateInfos[idx0].pStages) {
                for (uint32_t idx1=0; idx1<pCreateInfos[idx0].stageCount; ++idx1) {
                    if (pCreateInfos[idx0].pStages[idx1].module) {
                        skipCall |= validate_shader_module(device, pCreateInfos[idx0].pStages[idx1].module, VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT, false);
                    }
                }
            }
            if (pCreateInfos[idx0].renderPass) {
                skipCall |= validate_render_pass(device, pCreateInfos[idx0].renderPass, VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT, false);
            }
        }
    }
    if (pipelineCache) {
        skipCall |= validate_pipeline_cache(device, pipelineCache, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_CACHE_EXT, false);
    }
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = get_dispatch_table(object_tracker_device_table_map, device)->CreateGraphicsPipelines(device, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
    loader_platform_thread_lock_mutex(&objLock);
    if (result == VK_SUCCESS) {
        for (uint32_t idx2 = 0; idx2 < createInfoCount; ++idx2) {
            create_pipeline(device, pPipelines[idx2], VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT);
        }
    }
    loader_platform_thread_unlock_mutex(&objLock);
    return result;
}

// TODO: Add special case to codegen to cover validating all the pipelines instead of just the first
VkResult
explicit_CreateComputePipelines(
    VkDevice                           device,
    VkPipelineCache                    pipelineCache,
    uint32_t                           createInfoCount,
    const VkComputePipelineCreateInfo *pCreateInfos,
    const VkAllocationCallbacks       *pAllocator,
    VkPipeline                        *pPipelines)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
    if (pCreateInfos) {
        for (uint32_t idx0=0; idx0<createInfoCount; ++idx0) {
            if (pCreateInfos[idx0].basePipelineHandle) {
                skipCall |= validate_pipeline(device, pCreateInfos[idx0].basePipelineHandle, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT, true);
            }
            if (pCreateInfos[idx0].layout) {
                skipCall |= validate_pipeline_layout(device, pCreateInfos[idx0].layout, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT, false);
            }
            if (pCreateInfos[idx0].stage.module) {
                skipCall |= validate_shader_module(device, pCreateInfos[idx0].stage.module, VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT, false);
            }
        }
    }
    if (pipelineCache) {
        skipCall |= validate_pipeline_cache(device, pipelineCache, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_CACHE_EXT, false);
    }
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = get_dispatch_table(object_tracker_device_table_map, device)->CreateComputePipelines(device, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
    loader_platform_thread_lock_mutex(&objLock);
    if (result == VK_SUCCESS) {
        for (uint32_t idx1 = 0; idx1 < createInfoCount; ++idx1) {
            create_pipeline(device, pPipelines[idx1], VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT);
        }
    }
    loader_platform_thread_unlock_mutex(&objLock);
    return result;
}
