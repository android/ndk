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
 * Author: Tobin Ehlis <tobin@lunarg.com>
 * Author: Mark Lobodzinski <mark@lunarg.com>
 */

#include "vulkan/vk_layer.h"
#include <vector>

using namespace std;

// Device Limits ERROR codes
typedef enum _DEV_LIMITS_ERROR
{
    DEVLIMITS_NONE,                             // Used for INFO & other non-error messages
    DEVLIMITS_INVALID_INSTANCE,                 // Invalid instance used
    DEVLIMITS_INVALID_PHYSICAL_DEVICE,          // Invalid physical device used
    DEVLIMITS_INVALID_INHERITED_QUERY,          // Invalid use of inherited query
    DEVLIMITS_MUST_QUERY_COUNT,                 // Failed to make initial call to an API to query the count
    DEVLIMITS_MUST_QUERY_PROPERTIES,            // Failed to make initial call to an API to query properties
    DEVLIMITS_INVALID_CALL_SEQUENCE,            // Flag generic case of an invalid call sequence by the app
    DEVLIMITS_INVALID_FEATURE_REQUESTED,        // App requested a feature not supported by physical device
    DEVLIMITS_COUNT_MISMATCH,                   // App requesting a count value different than actual value
    DEVLIMITS_INVALID_QUEUE_CREATE_REQUEST,     // Invalid queue requested based on queue family properties
    DEVLIMITS_LIMITS_VIOLATION,                 // Driver-specified limits/properties were exceeded
    DEVLIMITS_INVALID_UNIFORM_BUFFER_OFFSET,    // Uniform buffer offset violates device limit granularity
    DEVLIMITS_INVALID_STORAGE_BUFFER_OFFSET,    // Storage buffer offset violates device limit granularity
} DEV_LIMITS_ERROR;

typedef enum _CALL_STATE
{
    UNCALLED,       // Function has not been called
    QUERY_COUNT,    // Function called once to query a count
    QUERY_DETAILS,  // Function called w/ a count to query details
} CALL_STATE;

typedef struct _INSTANCE_STATE
{
    // Track the call state and array size for physical devices
    CALL_STATE vkEnumeratePhysicalDevicesState;
    uint32_t physicalDevicesCount;
    _INSTANCE_STATE():vkEnumeratePhysicalDevicesState(UNCALLED), physicalDevicesCount(0) {};
} INSTANCE_STATE;

typedef struct _PHYSICAL_DEVICE_STATE
{
    // Track the call state and array sizes for various query functions
    CALL_STATE vkGetPhysicalDeviceQueueFamilyPropertiesState;
    uint32_t queueFamilyPropertiesCount;
    CALL_STATE vkGetPhysicalDeviceLayerPropertiesState;
    uint32_t deviceLayerCount;
    CALL_STATE vkGetPhysicalDeviceExtensionPropertiesState;
    uint32_t deviceExtensionCount;
    CALL_STATE vkGetPhysicalDeviceFeaturesState;
    _PHYSICAL_DEVICE_STATE():vkGetPhysicalDeviceQueueFamilyPropertiesState(UNCALLED), queueFamilyPropertiesCount(0),
                             vkGetPhysicalDeviceLayerPropertiesState(UNCALLED), deviceLayerCount(0),
                             vkGetPhysicalDeviceExtensionPropertiesState(UNCALLED), deviceExtensionCount(0),
                             vkGetPhysicalDeviceFeaturesState(UNCALLED) {};
} PHYSICAL_DEVICE_STATE;

