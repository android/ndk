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
 * Author: Jeremy Hayes <jeremy@lunarg.com>
 * Author: Tony Barbour <tony@LunarG.com>
 * Author: Mark Lobodzinski <mark@LunarG.com>
 * Author: Dustin Graves <dustin@lunarg.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <string>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "vk_loader_platform.h"
#include "vulkan/vk_layer.h"
#include "vk_layer_config.h"
#include "vk_enum_validate_helper.h"
#include "vk_struct_validate_helper.h"

#include "vk_layer_table.h"
#include "vk_layer_data.h"
#include "vk_layer_logging.h"
#include "vk_layer_extension_utils.h"
#include "vk_layer_utils.h"

#include "param_check.h"

struct layer_data {
    debug_report_data *report_data;
    std::vector<VkDebugReportCallbackEXT> logging_callback;

    //TODO: Split instance/device structs
    //Device Data
    //Map for queue family index to queue count
    std::unordered_map<uint32_t, uint32_t> queueFamilyIndexMap;

    layer_data() :
        report_data(nullptr)
    {};
};

static std::unordered_map<void*, layer_data*> layer_data_map;
static device_table_map pc_device_table_map;
static instance_table_map pc_instance_table_map;

// "my instance data"
debug_report_data *mid(VkInstance object)
{
    dispatch_key key = get_dispatch_key(object);
    layer_data *data = get_my_data_ptr(key, layer_data_map);
#if DISPATCH_MAP_DEBUG
    fprintf(stderr, "MID: map: %p, object: %p, key: %p, data: %p\n", &layer_data_map, object, key, data);
#endif
    assert(data != NULL);

    return data->report_data;
}

// "my device data"
debug_report_data *mdd(void* object)
{
    dispatch_key key = get_dispatch_key(object);
    layer_data *data = get_my_data_ptr(key, layer_data_map);
#if DISPATCH_MAP_DEBUG
    fprintf(stderr, "MDD: map: %p, object: %p, key: %p, data: %p\n", &layer_data_map, object, key, data);
#endif
    assert(data != NULL);
    return data->report_data;
}

static void InitParamChecker(layer_data *data, const VkAllocationCallbacks *pAllocator)
{
    VkDebugReportCallbackEXT callback;
    uint32_t report_flags = getLayerOptionFlags("ParamCheckerReportFlags", 0);

    uint32_t debug_action = 0;
    getLayerOptionEnum("ParamCheckerDebugAction", (uint32_t *) &debug_action);
    if(debug_action & VK_DBG_LAYER_ACTION_LOG_MSG)
    {
        FILE *log_output = NULL;
        const char* option_str = getLayerOption("ParamCheckerLogFilename");
        log_output = getLayerLogOutput(option_str, "ParamChecker");
        VkDebugReportCallbackCreateInfoEXT dbgCreateInfo;
        memset(&dbgCreateInfo, 0, sizeof(dbgCreateInfo));
        dbgCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
        dbgCreateInfo.flags = report_flags;
        dbgCreateInfo.pfnCallback = log_callback;
        dbgCreateInfo.pUserData = log_output;

        layer_create_msg_callback(data->report_data, &dbgCreateInfo, pAllocator, &callback);
        data->logging_callback.push_back(callback);
    }

    if (debug_action & VK_DBG_LAYER_ACTION_DEBUG_OUTPUT) {
        VkDebugReportCallbackCreateInfoEXT dbgCreateInfo;
        memset(&dbgCreateInfo, 0, sizeof(dbgCreateInfo));
        dbgCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
        dbgCreateInfo.flags = report_flags;
        dbgCreateInfo.pfnCallback = win32_debug_output_msg;
        dbgCreateInfo.pUserData = NULL;

        layer_create_msg_callback(data->report_data, &dbgCreateInfo, pAllocator, &callback);
        data->logging_callback.push_back(callback);
    }
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugReportCallbackEXT(
    VkInstance instance,
    const VkDebugReportCallbackCreateInfoEXT *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkDebugReportCallbackEXT* pMsgCallback)
{
    VkLayerInstanceDispatchTable *pTable = get_dispatch_table(pc_instance_table_map, instance);
    VkResult result =  pTable->CreateDebugReportCallbackEXT(instance, pCreateInfo, pAllocator, pMsgCallback);

    if (result == VK_SUCCESS)
    {
        layer_data *data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
        result = layer_create_msg_callback(data->report_data, pCreateInfo, pAllocator, pMsgCallback);
    }

    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyDebugReportCallbackEXT(
    VkInstance instance,
    VkDebugReportCallbackEXT msgCallback,
    const VkAllocationCallbacks *pAllocator)
{
    VkLayerInstanceDispatchTable *pTable = get_dispatch_table(pc_instance_table_map, instance);
    pTable->DestroyDebugReportCallbackEXT(instance, msgCallback, pAllocator);

    layer_data *data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    layer_destroy_msg_callback(data->report_data, msgCallback, pAllocator);
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
    VkLayerInstanceDispatchTable *pTable = get_dispatch_table(pc_instance_table_map, instance);
    pTable->DebugReportMessageEXT(instance, flags, objType, object, location, msgCode, pLayerPrefix, pMsg);
}

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
    return util_GetExtensionProperties(1, instance_extensions, pCount, pProperties);
}

static const VkLayerProperties pc_global_layers[] = {
    {
        "VK_LAYER_LUNARG_param_checker",
        VK_API_VERSION,
        1,
        "LunarG Validation Layer",
    }
};

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceLayerProperties(
        uint32_t *pCount,
        VkLayerProperties*    pProperties)
{
    return util_GetLayerProperties(ARRAY_SIZE(pc_global_layers),
                                   pc_global_layers,
                                   pCount, pProperties);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceExtensionProperties(
        VkPhysicalDevice                            physicalDevice,
        const char*                                 pLayerName,
        uint32_t*                                   pCount,
        VkExtensionProperties*                      pProperties)
{
    /* ParamChecker does not have any physical device extensions */
    if (pLayerName == NULL) {
        return get_dispatch_table(pc_instance_table_map, physicalDevice)->EnumerateDeviceExtensionProperties(
                                    physicalDevice,
                                    NULL,
                                    pCount,
                                    pProperties);
    } else {
        return util_GetExtensionProperties(0, NULL, pCount, pProperties);
    }
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceLayerProperties(
        VkPhysicalDevice                            physicalDevice,
        uint32_t*                                   pCount,
        VkLayerProperties*                          pProperties)
{

    /* ParamChecker's physical device layers are the same as global */
    return util_GetLayerProperties(ARRAY_SIZE(pc_global_layers), pc_global_layers,
                                   pCount, pProperties);
}

static
std::string EnumeratorString(VkResult const& enumerator)
{
    switch(enumerator)
    {
        case VK_RESULT_MAX_ENUM:
        {
            return "VK_RESULT_MAX_ENUM";
            break;
        }
        case VK_ERROR_LAYER_NOT_PRESENT:
        {
            return "VK_ERROR_LAYER_NOT_PRESENT";
            break;
        }
        case VK_ERROR_INCOMPATIBLE_DRIVER:
        {
            return "VK_ERROR_INCOMPATIBLE_DRIVER";
            break;
        }
        case VK_ERROR_MEMORY_MAP_FAILED:
        {
            return "VK_ERROR_MEMORY_MAP_FAILED";
            break;
        }
        case VK_INCOMPLETE:
        {
            return "VK_INCOMPLETE";
            break;
        }
        case VK_ERROR_OUT_OF_HOST_MEMORY:
        {
            return "VK_ERROR_OUT_OF_HOST_MEMORY";
            break;
        }
        case VK_ERROR_INITIALIZATION_FAILED:
        {
            return "VK_ERROR_INITIALIZATION_FAILED";
            break;
        }
        case VK_NOT_READY:
        {
            return "VK_NOT_READY";
            break;
        }
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
        {
            return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
            break;
        }
        case VK_EVENT_SET:
        {
            return "VK_EVENT_SET";
            break;
        }
        case VK_TIMEOUT:
        {
            return "VK_TIMEOUT";
            break;
        }
        case VK_EVENT_RESET:
        {
            return "VK_EVENT_RESET";
            break;
        }
        case VK_SUCCESS:
        {
            return "VK_SUCCESS";
            break;
        }
        case VK_ERROR_EXTENSION_NOT_PRESENT:
        {
            return "VK_ERROR_EXTENSION_NOT_PRESENT";
            break;
        }
        case VK_ERROR_DEVICE_LOST:
        {
            return "VK_ERROR_DEVICE_LOST";
            break;
        }
        default:
        {
            return "unrecognized enumerator";
            break;
        }
    }
}

static
bool ValidateEnumerator(VkFormatFeatureFlagBits const& enumerator)
{
    VkFormatFeatureFlagBits allFlags = (VkFormatFeatureFlagBits)(
        VK_FORMAT_FEATURE_STORAGE_IMAGE_ATOMIC_BIT |
        VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT |
        VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT |
        VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_ATOMIC_BIT |
        VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT |
        VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT |
        VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT |
        VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT |
        VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT |
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT |
        VK_FORMAT_FEATURE_BLIT_SRC_BIT |
        VK_FORMAT_FEATURE_BLIT_DST_BIT |
        VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT);
    if(enumerator & (~allFlags))
    {
        return false;
    }

    return true;
}

static
std::string EnumeratorString(VkFormatFeatureFlagBits const& enumerator)
{
    if(!ValidateEnumerator(enumerator))
    {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if(enumerator & VK_FORMAT_FEATURE_STORAGE_IMAGE_ATOMIC_BIT)
    {
        strings.push_back("VK_FORMAT_FEATURE_STORAGE_IMAGE_ATOMIC_BIT");
    }
    if(enumerator & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT)
    {
        strings.push_back("VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT");
    }
    if(enumerator & VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT)
    {
        strings.push_back("VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT");
    }
    if(enumerator & VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_ATOMIC_BIT)
    {
        strings.push_back("VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_ATOMIC_BIT");
    }
    if(enumerator & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)
    {
        strings.push_back("VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT");
    }
    if(enumerator & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT)
    {
        strings.push_back("VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT");
    }
    if(enumerator & VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT)
    {
        strings.push_back("VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT");
    }
    if(enumerator & VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT)
    {
        strings.push_back("VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT");
    }
    if(enumerator & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT)
    {
        strings.push_back("VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT");
    }
    if(enumerator & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
    {
        strings.push_back("VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT");
    }
    if(enumerator & VK_FORMAT_FEATURE_BLIT_SRC_BIT)
    {
        strings.push_back("VK_FORMAT_FEATURE_BLIT_SRC_BIT");
    }
    if(enumerator & VK_FORMAT_FEATURE_BLIT_DST_BIT)
    {
        strings.push_back("VK_FORMAT_FEATURE_BLIT_DST_BIT");
    }
    if(enumerator & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)
    {
        strings.push_back("VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT");
    }

    std::string enumeratorString;
    for(auto const& string : strings)
    {
        enumeratorString += string;

        if(string != strings.back())
        {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static
bool ValidateEnumerator(VkImageUsageFlagBits const& enumerator)
{
    VkImageUsageFlagBits allFlags = (VkImageUsageFlagBits)(VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT |
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
        VK_IMAGE_USAGE_STORAGE_BIT |
        VK_IMAGE_USAGE_SAMPLED_BIT |
        VK_IMAGE_USAGE_TRANSFER_DST_BIT |
        VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
    if(enumerator & (~allFlags))
    {
        return false;
    }

    return true;
}

static
std::string EnumeratorString(VkImageUsageFlagBits const& enumerator)
{
    if(!ValidateEnumerator(enumerator))
    {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if(enumerator & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT)
    {
        strings.push_back("VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT");
    }
    if(enumerator & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
    {
        strings.push_back("VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT");
    }
    if(enumerator & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
    {
        strings.push_back("VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT");
    }
    if(enumerator & VK_IMAGE_USAGE_STORAGE_BIT)
    {
        strings.push_back("VK_IMAGE_USAGE_STORAGE_BIT");
    }
    if(enumerator & VK_IMAGE_USAGE_SAMPLED_BIT)
    {
        strings.push_back("VK_IMAGE_USAGE_SAMPLED_BIT");
    }
    if(enumerator & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
    {
        strings.push_back("VK_IMAGE_USAGE_TRANSFER_DST_BIT");
    }
    if(enumerator & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT)
    {
        strings.push_back("VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT");
    }
    if(enumerator & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
    {
        strings.push_back("VK_IMAGE_USAGE_TRANSFER_SRC_BIT");
    }

    std::string enumeratorString;
    for(auto const& string : strings)
    {
        enumeratorString += string;

        if(string != strings.back())
        {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static
bool ValidateEnumerator(VkQueueFlagBits const& enumerator)
{
    VkQueueFlagBits allFlags = (VkQueueFlagBits)(
        VK_QUEUE_TRANSFER_BIT |
        VK_QUEUE_COMPUTE_BIT |
        VK_QUEUE_SPARSE_BINDING_BIT |
        VK_QUEUE_GRAPHICS_BIT);
    if(enumerator & (~allFlags))
    {
        return false;
    }

    return true;
}

static
std::string EnumeratorString(VkQueueFlagBits const& enumerator)
{
    if(!ValidateEnumerator(enumerator))
    {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if(enumerator & VK_QUEUE_TRANSFER_BIT)
    {
        strings.push_back("VK_QUEUE_TRANSFER_BIT");
    }
    if(enumerator & VK_QUEUE_COMPUTE_BIT)
    {
        strings.push_back("VK_QUEUE_COMPUTE_BIT");
    }
    if(enumerator & VK_QUEUE_SPARSE_BINDING_BIT)
    {
        strings.push_back("VK_QUEUE_SPARSE_BINDING_BIT");
    }
    if(enumerator & VK_QUEUE_GRAPHICS_BIT)
    {
        strings.push_back("VK_QUEUE_GRAPHICS_BIT");
    }

    std::string enumeratorString;
    for(auto const& string : strings)
    {
        enumeratorString += string;

        if(string != strings.back())
        {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static
bool ValidateEnumerator(VkMemoryPropertyFlagBits const& enumerator)
{
    VkMemoryPropertyFlagBits allFlags = (VkMemoryPropertyFlagBits)(VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_CACHED_BIT |
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if(enumerator & (~allFlags))
    {
        return false;
    }

    return true;
}

static
std::string EnumeratorString(VkMemoryPropertyFlagBits const& enumerator)
{
    if(!ValidateEnumerator(enumerator))
    {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if(enumerator & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT)
    {
        strings.push_back("VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT");
    }
    if(enumerator & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
    {
        strings.push_back("VK_MEMORY_PROPERTY_HOST_COHERENT_BIT");
    }
    if(enumerator & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    {
        strings.push_back("VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT");
    }
    if(enumerator & VK_MEMORY_PROPERTY_HOST_CACHED_BIT)
    {
        strings.push_back("VK_MEMORY_PROPERTY_HOST_CACHED_BIT");
    }
    if(enumerator & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
    {
        strings.push_back("VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT");
    }

    std::string enumeratorString;
    for(auto const& string : strings)
    {
        enumeratorString += string;

        if(string != strings.back())
        {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static
bool ValidateEnumerator(VkMemoryHeapFlagBits const& enumerator)
{
    VkMemoryHeapFlagBits allFlags = (VkMemoryHeapFlagBits)(VK_MEMORY_HEAP_DEVICE_LOCAL_BIT);
    if(enumerator & (~allFlags))
    {
        return false;
    }

    return true;
}

static
std::string EnumeratorString(VkMemoryHeapFlagBits const& enumerator)
{
    if(!ValidateEnumerator(enumerator))
    {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if(enumerator & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
    {
        strings.push_back("VK_MEMORY_HEAP_DEVICE_LOCAL_BIT");
    }

    std::string enumeratorString;
    for(auto const& string : strings)
    {
        enumeratorString += string;

        if(string != strings.back())
        {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static
bool ValidateEnumerator(VkSparseImageFormatFlagBits const& enumerator)
{
    VkSparseImageFormatFlagBits allFlags = (VkSparseImageFormatFlagBits)(VK_SPARSE_IMAGE_FORMAT_NONSTANDARD_BLOCK_SIZE_BIT |
        VK_SPARSE_IMAGE_FORMAT_ALIGNED_MIP_SIZE_BIT |
        VK_SPARSE_IMAGE_FORMAT_SINGLE_MIPTAIL_BIT);
    if(enumerator & (~allFlags))
    {
        return false;
    }

    return true;
}

static
std::string EnumeratorString(VkSparseImageFormatFlagBits const& enumerator)
{
    if(!ValidateEnumerator(enumerator))
    {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if(enumerator & VK_SPARSE_IMAGE_FORMAT_NONSTANDARD_BLOCK_SIZE_BIT)
    {
        strings.push_back("VK_SPARSE_IMAGE_FORMAT_NONSTANDARD_BLOCK_SIZE_BIT");
    }
    if(enumerator & VK_SPARSE_IMAGE_FORMAT_ALIGNED_MIP_SIZE_BIT)
    {
        strings.push_back("VK_SPARSE_IMAGE_FORMAT_ALIGNED_MIP_SIZE_BIT");
    }
    if(enumerator & VK_SPARSE_IMAGE_FORMAT_SINGLE_MIPTAIL_BIT)
    {
        strings.push_back("VK_SPARSE_IMAGE_FORMAT_SINGLE_MIPTAIL_BIT");
    }

    std::string enumeratorString;
    for(auto const& string : strings)
    {
        enumeratorString += string;

        if(string != strings.back())
        {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static
bool ValidateEnumerator(VkFenceCreateFlagBits const& enumerator)
{
    VkFenceCreateFlagBits allFlags = (VkFenceCreateFlagBits)(VK_FENCE_CREATE_SIGNALED_BIT);
    if(enumerator & (~allFlags))
    {
        return false;
    }

    return true;
}

static
std::string EnumeratorString(VkFenceCreateFlagBits const& enumerator)
{
    if(!ValidateEnumerator(enumerator))
    {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if(enumerator & VK_FENCE_CREATE_SIGNALED_BIT)
    {
        strings.push_back("VK_FENCE_CREATE_SIGNALED_BIT");
    }

    std::string enumeratorString;
    for(auto const& string : strings)
    {
        enumeratorString += string;

        if(string != strings.back())
        {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static
bool ValidateEnumerator(VkQueryPipelineStatisticFlagBits const& enumerator)
{
    VkQueryPipelineStatisticFlagBits allFlags = (VkQueryPipelineStatisticFlagBits)(VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT |
        VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT |
        VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT |
        VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT |
        VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT |
        VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_PRIMITIVES_BIT |
        VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT |
        VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT |
        VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_CONTROL_SHADER_PATCHES_BIT |
        VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_INVOCATIONS_BIT |
        VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_EVALUATION_SHADER_INVOCATIONS_BIT);
    if(enumerator & (~allFlags))
    {
        return false;
    }

    return true;
}

static
std::string EnumeratorString(VkQueryPipelineStatisticFlagBits const& enumerator)
{
    if(!ValidateEnumerator(enumerator))
    {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if(enumerator & VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT)
    {
        strings.push_back("VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT");
    }
    if(enumerator & VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT)
    {
        strings.push_back("VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT");
    }
    if(enumerator & VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT)
    {
        strings.push_back("VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT");
    }
    if(enumerator & VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT)
    {
        strings.push_back("VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT");
    }
    if(enumerator & VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT)
    {
        strings.push_back("VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT");
    }
    if(enumerator & VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_PRIMITIVES_BIT)
    {
        strings.push_back("VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_PRIMITIVES_BIT");
    }
    if(enumerator & VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT)
    {
        strings.push_back("VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT");
    }
    if(enumerator & VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT)
    {
        strings.push_back("VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT");
    }
    if(enumerator & VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_CONTROL_SHADER_PATCHES_BIT)
    {
        strings.push_back("VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_CONTROL_SHADER_PATCHES_BIT");
    }
    if(enumerator & VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_INVOCATIONS_BIT)
    {
        strings.push_back("VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_INVOCATIONS_BIT");
    }
    if(enumerator & VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_EVALUATION_SHADER_INVOCATIONS_BIT)
    {
        strings.push_back("VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_EVALUATION_SHADER_INVOCATIONS_BIT");
    }

    std::string enumeratorString;
    for(auto const& string : strings)
    {
        enumeratorString += string;

        if(string != strings.back())
        {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static
bool ValidateEnumerator(VkQueryResultFlagBits const& enumerator)
{
    VkQueryResultFlagBits allFlags = (VkQueryResultFlagBits)(VK_QUERY_RESULT_PARTIAL_BIT |
        VK_QUERY_RESULT_WITH_AVAILABILITY_BIT |
        VK_QUERY_RESULT_WAIT_BIT |
        VK_QUERY_RESULT_64_BIT);
    if(enumerator & (~allFlags))
    {
        return false;
    }

    return true;
}

static
std::string EnumeratorString(VkQueryResultFlagBits const& enumerator)
{
    if(!ValidateEnumerator(enumerator))
    {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if(enumerator & VK_QUERY_RESULT_PARTIAL_BIT)
    {
        strings.push_back("VK_QUERY_RESULT_PARTIAL_BIT");
    }
    if(enumerator & VK_QUERY_RESULT_WITH_AVAILABILITY_BIT)
    {
        strings.push_back("VK_QUERY_RESULT_WITH_AVAILABILITY_BIT");
    }
    if(enumerator & VK_QUERY_RESULT_WAIT_BIT)
    {
        strings.push_back("VK_QUERY_RESULT_WAIT_BIT");
    }
    if(enumerator & VK_QUERY_RESULT_64_BIT)
    {
        strings.push_back("VK_QUERY_RESULT_64_BIT");
    }

    std::string enumeratorString;
    for(auto const& string : strings)
    {
        enumeratorString += string;

        if(string != strings.back())
        {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static
bool ValidateEnumerator(VkBufferUsageFlagBits const& enumerator)
{
    VkBufferUsageFlagBits allFlags = (VkBufferUsageFlagBits)(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
        VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT |
        VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT |
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
        VK_BUFFER_USAGE_TRANSFER_DST_BIT |
        VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT |
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    if(enumerator & (~allFlags))
    {
        return false;
    }

    return true;
}

static
std::string EnumeratorString(VkBufferUsageFlagBits const& enumerator)
{
    if(!ValidateEnumerator(enumerator))
    {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if(enumerator & VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
    {
        strings.push_back("VK_BUFFER_USAGE_VERTEX_BUFFER_BIT");
    }
    if(enumerator & VK_BUFFER_USAGE_INDEX_BUFFER_BIT)
    {
        strings.push_back("VK_BUFFER_USAGE_INDEX_BUFFER_BIT");
    }
    if(enumerator & VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT)
    {
        strings.push_back("VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT");
    }
    if(enumerator & VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT)
    {
        strings.push_back("VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT");
    }
    if(enumerator & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
    {
        strings.push_back("VK_BUFFER_USAGE_STORAGE_BUFFER_BIT");
    }
    if(enumerator & VK_BUFFER_USAGE_TRANSFER_DST_BIT)
    {
        strings.push_back("VK_BUFFER_USAGE_TRANSFER_DST_BIT");
    }
    if(enumerator & VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT)
    {
        strings.push_back("VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT");
    }
    if(enumerator & VK_BUFFER_USAGE_TRANSFER_SRC_BIT)
    {
        strings.push_back("VK_BUFFER_USAGE_TRANSFER_SRC_BIT");
    }
    if(enumerator & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
    {
        strings.push_back("VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT");
    }

    std::string enumeratorString;
    for(auto const& string : strings)
    {
        enumeratorString += string;

        if(string != strings.back())
        {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static
bool ValidateEnumerator(VkBufferCreateFlagBits const& enumerator)
{
    VkBufferCreateFlagBits allFlags = (VkBufferCreateFlagBits)(VK_BUFFER_CREATE_SPARSE_ALIASED_BIT |
        VK_BUFFER_CREATE_SPARSE_RESIDENCY_BIT |
        VK_BUFFER_CREATE_SPARSE_BINDING_BIT);
    if(enumerator & (~allFlags))
    {
        return false;
    }

    return true;
}

static
std::string EnumeratorString(VkBufferCreateFlagBits const& enumerator)
{
    if(!ValidateEnumerator(enumerator))
    {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if(enumerator & VK_BUFFER_CREATE_SPARSE_ALIASED_BIT)
    {
        strings.push_back("VK_BUFFER_CREATE_SPARSE_ALIASED_BIT");
    }
    if(enumerator & VK_BUFFER_CREATE_SPARSE_RESIDENCY_BIT)
    {
        strings.push_back("VK_BUFFER_CREATE_SPARSE_RESIDENCY_BIT");
    }
    if(enumerator & VK_BUFFER_CREATE_SPARSE_BINDING_BIT)
    {
        strings.push_back("VK_BUFFER_CREATE_SPARSE_BINDING_BIT");
    }

    std::string enumeratorString;
    for(auto const& string : strings)
    {
        enumeratorString += string;

        if(string != strings.back())
        {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static
bool ValidateEnumerator(VkImageCreateFlagBits const& enumerator)
{
    VkImageCreateFlagBits allFlags = (VkImageCreateFlagBits)(VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT |
        VK_IMAGE_CREATE_SPARSE_ALIASED_BIT |
        VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT |
        VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT |
        VK_IMAGE_CREATE_SPARSE_BINDING_BIT);
    if(enumerator & (~allFlags))
    {
        return false;
    }

    return true;
}

static
std::string EnumeratorString(VkImageCreateFlagBits const& enumerator)
{
    if(!ValidateEnumerator(enumerator))
    {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if(enumerator & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT)
    {
        strings.push_back("VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT");
    }
    if(enumerator & VK_IMAGE_CREATE_SPARSE_ALIASED_BIT)
    {
        strings.push_back("VK_IMAGE_CREATE_SPARSE_ALIASED_BIT");
    }
    if(enumerator & VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT)
    {
        strings.push_back("VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT");
    }
    if(enumerator & VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT)
    {
        strings.push_back("VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT");
    }
    if(enumerator & VK_IMAGE_CREATE_SPARSE_BINDING_BIT)
    {
        strings.push_back("VK_IMAGE_CREATE_SPARSE_BINDING_BIT");
    }

    std::string enumeratorString;
    for(auto const& string : strings)
    {
        enumeratorString += string;

        if(string != strings.back())
        {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static
bool ValidateEnumerator(VkColorComponentFlagBits const& enumerator)
{
    VkColorComponentFlagBits allFlags = (VkColorComponentFlagBits)(VK_COLOR_COMPONENT_A_BIT |
        VK_COLOR_COMPONENT_B_BIT |
        VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_R_BIT);
    if(enumerator & (~allFlags))
    {
        return false;
    }

    return true;
}

static
std::string EnumeratorString(VkColorComponentFlagBits const& enumerator)
{
    if(!ValidateEnumerator(enumerator))
    {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if(enumerator & VK_COLOR_COMPONENT_A_BIT)
    {
        strings.push_back("VK_COLOR_COMPONENT_A_BIT");
    }
    if(enumerator & VK_COLOR_COMPONENT_B_BIT)
    {
        strings.push_back("VK_COLOR_COMPONENT_B_BIT");
    }
    if(enumerator & VK_COLOR_COMPONENT_G_BIT)
    {
        strings.push_back("VK_COLOR_COMPONENT_G_BIT");
    }
    if(enumerator & VK_COLOR_COMPONENT_R_BIT)
    {
        strings.push_back("VK_COLOR_COMPONENT_R_BIT");
    }

    std::string enumeratorString;
    for(auto const& string : strings)
    {
        enumeratorString += string;

        if(string != strings.back())
        {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static
bool ValidateEnumerator(VkPipelineCreateFlagBits const& enumerator)
{
    VkPipelineCreateFlagBits allFlags = (VkPipelineCreateFlagBits)(VK_PIPELINE_CREATE_DERIVATIVE_BIT |
        VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT |
        VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT);
    if(enumerator & (~allFlags))
    {
        return false;
    }

    return true;
}

static
std::string EnumeratorString(VkPipelineCreateFlagBits const& enumerator)
{
    if(!ValidateEnumerator(enumerator))
    {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if(enumerator & VK_PIPELINE_CREATE_DERIVATIVE_BIT)
    {
        strings.push_back("VK_PIPELINE_CREATE_DERIVATIVE_BIT");
    }
    if(enumerator & VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT)
    {
        strings.push_back("VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT");
    }
    if(enumerator & VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT)
    {
        strings.push_back("VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT");
    }

    std::string enumeratorString;
    for(auto const& string : strings)
    {
        enumeratorString += string;

        if(string != strings.back())
        {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static
bool ValidateEnumerator(VkShaderStageFlagBits const& enumerator)
{
    VkShaderStageFlagBits allFlags = (VkShaderStageFlagBits)(VK_SHADER_STAGE_ALL |
        VK_SHADER_STAGE_FRAGMENT_BIT |
        VK_SHADER_STAGE_GEOMETRY_BIT |
        VK_SHADER_STAGE_COMPUTE_BIT |
        VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT |
        VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT |
        VK_SHADER_STAGE_VERTEX_BIT);
    if(enumerator & (~allFlags))
    {
        return false;
    }

    return true;
}

static
std::string EnumeratorString(VkShaderStageFlagBits const& enumerator)
{
    if(!ValidateEnumerator(enumerator))
    {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if(enumerator & VK_SHADER_STAGE_ALL)
    {
        strings.push_back("VK_SHADER_STAGE_ALL");
    }
    if(enumerator & VK_SHADER_STAGE_FRAGMENT_BIT)
    {
        strings.push_back("VK_SHADER_STAGE_FRAGMENT_BIT");
    }
    if(enumerator & VK_SHADER_STAGE_GEOMETRY_BIT)
    {
        strings.push_back("VK_SHADER_STAGE_GEOMETRY_BIT");
    }
    if(enumerator & VK_SHADER_STAGE_COMPUTE_BIT)
    {
        strings.push_back("VK_SHADER_STAGE_COMPUTE_BIT");
    }
    if(enumerator & VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT)
    {
        strings.push_back("VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT");
    }
    if(enumerator & VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT)
    {
        strings.push_back("VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT");
    }
    if(enumerator & VK_SHADER_STAGE_VERTEX_BIT)
    {
        strings.push_back("VK_SHADER_STAGE_VERTEX_BIT");
    }

    std::string enumeratorString;
    for(auto const& string : strings)
    {
        enumeratorString += string;

        if(string != strings.back())
        {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static
bool ValidateEnumerator(VkPipelineStageFlagBits const& enumerator)
{
    VkPipelineStageFlagBits allFlags = (VkPipelineStageFlagBits)(
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT |
        VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT |
        VK_PIPELINE_STAGE_HOST_BIT |
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT |
        VK_PIPELINE_STAGE_TRANSFER_BIT |
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT |
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
        VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT |
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
        VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT |
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
        VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT |
        VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT |
        VK_PIPELINE_STAGE_VERTEX_SHADER_BIT |
        VK_PIPELINE_STAGE_VERTEX_INPUT_BIT |
        VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT |
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
    if(enumerator & (~allFlags))
    {
        return false;
    }

    return true;
}

static
std::string EnumeratorString(VkPipelineStageFlagBits const& enumerator)
{
    if(!ValidateEnumerator(enumerator))
    {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if(enumerator & VK_PIPELINE_STAGE_ALL_COMMANDS_BIT)
    {
        strings.push_back("VK_PIPELINE_STAGE_ALL_COMMANDS_BIT");
    }
    if(enumerator & VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT)
    {
        strings.push_back("VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT");
    }
    if(enumerator & VK_PIPELINE_STAGE_HOST_BIT)
    {
        strings.push_back("VK_PIPELINE_STAGE_HOST_BIT");
    }
    if(enumerator & VK_PIPELINE_STAGE_TRANSFER_BIT)
    {
        strings.push_back("VK_PIPELINE_STAGE_TRANSFER_BIT");
    }
    if(enumerator & VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT)
    {
        strings.push_back("VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT");
    }
    if(enumerator & VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT)
    {
        strings.push_back("VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT");
    }
    if(enumerator & VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT)
    {
        strings.push_back("VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT");
    }
    if(enumerator & VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT)
    {
        strings.push_back("VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT");
    }
    if(enumerator & VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT)
    {
        strings.push_back("VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT");
    }
    if(enumerator & VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT)
    {
        strings.push_back("VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT");
    }
    if(enumerator & VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT)
    {
        strings.push_back("VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT");
    }
    if(enumerator & VK_PIPELINE_STAGE_VERTEX_SHADER_BIT)
    {
        strings.push_back("VK_PIPELINE_STAGE_VERTEX_SHADER_BIT");
    }
    if(enumerator & VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT)
    {
        strings.push_back("VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT");
    }
    if(enumerator & VK_PIPELINE_STAGE_VERTEX_INPUT_BIT)
    {
        strings.push_back("VK_PIPELINE_STAGE_VERTEX_INPUT_BIT");
    }
    if(enumerator & VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT)
    {
        strings.push_back("VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT");
    }
    if(enumerator & VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT)
    {
        strings.push_back("VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT");
    }
    if(enumerator & VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT)
    {
        strings.push_back("VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT");
    }

    std::string enumeratorString;
    for(auto const& string : strings)
    {
        enumeratorString += string;

        if(string != strings.back())
        {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static
bool ValidateEnumerator(VkAccessFlagBits const& enumerator)
{
    VkAccessFlagBits allFlags = (VkAccessFlagBits)(
            VK_ACCESS_INDIRECT_COMMAND_READ_BIT |
            VK_ACCESS_INDEX_READ_BIT |
            VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT |
            VK_ACCESS_UNIFORM_READ_BIT |
            VK_ACCESS_INPUT_ATTACHMENT_READ_BIT |
            VK_ACCESS_SHADER_READ_BIT |
            VK_ACCESS_SHADER_WRITE_BIT |
            VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
            VK_ACCESS_TRANSFER_READ_BIT |
            VK_ACCESS_TRANSFER_WRITE_BIT |
            VK_ACCESS_HOST_READ_BIT |
            VK_ACCESS_HOST_WRITE_BIT |
            VK_ACCESS_MEMORY_READ_BIT |
            VK_ACCESS_MEMORY_WRITE_BIT);

    if(enumerator & (~allFlags))
    {
        return false;
    }

    return true;
}

static
std::string EnumeratorString(VkAccessFlagBits const& enumerator)
{
    if(!ValidateEnumerator(enumerator))
    {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if(enumerator & VK_ACCESS_INDIRECT_COMMAND_READ_BIT)
    {
        strings.push_back("VK_ACCESS_INDIRECT_COMMAND_READ_BIT");
    }
    if(enumerator & VK_ACCESS_INDEX_READ_BIT)
    {
        strings.push_back("VK_ACCESS_INDEX_READ_BIT");
    }
    if(enumerator & VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT)
    {
        strings.push_back("VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT");
    }
    if(enumerator & VK_ACCESS_UNIFORM_READ_BIT)
    {
        strings.push_back("VK_ACCESS_UNIFORM_READ_BIT");
    }
    if(enumerator & VK_ACCESS_INPUT_ATTACHMENT_READ_BIT)
    {
        strings.push_back("VK_ACCESS_INPUT_ATTACHMENT_READ_BIT");
    }
    if(enumerator & VK_ACCESS_SHADER_READ_BIT)
    {
        strings.push_back("VK_ACCESS_SHADER_READ_BIT");
    }
    if(enumerator & VK_ACCESS_SHADER_WRITE_BIT)
    {
        strings.push_back("VK_ACCESS_SHADER_WRITE_BIT");
    }
    if(enumerator & VK_ACCESS_COLOR_ATTACHMENT_READ_BIT)
    {
        strings.push_back("VK_ACCESS_COLOR_ATTACHMENT_READ_BIT");
    }
    if(enumerator & VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT)
    {
        strings.push_back("VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT");
    }
    if(enumerator & VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT)
    {
        strings.push_back("VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT");
    }
    if(enumerator & VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT)
    {
        strings.push_back("VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT");
    }
    if(enumerator & VK_ACCESS_TRANSFER_READ_BIT)
    {
        strings.push_back("VK_ACCESS_TRANSFER_READ_BIT");
    }
    if(enumerator & VK_ACCESS_TRANSFER_WRITE_BIT)
    {
        strings.push_back("VK_ACCESS_TRANSFER_WRITE_BIT");
    }
    if(enumerator & VK_ACCESS_HOST_READ_BIT)
    {
        strings.push_back("VK_ACCESS_HOST_READ_BIT");
    }
    if(enumerator & VK_ACCESS_HOST_WRITE_BIT)
    {
        strings.push_back("VK_ACCESS_HOST_WRITE_BIT");
    }
    if(enumerator & VK_ACCESS_MEMORY_READ_BIT)
    {
        strings.push_back("VK_ACCESS_MEMORY_READ_BIT");
    }
    if(enumerator & VK_ACCESS_MEMORY_WRITE_BIT)
    {
        strings.push_back("VK_ACCESS_MEMORY_WRITE_BIT");
    }

    std::string enumeratorString;
    for(auto const& string : strings)
    {
        enumeratorString += string;

        if(string != strings.back())
        {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static
bool ValidateEnumerator(VkCommandPoolCreateFlagBits const& enumerator)
{
    VkCommandPoolCreateFlagBits allFlags = (VkCommandPoolCreateFlagBits)(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT |
        VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
    if(enumerator & (~allFlags))
    {
        return false;
    }

    return true;
}

static
std::string EnumeratorString(VkCommandPoolCreateFlagBits const& enumerator)
{
    if(!ValidateEnumerator(enumerator))
    {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if(enumerator & VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT)
    {
        strings.push_back("VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT");
    }
    if(enumerator & VK_COMMAND_POOL_CREATE_TRANSIENT_BIT)
    {
        strings.push_back("VK_COMMAND_POOL_CREATE_TRANSIENT_BIT");
    }

    std::string enumeratorString;
    for(auto const& string : strings)
    {
        enumeratorString += string;

        if(string != strings.back())
        {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static
bool ValidateEnumerator(VkCommandPoolResetFlagBits const& enumerator)
{
    VkCommandPoolResetFlagBits allFlags = (VkCommandPoolResetFlagBits)(VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
    if(enumerator & (~allFlags))
    {
        return false;
    }

    return true;
}

static
std::string EnumeratorString(VkCommandPoolResetFlagBits const& enumerator)
{
    if(!ValidateEnumerator(enumerator))
    {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if(enumerator & VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT)
    {
        strings.push_back("VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT");
    }

    std::string enumeratorString;
    for(auto const& string : strings)
    {
        enumeratorString += string;

        if(string != strings.back())
        {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static
bool ValidateEnumerator(VkCommandBufferUsageFlags const& enumerator)
{
    VkCommandBufferUsageFlags allFlags = (VkCommandBufferUsageFlags)(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT |
        VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT |
        VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);
    if(enumerator & (~allFlags))
    {
        return false;
    }

    return true;
}

static
std::string EnumeratorString(VkCommandBufferUsageFlags const& enumerator)
{
    if(!ValidateEnumerator(enumerator))
    {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if(enumerator & VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT)
    {
        strings.push_back("VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT");
    }
    if(enumerator & VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT)
    {
        strings.push_back("VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT");
    }
    if(enumerator & VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT)
    {
        strings.push_back("VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT");
    }

    std::string enumeratorString;
    for(auto const& string : strings)
    {
        enumeratorString += string;

        if(string != strings.back())
        {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static
bool ValidateEnumerator(VkCommandBufferResetFlagBits const& enumerator)
{
    VkCommandBufferResetFlagBits allFlags = (VkCommandBufferResetFlagBits)(VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
    if(enumerator & (~allFlags))
    {
        return false;
    }

    return true;
}

static
std::string EnumeratorString(VkCommandBufferResetFlagBits const& enumerator)
{
    if(!ValidateEnumerator(enumerator))
    {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if(enumerator & VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT)
    {
        strings.push_back("VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT");
    }

    std::string enumeratorString;
    for(auto const& string : strings)
    {
        enumeratorString += string;

        if(string != strings.back())
        {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static
bool ValidateEnumerator(VkImageAspectFlagBits const& enumerator)
{
    VkImageAspectFlagBits allFlags = (VkImageAspectFlagBits)(VK_IMAGE_ASPECT_METADATA_BIT |
        VK_IMAGE_ASPECT_STENCIL_BIT |
        VK_IMAGE_ASPECT_DEPTH_BIT |
        VK_IMAGE_ASPECT_COLOR_BIT);
    if(enumerator & (~allFlags))
    {
        return false;
    }

    return true;
}

static
std::string EnumeratorString(VkImageAspectFlagBits const& enumerator)
{
    if(!ValidateEnumerator(enumerator))
    {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if(enumerator & VK_IMAGE_ASPECT_METADATA_BIT)
    {
        strings.push_back("VK_IMAGE_ASPECT_METADATA_BIT");
    }
    if(enumerator & VK_IMAGE_ASPECT_STENCIL_BIT)
    {
        strings.push_back("VK_IMAGE_ASPECT_STENCIL_BIT");
    }
    if(enumerator & VK_IMAGE_ASPECT_DEPTH_BIT)
    {
        strings.push_back("VK_IMAGE_ASPECT_DEPTH_BIT");
    }
    if(enumerator & VK_IMAGE_ASPECT_COLOR_BIT)
    {
        strings.push_back("VK_IMAGE_ASPECT_COLOR_BIT");
    }

    std::string enumeratorString;
    for(auto const& string : strings)
    {
        enumeratorString += string;

        if(string != strings.back())
        {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static
bool ValidateEnumerator(VkQueryControlFlagBits const& enumerator)
{
    VkQueryControlFlagBits allFlags = (VkQueryControlFlagBits)(VK_QUERY_CONTROL_PRECISE_BIT);
    if(enumerator & (~allFlags))
    {
        return false;
    }

    return true;
}

static
std::string EnumeratorString(VkQueryControlFlagBits const& enumerator)
{
    if(!ValidateEnumerator(enumerator))
    {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if(enumerator & VK_QUERY_CONTROL_PRECISE_BIT)
    {
        strings.push_back("VK_QUERY_CONTROL_PRECISE_BIT");
    }

    std::string enumeratorString;
    for(auto const& string : strings)
    {
        enumeratorString += string;

        if(string != strings.back())
        {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static const int MaxParamCheckerStringLength = 256;

static
VkBool32 validate_string(layer_data *my_data, const char *apiName, const char *stringName, const char *validateString)
{
    VkBool32 skipCall = VK_FALSE;

    VkStringErrorFlags result = vk_string_validate(MaxParamCheckerStringLength, validateString);

    if (result == VK_STRING_ERROR_NONE) {
        return skipCall;
    } else if (result & VK_STRING_ERROR_LENGTH) {
        skipCall = log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "%s: string %s exceeds max length %d", apiName, stringName, MaxParamCheckerStringLength);
    } else if (result & VK_STRING_ERROR_BAD_DATA) {
        skipCall = log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "%s: string %s contains invalid characters or is badly formed", apiName, stringName);
    }
    return skipCall;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateInstance(
    const VkInstanceCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkInstance* pInstance)
{
    VkResult    result         = VK_ERROR_VALIDATION_FAILED_EXT;
    VkBool32    skipCall       = VK_FALSE;

    if (skipCall == VK_FALSE) {
        VkLayerInstanceCreateInfo *chain_info     = get_chain_info(pCreateInfo, VK_LAYER_LINK_INFO);
        assert(chain_info->u.pLayerInfo);
        PFN_vkGetInstanceProcAddr fpGetInstanceProcAddr = chain_info->u.pLayerInfo->pfnNextGetInstanceProcAddr;
        PFN_vkCreateInstance fpCreateInstance = (PFN_vkCreateInstance) fpGetInstanceProcAddr(NULL, "vkCreateInstance");
        if (fpCreateInstance == NULL) {
            return VK_ERROR_INITIALIZATION_FAILED;
        }

        // Advance the link info for the next element on the chain
        chain_info->u.pLayerInfo = chain_info->u.pLayerInfo->pNext;

        result = fpCreateInstance(pCreateInfo, pAllocator, pInstance);
        if (result != VK_SUCCESS)
            return result;

        layer_data *my_data = get_my_data_ptr(get_dispatch_key(*pInstance), layer_data_map);
        VkLayerInstanceDispatchTable *pTable = initInstanceTable(*pInstance, fpGetInstanceProcAddr, pc_instance_table_map);

        my_data->report_data = debug_report_create_instance(
                                   pTable,
                                   *pInstance,
                                   pCreateInfo->enabledExtensionCount,
                                   pCreateInfo->ppEnabledExtensionNames);

        InitParamChecker(my_data, pAllocator);
    }

    // Ordinarily we'd check these before calling down the chain, but none of the layer
    // support is in place until now, if we survive we can report the issue now.
    layer_data *my_device_data = get_my_data_ptr(get_dispatch_key(*pInstance), layer_data_map);
    if (pCreateInfo->pApplicationInfo) {
        if (pCreateInfo->pApplicationInfo->pApplicationName) {
            skipCall |= validate_string(my_device_data, "vkCreateInstance()", "VkInstanceCreateInfo->VkApplicationInfo->pApplicationName",
                                        pCreateInfo->pApplicationInfo->pApplicationName);
        }

        if (pCreateInfo->pApplicationInfo->pEngineName) {
          skipCall |= validate_string(my_device_data, "vkCreateInstance()", "VkInstanceCreateInfo->VkApplicationInfo->pEngineName",
                                        pCreateInfo->pApplicationInfo->pEngineName);
        }
    }

    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyInstance(
    VkInstance instance,
    const VkAllocationCallbacks* pAllocator)
{
    // Grab the key before the instance is destroyed.
    dispatch_key key = get_dispatch_key(instance);
    VkLayerInstanceDispatchTable *pTable = get_dispatch_table(pc_instance_table_map, instance);
    pTable->DestroyInstance(instance, pAllocator);

    // Clean up logging callback, if any
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    while (my_data->logging_callback.size() > 0) {
        VkDebugReportCallbackEXT callback = my_data->logging_callback.back();
        layer_destroy_msg_callback(my_data->report_data, callback, pAllocator);
        my_data->logging_callback.pop_back();
    }

    layer_debug_report_destroy_instance(mid(instance));
    layer_data_map.erase(pTable);

    pc_instance_table_map.erase(key);
}

bool PostEnumeratePhysicalDevices(
    VkInstance instance,
    uint32_t* pPhysicalDeviceCount,
    VkPhysicalDevice* pPhysicalDevices,
    VkResult result)
{

    if(pPhysicalDeviceCount != nullptr)
    {
    }

    if(pPhysicalDevices != nullptr)
    {
    }

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkEnumeratePhysicalDevices parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mid(instance), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK", "%s", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumeratePhysicalDevices(
    VkInstance instance,
    uint32_t* pPhysicalDeviceCount,
    VkPhysicalDevice* pPhysicalDevices)
{
    VkResult    result      = VK_ERROR_VALIDATION_FAILED_EXT;
    VkBool32    skipCall    = VK_FALSE;
    layer_data* my_data     = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    assert(my_data != NULL);

    skipCall |= param_check_vkEnumeratePhysicalDevices(
        my_data->report_data,
        pPhysicalDeviceCount,
        pPhysicalDevices);

    if (skipCall == VK_FALSE) {
        result = get_dispatch_table(pc_instance_table_map, instance)->EnumeratePhysicalDevices(instance, pPhysicalDeviceCount, pPhysicalDevices);

        PostEnumeratePhysicalDevices(instance, pPhysicalDeviceCount, pPhysicalDevices, result);
    }

    return result;
}

bool PostGetPhysicalDeviceFeatures(
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceFeatures* pFeatures)
{

    if(pFeatures != nullptr)
    {
    }

    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceFeatures(
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceFeatures* pFeatures)
{
    VkBool32    skipCall    = VK_FALSE;
    layer_data* my_data     = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
    assert(my_data != NULL);

    skipCall |= param_check_vkGetPhysicalDeviceFeatures(
        my_data->report_data,
        pFeatures);

    if (skipCall == VK_FALSE) {
        get_dispatch_table(pc_instance_table_map, physicalDevice)->GetPhysicalDeviceFeatures(physicalDevice, pFeatures);

        PostGetPhysicalDeviceFeatures(physicalDevice, pFeatures);
    }
}

bool PostGetPhysicalDeviceFormatProperties(
    VkPhysicalDevice physicalDevice,
    VkFormat format,
    VkFormatProperties* pFormatProperties)
{

    if(format < VK_FORMAT_BEGIN_RANGE ||
        format > VK_FORMAT_END_RANGE)
    {
        log_msg(mdd(physicalDevice), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkGetPhysicalDeviceFormatProperties parameter, VkFormat format, is an unrecognized enumerator");
        return false;
    }

    if(pFormatProperties != nullptr)
    {
    }

    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceFormatProperties(
    VkPhysicalDevice physicalDevice,
    VkFormat format,
    VkFormatProperties* pFormatProperties)
{
    VkBool32    skipCall    = VK_FALSE;
    layer_data* my_data     = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
    assert(my_data != NULL);


    skipCall |= param_check_vkGetPhysicalDeviceFormatProperties(
        my_data->report_data,
        format,
        pFormatProperties);

    if (skipCall == VK_FALSE) {
        get_dispatch_table(pc_instance_table_map, physicalDevice)->GetPhysicalDeviceFormatProperties(physicalDevice, format, pFormatProperties);

        PostGetPhysicalDeviceFormatProperties(physicalDevice, format, pFormatProperties);
    }
}

bool PostGetPhysicalDeviceImageFormatProperties(
    VkPhysicalDevice physicalDevice,
    VkFormat format,
    VkImageType type,
    VkImageTiling tiling,
    VkImageUsageFlags usage,
    VkImageCreateFlags flags,
    VkImageFormatProperties* pImageFormatProperties,
    VkResult result)
{

    if(format < VK_FORMAT_BEGIN_RANGE ||
        format > VK_FORMAT_END_RANGE)
    {
        log_msg(mdd(physicalDevice), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkGetPhysicalDeviceImageFormatProperties parameter, VkFormat format, is an unrecognized enumerator");
        return false;
    }

    if(type < VK_IMAGE_TYPE_BEGIN_RANGE ||
        type > VK_IMAGE_TYPE_END_RANGE)
    {
        log_msg(mdd(physicalDevice), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkGetPhysicalDeviceImageFormatProperties parameter, VkImageType type, is an unrecognized enumerator");
        return false;
    }

    if(tiling < VK_IMAGE_TILING_BEGIN_RANGE ||
        tiling > VK_IMAGE_TILING_END_RANGE)
    {
        log_msg(mdd(physicalDevice), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkGetPhysicalDeviceImageFormatProperties parameter, VkImageTiling tiling, is an unrecognized enumerator");
        return false;
    }


    if(pImageFormatProperties != nullptr)
    {
    }

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkGetPhysicalDeviceImageFormatProperties parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(physicalDevice), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK", "%s", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceImageFormatProperties(
    VkPhysicalDevice physicalDevice,
    VkFormat format,
    VkImageType type,
    VkImageTiling tiling,
    VkImageUsageFlags usage,
    VkImageCreateFlags flags,
    VkImageFormatProperties* pImageFormatProperties)
{
    VkResult    result      = VK_ERROR_VALIDATION_FAILED_EXT;
    VkBool32    skipCall    = VK_FALSE;
    layer_data* my_data     = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
    assert(my_data != NULL);

    skipCall |= param_check_vkGetPhysicalDeviceImageFormatProperties(
        my_data->report_data,
        format,
        type,
        tiling,
        usage,
        flags,
        pImageFormatProperties);

    if (skipCall == VK_FALSE) {
        result = get_dispatch_table(pc_instance_table_map, physicalDevice)->GetPhysicalDeviceImageFormatProperties(physicalDevice, format, type, tiling, usage, flags, pImageFormatProperties);

        PostGetPhysicalDeviceImageFormatProperties(physicalDevice, format, type, tiling, usage, flags, pImageFormatProperties, result);
    }

    return result;
}

bool PostGetPhysicalDeviceProperties(
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceProperties* pProperties)
{

    if(pProperties != nullptr)
    {
    if(pProperties->deviceType < VK_PHYSICAL_DEVICE_TYPE_BEGIN_RANGE ||
        pProperties->deviceType > VK_PHYSICAL_DEVICE_TYPE_END_RANGE)
    {
        log_msg(mdd(physicalDevice), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkGetPhysicalDeviceProperties parameter, VkPhysicalDeviceType pProperties->deviceType, is an unrecognized enumerator");
        return false;
    }

    }

    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceProperties(
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceProperties* pProperties)
{
    VkBool32    skipCall    = VK_FALSE;
    layer_data* my_data     = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
    assert(my_data != NULL);

    skipCall |= param_check_vkGetPhysicalDeviceProperties(
        my_data->report_data,
        pProperties);

    if (skipCall == VK_FALSE) {
        get_dispatch_table(pc_instance_table_map, physicalDevice)->GetPhysicalDeviceProperties(physicalDevice, pProperties);

        PostGetPhysicalDeviceProperties(physicalDevice, pProperties);
    }
}

bool PostGetPhysicalDeviceQueueFamilyProperties(
    VkPhysicalDevice physicalDevice,
    uint32_t* pCount,
    VkQueueFamilyProperties* pQueueProperties)
{

    if(pQueueProperties == nullptr && pCount != nullptr)
    {
    }

    if(pQueueProperties != nullptr)
    {
    }

    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceQueueFamilyProperties(
    VkPhysicalDevice physicalDevice,
    uint32_t* pQueueFamilyPropertyCount,
    VkQueueFamilyProperties* pQueueFamilyProperties)
{
    VkBool32    skipCall    = VK_FALSE;
    layer_data* my_data     = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
    assert(my_data != NULL);

    skipCall |= param_check_vkGetPhysicalDeviceQueueFamilyProperties(
        my_data->report_data,
        pQueueFamilyPropertyCount,
        pQueueFamilyProperties);

    if (skipCall == VK_FALSE) {
        get_dispatch_table(pc_instance_table_map, physicalDevice)->GetPhysicalDeviceQueueFamilyProperties(physicalDevice, pQueueFamilyPropertyCount, pQueueFamilyProperties);

        PostGetPhysicalDeviceQueueFamilyProperties(physicalDevice, pQueueFamilyPropertyCount, pQueueFamilyProperties);
    }
}

bool PostGetPhysicalDeviceMemoryProperties(
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceMemoryProperties* pMemoryProperties)
{

    if(pMemoryProperties != nullptr)
    {
    }

    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceMemoryProperties(
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceMemoryProperties* pMemoryProperties)
{
    VkBool32    skipCall    = VK_FALSE;
    layer_data* my_data     = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
    assert(my_data != NULL);

    skipCall |= param_check_vkGetPhysicalDeviceMemoryProperties(
        my_data->report_data,
        pMemoryProperties);

    if (skipCall == VK_FALSE) {
        get_dispatch_table(pc_instance_table_map, physicalDevice)->GetPhysicalDeviceMemoryProperties(physicalDevice, pMemoryProperties);

        PostGetPhysicalDeviceMemoryProperties(physicalDevice, pMemoryProperties);
    }
}

void validateDeviceCreateInfo(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo* pCreateInfo, const std::vector<VkQueueFamilyProperties> properties) {
    std::unordered_set<uint32_t> set;
    for (uint32_t i = 0; i < pCreateInfo->queueCreateInfoCount; ++i) {
        if (set.count(pCreateInfo->pQueueCreateInfos[i].queueFamilyIndex)) {
            log_msg(mdd(physicalDevice), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
                "VkDeviceCreateInfo parameter, uint32_t pQueueCreateInfos[%d]->queueFamilyIndex, is not unique within this structure.", i);
        } else {
            set.insert(pCreateInfo->pQueueCreateInfos[i].queueFamilyIndex);
        }
        if (pCreateInfo->pQueueCreateInfos[i].queueCount == 0) {
            log_msg(mdd(physicalDevice), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
                "VkDeviceCreateInfo parameter, uint32_t pQueueCreateInfos[%d]->queueCount, cannot be zero.", i);
        }
        for (uint32_t j = 0; j < pCreateInfo->pQueueCreateInfos[i].queueCount; ++j) {
            if (pCreateInfo->pQueueCreateInfos[i].pQueuePriorities == nullptr) {
                log_msg(mdd(physicalDevice), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
                    "VkDeviceCreateInfo parameter, uint32_t pQueueCreateInfos[%d]->pQueuePriorities, must not be NULL.", i);
            } else if (pCreateInfo->pQueueCreateInfos[i].pQueuePriorities[j] < 0.f || pCreateInfo->pQueueCreateInfos[i].pQueuePriorities[j] > 1.f) {
                log_msg(mdd(physicalDevice), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
                    "VkDeviceCreateInfo parameter, uint32_t pQueueCreateInfos[%d]->pQueuePriorities[%d], must be between 0 and 1. Actual value is %f", i, j, pCreateInfo->pQueueCreateInfos[i].pQueuePriorities[j]);
            }
        }
        if (pCreateInfo->pQueueCreateInfos[i].queueFamilyIndex >= properties.size()) {
            log_msg(mdd(physicalDevice), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
                "VkDeviceCreateInfo parameter, uint32_t pQueueCreateInfos[%d]->queueFamilyIndex cannot be more than the number of queue families.", i);
        } else if (pCreateInfo->pQueueCreateInfos[i].queueCount > properties[pCreateInfo->pQueueCreateInfos[i].queueFamilyIndex].queueCount) {
            log_msg(mdd(physicalDevice), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
                "VkDeviceCreateInfo parameter, uint32_t pQueueCreateInfos[%d]->queueCount cannot be more than the number of queues for the given family index.", i);
        }
    }
}

void storeCreateDeviceData(VkDevice device, const VkDeviceCreateInfo* pCreateInfo) {
    layer_data *my_device_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    for (uint32_t i = 0; i < pCreateInfo->queueCreateInfoCount; ++i) {
        my_device_data->queueFamilyIndexMap.insert(
            std::make_pair(pCreateInfo->pQueueCreateInfos[i].queueFamilyIndex, pCreateInfo->pQueueCreateInfos[i].queueCount));
    }
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateDevice(
    VkPhysicalDevice physicalDevice,
    const VkDeviceCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDevice* pDevice)
{
    /*
     * NOTE: We do not validate physicalDevice or any dispatchable
     * object as the first parameter. We couldn't get here if it was wrong!
     */

    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    VkBool32 skipCall = VK_FALSE;
    layer_data *my_instance_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);

    if ((pCreateInfo->enabledLayerCount > 0) && (pCreateInfo->ppEnabledLayerNames != NULL)) {
        for (auto i = 0; i < pCreateInfo->enabledLayerCount; i++) {
            skipCall |= validate_string(my_instance_data, "vkCreateDevice()", "VkDeviceCreateInfo->ppEnabledLayerNames",
                                        pCreateInfo->ppEnabledLayerNames[i]);
        }
    }

    if ((pCreateInfo->enabledExtensionCount > 0) && (pCreateInfo->ppEnabledExtensionNames != NULL)) {
        for (auto i = 0; i < pCreateInfo->enabledExtensionCount; i++) {
            skipCall |= validate_string(my_instance_data, "vkCreateDevice()", "VkDeviceCreateInfo->ppEnabledExtensionNames",
                                        pCreateInfo->ppEnabledExtensionNames[i]);
        }
    }

    if (skipCall == VK_FALSE) {
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

        result = fpCreateDevice(physicalDevice, pCreateInfo, pAllocator, pDevice);
        if (result != VK_SUCCESS) {
            return result;
        }

        layer_data *my_device_data = get_my_data_ptr(get_dispatch_key(*pDevice), layer_data_map);
        my_device_data->report_data = layer_debug_report_create_device(my_instance_data->report_data, *pDevice);
        initDeviceTable(*pDevice, fpGetDeviceProcAddr, pc_device_table_map);

        uint32_t count;
        get_dispatch_table(pc_instance_table_map, physicalDevice)->GetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, nullptr);
        std::vector<VkQueueFamilyProperties> properties(count);
        get_dispatch_table(pc_instance_table_map, physicalDevice)->GetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, &properties[0]);

        validateDeviceCreateInfo(physicalDevice, pCreateInfo, properties);
        storeCreateDeviceData(*pDevice, pCreateInfo);
    }

    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyDevice(
    VkDevice device,
    const VkAllocationCallbacks* pAllocator)
{
    layer_debug_report_destroy_device(device);

    dispatch_key key = get_dispatch_key(device);
#if DISPATCH_MAP_DEBUG
    fprintf(stderr, "Device: %p, key: %p\n", device, key);
#endif

    get_dispatch_table(pc_device_table_map, device)->DestroyDevice(device, pAllocator);
    pc_device_table_map.erase(key);
}

bool PreGetDeviceQueue(
    VkDevice device,
    uint32_t queueFamilyIndex,
    uint32_t queueIndex)
{
    layer_data *my_device_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    auto queue_data = my_device_data->queueFamilyIndexMap.find(queueFamilyIndex);
    if (queue_data == my_device_data->queueFamilyIndexMap.end()) {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
            "VkGetDeviceQueue parameter, uint32_t queueFamilyIndex %d, must have been given when the device was created.", queueFamilyIndex);
        return false;
    }
    if (queue_data->second <= queueIndex) {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
            "VkGetDeviceQueue parameter, uint32_t queueIndex %d, must be less than the number of queues given when the device was created.", queueIndex);
        return false;
    }
    return true;
}

bool PostGetDeviceQueue(
    VkDevice device,
    uint32_t queueFamilyIndex,
    uint32_t queueIndex,
    VkQueue* pQueue)
{



    if(pQueue != nullptr)
    {
    }

    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetDeviceQueue(
    VkDevice device,
    uint32_t queueFamilyIndex,
    uint32_t queueIndex,
    VkQueue* pQueue)
{
    VkBool32    skipCall    = VK_FALSE;
    layer_data* my_data     = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skipCall |= param_check_vkGetDeviceQueue(
        my_data->report_data,
        queueFamilyIndex,
        queueIndex,
        pQueue);

    if (skipCall == VK_FALSE) {
        PreGetDeviceQueue(device, queueFamilyIndex, queueIndex);

        get_dispatch_table(pc_device_table_map, device)->GetDeviceQueue(device, queueFamilyIndex, queueIndex, pQueue);

        PostGetDeviceQueue(device, queueFamilyIndex, queueIndex, pQueue);
    }
}

bool PreQueueSubmit(
    VkQueue queue,
    const VkSubmitInfo* submit)
{
    if(submit->sType != VK_STRUCTURE_TYPE_SUBMIT_INFO) {
        log_msg(mdd(queue), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkQueueSubmit parameter, VkStructureType pSubmits->sType, is an invalid enumerator");
        return false;
    }

    if(submit->pCommandBuffers != nullptr)
    {
    }

    return true;
}

bool PostQueueSubmit(
    VkQueue queue,
    uint32_t commandBufferCount,
    VkFence fence,
    VkResult result)
{



    if(result < VK_SUCCESS)
    {
        std::string reason = "vkQueueSubmit parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(queue), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK", "%s", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkQueueSubmit(
    VkQueue queue,
    uint32_t submitCount,
    const VkSubmitInfo* pSubmits,
    VkFence fence)
{
    VkResult    result      = VK_ERROR_VALIDATION_FAILED_EXT;
    VkBool32    skipCall    = VK_FALSE;
    layer_data* my_data     = get_my_data_ptr(get_dispatch_key(queue), layer_data_map);
    assert(my_data != NULL);

    skipCall |= param_check_vkQueueSubmit(
        my_data->report_data,
        submitCount,
        pSubmits,
        fence);

    if (skipCall == VK_FALSE) {
        for (uint32_t i = 0; i < submitCount; i++) {
            PreQueueSubmit(queue, &pSubmits[i]);
        }

        result = get_dispatch_table(pc_device_table_map, queue)->QueueSubmit(queue, submitCount, pSubmits, fence);

        PostQueueSubmit(queue, submitCount, fence, result);
    }

    return result;
}

bool PostQueueWaitIdle(
    VkQueue queue,
    VkResult result)
{

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkQueueWaitIdle parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(queue), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK", "%s", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkQueueWaitIdle(
    VkQueue queue)
{
    VkResult result = get_dispatch_table(pc_device_table_map, queue)->QueueWaitIdle(queue);

    PostQueueWaitIdle(queue, result);

    return result;
}

bool PostDeviceWaitIdle(
    VkDevice device,
    VkResult result)
{

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkDeviceWaitIdle parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK", "%s", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkDeviceWaitIdle(
    VkDevice device)
{
    VkResult result = get_dispatch_table(pc_device_table_map, device)->DeviceWaitIdle(device);

    PostDeviceWaitIdle(device, result);

    return result;
}

bool PreAllocateMemory(
    VkDevice device,
    const VkMemoryAllocateInfo* pAllocateInfo)
{
    if(pAllocateInfo != nullptr)
    {
    if(pAllocateInfo->sType != VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkAllocateMemory parameter, VkStructureType pAllocateInfo->sType, is an invalid enumerator");
        return false;
    }
    }

    return true;
}

bool PostAllocateMemory(
    VkDevice device,
    VkDeviceMemory* pMemory,
    VkResult result)
{

    if(pMemory != nullptr)
    {
    }

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkAllocateMemory parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK", "%s", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkAllocateMemory(
    VkDevice device,
    const VkMemoryAllocateInfo* pAllocateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDeviceMemory* pMemory)
{
    VkResult    result      = VK_ERROR_VALIDATION_FAILED_EXT;
    VkBool32    skipCall    = VK_FALSE;
    layer_data* my_data     = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skipCall |= param_check_vkAllocateMemory(
        my_data->report_data,
        pAllocateInfo,
        pAllocator,
        pMemory);

    if (skipCall == VK_FALSE) {
        PreAllocateMemory(device, pAllocateInfo);

        result = get_dispatch_table(pc_device_table_map, device)->AllocateMemory(device, pAllocateInfo, pAllocator, pMemory);

        PostAllocateMemory(device, pMemory, result);
    }

    return result;
}

bool PostMapMemory(
    VkDevice device,
    VkDeviceMemory mem,
    VkDeviceSize offset,
    VkDeviceSize size,
    VkMemoryMapFlags flags,
    void** ppData,
    VkResult result)
{





    if(ppData != nullptr)
    {
    }

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkMapMemory parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK", "%s", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkMapMemory(
    VkDevice device,
    VkDeviceMemory memory,
    VkDeviceSize offset,
    VkDeviceSize size,
    VkMemoryMapFlags flags,
    void** ppData)
{
    VkResult    result      = VK_ERROR_VALIDATION_FAILED_EXT;
    VkBool32    skipCall    = VK_FALSE;
    layer_data* my_data     = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skipCall |= param_check_vkMapMemory(
        my_data->report_data,
        memory,
        offset,
        size,
        flags,
        ppData);

    if (skipCall == VK_FALSE) {
        result = get_dispatch_table(pc_device_table_map, device)->MapMemory(device, memory, offset, size, flags, ppData);

        PostMapMemory(device, memory, offset, size, flags, ppData, result);
    }

    return result;
}

bool PreFlushMappedMemoryRanges(
    VkDevice device,
    const VkMappedMemoryRange* pMemoryRanges)
{
    if(pMemoryRanges != nullptr)
    {
    if(pMemoryRanges->sType != VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkFlushMappedMemoryRanges parameter, VkStructureType pMemoryRanges->sType, is an invalid enumerator");
        return false;
    }
    }

    return true;
}

bool PostFlushMappedMemoryRanges(
    VkDevice device,
    uint32_t memoryRangeCount,
    VkResult result)
{


    if(result < VK_SUCCESS)
    {
        std::string reason = "vkFlushMappedMemoryRanges parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK", "%s", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkFlushMappedMemoryRanges(
    VkDevice device,
    uint32_t memoryRangeCount,
    const VkMappedMemoryRange* pMemoryRanges)
{
    VkResult    result      = VK_ERROR_VALIDATION_FAILED_EXT;
    VkBool32    skipCall    = VK_FALSE;
    layer_data* my_data     = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skipCall |= param_check_vkFlushMappedMemoryRanges(
        my_data->report_data,
        memoryRangeCount,
        pMemoryRanges);

    if (skipCall == VK_FALSE) {
        PreFlushMappedMemoryRanges(device, pMemoryRanges);

        result = get_dispatch_table(pc_device_table_map, device)->FlushMappedMemoryRanges(device, memoryRangeCount, pMemoryRanges);

        PostFlushMappedMemoryRanges(device, memoryRangeCount, result);
    }

    return result;
}

bool PreInvalidateMappedMemoryRanges(
    VkDevice device,
    const VkMappedMemoryRange* pMemoryRanges)
{
    if(pMemoryRanges != nullptr)
    {
    if(pMemoryRanges->sType != VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkInvalidateMappedMemoryRanges parameter, VkStructureType pMemoryRanges->sType, is an invalid enumerator");
        return false;
    }
    }

    return true;
}

bool PostInvalidateMappedMemoryRanges(
    VkDevice device,
    uint32_t memoryRangeCount,
    VkResult result)
{


    if(result < VK_SUCCESS)
    {
        std::string reason = "vkInvalidateMappedMemoryRanges parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK", "%s", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkInvalidateMappedMemoryRanges(
    VkDevice device,
    uint32_t memoryRangeCount,
    const VkMappedMemoryRange* pMemoryRanges)
{
    VkResult    result      = VK_ERROR_VALIDATION_FAILED_EXT;
    VkBool32    skipCall    = VK_FALSE;
    layer_data* my_data     = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skipCall |= param_check_vkInvalidateMappedMemoryRanges(
        my_data->report_data,
        memoryRangeCount,
        pMemoryRanges);

    if (skipCall == VK_FALSE) {
        PreInvalidateMappedMemoryRanges(device, pMemoryRanges);

        result = get_dispatch_table(pc_device_table_map, device)->InvalidateMappedMemoryRanges(device, memoryRangeCount, pMemoryRanges);

        PostInvalidateMappedMemoryRanges(device, memoryRangeCount, result);
    }

    return result;
}

bool PostGetDeviceMemoryCommitment(
    VkDevice device,
    VkDeviceMemory memory,
    VkDeviceSize* pCommittedMemoryInBytes)
{


    if(pCommittedMemoryInBytes != nullptr)
    {
    }

    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetDeviceMemoryCommitment(
    VkDevice device,
    VkDeviceMemory memory,
    VkDeviceSize* pCommittedMemoryInBytes)
{
    VkBool32    skipCall    = VK_FALSE;
    layer_data* my_data     = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skipCall |= param_check_vkGetDeviceMemoryCommitment(
        my_data->report_data,
        memory,
        pCommittedMemoryInBytes);

    if (skipCall == VK_FALSE) {
        get_dispatch_table(pc_device_table_map, device)->GetDeviceMemoryCommitment(device, memory, pCommittedMemoryInBytes);

        PostGetDeviceMemoryCommitment(device, memory, pCommittedMemoryInBytes);
    }
}

bool PostBindBufferMemory(
    VkDevice device,
    VkBuffer buffer,
    VkDeviceMemory mem,
    VkDeviceSize memoryOffset,
    VkResult result)
{




    if(result < VK_SUCCESS)
    {
        std::string reason = "vkBindBufferMemory parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK", "%s", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkBindBufferMemory(
    VkDevice device,
    VkBuffer buffer,
    VkDeviceMemory mem,
    VkDeviceSize memoryOffset)
{
    VkResult result = get_dispatch_table(pc_device_table_map, device)->BindBufferMemory(device, buffer, mem, memoryOffset);

    PostBindBufferMemory(device, buffer, mem, memoryOffset, result);

    return result;
}

bool PostBindImageMemory(
    VkDevice device,
    VkImage image,
    VkDeviceMemory mem,
    VkDeviceSize memoryOffset,
    VkResult result)
{




    if(result < VK_SUCCESS)
    {
        std::string reason = "vkBindImageMemory parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK", "%s", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkBindImageMemory(
    VkDevice device,
    VkImage image,
    VkDeviceMemory mem,
    VkDeviceSize memoryOffset)
{
    VkResult result = get_dispatch_table(pc_device_table_map, device)->BindImageMemory(device, image, mem, memoryOffset);

    PostBindImageMemory(device, image, mem, memoryOffset, result);

    return result;
}

bool PostGetBufferMemoryRequirements(
    VkDevice device,
    VkBuffer buffer,
    VkMemoryRequirements* pMemoryRequirements)
{


    if(pMemoryRequirements != nullptr)
    {
    }

    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetBufferMemoryRequirements(
    VkDevice device,
    VkBuffer buffer,
    VkMemoryRequirements* pMemoryRequirements)
{
    VkBool32    skipCall    = VK_FALSE;
    layer_data* my_data     = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skipCall |= param_check_vkGetBufferMemoryRequirements(
        my_data->report_data,
        buffer,
        pMemoryRequirements);

    if (skipCall == VK_FALSE) {
        get_dispatch_table(pc_device_table_map, device)->GetBufferMemoryRequirements(device, buffer, pMemoryRequirements);

        PostGetBufferMemoryRequirements(device, buffer, pMemoryRequirements);
    }
}

bool PostGetImageMemoryRequirements(
    VkDevice device,
    VkImage image,
    VkMemoryRequirements* pMemoryRequirements)
{


    if(pMemoryRequirements != nullptr)
    {
    }

    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetImageMemoryRequirements(
    VkDevice device,
    VkImage image,
    VkMemoryRequirements* pMemoryRequirements)
{
    VkBool32    skipCall    = VK_FALSE;
    layer_data* my_data     = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skipCall |= param_check_vkGetImageMemoryRequirements(
        my_data->report_data,
        image,
        pMemoryRequirements);

    if (skipCall == VK_FALSE) {
        get_dispatch_table(pc_device_table_map, device)->GetImageMemoryRequirements(device, image, pMemoryRequirements);

        PostGetImageMemoryRequirements(device, image, pMemoryRequirements);
    }
}

bool PostGetImageSparseMemoryRequirements(
    VkDevice device,
    VkImage image,
    uint32_t* pNumRequirements,
    VkSparseImageMemoryRequirements* pSparseMemoryRequirements)
{


    if(pNumRequirements != nullptr)
    {
    }

    if(pSparseMemoryRequirements != nullptr)
    {
    if ((pSparseMemoryRequirements->formatProperties.aspectMask &
       (VK_IMAGE_ASPECT_COLOR_BIT | VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT | VK_IMAGE_ASPECT_METADATA_BIT)) == 0)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkGetImageSparseMemoryRequirements parameter, VkImageAspect pSparseMemoryRequirements->formatProperties.aspectMask, is an unrecognized enumerator");
        return false;
    }
    }

    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetImageSparseMemoryRequirements(
    VkDevice device,
    VkImage image,
    uint32_t* pSparseMemoryRequirementCount,
    VkSparseImageMemoryRequirements* pSparseMemoryRequirements)
{
    VkBool32    skipCall    = VK_FALSE;
    layer_data* my_data     = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skipCall |= param_check_vkGetImageSparseMemoryRequirements(
        my_data->report_data,
        image,
        pSparseMemoryRequirementCount,
        pSparseMemoryRequirements);

    if (skipCall == VK_FALSE) {
        get_dispatch_table(pc_device_table_map, device)->GetImageSparseMemoryRequirements(device, image, pSparseMemoryRequirementCount, pSparseMemoryRequirements);

        PostGetImageSparseMemoryRequirements(device, image, pSparseMemoryRequirementCount, pSparseMemoryRequirements);
    }
}

bool PostGetPhysicalDeviceSparseImageFormatProperties(
    VkPhysicalDevice physicalDevice,
    VkFormat format,
    VkImageType type,
    VkSampleCountFlagBits samples,
    VkImageUsageFlags usage,
    VkImageTiling tiling,
    uint32_t* pNumProperties,
    VkSparseImageFormatProperties* pProperties)
{

    if(format < VK_FORMAT_BEGIN_RANGE ||
        format > VK_FORMAT_END_RANGE)
    {
        log_msg(mdd(physicalDevice), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkGetPhysicalDeviceSparseImageFormatProperties parameter, VkFormat format, is an unrecognized enumerator");
        return false;
    }

    if(type < VK_IMAGE_TYPE_BEGIN_RANGE ||
        type > VK_IMAGE_TYPE_END_RANGE)
    {
        log_msg(mdd(physicalDevice), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkGetPhysicalDeviceSparseImageFormatProperties parameter, VkImageType type, is an unrecognized enumerator");
        return false;
    }



    if(tiling < VK_IMAGE_TILING_BEGIN_RANGE ||
        tiling > VK_IMAGE_TILING_END_RANGE)
    {
        log_msg(mdd(physicalDevice), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkGetPhysicalDeviceSparseImageFormatProperties parameter, VkImageTiling tiling, is an unrecognized enumerator");
        return false;
    }

    if(pNumProperties != nullptr)
    {
    }

    if(pProperties != nullptr)
    {
    if ((pProperties->aspectMask &
       (VK_IMAGE_ASPECT_COLOR_BIT | VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT | VK_IMAGE_ASPECT_METADATA_BIT)) == 0)
    {
        log_msg(mdd(physicalDevice), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkGetPhysicalDeviceSparseImageFormatProperties parameter, VkImageAspect pProperties->aspectMask, is an unrecognized enumerator");
        return false;
    }
    }

    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceSparseImageFormatProperties(
    VkPhysicalDevice physicalDevice,
    VkFormat format,
    VkImageType type,
    VkSampleCountFlagBits samples,
    VkImageUsageFlags usage,
    VkImageTiling tiling,
    uint32_t* pPropertyCount,
    VkSparseImageFormatProperties* pProperties)
{
    VkBool32    skipCall    = VK_FALSE;
    layer_data* my_data     = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
    assert(my_data != NULL);

    skipCall |= param_check_vkGetPhysicalDeviceSparseImageFormatProperties(
        my_data->report_data,
        format,
        type,
        samples,
        usage,
        tiling,
        pPropertyCount,
        pProperties);

    if (skipCall == VK_FALSE) {
        get_dispatch_table(pc_instance_table_map, physicalDevice)->GetPhysicalDeviceSparseImageFormatProperties(physicalDevice, format, type, samples, usage, tiling, pPropertyCount, pProperties);

        PostGetPhysicalDeviceSparseImageFormatProperties(physicalDevice, format, type, samples, usage, tiling, pPropertyCount, pProperties);
    }
}

bool PreQueueBindSparse(
    VkQueue queue,
    uint32_t                                    bindInfoCount,
    const VkBindSparseInfo*                     pBindInfo)
{
    if(pBindInfo != nullptr)
    {
    }

    return true;
}

bool PostQueueBindSparse(
    VkQueue queue,
    uint32_t bindInfoCount,
    const VkBindSparseInfo* pBindInfo,
    VkFence fence,
    VkResult result)
{



    if(result < VK_SUCCESS)
    {
        std::string reason = "vkQueueBindSparse parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(queue), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK", "%s", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkQueueBindSparse(
    VkQueue queue,
    uint32_t bindInfoCount,
    const VkBindSparseInfo* pBindInfo,
    VkFence fence)
{
    VkResult    result      = VK_ERROR_VALIDATION_FAILED_EXT;
    VkBool32    skipCall    = VK_FALSE;
    layer_data* my_data     = get_my_data_ptr(get_dispatch_key(queue), layer_data_map);
    assert(my_data != NULL);

    skipCall |= param_check_vkQueueBindSparse(
        my_data->report_data,
        bindInfoCount,
        pBindInfo,
        fence);

    if (skipCall == VK_FALSE) {
        PreQueueBindSparse(queue, bindInfoCount, pBindInfo);

        result = get_dispatch_table(pc_device_table_map, queue)->QueueBindSparse(queue, bindInfoCount, pBindInfo, fence);

        PostQueueBindSparse(queue, bindInfoCount, pBindInfo, fence, result);
    }

    return result;
}

bool PreCreateFence(
    VkDevice device,
    const VkFenceCreateInfo* pCreateInfo)
{
    if(pCreateInfo != nullptr)
    {
    if(pCreateInfo->sType != VK_STRUCTURE_TYPE_FENCE_CREATE_INFO)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateFence parameter, VkStructureType pCreateInfo->sType, is an invalid enumerator");
        return false;
    }
    }

    return true;
}

bool PostCreateFence(
    VkDevice device,
    VkFence* pFence,
    VkResult result)
{

    if(pFence != nullptr)
    {
    }

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkCreateFence parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK", "%s", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateFence(
    VkDevice device,
    const VkFenceCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkFence* pFence)
{
    VkResult    result      = VK_ERROR_VALIDATION_FAILED_EXT;
    VkBool32    skipCall    = VK_FALSE;
    layer_data* my_data     = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skipCall |= param_check_vkCreateFence(
        my_data->report_data,
        pCreateInfo,
        pAllocator,
        pFence);

    if (skipCall == VK_FALSE) {
        PreCreateFence(device, pCreateInfo);

        result = get_dispatch_table(pc_device_table_map, device)->CreateFence(device, pCreateInfo, pAllocator, pFence);

        PostCreateFence(device, pFence, result);
    }

    return result;
}

bool PreResetFences(
    VkDevice device,
    const VkFence* pFences)
{
    if(pFences != nullptr)
    {
    }

    return true;
}

bool PostResetFences(
    VkDevice device,
    uint32_t fenceCount,
    VkResult result)
{


    if(result < VK_SUCCESS)
    {
        std::string reason = "vkResetFences parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK", "%s", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkResetFences(
    VkDevice device,
    uint32_t fenceCount,
    const VkFence* pFences)
{
    VkResult    result      = VK_ERROR_VALIDATION_FAILED_EXT;
    VkBool32    skipCall    = VK_FALSE;
    layer_data* my_data     = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skipCall |= param_check_vkResetFences(
        my_data->report_data,
        fenceCount,
        pFences);

    if (skipCall == VK_FALSE) {
        PreResetFences(device, pFences);

        result = get_dispatch_table(pc_device_table_map, device)->ResetFences(device, fenceCount, pFences);

        PostResetFences(device, fenceCount, result);
    }

    return result;
}

bool PostGetFenceStatus(
    VkDevice device,
    VkFence fence,
    VkResult result)
{


    if(result < VK_SUCCESS)
    {
        std::string reason = "vkGetFenceStatus parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK", "%s", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkGetFenceStatus(
    VkDevice device,
    VkFence fence)
{
    VkResult result = get_dispatch_table(pc_device_table_map, device)->GetFenceStatus(device, fence);

    PostGetFenceStatus(device, fence, result);

    return result;
}

bool PreWaitForFences(
    VkDevice device,
    const VkFence* pFences)
{
    if(pFences != nullptr)
    {
    }

    return true;
}

bool PostWaitForFences(
    VkDevice device,
    uint32_t fenceCount,
    VkBool32 waitAll,
    uint64_t timeout,
    VkResult result)
{




    if(result < VK_SUCCESS)
    {
        std::string reason = "vkWaitForFences parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK", "%s", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkWaitForFences(
    VkDevice device,
    uint32_t fenceCount,
    const VkFence* pFences,
    VkBool32 waitAll,
    uint64_t timeout)
{
    VkResult    result      = VK_ERROR_VALIDATION_FAILED_EXT;
    VkBool32    skipCall    = VK_FALSE;
    layer_data* my_data     = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skipCall |= param_check_vkWaitForFences(
        my_data->report_data,
        fenceCount,
        pFences,
        waitAll,
        timeout);

    if (skipCall == VK_FALSE) {
        PreWaitForFences(device, pFences);

        result = get_dispatch_table(pc_device_table_map, device)->WaitForFences(device, fenceCount, pFences, waitAll, timeout);

        PostWaitForFences(device, fenceCount, waitAll, timeout, result);
    }

    return result;
}

bool PreCreateSemaphore(
    VkDevice device,
    const VkSemaphoreCreateInfo* pCreateInfo)
{
    if(pCreateInfo != nullptr)
    {
    if(pCreateInfo->sType != VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateSemaphore parameter, VkStructureType pCreateInfo->sType, is an invalid enumerator");
        return false;
    }
    }

    return true;
}

bool PostCreateSemaphore(
    VkDevice device,
    VkSemaphore* pSemaphore,
    VkResult result)
{

    if(pSemaphore != nullptr)
    {
    }

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkCreateSemaphore parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK", "%s", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateSemaphore(
    VkDevice device,
    const VkSemaphoreCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkSemaphore* pSemaphore)
{
    VkResult    result      = VK_ERROR_VALIDATION_FAILED_EXT;
    VkBool32    skipCall    = VK_FALSE;
    layer_data* my_data     = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skipCall |= param_check_vkCreateSemaphore(
        my_data->report_data,
        pCreateInfo,
        pAllocator,
        pSemaphore);

    if (skipCall == VK_FALSE) {
        PreCreateSemaphore(device, pCreateInfo);

        result = get_dispatch_table(pc_device_table_map, device)->CreateSemaphore(device, pCreateInfo, pAllocator, pSemaphore);

        PostCreateSemaphore(device, pSemaphore, result);
    }

    return result;
}

bool PreCreateEvent(
    VkDevice device,
    const VkEventCreateInfo* pCreateInfo)
{
    if(pCreateInfo != nullptr)
    {
    if(pCreateInfo->sType != VK_STRUCTURE_TYPE_EVENT_CREATE_INFO)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateEvent parameter, VkStructureType pCreateInfo->sType, is an invalid enumerator");
        return false;
    }
    }

    return true;
}

bool PostCreateEvent(
    VkDevice device,
    VkEvent* pEvent,
    VkResult result)
{

    if(pEvent != nullptr)
    {
    }

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkCreateEvent parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK", "%s", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateEvent(
    VkDevice device,
    const VkEventCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkEvent* pEvent)
{
    VkResult    result      = VK_ERROR_VALIDATION_FAILED_EXT;
    VkBool32    skipCall    = VK_FALSE;
    layer_data* my_data     = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skipCall |= param_check_vkCreateEvent(
        my_data->report_data,
        pCreateInfo,
        pAllocator,
        pEvent);

    if (skipCall == VK_FALSE) {
        PreCreateEvent(device, pCreateInfo);

        result = get_dispatch_table(pc_device_table_map, device)->CreateEvent(device, pCreateInfo, pAllocator, pEvent);

        PostCreateEvent(device, pEvent, result);
    }

    return result;
}

bool PostGetEventStatus(
    VkDevice device,
    VkEvent event,
    VkResult result)
{


    if(result < VK_SUCCESS)
    {
        std::string reason = "vkGetEventStatus parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK", "%s", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkGetEventStatus(
    VkDevice device,
    VkEvent event)
{
    VkResult result = get_dispatch_table(pc_device_table_map, device)->GetEventStatus(device, event);

    PostGetEventStatus(device, event, result);

    return result;
}

bool PostSetEvent(
    VkDevice device,
    VkEvent event,
    VkResult result)
{


    if(result < VK_SUCCESS)
    {
        std::string reason = "vkSetEvent parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK", "%s", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkSetEvent(
    VkDevice device,
    VkEvent event)
{
    VkResult result = get_dispatch_table(pc_device_table_map, device)->SetEvent(device, event);

    PostSetEvent(device, event, result);

    return result;
}

bool PostResetEvent(
    VkDevice device,
    VkEvent event,
    VkResult result)
{


    if(result < VK_SUCCESS)
    {
        std::string reason = "vkResetEvent parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK", "%s", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkResetEvent(
    VkDevice device,
    VkEvent event)
{
    VkResult result = get_dispatch_table(pc_device_table_map, device)->ResetEvent(device, event);

    PostResetEvent(device, event, result);

    return result;
}

bool PreCreateQueryPool(
    VkDevice device,
    const VkQueryPoolCreateInfo* pCreateInfo)
{
    if(pCreateInfo != nullptr)
    {
    if(pCreateInfo->sType != VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateQueryPool parameter, VkStructureType pCreateInfo->sType, is an invalid enumerator");
        return false;
    }
    if(pCreateInfo->queryType < VK_QUERY_TYPE_BEGIN_RANGE ||
        pCreateInfo->queryType > VK_QUERY_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateQueryPool parameter, VkQueryType pCreateInfo->queryType, is an unrecognized enumerator");
        return false;
    }
    }

    return true;
}

bool PostCreateQueryPool(
    VkDevice device,
    VkQueryPool* pQueryPool,
    VkResult result)
{

    if(pQueryPool != nullptr)
    {
    }

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkCreateQueryPool parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK", "%s", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateQueryPool(
    VkDevice device,
    const VkQueryPoolCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkQueryPool* pQueryPool)
{
    VkResult    result      = VK_ERROR_VALIDATION_FAILED_EXT;
    VkBool32    skipCall    = VK_FALSE;
    layer_data* my_data     = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skipCall |= param_check_vkCreateQueryPool(
        my_data->report_data,
        pCreateInfo,
        pAllocator,
        pQueryPool);

    if (skipCall == VK_FALSE) {
        PreCreateQueryPool(device, pCreateInfo);

        result = get_dispatch_table(pc_device_table_map, device)->CreateQueryPool(device, pCreateInfo, pAllocator, pQueryPool);

        PostCreateQueryPool(device, pQueryPool, result);
    }

    return result;
}

bool PostGetQueryPoolResults(
    VkDevice device,
    VkQueryPool queryPool,
    uint32_t firstQuery,
    uint32_t queryCount,
    size_t dataSize,
    void* pData,
    VkDeviceSize stride,
    VkQueryResultFlags flags,
    VkResult result)
{




    if(pData != nullptr)
    {
    }


    if(result < VK_SUCCESS)
    {
        std::string reason = "vkGetQueryPoolResults parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK", "%s", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkGetQueryPoolResults(
    VkDevice device,
    VkQueryPool queryPool,
    uint32_t firstQuery,
    uint32_t queryCount,
    size_t dataSize,
    void* pData,
    VkDeviceSize stride,
    VkQueryResultFlags flags)
{
    VkResult    result      = VK_ERROR_VALIDATION_FAILED_EXT;
    VkBool32    skipCall    = VK_FALSE;
    layer_data* my_data     = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skipCall |= param_check_vkGetQueryPoolResults(
        my_data->report_data,
        queryPool,
        firstQuery,
        queryCount,
        dataSize,
        pData,
        stride,
        flags);

    if (skipCall == VK_FALSE) {
        result = get_dispatch_table(pc_device_table_map, device)->GetQueryPoolResults(device, queryPool, firstQuery, queryCount, dataSize, pData, stride, flags);

        PostGetQueryPoolResults(device, queryPool, firstQuery, queryCount, dataSize, pData, stride, flags, result);
    }

    return result;
}

bool PreCreateBuffer(
    VkDevice device,
    const VkBufferCreateInfo* pCreateInfo)
{
    if(pCreateInfo != nullptr)
    {
    if(pCreateInfo->sType != VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateBuffer parameter, VkStructureType pCreateInfo->sType, is an invalid enumerator");
        return false;
    }
    if(pCreateInfo->sharingMode < VK_SHARING_MODE_BEGIN_RANGE ||
        pCreateInfo->sharingMode > VK_SHARING_MODE_END_RANGE)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateBuffer parameter, VkSharingMode pCreateInfo->sharingMode, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfo->pQueueFamilyIndices != nullptr)
    {
    }
    }

    return true;
}

bool PostCreateBuffer(
    VkDevice device,
    VkBuffer* pBuffer,
    VkResult result)
{

    if(pBuffer != nullptr)
    {
    }

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkCreateBuffer parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK", "%s", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateBuffer(
    VkDevice device,
    const VkBufferCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkBuffer* pBuffer)
{
    VkResult    result      = VK_ERROR_VALIDATION_FAILED_EXT;
    VkBool32    skipCall    = VK_FALSE;
    layer_data* my_data     = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skipCall |= param_check_vkCreateBuffer(
        my_data->report_data,
        pCreateInfo,
        pAllocator,
        pBuffer);

    if (skipCall == VK_FALSE) {
        PreCreateBuffer(device, pCreateInfo);

        result = get_dispatch_table(pc_device_table_map, device)->CreateBuffer(device, pCreateInfo, pAllocator, pBuffer);

        PostCreateBuffer(device, pBuffer, result);
    }

    return result;
}

bool PreCreateBufferView(
    VkDevice device,
    const VkBufferViewCreateInfo* pCreateInfo)
{
    if(pCreateInfo != nullptr)
    {
    if(pCreateInfo->sType != VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateBufferView parameter, VkStructureType pCreateInfo->sType, is an invalid enumerator");
        return false;
    }
    if(pCreateInfo->format < VK_FORMAT_BEGIN_RANGE ||
        pCreateInfo->format > VK_FORMAT_END_RANGE)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateBufferView parameter, VkFormat pCreateInfo->format, is an unrecognized enumerator");
        return false;
    }
    }

    return true;
}

bool PostCreateBufferView(
    VkDevice device,
    VkBufferView* pView,
    VkResult result)
{

    if(pView != nullptr)
    {
    }

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkCreateBufferView parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK", "%s", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateBufferView(
    VkDevice device,
    const VkBufferViewCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkBufferView* pView)
{
    VkResult    result      = VK_ERROR_VALIDATION_FAILED_EXT;
    VkBool32    skipCall    = VK_FALSE;
    layer_data* my_data     = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skipCall |= param_check_vkCreateBufferView(
        my_data->report_data,
        pCreateInfo,
        pAllocator,
        pView);

    if (skipCall == VK_FALSE) {
        PreCreateBufferView(device, pCreateInfo);

        result = get_dispatch_table(pc_device_table_map, device)->CreateBufferView(device, pCreateInfo, pAllocator, pView);

        PostCreateBufferView(device, pView, result);
    }

    return result;
}

bool PreCreateImage(
    VkDevice device,
    const VkImageCreateInfo* pCreateInfo)
{
    if(pCreateInfo != nullptr)
    {
    if(pCreateInfo->sType != VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateImage parameter, VkStructureType pCreateInfo->sType, is an invalid enumerator");
        return false;
    }
    if(pCreateInfo->imageType < VK_IMAGE_TYPE_BEGIN_RANGE ||
        pCreateInfo->imageType > VK_IMAGE_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateImage parameter, VkImageType pCreateInfo->imageType, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfo->format < VK_FORMAT_BEGIN_RANGE ||
        pCreateInfo->format > VK_FORMAT_END_RANGE)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateImage parameter, VkFormat pCreateInfo->format, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfo->tiling < VK_IMAGE_TILING_BEGIN_RANGE ||
        pCreateInfo->tiling > VK_IMAGE_TILING_END_RANGE)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateImage parameter, VkImageTiling pCreateInfo->tiling, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfo->sharingMode < VK_SHARING_MODE_BEGIN_RANGE ||
        pCreateInfo->sharingMode > VK_SHARING_MODE_END_RANGE)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateImage parameter, VkSharingMode pCreateInfo->sharingMode, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfo->pQueueFamilyIndices != nullptr)
    {
    }
    }

    return true;
}

bool PostCreateImage(
    VkDevice device,
    VkImage* pImage,
    VkResult result)
{

    if(pImage != nullptr)
    {
    }

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkCreateImage parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK", "%s", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateImage(
    VkDevice device,
    const VkImageCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkImage* pImage)
{
    VkResult    result      = VK_ERROR_VALIDATION_FAILED_EXT;
    VkBool32    skipCall    = VK_FALSE;
    layer_data* my_data     = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skipCall |= param_check_vkCreateImage(
        my_data->report_data,
        pCreateInfo,
        pAllocator,
        pImage);

    if (skipCall == VK_FALSE) {
        PreCreateImage(device, pCreateInfo);

        result = get_dispatch_table(pc_device_table_map, device)->CreateImage(device, pCreateInfo, pAllocator, pImage);

        PostCreateImage(device, pImage, result);
    }

    return result;
}

bool PreGetImageSubresourceLayout(
    VkDevice device,
    const VkImageSubresource* pSubresource)
{
    if(pSubresource != nullptr)
    {
    if ((pSubresource->aspectMask &
       (VK_IMAGE_ASPECT_COLOR_BIT | VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT | VK_IMAGE_ASPECT_METADATA_BIT)) == 0)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkGetImageSubresourceLayout parameter, VkImageAspect pSubresource->aspectMask, is an unrecognized enumerator");
        return false;
    }
    }

    return true;
}

bool PostGetImageSubresourceLayout(
    VkDevice device,
    VkImage image,
    VkSubresourceLayout* pLayout)
{


    if(pLayout != nullptr)
    {
    }

    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetImageSubresourceLayout(
    VkDevice device,
    VkImage image,
    const VkImageSubresource* pSubresource,
    VkSubresourceLayout* pLayout)
{
    VkBool32    skipCall    = VK_FALSE;
    layer_data* my_data     = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skipCall |= param_check_vkGetImageSubresourceLayout(
        my_data->report_data,
        image,
        pSubresource,
        pLayout);

    if (skipCall == VK_FALSE) {
        PreGetImageSubresourceLayout(device, pSubresource);

        get_dispatch_table(pc_device_table_map, device)->GetImageSubresourceLayout(device, image, pSubresource, pLayout);

        PostGetImageSubresourceLayout(device, image, pLayout);
    }
}

bool PreCreateImageView(
    VkDevice device,
    const VkImageViewCreateInfo* pCreateInfo)
{
    if(pCreateInfo != nullptr)
    {
    if(pCreateInfo->sType != VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateImageView parameter, VkStructureType pCreateInfo->sType, is an invalid enumerator");
        return false;
    }
    if(pCreateInfo->viewType < VK_IMAGE_VIEW_TYPE_BEGIN_RANGE ||
        pCreateInfo->viewType > VK_IMAGE_VIEW_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateImageView parameter, VkImageViewType pCreateInfo->viewType, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfo->format < VK_FORMAT_BEGIN_RANGE ||
        pCreateInfo->format > VK_FORMAT_END_RANGE)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateImageView parameter, VkFormat pCreateInfo->format, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfo->components.r < VK_COMPONENT_SWIZZLE_BEGIN_RANGE ||
        pCreateInfo->components.r > VK_COMPONENT_SWIZZLE_END_RANGE)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateImageView parameter, VkComponentSwizzle pCreateInfo->components.r, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfo->components.g < VK_COMPONENT_SWIZZLE_BEGIN_RANGE ||
        pCreateInfo->components.g > VK_COMPONENT_SWIZZLE_END_RANGE)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateImageView parameter, VkComponentSwizzle pCreateInfo->components.g, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfo->components.b < VK_COMPONENT_SWIZZLE_BEGIN_RANGE ||
        pCreateInfo->components.b > VK_COMPONENT_SWIZZLE_END_RANGE)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateImageView parameter, VkComponentSwizzle pCreateInfo->components.b, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfo->components.a < VK_COMPONENT_SWIZZLE_BEGIN_RANGE ||
        pCreateInfo->components.a > VK_COMPONENT_SWIZZLE_END_RANGE)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateImageView parameter, VkComponentSwizzle pCreateInfo->components.a, is an unrecognized enumerator");
        return false;
    }
    }

    return true;
}

bool PostCreateImageView(
    VkDevice device,
    VkImageView* pView,
    VkResult result)
{

    if(pView != nullptr)
    {
    }

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkCreateImageView parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK", "%s", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateImageView(
    VkDevice device,
    const VkImageViewCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkImageView* pView)
{
    VkResult    result      = VK_ERROR_VALIDATION_FAILED_EXT;
    VkBool32    skipCall    = VK_FALSE;
    layer_data* my_data     = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skipCall |= param_check_vkCreateImageView(
        my_data->report_data,
        pCreateInfo,
        pAllocator,
        pView);

    if (skipCall == VK_FALSE) {
        PreCreateImageView(device, pCreateInfo);

        result = get_dispatch_table(pc_device_table_map, device)->CreateImageView(device, pCreateInfo, pAllocator, pView);

        PostCreateImageView(device, pView, result);
    }

    return result;
}

bool PreCreateShaderModule(
    VkDevice device,
    const VkShaderModuleCreateInfo* pCreateInfo)
{
    if(pCreateInfo) {
        if(pCreateInfo->sType != VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO) {
            log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
                "vkCreateShaderModule parameter, VkStructureType pCreateInfo->sType, is an invalid enumerator");
            return false;
        }
        if(!pCreateInfo->pCode) {
            log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
                "vkCreateShaderModule paramter, void* pCreateInfo->pCode, is null");
            return false;
        }
    } else {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
            "vkCreateShaderModule parameter, VkShaderModuleCreateInfo pCreateInfo, is null");
        return false;
    }

    return true;
}

bool PostCreateShaderModule(
    VkDevice device,
    VkShaderModule* pShaderModule,
    VkResult result)
{
    if(result < VK_SUCCESS) {
        std::string reason = "vkCreateShaderModule parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK", "%s", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateShaderModule(
    VkDevice device,
    const VkShaderModuleCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkShaderModule* pShaderModule)
{
    VkResult    result      = VK_ERROR_VALIDATION_FAILED_EXT;
    VkBool32    skipCall    = VK_FALSE;
    layer_data* my_data     = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skipCall |= param_check_vkCreateShaderModule(
        my_data->report_data,
        pCreateInfo,
        pAllocator,
        pShaderModule);

    if (skipCall == VK_FALSE) {
        PreCreateShaderModule(device, pCreateInfo);

        result = get_dispatch_table(pc_device_table_map, device)->CreateShaderModule(device, pCreateInfo, pAllocator, pShaderModule);

        PostCreateShaderModule(device, pShaderModule, result);
    }

    return result;
}

bool PreCreatePipelineCache(
    VkDevice device,
    const VkPipelineCacheCreateInfo* pCreateInfo)
{
    if(pCreateInfo != nullptr)
    {
    if(pCreateInfo->sType != VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreatePipelineCache parameter, VkStructureType pCreateInfo->sType, is an invalid enumerator");
        return false;
    }
    if(pCreateInfo->pInitialData != nullptr)
    {
    }
    }

    return true;
}

bool PostCreatePipelineCache(
    VkDevice device,
    VkPipelineCache* pPipelineCache,
    VkResult result)
{

    if(pPipelineCache != nullptr)
    {
    }

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkCreatePipelineCache parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK", "%s", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreatePipelineCache(
    VkDevice device,
    const VkPipelineCacheCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkPipelineCache* pPipelineCache)
{
    VkResult    result      = VK_ERROR_VALIDATION_FAILED_EXT;
    VkBool32    skipCall    = VK_FALSE;
    layer_data* my_data     = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skipCall |= param_check_vkCreatePipelineCache(
        my_data->report_data,
        pCreateInfo,
        pAllocator,
        pPipelineCache);

    if (skipCall == VK_FALSE) {
        PreCreatePipelineCache(device, pCreateInfo);

        result = get_dispatch_table(pc_device_table_map, device)->CreatePipelineCache(device, pCreateInfo, pAllocator, pPipelineCache);

        PostCreatePipelineCache(device, pPipelineCache, result);
    }

    return result;
}

bool PostGetPipelineCacheData(
    VkDevice device,
    VkPipelineCache pipelineCache,
    size_t* pDataSize,
    void* pData,
    VkResult result)
{


    if(pDataSize != nullptr)
    {
    }

    if(pData != nullptr)
    {
    }

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkGetPipelineCacheData parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK", "%s", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkGetPipelineCacheData(
    VkDevice device,
    VkPipelineCache pipelineCache,
    size_t* pDataSize,
    void* pData)
{
    VkResult    result      = VK_ERROR_VALIDATION_FAILED_EXT;
    VkBool32    skipCall    = VK_FALSE;
    layer_data* my_data     = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skipCall |= param_check_vkGetPipelineCacheData(
        my_data->report_data,
        pipelineCache,
        pDataSize,
        pData);

    if (skipCall == VK_FALSE) {
        result = get_dispatch_table(pc_device_table_map, device)->GetPipelineCacheData(device, pipelineCache, pDataSize, pData);

        PostGetPipelineCacheData(device, pipelineCache, pDataSize, pData, result);
    }

    return result;
}

bool PreMergePipelineCaches(
    VkDevice device,
    const VkPipelineCache* pSrcCaches)
{
    if(pSrcCaches != nullptr)
    {
    }

    return true;
}

bool PostMergePipelineCaches(
    VkDevice device,
    VkPipelineCache dstCache,
    uint32_t srcCacheCount,
    VkResult result)
{



    if(result < VK_SUCCESS)
    {
        std::string reason = "vkMergePipelineCaches parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK", "%s", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkMergePipelineCaches(
    VkDevice device,
    VkPipelineCache dstCache,
    uint32_t srcCacheCount,
    const VkPipelineCache* pSrcCaches)
{
    VkResult    result      = VK_ERROR_VALIDATION_FAILED_EXT;
    VkBool32    skipCall    = VK_FALSE;
    layer_data* my_data     = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skipCall |= param_check_vkMergePipelineCaches(
        my_data->report_data,
        dstCache,
        srcCacheCount,
        pSrcCaches);

    if (skipCall == VK_FALSE) {
        PreMergePipelineCaches(device, pSrcCaches);

        result = get_dispatch_table(pc_device_table_map, device)->MergePipelineCaches(device, dstCache, srcCacheCount, pSrcCaches);

        PostMergePipelineCaches(device, dstCache, srcCacheCount, result);
    }

    return result;
}

bool PreCreateGraphicsPipelines(
    VkDevice device,
    const VkGraphicsPipelineCreateInfo* pCreateInfos)
{
    layer_data *data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);

    // TODO: Handle count
    if(pCreateInfos != nullptr)
    {
    if(pCreateInfos->sType != VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelines parameter, VkStructureType pCreateInfos->sType, is an invalid enumerator");
        return false;
    }
    if(pCreateInfos->pStages != nullptr)
    {
    if(pCreateInfos->pStages->sType != VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelines parameter, VkStructureType pCreateInfos->pStages->sType, is an invalid enumerator");
        return false;
    }
    if(pCreateInfos->pStages->pSpecializationInfo != nullptr)
    {
    if(pCreateInfos->pStages->pSpecializationInfo->pMapEntries != nullptr)
    {
    }
    if(pCreateInfos->pStages->pSpecializationInfo->pData != nullptr)
    {
    }
    }
    }
    if(pCreateInfos->pVertexInputState != nullptr)
    {
    if(pCreateInfos->pVertexInputState->sType != VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelines parameter, VkStructureType pCreateInfos->pVertexInputState->sType, is an invalid enumerator");
        return false;
    }
    if(pCreateInfos->pVertexInputState->pVertexBindingDescriptions != nullptr)
    {
    if(pCreateInfos->pVertexInputState->pVertexBindingDescriptions->inputRate < VK_VERTEX_INPUT_RATE_BEGIN_RANGE ||
        pCreateInfos->pVertexInputState->pVertexBindingDescriptions->inputRate > VK_VERTEX_INPUT_RATE_END_RANGE)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelines parameter, VkVertexInputRate pCreateInfos->pVertexInputState->pVertexBindingDescriptions->inputRate, is an unrecognized enumerator");
        return false;
    }
    }
    if(pCreateInfos->pVertexInputState->pVertexAttributeDescriptions != nullptr)
    {
    if(pCreateInfos->pVertexInputState->pVertexAttributeDescriptions->format < VK_FORMAT_BEGIN_RANGE ||
        pCreateInfos->pVertexInputState->pVertexAttributeDescriptions->format > VK_FORMAT_END_RANGE)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelines parameter, VkFormat pCreateInfos->pVertexInputState->pVertexAttributeDescriptions->format, is an unrecognized enumerator");
        return false;
    }
    }
    }
    if(pCreateInfos->pInputAssemblyState != nullptr)
    {
    if(pCreateInfos->pInputAssemblyState->sType != VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelines parameter, VkStructureType pCreateInfos->pInputAssemblyState->sType, is an invalid enumerator");
        return false;
    }
    if(pCreateInfos->pInputAssemblyState->topology < VK_PRIMITIVE_TOPOLOGY_BEGIN_RANGE ||
        pCreateInfos->pInputAssemblyState->topology > VK_PRIMITIVE_TOPOLOGY_END_RANGE)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelines parameter, VkPrimitiveTopology pCreateInfos->pInputAssemblyState->topology, is an unrecognized enumerator");
        return false;
    }
    }
    if(pCreateInfos->pTessellationState != nullptr)
    {
    if(pCreateInfos->pTessellationState->sType != VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelines parameter, VkStructureType pCreateInfos->pTessellationState->sType, is an invalid enumerator");
        return false;
    }
    }
    if(pCreateInfos->pViewportState != nullptr)
    {
    if(pCreateInfos->pViewportState->sType != VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelines parameter, VkStructureType pCreateInfos->pViewportState->sType, is an invalid enumerator");
        return false;
    }
    }
    if(pCreateInfos->pRasterizationState != nullptr)
    {
    if(pCreateInfos->pRasterizationState->sType != VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelines parameter, VkStructureType pCreateInfos->pRasterizationState->sType, is an invalid enumerator");
        return false;
    }
    if(pCreateInfos->pRasterizationState->polygonMode < VK_POLYGON_MODE_BEGIN_RANGE ||
        pCreateInfos->pRasterizationState->polygonMode > VK_POLYGON_MODE_END_RANGE)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelines parameter, VkPolygonMode pCreateInfos->pRasterizationState->polygonMode, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfos->pRasterizationState->cullMode & ~VK_CULL_MODE_FRONT_AND_BACK)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelines parameter, VkCullMode pCreateInfos->pRasterizationState->cullMode, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfos->pRasterizationState->frontFace < VK_FRONT_FACE_BEGIN_RANGE ||
        pCreateInfos->pRasterizationState->frontFace > VK_FRONT_FACE_END_RANGE)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelines parameter, VkFrontFace pCreateInfos->pRasterizationState->frontFace, is an unrecognized enumerator");
        return false;
    }
    }
    if(pCreateInfos->pMultisampleState != nullptr)
    {
    if(pCreateInfos->pMultisampleState->sType != VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelines parameter, VkStructureType pCreateInfos->pMultisampleState->sType, is an invalid enumerator");
        return false;
    }
    }
    if(pCreateInfos->pDepthStencilState != nullptr)
    {
    if(pCreateInfos->pDepthStencilState->sType != VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelines parameter, VkStructureType pCreateInfos->pDepthStencilState->sType, is an invalid enumerator");
        return false;
    }
    if(pCreateInfos->pDepthStencilState->depthCompareOp < VK_COMPARE_OP_BEGIN_RANGE ||
        pCreateInfos->pDepthStencilState->depthCompareOp > VK_COMPARE_OP_END_RANGE)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelines parameter, VkCompareOp pCreateInfos->pDepthStencilState->depthCompareOp, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfos->pDepthStencilState->front.failOp < VK_STENCIL_OP_BEGIN_RANGE ||
        pCreateInfos->pDepthStencilState->front.failOp > VK_STENCIL_OP_END_RANGE)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelines parameter, VkStencilOp pCreateInfos->pDepthStencilState->front.failOp, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfos->pDepthStencilState->front.passOp < VK_STENCIL_OP_BEGIN_RANGE ||
        pCreateInfos->pDepthStencilState->front.passOp > VK_STENCIL_OP_END_RANGE)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelines parameter, VkStencilOp pCreateInfos->pDepthStencilState->front.passOp, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfos->pDepthStencilState->front.depthFailOp < VK_STENCIL_OP_BEGIN_RANGE ||
        pCreateInfos->pDepthStencilState->front.depthFailOp > VK_STENCIL_OP_END_RANGE)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelines parameter, VkStencilOp pCreateInfos->pDepthStencilState->front.depthFailOp, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfos->pDepthStencilState->front.compareOp < VK_COMPARE_OP_BEGIN_RANGE ||
        pCreateInfos->pDepthStencilState->front.compareOp > VK_COMPARE_OP_END_RANGE)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelines parameter, VkCompareOp pCreateInfos->pDepthStencilState->front.compareOp, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfos->pDepthStencilState->back.failOp < VK_STENCIL_OP_BEGIN_RANGE ||
        pCreateInfos->pDepthStencilState->back.failOp > VK_STENCIL_OP_END_RANGE)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelines parameter, VkStencilOp pCreateInfos->pDepthStencilState->back.failOp, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfos->pDepthStencilState->back.passOp < VK_STENCIL_OP_BEGIN_RANGE ||
        pCreateInfos->pDepthStencilState->back.passOp > VK_STENCIL_OP_END_RANGE)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelines parameter, VkStencilOp pCreateInfos->pDepthStencilState->back.passOp, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfos->pDepthStencilState->back.depthFailOp < VK_STENCIL_OP_BEGIN_RANGE ||
        pCreateInfos->pDepthStencilState->back.depthFailOp > VK_STENCIL_OP_END_RANGE)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelines parameter, VkStencilOp pCreateInfos->pDepthStencilState->back.depthFailOp, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfos->pDepthStencilState->back.compareOp < VK_COMPARE_OP_BEGIN_RANGE ||
        pCreateInfos->pDepthStencilState->back.compareOp > VK_COMPARE_OP_END_RANGE)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelines parameter, VkCompareOp pCreateInfos->pDepthStencilState->back.compareOp, is an unrecognized enumerator");
        return false;
    }
    }
    if(pCreateInfos->pColorBlendState != nullptr)
    {
    if(pCreateInfos->pColorBlendState->sType != VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelines parameter, VkStructureType pCreateInfos->pColorBlendState->sType, is an invalid enumerator");
        return false;
    }
    if (pCreateInfos->pColorBlendState->logicOpEnable == VK_TRUE &&
        (pCreateInfos->pColorBlendState->logicOp < VK_LOGIC_OP_BEGIN_RANGE ||
         pCreateInfos->pColorBlendState->logicOp > VK_LOGIC_OP_END_RANGE)) {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelines parameter, VkLogicOp pCreateInfos->pColorBlendState->logicOp, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfos->pColorBlendState->pAttachments != nullptr && pCreateInfos->pColorBlendState->pAttachments->blendEnable == VK_TRUE)
    {
    if(pCreateInfos->pColorBlendState->pAttachments->srcColorBlendFactor < VK_BLEND_FACTOR_BEGIN_RANGE ||
        pCreateInfos->pColorBlendState->pAttachments->srcColorBlendFactor > VK_BLEND_FACTOR_END_RANGE)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelines parameter, VkBlendFactor pCreateInfos->pColorBlendState->pAttachments->srcColorBlendFactor, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfos->pColorBlendState->pAttachments->dstColorBlendFactor < VK_BLEND_FACTOR_BEGIN_RANGE ||
        pCreateInfos->pColorBlendState->pAttachments->dstColorBlendFactor > VK_BLEND_FACTOR_END_RANGE)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelines parameter, VkBlendFactor pCreateInfos->pColorBlendState->pAttachments->dstColorBlendFactor, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfos->pColorBlendState->pAttachments->colorBlendOp < VK_BLEND_OP_BEGIN_RANGE ||
        pCreateInfos->pColorBlendState->pAttachments->colorBlendOp > VK_BLEND_OP_END_RANGE)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelines parameter, VkBlendOp pCreateInfos->pColorBlendState->pAttachments->colorBlendOp, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfos->pColorBlendState->pAttachments->srcAlphaBlendFactor < VK_BLEND_FACTOR_BEGIN_RANGE ||
        pCreateInfos->pColorBlendState->pAttachments->srcAlphaBlendFactor > VK_BLEND_FACTOR_END_RANGE)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelines parameter, VkBlendFactor pCreateInfos->pColorBlendState->pAttachments->srcAlphaBlendFactor, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfos->pColorBlendState->pAttachments->dstAlphaBlendFactor < VK_BLEND_FACTOR_BEGIN_RANGE ||
        pCreateInfos->pColorBlendState->pAttachments->dstAlphaBlendFactor > VK_BLEND_FACTOR_END_RANGE)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelines parameter, VkBlendFactor pCreateInfos->pColorBlendState->pAttachments->dstAlphaBlendFactor, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfos->pColorBlendState->pAttachments->alphaBlendOp < VK_BLEND_OP_BEGIN_RANGE ||
        pCreateInfos->pColorBlendState->pAttachments->alphaBlendOp > VK_BLEND_OP_END_RANGE)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelines parameter, VkBlendOp pCreateInfos->pColorBlendState->pAttachments->alphaBlendOp, is an unrecognized enumerator");
        return false;
    }
    }
    }
    if(pCreateInfos->renderPass == VK_NULL_HANDLE)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelines parameter, VkRenderPass pCreateInfos->renderPass, is null pointer");
    }

    int i = 0;
    for (auto j = 0; j < pCreateInfos[i].stageCount; j++) {
        validate_string(data, "vkCreateGraphicsPipelines()", "pCreateInfos[i].pStages[j].pName", pCreateInfos[i].pStages[j].pName);
    }

    }

    return true;
}

bool PostCreateGraphicsPipelines(
    VkDevice device,
    VkPipelineCache pipelineCache,
    uint32_t count,
    VkPipeline* pPipelines,
    VkResult result)
{



    if(pPipelines != nullptr)
    {
    }

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkCreateGraphicsPipelines parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK", "%s", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateGraphicsPipelines(
    VkDevice device,
    VkPipelineCache pipelineCache,
    uint32_t createInfoCount,
    const VkGraphicsPipelineCreateInfo* pCreateInfos,
    const VkAllocationCallbacks* pAllocator,
    VkPipeline* pPipelines)
{
    VkResult    result      = VK_ERROR_VALIDATION_FAILED_EXT;
    VkBool32    skipCall    = VK_FALSE;
    layer_data* my_data     = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skipCall |= param_check_vkCreateGraphicsPipelines(
        my_data->report_data,
        pipelineCache,
        createInfoCount,
        pCreateInfos,
        pAllocator,
        pPipelines);

    if (skipCall == VK_FALSE) {
        PreCreateGraphicsPipelines(device, pCreateInfos);

        result = get_dispatch_table(pc_device_table_map, device)->CreateGraphicsPipelines(device, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);

        PostCreateGraphicsPipelines(device, pipelineCache, createInfoCount, pPipelines, result);
    }

    return result;
}

bool PreCreateComputePipelines(
    VkDevice device,
    const VkComputePipelineCreateInfo* pCreateInfos)
{
    layer_data *data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);

    if(pCreateInfos != nullptr)
    {
    // TODO: Handle count!
    if(pCreateInfos->sType != VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateComputePipelines parameter, VkStructureType pCreateInfos->sType, is an invalid enumerator");
        return false;
    }
    if(pCreateInfos->stage.sType != VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateComputePipelines parameter, VkStructureType pCreateInfos->cs.sType, is an invalid enumerator");
        return false;
    }
    if(pCreateInfos->stage.pSpecializationInfo != nullptr)
    {
    if(pCreateInfos->stage.pSpecializationInfo->pMapEntries != nullptr)
    {
    }
    if(pCreateInfos->stage.pSpecializationInfo->pData != nullptr)
    {
    }
    }

    int i = 0;
    validate_string(data, "vkCreateComputePipelines()", "pCreateInfos[i].stage.pName", pCreateInfos[i].stage.pName);
    }

    return true;
}

bool PostCreateComputePipelines(
    VkDevice device,
    VkPipelineCache pipelineCache,
    uint32_t count,
    VkPipeline* pPipelines,
    VkResult result)
{



    if(pPipelines != nullptr)
    {
    }

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkCreateComputePipelines parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK", "%s", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateComputePipelines(
    VkDevice device,
    VkPipelineCache pipelineCache,
    uint32_t createInfoCount,
    const VkComputePipelineCreateInfo* pCreateInfos,
    const VkAllocationCallbacks* pAllocator,
    VkPipeline* pPipelines)
{
    VkResult    result      = VK_ERROR_VALIDATION_FAILED_EXT;
    VkBool32    skipCall    = VK_FALSE;
    layer_data* my_data     = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skipCall |= param_check_vkCreateComputePipelines(
        my_data->report_data,
        pipelineCache,
        createInfoCount,
        pCreateInfos,
        pAllocator,
        pPipelines);

    if (skipCall == VK_FALSE) {
        PreCreateComputePipelines(device, pCreateInfos);

        result = get_dispatch_table(pc_device_table_map, device)->CreateComputePipelines(device, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);

        PostCreateComputePipelines(device, pipelineCache, createInfoCount, pPipelines, result);
    }

    return result;
}

bool PreCreatePipelineLayout(
    VkDevice device,
    const VkPipelineLayoutCreateInfo* pCreateInfo)
{
    if(pCreateInfo != nullptr)
    {
    if(pCreateInfo->sType != VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreatePipelineLayout parameter, VkStructureType pCreateInfo->sType, is an invalid enumerator");
        return false;
    }
    if(pCreateInfo->pSetLayouts != nullptr)
    {
    }
    if(pCreateInfo->pPushConstantRanges != nullptr)
    {
    }
    }

    return true;
}

bool PostCreatePipelineLayout(
    VkDevice device,
    VkPipelineLayout* pPipelineLayout,
    VkResult result)
{

    if(pPipelineLayout != nullptr)
    {
    }

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkCreatePipelineLayout parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK", "%s", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreatePipelineLayout(
    VkDevice device,
    const VkPipelineLayoutCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkPipelineLayout* pPipelineLayout)
{
    VkResult    result      = VK_ERROR_VALIDATION_FAILED_EXT;
    VkBool32    skipCall    = VK_FALSE;
    layer_data* my_data     = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skipCall |= param_check_vkCreatePipelineLayout(
        my_data->report_data,
        pCreateInfo,
        pAllocator,
        pPipelineLayout);

    if (skipCall == VK_FALSE) {
        PreCreatePipelineLayout(device, pCreateInfo);

        result = get_dispatch_table(pc_device_table_map, device)->CreatePipelineLayout(device, pCreateInfo, pAllocator, pPipelineLayout);

        PostCreatePipelineLayout(device, pPipelineLayout, result);
    }

    return result;
}

bool PreCreateSampler(
    VkDevice device,
    const VkSamplerCreateInfo* pCreateInfo)
{
    if(pCreateInfo != nullptr)
    {
    if(pCreateInfo->sType != VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateSampler parameter, VkStructureType pCreateInfo->sType, is an invalid enumerator");
        return false;
    }
    if(pCreateInfo->magFilter < VK_FILTER_BEGIN_RANGE ||
        pCreateInfo->magFilter > VK_FILTER_END_RANGE)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateSampler parameter, VkFilter pCreateInfo->magFilter, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfo->minFilter < VK_FILTER_BEGIN_RANGE ||
        pCreateInfo->minFilter > VK_FILTER_END_RANGE)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateSampler parameter, VkFilter pCreateInfo->minFilter, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfo->mipmapMode < VK_SAMPLER_MIPMAP_MODE_BEGIN_RANGE ||
        pCreateInfo->mipmapMode > VK_SAMPLER_MIPMAP_MODE_END_RANGE)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateSampler parameter, VkSamplerMipmapMode pCreateInfo->mipmapMode, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfo->addressModeU < VK_SAMPLER_ADDRESS_MODE_BEGIN_RANGE ||
        pCreateInfo->addressModeU > VK_SAMPLER_ADDRESS_MODE_END_RANGE)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateSampler parameter, VkTexAddress pCreateInfo->addressModeU, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfo->addressModeV < VK_SAMPLER_ADDRESS_MODE_BEGIN_RANGE ||
        pCreateInfo->addressModeV > VK_SAMPLER_ADDRESS_MODE_END_RANGE)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateSampler parameter, VkTexAddress pCreateInfo->addressModeV, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfo->addressModeW < VK_SAMPLER_ADDRESS_MODE_BEGIN_RANGE ||
        pCreateInfo->addressModeW > VK_SAMPLER_ADDRESS_MODE_END_RANGE)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateSampler parameter, VkTexAddress pCreateInfo->addressModeW, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfo->compareEnable)
    {
        if(pCreateInfo->compareOp < VK_COMPARE_OP_BEGIN_RANGE ||
            pCreateInfo->compareOp > VK_COMPARE_OP_END_RANGE)
        {
            log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
            "vkCreateSampler parameter, VkCompareOp pCreateInfo->compareOp, is an unrecognized enumerator");
            return false;
        }
    }
    if(pCreateInfo->borderColor < VK_BORDER_COLOR_BEGIN_RANGE ||
        pCreateInfo->borderColor > VK_BORDER_COLOR_END_RANGE)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateSampler parameter, VkBorderColor pCreateInfo->borderColor, is an unrecognized enumerator");
        return false;
    }
    }

    return true;
}

bool PostCreateSampler(
    VkDevice device,
    VkSampler* pSampler,
    VkResult result)
{

    if(pSampler != nullptr)
    {
    }

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkCreateSampler parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK", "%s", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateSampler(
    VkDevice device,
    const VkSamplerCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkSampler* pSampler)
{
    VkResult    result      = VK_ERROR_VALIDATION_FAILED_EXT;
    VkBool32    skipCall    = VK_FALSE;
    layer_data* my_data     = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skipCall |= param_check_vkCreateSampler(
        my_data->report_data,
        pCreateInfo,
        pAllocator,
        pSampler);

    if (skipCall == VK_FALSE) {
        PreCreateSampler(device, pCreateInfo);

        result = get_dispatch_table(pc_device_table_map, device)->CreateSampler(device, pCreateInfo, pAllocator, pSampler);

        PostCreateSampler(device, pSampler, result);
    }

    return result;
}

bool PreCreateDescriptorSetLayout(
    VkDevice device,
    const VkDescriptorSetLayoutCreateInfo* pCreateInfo)
{
    if(pCreateInfo != nullptr)
    {
    if(pCreateInfo->sType != VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateDescriptorSetLayout parameter, VkStructureType pCreateInfo->sType, is an invalid enumerator");
        return false;
    }
    if(pCreateInfo->pBindings != nullptr)
    {
    if(pCreateInfo->pBindings->descriptorType < VK_DESCRIPTOR_TYPE_BEGIN_RANGE ||
        pCreateInfo->pBindings->descriptorType > VK_DESCRIPTOR_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateDescriptorSetLayout parameter, VkDescriptorType pCreateInfo->pBindings->descriptorType, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfo->pBindings->pImmutableSamplers != nullptr)
    {
    }
    }
    }

    return true;
}

bool PostCreateDescriptorSetLayout(
    VkDevice device,
    VkDescriptorSetLayout* pSetLayout,
    VkResult result)
{

    if(pSetLayout != nullptr)
    {
    }

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkCreateDescriptorSetLayout parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK", "%s", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateDescriptorSetLayout(
    VkDevice device,
    const VkDescriptorSetLayoutCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDescriptorSetLayout* pSetLayout)
{
    VkResult    result      = VK_ERROR_VALIDATION_FAILED_EXT;
    VkBool32    skipCall    = VK_FALSE;
    layer_data* my_data     = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skipCall |= param_check_vkCreateDescriptorSetLayout(
        my_data->report_data,
        pCreateInfo,
        pAllocator,
        pSetLayout);

    if (skipCall == VK_FALSE) {
        PreCreateDescriptorSetLayout(device, pCreateInfo);

        result = get_dispatch_table(pc_device_table_map, device)->CreateDescriptorSetLayout(device, pCreateInfo, pAllocator, pSetLayout);

        PostCreateDescriptorSetLayout(device, pSetLayout, result);
    }

    return result;
}

bool PreCreateDescriptorPool(
    VkDevice device,
    const VkDescriptorPoolCreateInfo* pCreateInfo)
{
    if(pCreateInfo != nullptr)
    {
    if(pCreateInfo->sType != VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateDescriptorPool parameter, VkStructureType pCreateInfo->sType, is an invalid enumerator");
        return false;
    }
    if(pCreateInfo->pPoolSizes != nullptr)
    {
    if(pCreateInfo->pPoolSizes->type < VK_DESCRIPTOR_TYPE_BEGIN_RANGE ||
        pCreateInfo->pPoolSizes->type > VK_DESCRIPTOR_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateDescriptorPool parameter, VkDescriptorType pCreateInfo->pTypeCount->type, is an unrecognized enumerator");
        return false;
    }
    }
    }

    return true;
}

bool PostCreateDescriptorPool(
    VkDevice device,
    uint32_t maxSets,
    VkDescriptorPool* pDescriptorPool,
    VkResult result)
{

    /* TODOVV: How do we validate maxSets? Probably belongs in the limits layer? */

    if(pDescriptorPool != nullptr)
    {
    }

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkCreateDescriptorPool parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK", "%s", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateDescriptorPool(
    VkDevice device,
    const VkDescriptorPoolCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDescriptorPool* pDescriptorPool)
{
    VkResult    result      = VK_ERROR_VALIDATION_FAILED_EXT;
    VkBool32    skipCall    = VK_FALSE;
    layer_data* my_data     = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skipCall |= param_check_vkCreateDescriptorPool(
        my_data->report_data,
        pCreateInfo,
        pAllocator,
        pDescriptorPool);

    if (skipCall == VK_FALSE) {
        PreCreateDescriptorPool(device, pCreateInfo);

        result = get_dispatch_table(pc_device_table_map, device)->CreateDescriptorPool(device, pCreateInfo, pAllocator, pDescriptorPool);

        PostCreateDescriptorPool(device, pCreateInfo->maxSets, pDescriptorPool, result);
    }

    return result;
}

bool PostResetDescriptorPool(
    VkDevice device,
    VkDescriptorPool descriptorPool,
    VkResult result)
{


    if(result < VK_SUCCESS)
    {
        std::string reason = "vkResetDescriptorPool parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK", "%s", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkResetDescriptorPool(
    VkDevice device,
    VkDescriptorPool descriptorPool,
    VkDescriptorPoolResetFlags flags)
{
    VkResult result = get_dispatch_table(pc_device_table_map, device)->ResetDescriptorPool(device, descriptorPool, flags);

    PostResetDescriptorPool(device, descriptorPool, result);

    return result;
}

bool PreAllocateDescriptorSets(
    VkDevice device,
    const VkDescriptorSetLayout* pSetLayouts)
{
    if(pSetLayouts != nullptr)
    {
    }

    return true;
}

bool PostAllocateDescriptorSets(
    VkDevice device,
    VkDescriptorPool descriptorPool,
    uint32_t count,
    VkDescriptorSet* pDescriptorSets,
    VkResult result)
{


    if(pDescriptorSets != nullptr)
    {
    }

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkAllocateDescriptorSets parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK", "%s", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkAllocateDescriptorSets(
    VkDevice device,
    const VkDescriptorSetAllocateInfo* pAllocateInfo,
    VkDescriptorSet* pDescriptorSets)
{
    VkResult    result      = VK_ERROR_VALIDATION_FAILED_EXT;
    VkBool32    skipCall    = VK_FALSE;
    layer_data* my_data     = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skipCall |= param_check_vkAllocateDescriptorSets(
        my_data->report_data,
        pAllocateInfo,
        pDescriptorSets);

    if (skipCall == VK_FALSE) {
        PreAllocateDescriptorSets(device, pAllocateInfo->pSetLayouts);

        result = get_dispatch_table(pc_device_table_map, device)->AllocateDescriptorSets(device, pAllocateInfo, pDescriptorSets);

        PostAllocateDescriptorSets(device, pAllocateInfo->descriptorPool, pAllocateInfo->descriptorSetCount, pDescriptorSets, result);
    }

    return result;
}

bool PreFreeDescriptorSets(
    VkDevice device,
    const VkDescriptorSet* pDescriptorSets)
{
    if(pDescriptorSets != nullptr)
    {
    }

    return true;
}

bool PostFreeDescriptorSets(
    VkDevice device,
    VkDescriptorPool descriptorPool,
    uint32_t count,
    VkResult result)
{



    if(result < VK_SUCCESS)
    {
        std::string reason = "vkFreeDescriptorSets parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK", "%s", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkFreeDescriptorSets(
    VkDevice device,
    VkDescriptorPool descriptorPool,
    uint32_t descriptorSetCount,
    const VkDescriptorSet* pDescriptorSets)
{
    VkResult    result      = VK_ERROR_VALIDATION_FAILED_EXT;
    VkBool32    skipCall    = VK_FALSE;
    layer_data* my_data     = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skipCall |= param_check_vkFreeDescriptorSets(
        my_data->report_data,
        descriptorPool,
        descriptorSetCount,
        pDescriptorSets);

    if (skipCall == VK_FALSE) {
        PreFreeDescriptorSets(device, pDescriptorSets);

        result = get_dispatch_table(pc_device_table_map, device)->FreeDescriptorSets(device, descriptorPool, descriptorSetCount, pDescriptorSets);

        PostFreeDescriptorSets(device, descriptorPool, descriptorSetCount, result);
    }

    return result;
}

bool PreUpdateDescriptorSets(
    VkDevice device,
    const VkWriteDescriptorSet* pDescriptorWrites,
    const VkCopyDescriptorSet* pDescriptorCopies)
{
    if(pDescriptorWrites != nullptr)
    {
    if(pDescriptorWrites->sType != VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkUpdateDescriptorSets parameter, VkStructureType pDescriptorWrites->sType, is an invalid enumerator");
        return false;
    }
    if(pDescriptorWrites->descriptorType < VK_DESCRIPTOR_TYPE_BEGIN_RANGE ||
        pDescriptorWrites->descriptorType > VK_DESCRIPTOR_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkUpdateDescriptorSets parameter, VkDescriptorType pDescriptorWrites->descriptorType, is an unrecognized enumerator");
        return false;
    }
    /* TODO: Validate other parts of pImageInfo, pBufferInfo, pTexelBufferView? */
    /* TODO: This test should probably only be done if descriptorType is correct type of descriptor */
    if(pDescriptorWrites->pImageInfo != nullptr)
    {
    if (((pDescriptorWrites->pImageInfo->imageLayout < VK_IMAGE_LAYOUT_BEGIN_RANGE) ||
         (pDescriptorWrites->pImageInfo->imageLayout > VK_IMAGE_LAYOUT_END_RANGE))  &&
        (pDescriptorWrites->pImageInfo->imageLayout != VK_IMAGE_LAYOUT_PRESENT_SRC_KHR))
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkUpdateDescriptorSets parameter, VkImageLayout pDescriptorWrites->pDescriptors->imageLayout, is an unrecognized enumerator");
        return false;
    }
    }
    }

    if(pDescriptorCopies != nullptr)
    {
    if(pDescriptorCopies->sType != VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkUpdateDescriptorSets parameter, VkStructureType pDescriptorCopies->sType, is an invalid enumerator");
        return false;
    }
    }

    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkUpdateDescriptorSets(
    VkDevice device,
    uint32_t descriptorWriteCount,
    const VkWriteDescriptorSet* pDescriptorWrites,
    uint32_t descriptorCopyCount,
    const VkCopyDescriptorSet* pDescriptorCopies)
{
    VkBool32    skipCall    = VK_FALSE;
    layer_data* my_data     = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skipCall |= param_check_vkUpdateDescriptorSets(
        my_data->report_data,
        descriptorWriteCount,
        pDescriptorWrites,
        descriptorCopyCount,
        pDescriptorCopies);

    if (skipCall == VK_FALSE) {
        PreUpdateDescriptorSets(device, pDescriptorWrites, pDescriptorCopies);

        get_dispatch_table(pc_device_table_map, device)->UpdateDescriptorSets(device, descriptorWriteCount, pDescriptorWrites, descriptorCopyCount, pDescriptorCopies);
    }
}

bool PreCreateFramebuffer(
    VkDevice device,
    const VkFramebufferCreateInfo* pCreateInfo)
{
    if(pCreateInfo != nullptr)
    {
    if(pCreateInfo->sType != VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateFramebuffer parameter, VkStructureType pCreateInfo->sType, is an invalid enumerator");
        return false;
    }
    if(pCreateInfo->pAttachments != nullptr)
    {
    }
    }

    return true;
}

bool PostCreateFramebuffer(
    VkDevice device,
    VkFramebuffer* pFramebuffer,
    VkResult result)
{

    if(pFramebuffer != nullptr)
    {
    }

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkCreateFramebuffer parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK", "%s", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateFramebuffer(
    VkDevice device,
    const VkFramebufferCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkFramebuffer* pFramebuffer)
{
    VkResult    result      = VK_ERROR_VALIDATION_FAILED_EXT;
    VkBool32    skipCall    = VK_FALSE;
    layer_data* my_data     = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skipCall |= param_check_vkCreateFramebuffer(
        my_data->report_data,
        pCreateInfo,
        pAllocator,
        pFramebuffer);

    if (skipCall == VK_FALSE) {
        PreCreateFramebuffer(device, pCreateInfo);

        result = get_dispatch_table(pc_device_table_map, device)->CreateFramebuffer(device, pCreateInfo, pAllocator, pFramebuffer);

        PostCreateFramebuffer(device, pFramebuffer, result);
    }

    return result;
}

bool PreCreateRenderPass(
    VkDevice device,
    const VkRenderPassCreateInfo* pCreateInfo)
{
    if(pCreateInfo != nullptr)
    {
    if(pCreateInfo->sType != VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateRenderPass parameter, VkStructureType pCreateInfo->sType, is an invalid enumerator");
        return false;
    }
    if(pCreateInfo->pAttachments != nullptr)
    {
    if(pCreateInfo->pAttachments->format < VK_FORMAT_BEGIN_RANGE ||
        pCreateInfo->pAttachments->format > VK_FORMAT_END_RANGE)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateRenderPass parameter, VkFormat pCreateInfo->pAttachments->format, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfo->pAttachments->loadOp < VK_ATTACHMENT_LOAD_OP_BEGIN_RANGE ||
        pCreateInfo->pAttachments->loadOp > VK_ATTACHMENT_LOAD_OP_END_RANGE)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateRenderPass parameter, VkAttachmentLoadOp pCreateInfo->pAttachments->loadOp, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfo->pAttachments->storeOp < VK_ATTACHMENT_STORE_OP_BEGIN_RANGE ||
        pCreateInfo->pAttachments->storeOp > VK_ATTACHMENT_STORE_OP_END_RANGE)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateRenderPass parameter, VkAttachmentStoreOp pCreateInfo->pAttachments->storeOp, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfo->pAttachments->stencilLoadOp < VK_ATTACHMENT_LOAD_OP_BEGIN_RANGE ||
        pCreateInfo->pAttachments->stencilLoadOp > VK_ATTACHMENT_LOAD_OP_END_RANGE)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateRenderPass parameter, VkAttachmentLoadOp pCreateInfo->pAttachments->stencilLoadOp, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfo->pAttachments->stencilStoreOp < VK_ATTACHMENT_STORE_OP_BEGIN_RANGE ||
        pCreateInfo->pAttachments->stencilStoreOp > VK_ATTACHMENT_STORE_OP_END_RANGE)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateRenderPass parameter, VkAttachmentStoreOp pCreateInfo->pAttachments->stencilStoreOp, is an unrecognized enumerator");
        return false;
    }
    if (((pCreateInfo->pAttachments->initialLayout < VK_IMAGE_LAYOUT_BEGIN_RANGE) ||
         (pCreateInfo->pAttachments->initialLayout > VK_IMAGE_LAYOUT_END_RANGE))  &&
        (pCreateInfo->pAttachments->initialLayout != VK_IMAGE_LAYOUT_PRESENT_SRC_KHR))
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateRenderPass parameter, VkImageLayout pCreateInfo->pAttachments->initialLayout, is an unrecognized enumerator");
        return false;
    }
    if (((pCreateInfo->pAttachments->initialLayout < VK_IMAGE_LAYOUT_BEGIN_RANGE) ||
         (pCreateInfo->pAttachments->initialLayout > VK_IMAGE_LAYOUT_END_RANGE))  &&
        (pCreateInfo->pAttachments->initialLayout != VK_IMAGE_LAYOUT_PRESENT_SRC_KHR))
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateRenderPass parameter, VkImageLayout pCreateInfo->pAttachments->finalLayout, is an unrecognized enumerator");
        return false;
    }
    }
    if(pCreateInfo->pSubpasses != nullptr)
    {
    if(pCreateInfo->pSubpasses->pipelineBindPoint < VK_PIPELINE_BIND_POINT_BEGIN_RANGE ||
        pCreateInfo->pSubpasses->pipelineBindPoint > VK_PIPELINE_BIND_POINT_END_RANGE)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateRenderPass parameter, VkPipelineBindPoint pCreateInfo->pSubpasses->pipelineBindPoint, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfo->pSubpasses->pInputAttachments != nullptr)
    {
    if (((pCreateInfo->pSubpasses->pInputAttachments->layout < VK_IMAGE_LAYOUT_BEGIN_RANGE) ||
         (pCreateInfo->pSubpasses->pInputAttachments->layout > VK_IMAGE_LAYOUT_END_RANGE))  &&
        (pCreateInfo->pSubpasses->pInputAttachments->layout != VK_IMAGE_LAYOUT_PRESENT_SRC_KHR))
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateRenderPass parameter, VkImageLayout pCreateInfo->pSubpasses->pInputAttachments->layout, is an unrecognized enumerator");
        return false;
    }
    }
    if(pCreateInfo->pSubpasses->pColorAttachments != nullptr)
    {
    if (((pCreateInfo->pSubpasses->pColorAttachments->layout < VK_IMAGE_LAYOUT_BEGIN_RANGE) ||
         (pCreateInfo->pSubpasses->pColorAttachments->layout > VK_IMAGE_LAYOUT_END_RANGE))  &&
        (pCreateInfo->pSubpasses->pColorAttachments->layout != VK_IMAGE_LAYOUT_PRESENT_SRC_KHR))
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateRenderPass parameter, VkImageLayout pCreateInfo->pSubpasses->pColorAttachments->layout, is an unrecognized enumerator");
        return false;
    }
    }
    if(pCreateInfo->pSubpasses->pResolveAttachments != nullptr)
    {
    if (((pCreateInfo->pSubpasses->pResolveAttachments->layout < VK_IMAGE_LAYOUT_BEGIN_RANGE) ||
         (pCreateInfo->pSubpasses->pResolveAttachments->layout > VK_IMAGE_LAYOUT_END_RANGE))  &&
        (pCreateInfo->pSubpasses->pResolveAttachments->layout != VK_IMAGE_LAYOUT_PRESENT_SRC_KHR))
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateRenderPass parameter, VkImageLayout pCreateInfo->pSubpasses->pResolveAttachments->layout, is an unrecognized enumerator");
        return false;
    }
    }
    if(pCreateInfo->pSubpasses->pDepthStencilAttachment &&
        ((pCreateInfo->pSubpasses->pDepthStencilAttachment->layout < VK_IMAGE_LAYOUT_BEGIN_RANGE) ||
         (pCreateInfo->pSubpasses->pDepthStencilAttachment->layout > VK_IMAGE_LAYOUT_END_RANGE))  &&
        (pCreateInfo->pSubpasses->pDepthStencilAttachment->layout != VK_IMAGE_LAYOUT_PRESENT_SRC_KHR))
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateRenderPass parameter, VkImageLayout pCreateInfo->pSubpasses->pDepthStencilAttachment->layout, is an unrecognized enumerator");
        return false;
    }
    }
    if(pCreateInfo->pDependencies != nullptr)
    {
    }
    }

    return true;
}

bool PostCreateRenderPass(
    VkDevice device,
    VkRenderPass* pRenderPass,
    VkResult result)
{

    if(pRenderPass != nullptr)
    {
    }

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkCreateRenderPass parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK", "%s", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateRenderPass(
    VkDevice device,
    const VkRenderPassCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkRenderPass* pRenderPass)
{
    VkResult    result      = VK_ERROR_VALIDATION_FAILED_EXT;
    VkBool32    skipCall    = VK_FALSE;
    layer_data* my_data     = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skipCall |= param_check_vkCreateRenderPass(
        my_data->report_data,
        pCreateInfo,
        pAllocator,
        pRenderPass);

    if (skipCall == VK_FALSE) {
        PreCreateRenderPass(device, pCreateInfo);

        result = get_dispatch_table(pc_device_table_map, device)->CreateRenderPass(device, pCreateInfo, pAllocator, pRenderPass);

        PostCreateRenderPass(device, pRenderPass, result);
    }

    return result;
}

bool PostGetRenderAreaGranularity(
    VkDevice device,
    VkRenderPass renderPass,
    VkExtent2D* pGranularity)
{


    if(pGranularity != nullptr)
    {
    }

    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetRenderAreaGranularity(
    VkDevice device,
    VkRenderPass renderPass,
    VkExtent2D* pGranularity)
{
    VkBool32    skipCall    = VK_FALSE;
    layer_data* my_data     = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skipCall |= param_check_vkGetRenderAreaGranularity(
        my_data->report_data,
        renderPass,
        pGranularity);

    if (skipCall == VK_FALSE) {
        get_dispatch_table(pc_device_table_map, device)->GetRenderAreaGranularity(device, renderPass, pGranularity);

        PostGetRenderAreaGranularity(device, renderPass, pGranularity);
    }
}

bool PreCreateCommandPool(
    VkDevice device,
    const VkCommandPoolCreateInfo* pCreateInfo)
{
    if(pCreateInfo != nullptr)
    {
    if(pCreateInfo->sType != VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCreateCommandPool parameter, VkStructureType pCreateInfo->sType, is an invalid enumerator");
        return false;
    }
    }

    return true;
}

bool PostCreateCommandPool(
    VkDevice device,
    VkCommandPool* pCommandPool,
    VkResult result)
{

    if(pCommandPool != nullptr)
    {
    }

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkCreateCommandPool parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK", "%s", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateCommandPool(
    VkDevice device,
    const VkCommandPoolCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkCommandPool* pCommandPool)
{
    VkResult    result      = VK_ERROR_VALIDATION_FAILED_EXT;
    VkBool32    skipCall    = VK_FALSE;
    layer_data* my_data     = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skipCall |= param_check_vkCreateCommandPool(
        my_data->report_data,
        pCreateInfo,
        pAllocator,
        pCommandPool);

    if (skipCall == VK_FALSE) {
        PreCreateCommandPool(device, pCreateInfo);

        result = get_dispatch_table(pc_device_table_map, device)->CreateCommandPool(device, pCreateInfo, pAllocator, pCommandPool);

        PostCreateCommandPool(device, pCommandPool, result);
    }

    return result;
}

bool PostResetCommandPool(
    VkDevice device,
    VkCommandPool commandPool,
    VkCommandPoolResetFlags flags,
    VkResult result)
{



    if(result < VK_SUCCESS)
    {
        std::string reason = "vkResetCommandPool parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK", "%s", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkResetCommandPool(
    VkDevice device,
    VkCommandPool commandPool,
    VkCommandPoolResetFlags flags)
{
    VkResult result = get_dispatch_table(pc_device_table_map, device)->ResetCommandPool(device, commandPool, flags);

    PostResetCommandPool(device, commandPool, flags, result);

    return result;
}

bool PreCreateCommandBuffer(
    VkDevice device,
    const VkCommandBufferAllocateInfo* pCreateInfo)
{
    if(pCreateInfo != nullptr)
    {
    if(pCreateInfo->sType != VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkAllocateCommandBuffers parameter, VkStructureType pCreateInfo->sType, is an invalid enumerator");
        return false;
    }
    if(pCreateInfo->level < VK_COMMAND_BUFFER_LEVEL_BEGIN_RANGE ||
        pCreateInfo->level > VK_COMMAND_BUFFER_LEVEL_END_RANGE)
    {
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkAllocateCommandBuffers parameter, VkCommandBufferLevel pCreateInfo->level, is an unrecognized enumerator");
        return false;
    }
    }

    return true;
}

bool PostCreateCommandBuffer(
    VkDevice device,
    VkCommandBuffer* pCommandBuffer,
    VkResult result)
{

    if(pCommandBuffer != nullptr)
    {
    }

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkAllocateCommandBuffers parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK", "%s", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkAllocateCommandBuffers(
    VkDevice device,
    const VkCommandBufferAllocateInfo* pAllocateInfo,
    VkCommandBuffer* pCommandBuffers)
{
    VkResult    result      = VK_ERROR_VALIDATION_FAILED_EXT;
    VkBool32    skipCall    = VK_FALSE;
    layer_data* my_data     = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    assert(my_data != NULL);

    skipCall |= param_check_vkAllocateCommandBuffers(
        my_data->report_data,
        pAllocateInfo,
        pCommandBuffers);

    if (skipCall == VK_FALSE) {
        PreCreateCommandBuffer(device, pAllocateInfo);

        result = get_dispatch_table(pc_device_table_map, device)->AllocateCommandBuffers(device, pAllocateInfo, pCommandBuffers);

        PostCreateCommandBuffer(device, pCommandBuffers, result);
    }

    return result;
}

bool PreBeginCommandBuffer(
    VkCommandBuffer commandBuffer,
    const VkCommandBufferBeginInfo* pBeginInfo)
{
    if(pBeginInfo != nullptr)
    {
    if(pBeginInfo->sType != VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO)
    {
        log_msg(mdd(commandBuffer), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkBeginCommandBuffer parameter, VkStructureType pBeginInfo->sType, is an invalid enumerator");
        return false;
    }
    }

    return true;
}

bool PostBeginCommandBuffer(
    VkCommandBuffer commandBuffer,
    VkResult result)
{

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkBeginCommandBuffer parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(commandBuffer), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK", "%s", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkBeginCommandBuffer(
    VkCommandBuffer commandBuffer,
    const VkCommandBufferBeginInfo* pBeginInfo)
{
    VkResult    result      = VK_ERROR_VALIDATION_FAILED_EXT;
    VkBool32    skipCall    = VK_FALSE;
    layer_data* my_data     = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    assert(my_data != NULL);

    skipCall |= param_check_vkBeginCommandBuffer(
        my_data->report_data,
        pBeginInfo);

    if (skipCall == VK_FALSE) {
        PreBeginCommandBuffer(commandBuffer, pBeginInfo);

        result = get_dispatch_table(pc_device_table_map, commandBuffer)->BeginCommandBuffer(commandBuffer, pBeginInfo);

        PostBeginCommandBuffer(commandBuffer, result);
    }

    return result;
}

bool PostEndCommandBuffer(
    VkCommandBuffer commandBuffer,
    VkResult result)
{

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkEndCommandBuffer parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(commandBuffer), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK", "%s", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEndCommandBuffer(
    VkCommandBuffer commandBuffer)
{
    VkResult result = get_dispatch_table(pc_device_table_map, commandBuffer)->EndCommandBuffer(commandBuffer);

    PostEndCommandBuffer(commandBuffer, result);

    return result;
}

bool PostResetCommandBuffer(
    VkCommandBuffer commandBuffer,
    VkCommandBufferResetFlags flags,
    VkResult result)
{


    if(result < VK_SUCCESS)
    {
        std::string reason = "vkResetCommandBuffer parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(commandBuffer), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK", "%s", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkResetCommandBuffer(
    VkCommandBuffer commandBuffer,
    VkCommandBufferResetFlags flags)
{
    VkResult result = get_dispatch_table(pc_device_table_map, commandBuffer)->ResetCommandBuffer(commandBuffer, flags);

    PostResetCommandBuffer(commandBuffer, flags, result);

    return result;
}

bool PostCmdBindPipeline(
    VkCommandBuffer commandBuffer,
    VkPipelineBindPoint pipelineBindPoint,
    VkPipeline pipeline)
{

    if(pipelineBindPoint < VK_PIPELINE_BIND_POINT_BEGIN_RANGE ||
        pipelineBindPoint > VK_PIPELINE_BIND_POINT_END_RANGE)
    {
        log_msg(mdd(commandBuffer), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCmdBindPipeline parameter, VkPipelineBindPoint pipelineBindPoint, is an unrecognized enumerator");
        return false;
    }


    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdBindPipeline(
    VkCommandBuffer commandBuffer,
    VkPipelineBindPoint pipelineBindPoint,
    VkPipeline pipeline)
{
    get_dispatch_table(pc_device_table_map, commandBuffer)->CmdBindPipeline(commandBuffer, pipelineBindPoint, pipeline);

    PostCmdBindPipeline(commandBuffer, pipelineBindPoint, pipeline);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetViewport(VkCommandBuffer commandBuffer, uint32_t firstViewport, uint32_t viewportCount, const VkViewport* pViewports)
{
    VkBool32    skipCall    = VK_FALSE;
    layer_data* my_data     = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    assert(my_data != NULL);

    skipCall |= param_check_vkCmdSetViewport(
        my_data->report_data,
        firstViewport,
        viewportCount,
        pViewports);

    if (skipCall == VK_FALSE) {
        get_dispatch_table(pc_device_table_map, commandBuffer)->CmdSetViewport(commandBuffer, firstViewport, viewportCount, pViewports);
    }
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetScissor(VkCommandBuffer commandBuffer, uint32_t firstScissor, uint32_t scissorCount, const VkRect2D* pScissors)
{
    VkBool32    skipCall    = VK_FALSE;
    layer_data* my_data     = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    assert(my_data != NULL);

    skipCall |= param_check_vkCmdSetScissor(
        my_data->report_data,
        firstScissor,
        scissorCount,
        pScissors);

    if (skipCall == VK_FALSE) {
        get_dispatch_table(pc_device_table_map, commandBuffer)->CmdSetScissor(commandBuffer, firstScissor, scissorCount, pScissors);
    }
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetLineWidth(VkCommandBuffer commandBuffer, float lineWidth)
{
    get_dispatch_table(pc_device_table_map, commandBuffer)->CmdSetLineWidth(commandBuffer, lineWidth);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetDepthBias(VkCommandBuffer commandBuffer, float depthBiasConstantFactor, float depthBiasClamp, float depthBiasSlopeFactor)
{
    get_dispatch_table(pc_device_table_map, commandBuffer)->CmdSetDepthBias(commandBuffer, depthBiasConstantFactor, depthBiasClamp, depthBiasSlopeFactor);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetBlendConstants(VkCommandBuffer commandBuffer, const float blendConstants[4])
{
    VkBool32    skipCall    = VK_FALSE;
    layer_data* my_data     = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    assert(my_data != NULL);

    skipCall |= param_check_vkCmdSetBlendConstants(
        my_data->report_data,
        blendConstants);

    if (skipCall == VK_FALSE) {
        get_dispatch_table(pc_device_table_map, commandBuffer)->CmdSetBlendConstants(commandBuffer, blendConstants);
    }
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetDepthBounds(VkCommandBuffer commandBuffer, float minDepthBounds, float maxDepthBounds)
{
    get_dispatch_table(pc_device_table_map, commandBuffer)->CmdSetDepthBounds(commandBuffer, minDepthBounds, maxDepthBounds);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetStencilCompareMask(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t compareMask)
{
    get_dispatch_table(pc_device_table_map, commandBuffer)->CmdSetStencilCompareMask(commandBuffer, faceMask, compareMask);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetStencilWriteMask(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t writeMask)
{
    get_dispatch_table(pc_device_table_map, commandBuffer)->CmdSetStencilWriteMask(commandBuffer, faceMask, writeMask);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetStencilReference(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t reference)
{
    get_dispatch_table(pc_device_table_map, commandBuffer)->CmdSetStencilReference(commandBuffer, faceMask, reference);
}

bool PreCmdBindDescriptorSets(
    VkCommandBuffer commandBuffer,
    const VkDescriptorSet* pDescriptorSets,
    const uint32_t* pDynamicOffsets)
{
    if(pDescriptorSets != nullptr)
    {
    }

    if(pDynamicOffsets != nullptr)
    {
    }

    return true;
}

bool PostCmdBindDescriptorSets(
    VkCommandBuffer commandBuffer,
    VkPipelineBindPoint pipelineBindPoint,
    VkPipelineLayout layout,
    uint32_t firstSet,
    uint32_t setCount,
    uint32_t dynamicOffsetCount)
{

    if(pipelineBindPoint < VK_PIPELINE_BIND_POINT_BEGIN_RANGE ||
        pipelineBindPoint > VK_PIPELINE_BIND_POINT_END_RANGE)
    {
        log_msg(mdd(commandBuffer), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCmdBindDescriptorSets parameter, VkPipelineBindPoint pipelineBindPoint, is an unrecognized enumerator");
        return false;
    }





    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdBindDescriptorSets(
    VkCommandBuffer commandBuffer,
    VkPipelineBindPoint pipelineBindPoint,
    VkPipelineLayout layout,
    uint32_t firstSet,
    uint32_t descriptorSetCount,
    const VkDescriptorSet* pDescriptorSets,
    uint32_t dynamicOffsetCount,
    const uint32_t* pDynamicOffsets)
{
    VkBool32    skipCall    = VK_FALSE;
    layer_data* my_data     = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    assert(my_data != NULL);

    skipCall |= param_check_vkCmdBindDescriptorSets(
        my_data->report_data,
        pipelineBindPoint,
        layout,
        firstSet,
        descriptorSetCount,
        pDescriptorSets,
        dynamicOffsetCount,
        pDynamicOffsets);

    if (skipCall == VK_FALSE) {
        PreCmdBindDescriptorSets(commandBuffer, pDescriptorSets, pDynamicOffsets);

        get_dispatch_table(pc_device_table_map, commandBuffer)->CmdBindDescriptorSets(commandBuffer, pipelineBindPoint, layout, firstSet, descriptorSetCount, pDescriptorSets, dynamicOffsetCount, pDynamicOffsets);

        PostCmdBindDescriptorSets(commandBuffer, pipelineBindPoint, layout, firstSet, descriptorSetCount, dynamicOffsetCount);
    }
}

bool PostCmdBindIndexBuffer(
    VkCommandBuffer commandBuffer,
    VkBuffer buffer,
    VkDeviceSize offset,
    VkIndexType indexType)
{



    if(indexType < VK_INDEX_TYPE_BEGIN_RANGE ||
        indexType > VK_INDEX_TYPE_END_RANGE)
    {
        log_msg(mdd(commandBuffer), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCmdBindIndexBuffer parameter, VkIndexType indexType, is an unrecognized enumerator");
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdBindIndexBuffer(
    VkCommandBuffer commandBuffer,
    VkBuffer buffer,
    VkDeviceSize offset,
    VkIndexType indexType)
{
    get_dispatch_table(pc_device_table_map, commandBuffer)->CmdBindIndexBuffer(commandBuffer, buffer, offset, indexType);

    PostCmdBindIndexBuffer(commandBuffer, buffer, offset, indexType);
}

bool PreCmdBindVertexBuffers(
    VkCommandBuffer commandBuffer,
    const VkBuffer* pBuffers,
    const VkDeviceSize* pOffsets)
{
    if(pBuffers != nullptr)
    {
    }

    if(pOffsets != nullptr)
    {
    }

    return true;
}

bool PostCmdBindVertexBuffers(
    VkCommandBuffer commandBuffer,
    uint32_t firstBinding,
    uint32_t bindingCount)
{



    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdBindVertexBuffers(
    VkCommandBuffer commandBuffer,
    uint32_t firstBinding,
    uint32_t bindingCount,
    const VkBuffer* pBuffers,
    const VkDeviceSize* pOffsets)
{
    VkBool32    skipCall    = VK_FALSE;
    layer_data* my_data     = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    assert(my_data != NULL);

    skipCall |= param_check_vkCmdBindVertexBuffers(
        my_data->report_data,
        firstBinding,
        bindingCount,
        pBuffers,
        pOffsets);

    if (skipCall == VK_FALSE) {
        PreCmdBindVertexBuffers(commandBuffer, pBuffers, pOffsets);

        get_dispatch_table(pc_device_table_map, commandBuffer)->CmdBindVertexBuffers(commandBuffer, firstBinding, bindingCount, pBuffers, pOffsets);

        PostCmdBindVertexBuffers(commandBuffer, firstBinding, bindingCount);
    }
}

bool PreCmdDraw(
    VkCommandBuffer commandBuffer,
    uint32_t vertexCount,
    uint32_t instanceCount,
    uint32_t firstVertex,
    uint32_t firstInstance)
{
    if (vertexCount == 0) {
        // TODO: Verify against Valid Usage section. I don't see a non-zero vertexCount listed, may need to add that and make
        // this an error or leave as is.
        log_msg(mdd(commandBuffer), VK_DEBUG_REPORT_WARNING_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCmdDraw parameter, uint32_t vertexCount, is 0");
        return false;
    }

    if (instanceCount == 0) {
        // TODO: Verify against Valid Usage section. I don't see a non-zero instanceCount listed, may need to add that and make
        // this an error or leave as is.
        log_msg(mdd(commandBuffer), VK_DEBUG_REPORT_WARNING_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCmdDraw parameter, uint32_t instanceCount, is 0");
        return false;
    }

    return true;
}

bool PostCmdDraw(
    VkCommandBuffer commandBuffer,
    uint32_t firstVertex,
    uint32_t vertexCount,
    uint32_t firstInstance,
    uint32_t instanceCount)
{





    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdDraw(
    VkCommandBuffer commandBuffer,
    uint32_t vertexCount,
    uint32_t instanceCount,
    uint32_t firstVertex,
    uint32_t firstInstance)
{
    PreCmdDraw(commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);

    get_dispatch_table(pc_device_table_map, commandBuffer)->CmdDraw(commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);

    PostCmdDraw(commandBuffer, firstVertex, vertexCount, firstInstance, instanceCount);
}

bool PostCmdDrawIndexed(
    VkCommandBuffer commandBuffer,
    uint32_t firstIndex,
    uint32_t indexCount,
    int32_t vertexOffset,
    uint32_t firstInstance,
    uint32_t instanceCount)
{






    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdDrawIndexed(
    VkCommandBuffer commandBuffer,
    uint32_t indexCount,
    uint32_t instanceCount,
    uint32_t firstIndex,
    int32_t vertexOffset,
    uint32_t firstInstance)
{
    get_dispatch_table(pc_device_table_map, commandBuffer)->CmdDrawIndexed(commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);

    PostCmdDrawIndexed(commandBuffer, firstIndex, indexCount, vertexOffset, firstInstance, instanceCount);
}

bool PostCmdDrawIndirect(
    VkCommandBuffer commandBuffer,
    VkBuffer buffer,
    VkDeviceSize offset,
    uint32_t count,
    uint32_t stride)
{





    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdDrawIndirect(
    VkCommandBuffer commandBuffer,
    VkBuffer buffer,
    VkDeviceSize offset,
    uint32_t count,
    uint32_t stride)
{
    get_dispatch_table(pc_device_table_map, commandBuffer)->CmdDrawIndirect(commandBuffer, buffer, offset, count, stride);

    PostCmdDrawIndirect(commandBuffer, buffer, offset, count, stride);
}

bool PostCmdDrawIndexedIndirect(
    VkCommandBuffer commandBuffer,
    VkBuffer buffer,
    VkDeviceSize offset,
    uint32_t count,
    uint32_t stride)
{





    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdDrawIndexedIndirect(
    VkCommandBuffer commandBuffer,
    VkBuffer buffer,
    VkDeviceSize offset,
    uint32_t count,
    uint32_t stride)
{
    get_dispatch_table(pc_device_table_map, commandBuffer)->CmdDrawIndexedIndirect(commandBuffer, buffer, offset, count, stride);

    PostCmdDrawIndexedIndirect(commandBuffer, buffer, offset, count, stride);
}

bool PostCmdDispatch(
    VkCommandBuffer commandBuffer,
    uint32_t x,
    uint32_t y,
    uint32_t z)
{




    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdDispatch(
    VkCommandBuffer commandBuffer,
    uint32_t x,
    uint32_t y,
    uint32_t z)
{
    get_dispatch_table(pc_device_table_map, commandBuffer)->CmdDispatch(commandBuffer, x, y, z);

    PostCmdDispatch(commandBuffer, x, y, z);
}

bool PostCmdDispatchIndirect(
    VkCommandBuffer commandBuffer,
    VkBuffer buffer,
    VkDeviceSize offset)
{



    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdDispatchIndirect(
    VkCommandBuffer commandBuffer,
    VkBuffer buffer,
    VkDeviceSize offset)
{
    get_dispatch_table(pc_device_table_map, commandBuffer)->CmdDispatchIndirect(commandBuffer, buffer, offset);

    PostCmdDispatchIndirect(commandBuffer, buffer, offset);
}

bool PreCmdCopyBuffer(
    VkCommandBuffer commandBuffer,
    const VkBufferCopy* pRegions)
{
    if(pRegions != nullptr)
    {
    }

    return true;
}

bool PostCmdCopyBuffer(
    VkCommandBuffer commandBuffer,
    VkBuffer srcBuffer,
    VkBuffer dstBuffer,
    uint32_t regionCount)
{




    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdCopyBuffer(
    VkCommandBuffer commandBuffer,
    VkBuffer srcBuffer,
    VkBuffer dstBuffer,
    uint32_t regionCount,
    const VkBufferCopy* pRegions)
{
    VkBool32    skipCall    = VK_FALSE;
    layer_data* my_data     = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    assert(my_data != NULL);

    skipCall |= param_check_vkCmdCopyBuffer(
        my_data->report_data,
        srcBuffer,
        dstBuffer,
        regionCount,
        pRegions);

    if (skipCall == VK_FALSE) {
        PreCmdCopyBuffer(commandBuffer, pRegions);

        get_dispatch_table(pc_device_table_map, commandBuffer)->CmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, regionCount, pRegions);

        PostCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, regionCount);
    }
}

bool PreCmdCopyImage(
    VkCommandBuffer commandBuffer,
    const VkImageCopy* pRegions)
{
    if(pRegions != nullptr)
    {
    if ((pRegions->srcSubresource.aspectMask &
       (VK_IMAGE_ASPECT_COLOR_BIT | VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT | VK_IMAGE_ASPECT_METADATA_BIT)) == 0)
    {
        log_msg(mdd(commandBuffer), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCmdCopyImage parameter, VkImageAspect pRegions->srcSubresource.aspectMask, is an unrecognized enumerator");
        return false;
    }
    if ((pRegions->dstSubresource.aspectMask &
       (VK_IMAGE_ASPECT_COLOR_BIT | VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT | VK_IMAGE_ASPECT_METADATA_BIT)) == 0)
    {
        log_msg(mdd(commandBuffer), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCmdCopyImage parameter, VkImageAspect pRegions->dstSubresource.aspectMask, is an unrecognized enumerator");
        return false;
    }
    }

    return true;
}

bool PostCmdCopyImage(
    VkCommandBuffer commandBuffer,
    VkImage srcImage,
    VkImageLayout srcImageLayout,
    VkImage dstImage,
    VkImageLayout dstImageLayout,
    uint32_t regionCount)
{
    if (((srcImageLayout < VK_IMAGE_LAYOUT_BEGIN_RANGE) ||
         (srcImageLayout > VK_IMAGE_LAYOUT_END_RANGE))  &&
        (srcImageLayout != VK_IMAGE_LAYOUT_PRESENT_SRC_KHR))
    {
        log_msg(mdd(commandBuffer), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCmdCopyImage parameter, VkImageLayout srcImageLayout, is an unrecognized enumerator");
        return false;
    }


    if (((dstImageLayout < VK_IMAGE_LAYOUT_BEGIN_RANGE) ||
         (dstImageLayout > VK_IMAGE_LAYOUT_END_RANGE))  &&
        (dstImageLayout != VK_IMAGE_LAYOUT_PRESENT_SRC_KHR))
    {
        log_msg(mdd(commandBuffer), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCmdCopyImage parameter, VkImageLayout dstImageLayout, is an unrecognized enumerator");
        return false;
    }


    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdCopyImage(
    VkCommandBuffer commandBuffer,
    VkImage srcImage,
    VkImageLayout srcImageLayout,
    VkImage dstImage,
    VkImageLayout dstImageLayout,
    uint32_t regionCount,
    const VkImageCopy* pRegions)
{
    VkBool32    skipCall    = VK_FALSE;
    layer_data* my_data     = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    assert(my_data != NULL);

    skipCall |= param_check_vkCmdCopyImage(
        my_data->report_data,
        srcImage,
        srcImageLayout,
        dstImage,
        dstImageLayout,
        regionCount,
        pRegions);

    if (skipCall == VK_FALSE) {
        PreCmdCopyImage(commandBuffer, pRegions);

        get_dispatch_table(pc_device_table_map, commandBuffer)->CmdCopyImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions);

        PostCmdCopyImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount);
    }
}

bool PreCmdBlitImage(
    VkCommandBuffer commandBuffer,
    const VkImageBlit* pRegions)
{
    if(pRegions != nullptr)
    {
    if ((pRegions->srcSubresource.aspectMask &
       (VK_IMAGE_ASPECT_COLOR_BIT | VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT | VK_IMAGE_ASPECT_METADATA_BIT)) == 0)
    {
        log_msg(mdd(commandBuffer), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCmdCopyImage parameter, VkImageAspect pRegions->srcSubresource.aspectMask, is an unrecognized enumerator");
        return false;
    }
    if ((pRegions->dstSubresource.aspectMask &
       (VK_IMAGE_ASPECT_COLOR_BIT | VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT | VK_IMAGE_ASPECT_METADATA_BIT)) == 0)
    {
        log_msg(mdd(commandBuffer), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCmdCopyImage parameter, VkImageAspect pRegions->dstSubresource.aspectMask, is an unrecognized enumerator");
        return false;
    }
    }

    return true;
}

bool PostCmdBlitImage(
    VkCommandBuffer commandBuffer,
    VkImage srcImage,
    VkImageLayout srcImageLayout,
    VkImage dstImage,
    VkImageLayout dstImageLayout,
    uint32_t regionCount,
    VkFilter filter)
{


    if (((srcImageLayout < VK_IMAGE_LAYOUT_BEGIN_RANGE) ||
         (srcImageLayout > VK_IMAGE_LAYOUT_END_RANGE))  &&
        (srcImageLayout != VK_IMAGE_LAYOUT_PRESENT_SRC_KHR))
    {
        log_msg(mdd(commandBuffer), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCmdBlitImage parameter, VkImageLayout srcImageLayout, is an unrecognized enumerator");
        return false;
    }


    if (((dstImageLayout < VK_IMAGE_LAYOUT_BEGIN_RANGE) ||
         (dstImageLayout > VK_IMAGE_LAYOUT_END_RANGE))  &&
        (dstImageLayout != VK_IMAGE_LAYOUT_PRESENT_SRC_KHR))
    {
        log_msg(mdd(commandBuffer), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCmdBlitImage parameter, VkImageLayout dstImageLayout, is an unrecognized enumerator");
        return false;
    }


    if(filter < VK_FILTER_BEGIN_RANGE ||
        filter > VK_FILTER_END_RANGE)
    {
        log_msg(mdd(commandBuffer), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCmdBlitImage parameter, VkFilter filter, is an unrecognized enumerator");
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdBlitImage(
    VkCommandBuffer commandBuffer,
    VkImage srcImage,
    VkImageLayout srcImageLayout,
    VkImage dstImage,
    VkImageLayout dstImageLayout,
    uint32_t regionCount,
    const VkImageBlit* pRegions,
    VkFilter filter)
{
    VkBool32    skipCall    = VK_FALSE;
    layer_data* my_data     = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    assert(my_data != NULL);

    skipCall |= param_check_vkCmdBlitImage(
        my_data->report_data,
        srcImage,
        srcImageLayout,
        dstImage,
        dstImageLayout,
        regionCount,
        pRegions,
        filter);

    if (skipCall == VK_FALSE) {
        PreCmdBlitImage(commandBuffer, pRegions);

        get_dispatch_table(pc_device_table_map, commandBuffer)->CmdBlitImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions, filter);

        PostCmdBlitImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, filter);
    }
}

bool PreCmdCopyBufferToImage(
    VkCommandBuffer commandBuffer,
    const VkBufferImageCopy* pRegions)
{
    if(pRegions != nullptr)
    {
    if ((pRegions->imageSubresource.aspectMask &
       (VK_IMAGE_ASPECT_COLOR_BIT | VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT | VK_IMAGE_ASPECT_METADATA_BIT)) == 0)
    {
        log_msg(mdd(commandBuffer), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCmdCopyBufferToImage parameter, VkImageAspect pRegions->imageSubresource.aspectMask, is an unrecognized enumerator");
        return false;
    }
    }

    return true;
}

bool PostCmdCopyBufferToImage(
    VkCommandBuffer commandBuffer,
    VkBuffer srcBuffer,
    VkImage dstImage,
    VkImageLayout dstImageLayout,
    uint32_t regionCount)
{



    if (((dstImageLayout < VK_IMAGE_LAYOUT_BEGIN_RANGE) ||
         (dstImageLayout > VK_IMAGE_LAYOUT_END_RANGE))  &&
        (dstImageLayout != VK_IMAGE_LAYOUT_PRESENT_SRC_KHR))
    {
        log_msg(mdd(commandBuffer), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCmdCopyBufferToImage parameter, VkImageLayout dstImageLayout, is an unrecognized enumerator");
        return false;
    }


    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdCopyBufferToImage(
    VkCommandBuffer commandBuffer,
    VkBuffer srcBuffer,
    VkImage dstImage,
    VkImageLayout dstImageLayout,
    uint32_t regionCount,
    const VkBufferImageCopy* pRegions)
{
    VkBool32    skipCall    = VK_FALSE;
    layer_data* my_data     = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    assert(my_data != NULL);

    skipCall |= param_check_vkCmdCopyBufferToImage(
        my_data->report_data,
        srcBuffer,
        dstImage,
        dstImageLayout,
        regionCount,
        pRegions);

    if (skipCall == VK_FALSE) {
        PreCmdCopyBufferToImage(commandBuffer, pRegions);

        get_dispatch_table(pc_device_table_map, commandBuffer)->CmdCopyBufferToImage(commandBuffer, srcBuffer, dstImage, dstImageLayout, regionCount, pRegions);

        PostCmdCopyBufferToImage(commandBuffer, srcBuffer, dstImage, dstImageLayout, regionCount);
    }
}

bool PreCmdCopyImageToBuffer(
    VkCommandBuffer commandBuffer,
    const VkBufferImageCopy* pRegions)
{
    if(pRegions != nullptr)
    {
    if ((pRegions->imageSubresource.aspectMask &
       (VK_IMAGE_ASPECT_COLOR_BIT | VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT | VK_IMAGE_ASPECT_METADATA_BIT)) == 0)
    {
        log_msg(mdd(commandBuffer), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCmdCopyImageToBuffer parameter, VkImageAspect pRegions->imageSubresource.aspectMask, is an unrecognized enumerator");
        return false;
    }
    }

    return true;
}

bool PostCmdCopyImageToBuffer(
    VkCommandBuffer commandBuffer,
    VkImage srcImage,
    VkImageLayout srcImageLayout,
    VkBuffer dstBuffer,
    uint32_t regionCount)
{


    if (((srcImageLayout < VK_IMAGE_LAYOUT_BEGIN_RANGE) ||
         (srcImageLayout > VK_IMAGE_LAYOUT_END_RANGE))  &&
        (srcImageLayout != VK_IMAGE_LAYOUT_PRESENT_SRC_KHR))
    {
        log_msg(mdd(commandBuffer), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCmdCopyImageToBuffer parameter, VkImageLayout srcImageLayout, is an unrecognized enumerator");
        return false;
    }



    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdCopyImageToBuffer(
    VkCommandBuffer commandBuffer,
    VkImage srcImage,
    VkImageLayout srcImageLayout,
    VkBuffer dstBuffer,
    uint32_t regionCount,
    const VkBufferImageCopy* pRegions)
{
    VkBool32    skipCall    = VK_FALSE;
    layer_data* my_data     = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    assert(my_data != NULL);

    skipCall |= param_check_vkCmdCopyImageToBuffer(
        my_data->report_data,
        srcImage,
        srcImageLayout,
        dstBuffer,
        regionCount,
        pRegions);

    if (skipCall == VK_FALSE) {
        PreCmdCopyImageToBuffer(commandBuffer, pRegions);

        get_dispatch_table(pc_device_table_map, commandBuffer)->CmdCopyImageToBuffer(commandBuffer, srcImage, srcImageLayout, dstBuffer, regionCount, pRegions);

        PostCmdCopyImageToBuffer(commandBuffer, srcImage, srcImageLayout, dstBuffer, regionCount);
    }
}

bool PreCmdUpdateBuffer(
    VkCommandBuffer commandBuffer,
    const uint32_t* pData)
{
    if(pData != nullptr)
    {
    }

    return true;
}

bool PostCmdUpdateBuffer(
    VkCommandBuffer commandBuffer,
    VkBuffer dstBuffer,
    VkDeviceSize dstOffset,
    VkDeviceSize dataSize)
{




    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdUpdateBuffer(
    VkCommandBuffer commandBuffer,
    VkBuffer dstBuffer,
    VkDeviceSize dstOffset,
    VkDeviceSize dataSize,
    const uint32_t* pData)
{
    VkBool32    skipCall    = VK_FALSE;
    layer_data* my_data     = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    assert(my_data != NULL);

    skipCall |= param_check_vkCmdUpdateBuffer(
        my_data->report_data,
        dstBuffer,
        dstOffset,
        dataSize,
        pData);

    if (skipCall == VK_FALSE) {
        PreCmdUpdateBuffer(commandBuffer, pData);

        get_dispatch_table(pc_device_table_map, commandBuffer)->CmdUpdateBuffer(commandBuffer, dstBuffer, dstOffset, dataSize, pData);

        PostCmdUpdateBuffer(commandBuffer, dstBuffer, dstOffset, dataSize);
    }
}

bool PostCmdFillBuffer(
    VkCommandBuffer commandBuffer,
    VkBuffer dstBuffer,
    VkDeviceSize dstOffset,
    VkDeviceSize size,
    uint32_t data)
{





    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdFillBuffer(
    VkCommandBuffer commandBuffer,
    VkBuffer dstBuffer,
    VkDeviceSize dstOffset,
    VkDeviceSize size,
    uint32_t data)
{
    get_dispatch_table(pc_device_table_map, commandBuffer)->CmdFillBuffer(commandBuffer, dstBuffer, dstOffset, size, data);

    PostCmdFillBuffer(commandBuffer, dstBuffer, dstOffset, size, data);
}

bool PreCmdClearColorImage(
    VkCommandBuffer commandBuffer,
    const VkClearColorValue* pColor,
    const VkImageSubresourceRange* pRanges)
{
    if(pColor != nullptr)
    {
    }

    if(pRanges != nullptr)
    {
        /* TODO: How should we validate pRanges->aspectMask */
    }

    return true;
}

bool PostCmdClearColorImage(
    VkCommandBuffer commandBuffer,
    VkImage image,
    VkImageLayout imageLayout,
    uint32_t rangeCount)
{


    if (((imageLayout < VK_IMAGE_LAYOUT_BEGIN_RANGE) ||
         (imageLayout > VK_IMAGE_LAYOUT_END_RANGE))  &&
        (imageLayout != VK_IMAGE_LAYOUT_PRESENT_SRC_KHR))
    {
        log_msg(mdd(commandBuffer), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCmdClearColorImage parameter, VkImageLayout imageLayout, is an unrecognized enumerator");
        return false;
    }


    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdClearColorImage(
    VkCommandBuffer commandBuffer,
    VkImage image,
    VkImageLayout imageLayout,
    const VkClearColorValue* pColor,
    uint32_t rangeCount,
    const VkImageSubresourceRange* pRanges)
{
    VkBool32    skipCall    = VK_FALSE;
    layer_data* my_data     = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    assert(my_data != NULL);

    skipCall |= param_check_vkCmdClearColorImage(
        my_data->report_data,
        image,
        imageLayout,
        pColor,
        rangeCount,
        pRanges);

    if (skipCall == VK_FALSE) {
        PreCmdClearColorImage(commandBuffer, pColor, pRanges);

        get_dispatch_table(pc_device_table_map, commandBuffer)->CmdClearColorImage(commandBuffer, image, imageLayout, pColor, rangeCount, pRanges);

        PostCmdClearColorImage(commandBuffer, image, imageLayout, rangeCount);
    }
}

bool PreCmdClearDepthStencilImage(
    VkCommandBuffer commandBuffer,
    const VkImageSubresourceRange* pRanges)
{
    if(pRanges != nullptr)
    {
        /*
         * TODO: How do we validation pRanges->aspectMask?
         * Allows values are: VK_IMAGE_ASPECT_DEPTH_BIT and
         * VK_IMAGE_ASPECT_STENCIL_BIT.
         */
    }

    return true;
}

bool PostCmdClearDepthStencilImage(
    VkCommandBuffer commandBuffer,
    VkImage image,
    VkImageLayout imageLayout,
    const VkClearDepthStencilValue* pDepthStencil,
    uint32_t rangeCount)
{


    if (((imageLayout < VK_IMAGE_LAYOUT_BEGIN_RANGE) ||
         (imageLayout > VK_IMAGE_LAYOUT_END_RANGE))  &&
        (imageLayout != VK_IMAGE_LAYOUT_PRESENT_SRC_KHR))
    {
        log_msg(mdd(commandBuffer), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCmdClearDepthStencilImage parameter, VkImageLayout imageLayout, is an unrecognized enumerator");
        return false;
    }




    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdClearDepthStencilImage(
    VkCommandBuffer commandBuffer,
    VkImage image,
    VkImageLayout imageLayout,
    const VkClearDepthStencilValue* pDepthStencil,
    uint32_t rangeCount,
    const VkImageSubresourceRange* pRanges)
{
    VkBool32    skipCall    = VK_FALSE;
    layer_data* my_data     = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    assert(my_data != NULL);

    skipCall |= param_check_vkCmdClearDepthStencilImage(
        my_data->report_data,
        image,
        imageLayout,
        pDepthStencil,
        rangeCount,
        pRanges);

    if (skipCall == VK_FALSE) {
        PreCmdClearDepthStencilImage(commandBuffer, pRanges);

        get_dispatch_table(pc_device_table_map, commandBuffer)->CmdClearDepthStencilImage(commandBuffer, image, imageLayout, pDepthStencil, rangeCount, pRanges);

        PostCmdClearDepthStencilImage(commandBuffer, image, imageLayout, pDepthStencil, rangeCount);
    }
}

bool PreCmdClearAttachments(
    VkCommandBuffer commandBuffer,
    const VkClearColorValue* pColor,
    const VkClearRect* pRects)
{
    if(pColor != nullptr)
    {
    }

    if(pRects != nullptr)
    {
    }

    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdClearAttachments(
    VkCommandBuffer                                 commandBuffer,
    uint32_t                                    attachmentCount,
    const VkClearAttachment*                    pAttachments,
    uint32_t                                    rectCount,
    const VkClearRect*                          pRects)
{
    VkBool32    skipCall    = VK_FALSE;
    layer_data* my_data     = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    assert(my_data != NULL);

    skipCall |= param_check_vkCmdClearAttachments(
        my_data->report_data,
        attachmentCount,
        pAttachments,
        rectCount,
        pRects);

    if (skipCall == VK_FALSE) {
        for (uint32_t i = 0; i < attachmentCount; i++) {
            PreCmdClearAttachments(commandBuffer, &pAttachments[i].clearValue.color, pRects);
        }

        get_dispatch_table(pc_device_table_map, commandBuffer)->CmdClearAttachments(commandBuffer, attachmentCount, pAttachments, rectCount, pRects);
    }
}

bool PreCmdResolveImage(
    VkCommandBuffer commandBuffer,
    const VkImageResolve* pRegions)
{
    if(pRegions != nullptr)
    {
    if ((pRegions->srcSubresource.aspectMask &
       (VK_IMAGE_ASPECT_COLOR_BIT | VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT | VK_IMAGE_ASPECT_METADATA_BIT)) == 0)
    {
        log_msg(mdd(commandBuffer), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCmdResolveImage parameter, VkImageAspect pRegions->srcSubresource.aspectMask, is an unrecognized enumerator");
        return false;
    }
    if ((pRegions->dstSubresource.aspectMask &
       (VK_IMAGE_ASPECT_COLOR_BIT | VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT | VK_IMAGE_ASPECT_METADATA_BIT)) == 0)
    {
        log_msg(mdd(commandBuffer), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCmdResolveImage parameter, VkImageAspect pRegions->dstSubresource.aspectMask, is an unrecognized enumerator");
        return false;
    }
    }

    return true;
}

bool PostCmdResolveImage(
    VkCommandBuffer commandBuffer,
    VkImage srcImage,
    VkImageLayout srcImageLayout,
    VkImage dstImage,
    VkImageLayout dstImageLayout,
    uint32_t regionCount)
{


    if (((srcImageLayout < VK_IMAGE_LAYOUT_BEGIN_RANGE) ||
         (srcImageLayout > VK_IMAGE_LAYOUT_END_RANGE))  &&
        (srcImageLayout != VK_IMAGE_LAYOUT_PRESENT_SRC_KHR))
    {
        log_msg(mdd(commandBuffer), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCmdResolveImage parameter, VkImageLayout srcImageLayout, is an unrecognized enumerator");
        return false;
    }


    if (((dstImageLayout < VK_IMAGE_LAYOUT_BEGIN_RANGE) ||
         (dstImageLayout > VK_IMAGE_LAYOUT_END_RANGE))  &&
        (dstImageLayout != VK_IMAGE_LAYOUT_PRESENT_SRC_KHR))
    {
        log_msg(mdd(commandBuffer), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCmdResolveImage parameter, VkImageLayout dstImageLayout, is an unrecognized enumerator");
        return false;
    }


    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdResolveImage(
    VkCommandBuffer commandBuffer,
    VkImage srcImage,
    VkImageLayout srcImageLayout,
    VkImage dstImage,
    VkImageLayout dstImageLayout,
    uint32_t regionCount,
    const VkImageResolve* pRegions)
{
    VkBool32    skipCall    = VK_FALSE;
    layer_data* my_data     = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    assert(my_data != NULL);

    skipCall |= param_check_vkCmdResolveImage(
        my_data->report_data,
        srcImage,
        srcImageLayout,
        dstImage,
        dstImageLayout,
        regionCount,
        pRegions);

    if (skipCall == VK_FALSE) {
        PreCmdResolveImage(commandBuffer, pRegions);

        get_dispatch_table(pc_device_table_map, commandBuffer)->CmdResolveImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions);

        PostCmdResolveImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount);
    }
}

bool PostCmdSetEvent(
    VkCommandBuffer commandBuffer,
    VkEvent event,
    VkPipelineStageFlags stageMask)
{



    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetEvent(
    VkCommandBuffer commandBuffer,
    VkEvent event,
    VkPipelineStageFlags stageMask)
{
    get_dispatch_table(pc_device_table_map, commandBuffer)->CmdSetEvent(commandBuffer, event, stageMask);

    PostCmdSetEvent(commandBuffer, event, stageMask);
}

bool PostCmdResetEvent(
    VkCommandBuffer commandBuffer,
    VkEvent event,
    VkPipelineStageFlags stageMask)
{



    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdResetEvent(
    VkCommandBuffer commandBuffer,
    VkEvent event,
    VkPipelineStageFlags stageMask)
{
    get_dispatch_table(pc_device_table_map, commandBuffer)->CmdResetEvent(commandBuffer, event, stageMask);

    PostCmdResetEvent(commandBuffer, event, stageMask);
}

bool PreCmdWaitEvents(
    VkCommandBuffer commandBuffer,
    const VkEvent* pEvents,
    uint32_t                     memoryBarrierCount,
    const VkMemoryBarrier       *pMemoryBarriers,
    uint32_t                     bufferMemoryBarrierCount,
    const VkBufferMemoryBarrier *pBufferMemoryBarriers,
    uint32_t                     imageMemoryBarrierCount,
    const VkImageMemoryBarrier  *pImageMemoryBarriers)
{
    if(pEvents != nullptr)
    {
    }

    if(pMemoryBarriers != nullptr)
    {
    }

    return true;
}

bool PostCmdWaitEvents(
    VkCommandBuffer commandBuffer,
    uint32_t eventCount,
    VkPipelineStageFlags srcStageMask,
    VkPipelineStageFlags dstStageMask,
    uint32_t memoryBarrierCount)
{





    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdWaitEvents(
    VkCommandBuffer              commandBuffer,
    uint32_t                     eventCount,
    const VkEvent               *pEvents,
    VkPipelineStageFlags         srcStageMask,
    VkPipelineStageFlags         dstStageMask,
    uint32_t                     memoryBarrierCount,
    const VkMemoryBarrier       *pMemoryBarriers,
    uint32_t                     bufferMemoryBarrierCount,
    const VkBufferMemoryBarrier *pBufferMemoryBarriers,
    uint32_t                     imageMemoryBarrierCount,
    const VkImageMemoryBarrier  *pImageMemoryBarriers)
{
    VkBool32    skipCall    = VK_FALSE;
    layer_data* my_data     = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    assert(my_data != NULL);

    skipCall |= param_check_vkCmdWaitEvents(
        my_data->report_data,
        eventCount,
        pEvents,
        srcStageMask,
        dstStageMask,
        memoryBarrierCount,
        pMemoryBarriers,
        bufferMemoryBarrierCount,
        pBufferMemoryBarriers,
        imageMemoryBarrierCount,
        pImageMemoryBarriers);

    if (skipCall == VK_FALSE) {
        PreCmdWaitEvents(commandBuffer, pEvents, memoryBarrierCount, pMemoryBarriers, bufferMemoryBarrierCount, pBufferMemoryBarriers, imageMemoryBarrierCount, pImageMemoryBarriers);

        get_dispatch_table(pc_device_table_map, commandBuffer)->CmdWaitEvents(commandBuffer, eventCount, pEvents, srcStageMask, dstStageMask, memoryBarrierCount, pMemoryBarriers, bufferMemoryBarrierCount, pBufferMemoryBarriers, imageMemoryBarrierCount, pImageMemoryBarriers);

        PostCmdWaitEvents(commandBuffer, eventCount, srcStageMask, dstStageMask, memoryBarrierCount);
    }
}

bool PreCmdPipelineBarrier(
    VkCommandBuffer              commandBuffer,
    uint32_t                     memoryBarrierCount,
    const VkMemoryBarrier       *pMemoryBarriers,
    uint32_t                     bufferMemoryBarrierCount,
    const VkBufferMemoryBarrier *pBufferMemoryBarriers,
    uint32_t                     imageMemoryBarrierCount,
    const VkImageMemoryBarrier  *pImageMemoryBarriers)
{
    if(pMemoryBarriers != nullptr)
    {
    }

    return true;
}

bool PostCmdPipelineBarrier(
    VkCommandBuffer commandBuffer,
    VkPipelineStageFlags srcStageMask,
    VkPipelineStageFlags dstStageMask,
    VkDependencyFlags dependencyFlags,
    uint32_t memoryBarrierCount)
{





    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdPipelineBarrier(
    VkCommandBuffer              commandBuffer,
    VkPipelineStageFlags         srcStageMask,
    VkPipelineStageFlags         dstStageMask,
    VkDependencyFlags            dependencyFlags,
    uint32_t                     memoryBarrierCount,
    const VkMemoryBarrier       *pMemoryBarriers,
    uint32_t                     bufferMemoryBarrierCount,
    const VkBufferMemoryBarrier *pBufferMemoryBarriers,
    uint32_t                     imageMemoryBarrierCount,
    const VkImageMemoryBarrier  *pImageMemoryBarriers)
{
    VkBool32    skipCall    = VK_FALSE;
    layer_data* my_data     = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    assert(my_data != NULL);

    skipCall |= param_check_vkCmdPipelineBarrier(
        my_data->report_data,
        srcStageMask,
        dstStageMask,
        dependencyFlags,
        memoryBarrierCount,
        pMemoryBarriers,
        bufferMemoryBarrierCount,
        pBufferMemoryBarriers,
        imageMemoryBarrierCount,
        pImageMemoryBarriers);

    if (skipCall == VK_FALSE) {
        PreCmdPipelineBarrier(commandBuffer, memoryBarrierCount, pMemoryBarriers, bufferMemoryBarrierCount, pBufferMemoryBarriers, imageMemoryBarrierCount, pImageMemoryBarriers);

        get_dispatch_table(pc_device_table_map, commandBuffer)->CmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask, dependencyFlags, memoryBarrierCount, pMemoryBarriers, bufferMemoryBarrierCount, pBufferMemoryBarriers, imageMemoryBarrierCount, pImageMemoryBarriers);

        PostCmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask, dependencyFlags, memoryBarrierCount);
    }
}

bool PostCmdBeginQuery(
    VkCommandBuffer commandBuffer,
    VkQueryPool queryPool,
    uint32_t slot,
    VkQueryControlFlags flags)
{




    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdBeginQuery(
    VkCommandBuffer commandBuffer,
    VkQueryPool queryPool,
    uint32_t slot,
    VkQueryControlFlags flags)
{
    get_dispatch_table(pc_device_table_map, commandBuffer)->CmdBeginQuery(commandBuffer, queryPool, slot, flags);

    PostCmdBeginQuery(commandBuffer, queryPool, slot, flags);
}

bool PostCmdEndQuery(
    VkCommandBuffer commandBuffer,
    VkQueryPool queryPool,
    uint32_t slot)
{



    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdEndQuery(
    VkCommandBuffer commandBuffer,
    VkQueryPool queryPool,
    uint32_t slot)
{
    get_dispatch_table(pc_device_table_map, commandBuffer)->CmdEndQuery(commandBuffer, queryPool, slot);

    PostCmdEndQuery(commandBuffer, queryPool, slot);
}

bool PostCmdResetQueryPool(
    VkCommandBuffer commandBuffer,
    VkQueryPool queryPool,
    uint32_t firstQuery,
    uint32_t queryCount)
{




    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdResetQueryPool(
    VkCommandBuffer commandBuffer,
    VkQueryPool queryPool,
    uint32_t firstQuery,
    uint32_t queryCount)
{
    get_dispatch_table(pc_device_table_map, commandBuffer)->CmdResetQueryPool(commandBuffer, queryPool, firstQuery, queryCount);

    PostCmdResetQueryPool(commandBuffer, queryPool, firstQuery, queryCount);
}

bool PostCmdWriteTimestamp(
    VkCommandBuffer commandBuffer,
    VkPipelineStageFlagBits pipelineStage,
    VkQueryPool queryPool,
    uint32_t slot)
{

    ValidateEnumerator(pipelineStage);

    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdWriteTimestamp(
    VkCommandBuffer commandBuffer,
    VkPipelineStageFlagBits pipelineStage,
    VkQueryPool queryPool,
    uint32_t slot)
{
    get_dispatch_table(pc_device_table_map, commandBuffer)->CmdWriteTimestamp(commandBuffer, pipelineStage, queryPool, slot);

    PostCmdWriteTimestamp(commandBuffer, pipelineStage, queryPool, slot);
}

bool PostCmdCopyQueryPoolResults(
    VkCommandBuffer commandBuffer,
    VkQueryPool queryPool,
    uint32_t firstQuery,
    uint32_t queryCount,
    VkBuffer dstBuffer,
    VkDeviceSize dstOffset,
    VkDeviceSize stride,
    VkQueryResultFlags flags)
{








    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdCopyQueryPoolResults(
    VkCommandBuffer commandBuffer,
    VkQueryPool queryPool,
    uint32_t firstQuery,
    uint32_t queryCount,
    VkBuffer dstBuffer,
    VkDeviceSize dstOffset,
    VkDeviceSize stride,
    VkQueryResultFlags flags)
{
    get_dispatch_table(pc_device_table_map, commandBuffer)->CmdCopyQueryPoolResults(commandBuffer, queryPool, firstQuery, queryCount, dstBuffer, dstOffset, stride, flags);

    PostCmdCopyQueryPoolResults(commandBuffer, queryPool, firstQuery, queryCount, dstBuffer, dstOffset, stride, flags);
}

bool PreCmdPushConstants(
    VkCommandBuffer commandBuffer,
    const void* pValues)
{
    if(pValues != nullptr)
    {
    }

    return true;
}

bool PostCmdPushConstants(
    VkCommandBuffer commandBuffer,
    VkPipelineLayout layout,
    VkShaderStageFlags stageFlags,
    uint32_t offset,
    uint32_t size)
{





    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdPushConstants(
    VkCommandBuffer commandBuffer,
    VkPipelineLayout layout,
    VkShaderStageFlags stageFlags,
    uint32_t offset,
    uint32_t size,
    const void* pValues)
{
    VkBool32    skipCall    = VK_FALSE;
    layer_data* my_data     = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    assert(my_data != NULL);

    skipCall |= param_check_vkCmdPushConstants(
        my_data->report_data,
        layout,
        stageFlags,
        offset,
        size,
        pValues);

    if (skipCall == VK_FALSE) {
        PreCmdPushConstants(commandBuffer, pValues);

        get_dispatch_table(pc_device_table_map, commandBuffer)->CmdPushConstants(commandBuffer, layout, stageFlags, offset, size, pValues);

        PostCmdPushConstants(commandBuffer, layout, stageFlags, offset, size);
    }
}

bool PreCmdBeginRenderPass(
    VkCommandBuffer commandBuffer,
    const VkRenderPassBeginInfo* pRenderPassBegin)
{
    if(pRenderPassBegin != nullptr)
    {
    if(pRenderPassBegin->sType != VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO)
    {
        log_msg(mdd(commandBuffer), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCmdBeginRenderPass parameter, VkStructureType pRenderPassBegin->sType, is an invalid enumerator");
        return false;
    }
    if(pRenderPassBegin->pClearValues != nullptr)
    {
    }
    }

    return true;
}

bool PostCmdBeginRenderPass(
    VkCommandBuffer commandBuffer,
    VkSubpassContents contents)
{

    if(contents < VK_SUBPASS_CONTENTS_BEGIN_RANGE ||
        contents > VK_SUBPASS_CONTENTS_END_RANGE)
    {
        log_msg(mdd(commandBuffer), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCmdBeginRenderPass parameter, VkSubpassContents contents, is an unrecognized enumerator");
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdBeginRenderPass(
    VkCommandBuffer commandBuffer,
    const VkRenderPassBeginInfo* pRenderPassBegin,
    VkSubpassContents contents)
{
    VkBool32    skipCall    = VK_FALSE;
    layer_data* my_data     = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    assert(my_data != NULL);

    skipCall |= param_check_vkCmdBeginRenderPass(
        my_data->report_data,
        pRenderPassBegin,
        contents);

    if (skipCall == VK_FALSE) {
        PreCmdBeginRenderPass(commandBuffer, pRenderPassBegin);

        get_dispatch_table(pc_device_table_map, commandBuffer)->CmdBeginRenderPass(commandBuffer, pRenderPassBegin, contents);

        PostCmdBeginRenderPass(commandBuffer, contents);
    }
}

bool PostCmdNextSubpass(
    VkCommandBuffer commandBuffer,
    VkSubpassContents contents)
{

    if(contents < VK_SUBPASS_CONTENTS_BEGIN_RANGE ||
        contents > VK_SUBPASS_CONTENTS_END_RANGE)
    {
        log_msg(mdd(commandBuffer), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "PARAMCHECK",
        "vkCmdNextSubpass parameter, VkSubpassContents contents, is an unrecognized enumerator");
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdNextSubpass(
    VkCommandBuffer commandBuffer,
    VkSubpassContents contents)
{
    get_dispatch_table(pc_device_table_map, commandBuffer)->CmdNextSubpass(commandBuffer, contents);

    PostCmdNextSubpass(commandBuffer, contents);
}

bool PostCmdEndRenderPass(
    VkCommandBuffer commandBuffer)
{

    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdEndRenderPass(
    VkCommandBuffer commandBuffer)
{
    get_dispatch_table(pc_device_table_map, commandBuffer)->CmdEndRenderPass(commandBuffer);

    PostCmdEndRenderPass(commandBuffer);
}

bool PreCmdExecuteCommands(
    VkCommandBuffer commandBuffer,
    const VkCommandBuffer* pCommandBuffers)
{
    if(pCommandBuffers != nullptr)
    {
    }

    return true;
}

bool PostCmdExecuteCommands(
    VkCommandBuffer commandBuffer,
    uint32_t commandBuffersCount)
{


    return true;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdExecuteCommands(
    VkCommandBuffer commandBuffer,
    uint32_t commandBufferCount,
    const VkCommandBuffer* pCommandBuffers)
{
    VkBool32    skipCall    = VK_FALSE;
    layer_data* my_data     = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    assert(my_data != NULL);

    skipCall |= param_check_vkCmdExecuteCommands(
        my_data->report_data,
        commandBufferCount,
        pCommandBuffers);

    if (skipCall == VK_FALSE) {
        PreCmdExecuteCommands(commandBuffer, pCommandBuffers);

        get_dispatch_table(pc_device_table_map, commandBuffer)->CmdExecuteCommands(commandBuffer, commandBufferCount, pCommandBuffers);

        PostCmdExecuteCommands(commandBuffer, commandBufferCount);
    }
}

VK_LAYER_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr(VkDevice device, const char* funcName)
{
    layer_data *data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);

    if (validate_string(data, "vkGetDeviceProcAddr()", "funcName", funcName) == VK_TRUE) {
        return NULL;
    }

    if (!strcmp(funcName, "vkGetDeviceProcAddr"))
        return (PFN_vkVoidFunction) vkGetDeviceProcAddr;
    if (!strcmp(funcName, "vkDestroyDevice"))
        return (PFN_vkVoidFunction) vkDestroyDevice;
    if (!strcmp(funcName, "vkGetDeviceQueue"))
        return (PFN_vkVoidFunction) vkGetDeviceQueue;
    if (!strcmp(funcName, "vkQueueSubmit"))
        return (PFN_vkVoidFunction) vkQueueSubmit;
    if (!strcmp(funcName, "vkQueueWaitIdle"))
        return (PFN_vkVoidFunction) vkQueueWaitIdle;
    if (!strcmp(funcName, "vkDeviceWaitIdle"))
        return (PFN_vkVoidFunction) vkDeviceWaitIdle;
    if (!strcmp(funcName, "vkAllocateMemory"))
        return (PFN_vkVoidFunction) vkAllocateMemory;
    if (!strcmp(funcName, "vkMapMemory"))
        return (PFN_vkVoidFunction) vkMapMemory;
    if (!strcmp(funcName, "vkFlushMappedMemoryRanges"))
        return (PFN_vkVoidFunction) vkFlushMappedMemoryRanges;
    if (!strcmp(funcName, "vkInvalidateMappedMemoryRanges"))
        return (PFN_vkVoidFunction) vkInvalidateMappedMemoryRanges;
    if (!strcmp(funcName, "vkCreateFence"))
        return (PFN_vkVoidFunction) vkCreateFence;
    if (!strcmp(funcName, "vkResetFences"))
        return (PFN_vkVoidFunction) vkResetFences;
    if (!strcmp(funcName, "vkGetFenceStatus"))
        return (PFN_vkVoidFunction) vkGetFenceStatus;
    if (!strcmp(funcName, "vkWaitForFences"))
        return (PFN_vkVoidFunction) vkWaitForFences;
    if (!strcmp(funcName, "vkCreateSemaphore"))
        return (PFN_vkVoidFunction) vkCreateSemaphore;
    if (!strcmp(funcName, "vkCreateEvent"))
        return (PFN_vkVoidFunction) vkCreateEvent;
    if (!strcmp(funcName, "vkGetEventStatus"))
        return (PFN_vkVoidFunction) vkGetEventStatus;
    if (!strcmp(funcName, "vkSetEvent"))
        return (PFN_vkVoidFunction) vkSetEvent;
    if (!strcmp(funcName, "vkResetEvent"))
        return (PFN_vkVoidFunction) vkResetEvent;
    if (!strcmp(funcName, "vkCreateQueryPool"))
        return (PFN_vkVoidFunction) vkCreateQueryPool;
    if (!strcmp(funcName, "vkGetQueryPoolResults"))
        return (PFN_vkVoidFunction) vkGetQueryPoolResults;
    if (!strcmp(funcName, "vkCreateBuffer"))
        return (PFN_vkVoidFunction) vkCreateBuffer;
    if (!strcmp(funcName, "vkCreateBufferView"))
        return (PFN_vkVoidFunction) vkCreateBufferView;
    if (!strcmp(funcName, "vkCreateImage"))
        return (PFN_vkVoidFunction) vkCreateImage;
    if (!strcmp(funcName, "vkGetImageSubresourceLayout"))
        return (PFN_vkVoidFunction) vkGetImageSubresourceLayout;
    if (!strcmp(funcName, "vkCreateImageView"))
        return (PFN_vkVoidFunction) vkCreateImageView;
    if (!strcmp(funcName, "vkCreateShaderModule"))
        return (PFN_vkVoidFunction) vkCreateShaderModule;
    if (!strcmp(funcName, "vkCreateGraphicsPipelines"))
        return (PFN_vkVoidFunction) vkCreateGraphicsPipelines;
    if (!strcmp(funcName, "vkCreateComputePipelines"))
        return (PFN_vkVoidFunction) vkCreateComputePipelines;
    if (!strcmp(funcName, "vkCreatePipelineLayout"))
        return (PFN_vkVoidFunction) vkCreatePipelineLayout;
    if (!strcmp(funcName, "vkCreateSampler"))
        return (PFN_vkVoidFunction) vkCreateSampler;
    if (!strcmp(funcName, "vkCreateDescriptorSetLayout"))
        return (PFN_vkVoidFunction) vkCreateDescriptorSetLayout;
    if (!strcmp(funcName, "vkCreateDescriptorPool"))
        return (PFN_vkVoidFunction) vkCreateDescriptorPool;
    if (!strcmp(funcName, "vkResetDescriptorPool"))
        return (PFN_vkVoidFunction) vkResetDescriptorPool;
    if (!strcmp(funcName, "vkAllocateDescriptorSets"))
        return (PFN_vkVoidFunction) vkAllocateDescriptorSets;
    if (!strcmp(funcName, "vkCmdSetViewport"))
        return (PFN_vkVoidFunction) vkCmdSetViewport;
    if (!strcmp(funcName, "vkCmdSetScissor"))
        return (PFN_vkVoidFunction) vkCmdSetScissor;
    if (!strcmp(funcName, "vkCmdSetLineWidth"))
        return (PFN_vkVoidFunction) vkCmdSetLineWidth;
    if (!strcmp(funcName, "vkCmdSetDepthBias"))
        return (PFN_vkVoidFunction) vkCmdSetDepthBias;
    if (!strcmp(funcName, "vkCmdSetBlendConstants"))
        return (PFN_vkVoidFunction) vkCmdSetBlendConstants;
    if (!strcmp(funcName, "vkCmdSetDepthBounds"))
        return (PFN_vkVoidFunction) vkCmdSetDepthBounds;
    if (!strcmp(funcName, "vkCmdSetStencilCompareMask"))
        return (PFN_vkVoidFunction) vkCmdSetStencilCompareMask;
    if (!strcmp(funcName, "vkCmdSetStencilWriteMask"))
        return (PFN_vkVoidFunction) vkCmdSetStencilWriteMask;
    if (!strcmp(funcName, "vkCmdSetStencilReference"))
        return (PFN_vkVoidFunction) vkCmdSetStencilReference;
    if (!strcmp(funcName, "vkAllocateCommandBuffers"))
        return (PFN_vkVoidFunction) vkAllocateCommandBuffers;
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
        return (PFN_vkVoidFunction) vkCmdDispatch;
    if (!strcmp(funcName, "vkCmdDispatchIndirect"))
        return (PFN_vkVoidFunction) vkCmdDispatchIndirect;
    if (!strcmp(funcName, "vkCmdCopyBuffer"))
        return (PFN_vkVoidFunction) vkCmdCopyBuffer;
    if (!strcmp(funcName, "vkCmdCopyImage"))
        return (PFN_vkVoidFunction) vkCmdCopyImage;
    if (!strcmp(funcName, "vkCmdBlitImage"))
        return (PFN_vkVoidFunction) vkCmdBlitImage;
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
    if (!strcmp(funcName, "vkCmdResolveImage"))
        return (PFN_vkVoidFunction) vkCmdResolveImage;
    if (!strcmp(funcName, "vkCmdSetEvent"))
        return (PFN_vkVoidFunction) vkCmdSetEvent;
    if (!strcmp(funcName, "vkCmdResetEvent"))
        return (PFN_vkVoidFunction) vkCmdResetEvent;
    if (!strcmp(funcName, "vkCmdWaitEvents"))
        return (PFN_vkVoidFunction) vkCmdWaitEvents;
    if (!strcmp(funcName, "vkCmdPipelineBarrier"))
        return (PFN_vkVoidFunction) vkCmdPipelineBarrier;
    if (!strcmp(funcName, "vkCmdBeginQuery"))
        return (PFN_vkVoidFunction) vkCmdBeginQuery;
    if (!strcmp(funcName, "vkCmdEndQuery"))
        return (PFN_vkVoidFunction) vkCmdEndQuery;
    if (!strcmp(funcName, "vkCmdResetQueryPool"))
        return (PFN_vkVoidFunction) vkCmdResetQueryPool;
    if (!strcmp(funcName, "vkCmdWriteTimestamp"))
        return (PFN_vkVoidFunction) vkCmdWriteTimestamp;
    if (!strcmp(funcName, "vkCmdCopyQueryPoolResults"))
        return (PFN_vkVoidFunction) vkCmdCopyQueryPoolResults;
    if (!strcmp(funcName, "vkCreateFramebuffer"))
        return (PFN_vkVoidFunction) vkCreateFramebuffer;
    if (!strcmp(funcName, "vkCreateRenderPass"))
        return (PFN_vkVoidFunction) vkCreateRenderPass;
    if (!strcmp(funcName, "vkCmdBeginRenderPass"))
        return (PFN_vkVoidFunction) vkCmdBeginRenderPass;
    if (!strcmp(funcName, "vkCmdNextSubpass"))
        return (PFN_vkVoidFunction) vkCmdNextSubpass;

    if (device == NULL) {
        return NULL;
    }

    if (get_dispatch_table(pc_device_table_map, device)->GetDeviceProcAddr == NULL)
        return NULL;
    return get_dispatch_table(pc_device_table_map, device)->GetDeviceProcAddr(device, funcName);
}

VK_LAYER_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance instance, const char* funcName)
{
    if (!strcmp(funcName, "vkGetInstanceProcAddr"))
        return (PFN_vkVoidFunction) vkGetInstanceProcAddr;
    if (!strcmp(funcName, "vkCreateInstance"))
        return (PFN_vkVoidFunction) vkCreateInstance;
    if (!strcmp(funcName, "vkDestroyInstance"))
        return (PFN_vkVoidFunction) vkDestroyInstance;
    if (!strcmp(funcName, "vkCreateDevice"))
        return (PFN_vkVoidFunction) vkCreateDevice;
    if (!strcmp(funcName, "vkEnumeratePhysicalDevices"))
        return (PFN_vkVoidFunction) vkEnumeratePhysicalDevices;
    if (!strcmp(funcName, "vkGetPhysicalDeviceProperties"))
        return (PFN_vkVoidFunction) vkGetPhysicalDeviceProperties;
    if (!strcmp(funcName, "vkGetPhysicalDeviceFeatures"))
        return (PFN_vkVoidFunction) vkGetPhysicalDeviceFeatures;
    if (!strcmp(funcName, "vkGetPhysicalDeviceFormatProperties"))
        return (PFN_vkVoidFunction) vkGetPhysicalDeviceFormatProperties;
    if (!strcmp(funcName, "vkEnumerateInstanceLayerProperties"))
        return (PFN_vkVoidFunction) vkEnumerateInstanceLayerProperties;
    if (!strcmp(funcName, "vkEnumerateInstanceExtensionProperties"))
        return (PFN_vkVoidFunction) vkEnumerateInstanceExtensionProperties;
    if (!strcmp(funcName, "vkEnumerateDeviceLayerProperties"))
        return (PFN_vkVoidFunction) vkEnumerateDeviceLayerProperties;
    if (!strcmp(funcName, "vkEnumerateDeviceExtensionProperties"))
        return (PFN_vkVoidFunction) vkEnumerateDeviceExtensionProperties;

    if (instance == NULL) {
        return NULL;
    }

    layer_data *data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);

    PFN_vkVoidFunction fptr = debug_report_get_instance_proc_addr(data->report_data, funcName);
    if(fptr)
        return fptr;

    if (get_dispatch_table(pc_instance_table_map, instance)->GetInstanceProcAddr == NULL)
        return NULL;
    return get_dispatch_table(pc_instance_table_map, instance)->GetInstanceProcAddr(instance, funcName);
}
