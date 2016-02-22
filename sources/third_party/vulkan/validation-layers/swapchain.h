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

#ifndef SWAPCHAIN_H
#define SWAPCHAIN_H

#include "vulkan/vk_layer.h"
#include "vk_layer_config.h"
#include "vk_layer_logging.h"
#include <vector>
#include <unordered_map>

using namespace std;


// Swapchain ERROR codes
typedef enum _SWAPCHAIN_ERROR
{
    SWAPCHAIN_INVALID_HANDLE,                   // Handle used that isn't currently valid
    SWAPCHAIN_NULL_POINTER,                     // Pointer set to NULL, instead of being a valid pointer
    SWAPCHAIN_EXT_NOT_ENABLED_BUT_USED,         // Did not enable WSI extension, but called WSI function 
    SWAPCHAIN_DEL_OBJECT_BEFORE_CHILDREN,     // Called vkDestroyDevice() before vkDestroySwapchainKHR()
    SWAPCHAIN_CREATE_UNSUPPORTED_SURFACE,       // Called vkCreateSwapchainKHR() with a pCreateInfo->surface that wasn't seen as supported by vkGetPhysicalDeviceSurfaceSupportKHR for the device
    SWAPCHAIN_CREATE_SWAP_WITHOUT_QUERY,        // Called vkCreateSwapchainKHR() without calling a query (e.g. vkGetPhysicalDeviceSurfaceCapabilitiesKHR())
    SWAPCHAIN_CREATE_SWAP_BAD_MIN_IMG_COUNT,    // Called vkCreateSwapchainKHR() with out-of-bounds minImageCount
    SWAPCHAIN_CREATE_SWAP_OUT_OF_BOUNDS_EXTENTS,// Called vkCreateSwapchainKHR() with out-of-bounds imageExtent
    SWAPCHAIN_CREATE_SWAP_EXTENTS_NO_MATCH_WIN, // Called vkCreateSwapchainKHR() with imageExtent that doesn't match window's extent
    SWAPCHAIN_CREATE_SWAP_BAD_PRE_TRANSFORM,    // Called vkCreateSwapchainKHR() with a non-supported preTransform
    SWAPCHAIN_CREATE_SWAP_BAD_COMPOSITE_ALPHA,  // Called vkCreateSwapchainKHR() with a non-supported compositeAlpha
    SWAPCHAIN_CREATE_SWAP_BAD_IMG_ARRAY_SIZE,   // Called vkCreateSwapchainKHR() with a non-supported imageArraySize
    SWAPCHAIN_CREATE_SWAP_BAD_IMG_USAGE_FLAGS,  // Called vkCreateSwapchainKHR() with a non-supported imageUsageFlags
    SWAPCHAIN_CREATE_SWAP_BAD_IMG_COLOR_SPACE,  // Called vkCreateSwapchainKHR() with a non-supported imageColorSpace
    SWAPCHAIN_CREATE_SWAP_BAD_IMG_FORMAT,       // Called vkCreateSwapchainKHR() with a non-supported imageFormat
    SWAPCHAIN_CREATE_SWAP_BAD_IMG_FMT_CLR_SP,   // Called vkCreateSwapchainKHR() with a non-supported imageColorSpace
    SWAPCHAIN_CREATE_SWAP_BAD_PRESENT_MODE,     // Called vkCreateSwapchainKHR() with a non-supported presentMode
    SWAPCHAIN_CREATE_SWAP_BAD_SHARING_MODE,     // Called vkCreateSwapchainKHR() with a non-supported imageSharingMode
    SWAPCHAIN_CREATE_SWAP_BAD_SHARING_VALUES,   // Called vkCreateSwapchainKHR() with bad values when imageSharingMode is VK_SHARING_MODE_CONCURRENT
    SWAPCHAIN_CREATE_SWAP_DIFF_SURFACE,         // Called vkCreateSwapchainKHR() with pCreateInfo->oldSwapchain that has a different surface than pCreateInfo->surface
    SWAPCHAIN_DESTROY_SWAP_DIFF_DEVICE,         // Called vkDestroySwapchainKHR() with a different VkDevice than vkCreateSwapchainKHR()
    SWAPCHAIN_APP_OWNS_TOO_MANY_IMAGES,         // vkAcquireNextImageKHR() asked for more images than are available
    SWAPCHAIN_INDEX_TOO_LARGE,                  // Index is too large for swapchain
    SWAPCHAIN_INDEX_NOT_IN_USE,                 // vkQueuePresentKHR() given index that is not owned by app
    SWAPCHAIN_BAD_BOOL,                         // VkBool32 that doesn't have value of VK_TRUE or VK_FALSE (e.g. is a non-zero form of true)
    SWAPCHAIN_INVALID_COUNT,                    // Second time a query called, the pCount value didn't match first time
    SWAPCHAIN_WRONG_STYPE,                      // The sType for a struct has the wrong value
    SWAPCHAIN_WRONG_NEXT,                       // The pNext for a struct is not NULL
    SWAPCHAIN_ZERO_VALUE,                       // A value should be non-zero
    SWAPCHAIN_INCOMPATIBLE_ALLOCATOR,           // pAllocator must be compatible (i.e. NULL or not) when object is created and destroyed
    SWAPCHAIN_DID_NOT_QUERY_QUEUE_FAMILIES,     // A function using a queueFamilyIndex was called before vkGetPhysicalDeviceQueueFamilyProperties() was called
    SWAPCHAIN_QUEUE_FAMILY_INDEX_TOO_LARGE,     // A queueFamilyIndex value is not less than pQueueFamilyPropertyCount returned by vkGetPhysicalDeviceQueueFamilyProperties()
    SWAPCHAIN_SURFACE_NOT_SUPPORTED_WITH_QUEUE, // A surface is not supported by a given queueFamilyIndex, as seen by vkGetPhysicalDeviceSurfaceSupportKHR()
} SWAPCHAIN_ERROR;


// The following is for logging error messages:
#define LAYER_NAME (char *) "Swapchain"
#define LOG_ERROR_NON_VALID_OBJ(objType, type, obj)                     \
    (my_data) ?                                                         \
        log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (objType), \
                (uint64_t) (obj), __LINE__, SWAPCHAIN_INVALID_HANDLE, LAYER_NAME, \
                "%s() called with a non-valid %s.", __FUNCTION__, (obj)) \
    : VK_FALSE
#define LOG_ERROR_NULL_POINTER(objType, type, obj)                     \
    (my_data) ?                                                         \
        log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (objType), \
                (uint64_t) (obj), 0, SWAPCHAIN_NULL_POINTER, LAYER_NAME, \
                "%s() called with NULL pointer %s.", __FUNCTION__, (obj)) \
    : VK_FALSE
#define LOG_ERROR_INVALID_COUNT(objType, type, obj, obj2, val, val2)    \
    (my_data) ?                                                         \
        log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (objType), \
                (uint64_t) (obj), 0, SWAPCHAIN_INVALID_COUNT, LAYER_NAME, \
                "%s() called with non-NULL %s, and with %s set to a "   \
                "value (%d) that is greater than the value (%d) that "  \
                "was returned when %s was NULL.",                      \
                __FUNCTION__, (obj2), (obj), (val), (val2), (obj2))    \
    : VK_FALSE
#define LOG_ERROR_WRONG_STYPE(objType, type, obj, val)          \
    (my_data) ?                                                         \
        log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (objType), \
                (uint64_t) (obj), 0, SWAPCHAIN_WRONG_STYPE, LAYER_NAME, \
                "%s() called with the wrong value for %s->sType "       \
                "(expected %s).",                                       \
                __FUNCTION__, (obj), (val))                             \
    : VK_FALSE
#define LOG_ERROR_ZERO_VALUE(objType, type, obj)                        \
    (my_data) ?                                                         \
        log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (objType), \
                (uint64_t) (obj), 0, SWAPCHAIN_ZERO_VALUE, LAYER_NAME,  \
                "%s() called with a zero value for %s.",                \
                __FUNCTION__, (obj))                                    \
    : VK_FALSE
#define LOG_ERROR(objType, type, obj, enm, fmt, ...)                    \
    (my_data) ?                                                         \
        log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (objType), \
                (uint64_t) (obj), __LINE__, (enm), LAYER_NAME, (fmt), __VA_ARGS__) \
    : VK_FALSE
#define LOG_ERROR_QUEUE_FAMILY_INDEX_TOO_LARGE(objType, type, obj, val1, val2) \
    (my_data) ?                                                         \
        log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (objType), \
                (uint64_t) (obj), 0, SWAPCHAIN_QUEUE_FAMILY_INDEX_TOO_LARGE, LAYER_NAME,  \
                "%s() called with a queueFamilyIndex that is too "      \
                "large (i.e. %d).  The maximum value (returned "        \
                "by vkGetPhysicalDeviceQueueFamilyProperties) is "      \
                "only %d.\n",                                           \
                __FUNCTION__, (val1), (val2))                           \
    : VK_FALSE
#define LOG_PERF_WARNING(objType, type, obj, enm, fmt, ...)             \
    (my_data) ?                                                         \
        log_msg(my_data->report_data, VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT, (objType), \
                (uint64_t) (obj), __LINE__, (enm), LAYER_NAME, (fmt), __VA_ARGS__) \
    : VK_FALSE
#define LOG_INFO_WRONG_NEXT(objType, type, obj)                         \
    (my_data) ?                                                         \
        log_msg(my_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, (objType), \
                (uint64_t) (obj), 0, SWAPCHAIN_WRONG_NEXT, LAYER_NAME, \
                "%s() called with non-NULL value for %s->pNext.",       \
                __FUNCTION__, (obj))                                    \
    : VK_FALSE


// NOTE: The following struct's/typedef's are for keeping track of
// info that is used for validating the WSI extensions.

// Forward declarations:
struct _SwpInstance;
struct _SwpSurface;
struct _SwpPhysicalDevice;
struct _SwpDevice;
struct _SwpSwapchain;
struct _SwpImage;
struct _SwpQueue;

typedef _SwpInstance SwpInstance;
typedef _SwpSurface SwpSurface;;
typedef _SwpPhysicalDevice SwpPhysicalDevice;
typedef _SwpDevice SwpDevice;
typedef _SwpSwapchain SwpSwapchain;
typedef _SwpImage SwpImage;
typedef _SwpQueue SwpQueue;

// Create one of these for each VkInstance:
struct _SwpInstance {
    // The actual handle for this VkInstance:
    VkInstance instance;

    // Remember the VkSurfaceKHR's that are created for this VkInstance:
    unordered_map<VkSurfaceKHR, SwpSurface*> surfaces;

    // When vkEnumeratePhysicalDevices is called, the VkPhysicalDevice's are
    // remembered:
    unordered_map<const void*, SwpPhysicalDevice*> physicalDevices;

    // Set to true if VK_KHR_SURFACE_EXTENSION_NAME was enabled for this VkInstance:
    bool surfaceExtensionEnabled;

    // TODO: Add additional booleans for platform-specific extensions:
#ifdef VK_USE_PLATFORM_ANDROID_KHR
    // Set to true if VK_KHR_ANDROID_SURFACE_EXTENSION_NAME was enabled for this VkInstance:
    bool androidSurfaceExtensionEnabled;
#endif // VK_USE_PLATFORM_ANDROID_KHR
#ifdef VK_USE_PLATFORM_MIR_KHR
    // Set to true if VK_KHR_MIR_SURFACE_EXTENSION_NAME was enabled for this VkInstance:
    bool mirSurfaceExtensionEnabled;
#endif // VK_USE_PLATFORM_MIR_KHR
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
    // Set to true if VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME was enabled for this VkInstance:
    bool waylandSurfaceExtensionEnabled;
#endif // VK_USE_PLATFORM_WAYLAND_KHR
#ifdef VK_USE_PLATFORM_WIN32_KHR
    // Set to true if VK_KHR_WIN32_SURFACE_EXTENSION_NAME was enabled for this VkInstance:
    bool win32SurfaceExtensionEnabled;
#endif // VK_USE_PLATFORM_WIN32_KHR
#ifdef VK_USE_PLATFORM_XCB_KHR
    // Set to true if VK_KHR_XCB_SURFACE_EXTENSION_NAME was enabled for this VkInstance:
    bool xcbSurfaceExtensionEnabled;
#endif // VK_USE_PLATFORM_XCB_KHR
#ifdef VK_USE_PLATFORM_XLIB_KHR
    // Set to true if VK_KHR_XLIB_SURFACE_EXTENSION_NAME was enabled for this VkInstance:
    bool xlibSurfaceExtensionEnabled;
#endif // VK_USE_PLATFORM_XLIB_KHR
};
 
// Create one of these for each VkSurfaceKHR:
struct _SwpSurface {
    // The actual handle for this VkSurfaceKHR:
    VkSurfaceKHR surface;

    // VkInstance that this VkSurfaceKHR is associated with:
    SwpInstance *pInstance;

    // When vkCreateSwapchainKHR is called, the VkSwapchainKHR's are
    // remembered:
    unordered_map<VkSwapchainKHR, SwpSwapchain*> swapchains;

    // 'true' if pAllocator was non-NULL when vkCreate*SurfaceKHR was called:
    bool usedAllocatorToCreate;

    // Value of pQueueFamilyPropertyCount that was returned by the
    // vkGetPhysicalDeviceQueueFamilyProperties() function:
    uint32_t numQueueFamilyIndexSupport;
    // Array of VkBool32's that is intialized by the
    // vkGetPhysicalDeviceSurfaceSupportKHR() function.  First call for a given
    // surface allocates and initializes this array to false for all
    // queueFamilyIndex's (and sets numQueueFamilyIndexSupport to non-zero).
    // All calls set the entry for a given queueFamilyIndex:
    VkBool32 *pQueueFamilyIndexSupport;
};

// Create one of these for each VkPhysicalDevice within a VkInstance:
struct _SwpPhysicalDevice {
    // The actual handle for this VkPhysicalDevice:
    VkPhysicalDevice physicalDevice;

    // Corresponding VkDevice (and info) to this VkPhysicalDevice:
    SwpDevice *pDevice;

    // VkInstance that this VkPhysicalDevice is associated with:
    SwpInstance *pInstance;

    // Records results of vkGetPhysicalDeviceQueueFamilyProperties()'s
    // numOfQueueFamilies parameter when pQueueFamilyProperties is NULL:
    bool gotQueueFamilyPropertyCount;
    uint32_t numOfQueueFamilies;

    // Record all surfaces that vkGetPhysicalDeviceSurfaceSupportKHR() was
    // called for:
    unordered_map<VkSurfaceKHR, SwpSurface*> supportedSurfaces;

// TODO: Record/use this info per-surface, not per-device, once a
// non-dispatchable surface object is added to WSI:
    // Results of vkGetPhysicalDeviceSurfaceCapabilitiesKHR():
    bool gotSurfaceCapabilities;
    VkSurfaceCapabilitiesKHR surfaceCapabilities;

// TODO: Record/use this info per-surface, not per-device, once a
// non-dispatchable surface object is added to WSI:
    // Count and VkSurfaceFormatKHR's returned by vkGetPhysicalDeviceSurfaceFormatsKHR():
    uint32_t surfaceFormatCount;
    VkSurfaceFormatKHR* pSurfaceFormats;

// TODO: Record/use this info per-surface, not per-device, once a
// non-dispatchable surface object is added to WSI:
    // Count and VkPresentModeKHR's returned by vkGetPhysicalDeviceSurfacePresentModesKHR():
    uint32_t presentModeCount;
    VkPresentModeKHR* pPresentModes;
};

// Create one of these for each VkDevice within a VkInstance:
struct _SwpDevice {
    // The actual handle for this VkDevice:
    VkDevice device;

    // Corresponding VkPhysicalDevice (and info) to this VkDevice:
    SwpPhysicalDevice *pPhysicalDevice;

    // Set to true if VK_KHR_SWAPCHAIN_EXTENSION_NAME was enabled:
    bool swapchainExtensionEnabled;

    // When vkCreateSwapchainKHR is called, the VkSwapchainKHR's are
    // remembered:
    unordered_map<VkSwapchainKHR, SwpSwapchain*> swapchains;

    // When vkGetDeviceQueue is called, the VkQueue's are remembered:
    unordered_map<VkQueue, SwpQueue*> queues;
};

// Create one of these for each VkImage within a VkSwapchainKHR:
struct _SwpImage {
    // The actual handle for this VkImage:
    VkImage image;

    // Corresponding VkSwapchainKHR (and info) to this VkImage:
    SwpSwapchain *pSwapchain;

    // true if application got this image from vkAcquireNextImageKHR(), and
    // hasn't yet called vkQueuePresentKHR() for it; otherwise false:
    bool ownedByApp;
};

// Create one of these for each VkSwapchainKHR within a VkDevice:
struct _SwpSwapchain {
    // The actual handle for this VkSwapchainKHR:
    VkSwapchainKHR swapchain;

    // Corresponding VkDevice (and info) to this VkSwapchainKHR:
    SwpDevice *pDevice;

    // Corresponding VkSurfaceKHR to this VkSwapchainKHR:
    SwpSurface *pSurface;

    // When vkGetSwapchainImagesKHR is called, the VkImage's are
    // remembered:
    uint32_t imageCount;
    unordered_map<int, SwpImage> images;

    // 'true' if pAllocator was non-NULL when vkCreateSwapchainKHR was called:
    bool usedAllocatorToCreate;
};

// Create one of these for each VkQueue within a VkDevice:
struct _SwpQueue {
    // The actual handle for this VkQueue:
    VkQueue queue;

    // Corresponding VkDevice (and info) to this VkSwapchainKHR:
    SwpDevice *pDevice;

    // Which queueFamilyIndex this VkQueue is associated with:
    uint32_t queueFamilyIndex;
};

struct layer_data {
    debug_report_data *report_data;
    std::vector<VkDebugReportCallbackEXT> logging_callback;
    VkLayerDispatchTable* device_dispatch_table;
    VkLayerInstanceDispatchTable* instance_dispatch_table;
    // NOTE: The following are for keeping track of info that is used for
    // validating the WSI extensions.
    std::unordered_map<void *, SwpInstance>       instanceMap;
    std::unordered_map<VkSurfaceKHR, SwpSurface>        surfaceMap;
    std::unordered_map<void *, SwpPhysicalDevice> physicalDeviceMap;
    std::unordered_map<void *, SwpDevice>         deviceMap;
    std::unordered_map<VkSwapchainKHR, SwpSwapchain>    swapchainMap;
    std::unordered_map<void *, SwpQueue>          queueMap;

    layer_data() :
        report_data(nullptr),
        device_dispatch_table(nullptr),
        instance_dispatch_table(nullptr)
    {};
};

#endif // SWAPCHAIN_H
