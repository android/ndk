/*
** Copyright (c) 2015 The Khronos Group Inc.
**
** Permission is hereby granted, free of charge, to any person obtaining a
** copy of this software and/or associated documentation files (the
** "Materials"), to deal in the Materials without restriction, including
** without limitation the rights to use, copy, modify, merge, publish,
** distribute, sublicense, and/or sell copies of the Materials, and to
** permit persons to whom the Materials are furnished to do so, subject to
** the following conditions:
**
** The above copyright notice and this permission notice shall be included
** in all copies or substantial portions of the Materials.
**
** THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
** IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
** CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
** TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
** MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
*/

/*
** This header is generated from the Khronos Vulkan XML API Registry.
**
*/

#ifndef PARAM_CHECK_H
#define PARAM_CHECK_H 1

#include "vulkan/vulkan.h"
#include "param_checker_utils.h"

#ifndef UNUSED_PARAMETER
#define UNUSED_PARAMETER(x) (void)(x)
#endif // UNUSED_PARAMETER

static VkBool32 param_check_vkEnumeratePhysicalDevices(
    debug_report_data*                          report_data,
    uint32_t*                                   pPhysicalDeviceCount,
    VkPhysicalDevice*                           pPhysicalDevices)
{
    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_array(report_data, "vkEnumeratePhysicalDevices", "pPhysicalDeviceCount", "pPhysicalDevices", pPhysicalDeviceCount, pPhysicalDevices, VK_TRUE, VK_FALSE, VK_FALSE);

    return skipCall;
}

static VkBool32 param_check_vkGetPhysicalDeviceFeatures(
    debug_report_data*                          report_data,
    VkPhysicalDeviceFeatures*                   pFeatures)
{
    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_required_pointer(report_data, "vkGetPhysicalDeviceFeatures", "pFeatures", pFeatures);

    return skipCall;
}

static VkBool32 param_check_vkGetPhysicalDeviceFormatProperties(
    debug_report_data*                          report_data,
    VkFormat                                    format,
    VkFormatProperties*                         pFormatProperties)
{
    UNUSED_PARAMETER(format);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_required_pointer(report_data, "vkGetPhysicalDeviceFormatProperties", "pFormatProperties", pFormatProperties);

    return skipCall;
}

static VkBool32 param_check_vkGetPhysicalDeviceImageFormatProperties(
    debug_report_data*                          report_data,
    VkFormat                                    format,
    VkImageType                                 type,
    VkImageTiling                               tiling,
    VkImageUsageFlags                           usage,
    VkImageCreateFlags                          flags,
    VkImageFormatProperties*                    pImageFormatProperties)
{
    UNUSED_PARAMETER(format);
    UNUSED_PARAMETER(type);
    UNUSED_PARAMETER(tiling);
    UNUSED_PARAMETER(usage);
    UNUSED_PARAMETER(flags);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_required_pointer(report_data, "vkGetPhysicalDeviceImageFormatProperties", "pImageFormatProperties", pImageFormatProperties);

    return skipCall;
}

static VkBool32 param_check_vkGetPhysicalDeviceProperties(
    debug_report_data*                          report_data,
    VkPhysicalDeviceProperties*                 pProperties)
{
    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_required_pointer(report_data, "vkGetPhysicalDeviceProperties", "pProperties", pProperties);

    return skipCall;
}

static VkBool32 param_check_vkGetPhysicalDeviceQueueFamilyProperties(
    debug_report_data*                          report_data,
    uint32_t*                                   pQueueFamilyPropertyCount,
    VkQueueFamilyProperties*                    pQueueFamilyProperties)
{
    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_array(report_data, "vkGetPhysicalDeviceQueueFamilyProperties", "pQueueFamilyPropertyCount", "pQueueFamilyProperties", pQueueFamilyPropertyCount, pQueueFamilyProperties, VK_TRUE, VK_FALSE, VK_FALSE);

    return skipCall;
}

static VkBool32 param_check_vkGetPhysicalDeviceMemoryProperties(
    debug_report_data*                          report_data,
    VkPhysicalDeviceMemoryProperties*           pMemoryProperties)
{
    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_required_pointer(report_data, "vkGetPhysicalDeviceMemoryProperties", "pMemoryProperties", pMemoryProperties);

    return skipCall;
}

static VkBool32 param_check_vkEnumerateInstanceExtensionProperties(
    debug_report_data*                          report_data,
    uint32_t*                                   pPropertyCount,
    VkExtensionProperties*                      pProperties)
{
    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_array(report_data, "vkEnumerateInstanceExtensionProperties", "pPropertyCount", "pProperties", pPropertyCount, pProperties, VK_TRUE, VK_FALSE, VK_FALSE);

    return skipCall;
}

static VkBool32 param_check_vkEnumerateDeviceExtensionProperties(
    debug_report_data*                          report_data,
    const char*                                 pLayerName,
    uint32_t*                                   pPropertyCount,
    VkExtensionProperties*                      pProperties)
{
    UNUSED_PARAMETER(pLayerName);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_array(report_data, "vkEnumerateDeviceExtensionProperties", "pPropertyCount", "pProperties", pPropertyCount, pProperties, VK_TRUE, VK_FALSE, VK_FALSE);

    return skipCall;
}

static VkBool32 param_check_vkGetDeviceQueue(
    debug_report_data*                          report_data,
    uint32_t                                    queueFamilyIndex,
    uint32_t                                    queueIndex,
    VkQueue*                                    pQueue)
{
    UNUSED_PARAMETER(queueFamilyIndex);
    UNUSED_PARAMETER(queueIndex);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_required_pointer(report_data, "vkGetDeviceQueue", "pQueue", pQueue);

    return skipCall;
}

static VkBool32 param_check_vkQueueSubmit(
    debug_report_data*                          report_data,
    uint32_t                                    submitCount,
    const VkSubmitInfo*                         pSubmits,
    VkFence                                     fence)
{
    UNUSED_PARAMETER(fence);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_struct_type_array(report_data, "vkQueueSubmit", "submitCount", "pSubmits", "VK_STRUCTURE_TYPE_SUBMIT_INFO", submitCount, pSubmits, VK_STRUCTURE_TYPE_SUBMIT_INFO, VK_FALSE, VK_TRUE);

    return skipCall;
}

static VkBool32 param_check_vkAllocateMemory(
    debug_report_data*                          report_data,
    const VkMemoryAllocateInfo*                 pAllocateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkDeviceMemory*                             pMemory)
{
    UNUSED_PARAMETER(pAllocator);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_struct_type(report_data, "vkAllocateMemory", "pAllocateInfo", "VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO", pAllocateInfo, VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, VK_TRUE);

    skipCall |= validate_required_pointer(report_data, "vkAllocateMemory", "pMemory", pMemory);

    return skipCall;
}

static VkBool32 param_check_vkMapMemory(
    debug_report_data*                          report_data,
    VkDeviceMemory                              memory,
    VkDeviceSize                                offset,
    VkDeviceSize                                size,
    VkMemoryMapFlags                            flags,
    void**                                      ppData)
{
    UNUSED_PARAMETER(memory);
    UNUSED_PARAMETER(offset);
    UNUSED_PARAMETER(size);
    UNUSED_PARAMETER(flags);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_required_pointer(report_data, "vkMapMemory", "ppData", ppData);

    return skipCall;
}

static VkBool32 param_check_vkFlushMappedMemoryRanges(
    debug_report_data*                          report_data,
    uint32_t                                    memoryRangeCount,
    const VkMappedMemoryRange*                  pMemoryRanges)
{
    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_struct_type_array(report_data, "vkFlushMappedMemoryRanges", "memoryRangeCount", "pMemoryRanges", "VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE", memoryRangeCount, pMemoryRanges, VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE, VK_TRUE, VK_TRUE);

    return skipCall;
}

static VkBool32 param_check_vkInvalidateMappedMemoryRanges(
    debug_report_data*                          report_data,
    uint32_t                                    memoryRangeCount,
    const VkMappedMemoryRange*                  pMemoryRanges)
{
    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_struct_type_array(report_data, "vkInvalidateMappedMemoryRanges", "memoryRangeCount", "pMemoryRanges", "VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE", memoryRangeCount, pMemoryRanges, VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE, VK_TRUE, VK_TRUE);

    return skipCall;
}

static VkBool32 param_check_vkGetDeviceMemoryCommitment(
    debug_report_data*                          report_data,
    VkDeviceMemory                              memory,
    VkDeviceSize*                               pCommittedMemoryInBytes)
{
    UNUSED_PARAMETER(memory);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_required_pointer(report_data, "vkGetDeviceMemoryCommitment", "pCommittedMemoryInBytes", pCommittedMemoryInBytes);

    return skipCall;
}

static VkBool32 param_check_vkGetBufferMemoryRequirements(
    debug_report_data*                          report_data,
    VkBuffer                                    buffer,
    VkMemoryRequirements*                       pMemoryRequirements)
{
    UNUSED_PARAMETER(buffer);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_required_pointer(report_data, "vkGetBufferMemoryRequirements", "pMemoryRequirements", pMemoryRequirements);

    return skipCall;
}

static VkBool32 param_check_vkGetImageMemoryRequirements(
    debug_report_data*                          report_data,
    VkImage                                     image,
    VkMemoryRequirements*                       pMemoryRequirements)
{
    UNUSED_PARAMETER(image);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_required_pointer(report_data, "vkGetImageMemoryRequirements", "pMemoryRequirements", pMemoryRequirements);

    return skipCall;
}

static VkBool32 param_check_vkGetImageSparseMemoryRequirements(
    debug_report_data*                          report_data,
    VkImage                                     image,
    uint32_t*                                   pSparseMemoryRequirementCount,
    VkSparseImageMemoryRequirements*            pSparseMemoryRequirements)
{
    UNUSED_PARAMETER(image);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_array(report_data, "vkGetImageSparseMemoryRequirements", "pSparseMemoryRequirementCount", "pSparseMemoryRequirements", pSparseMemoryRequirementCount, pSparseMemoryRequirements, VK_TRUE, VK_TRUE, VK_FALSE);

    return skipCall;
}

static VkBool32 param_check_vkGetPhysicalDeviceSparseImageFormatProperties(
    debug_report_data*                          report_data,
    VkFormat                                    format,
    VkImageType                                 type,
    VkSampleCountFlagBits                       samples,
    VkImageUsageFlags                           usage,
    VkImageTiling                               tiling,
    uint32_t*                                   pPropertyCount,
    VkSparseImageFormatProperties*              pProperties)
{
    UNUSED_PARAMETER(format);
    UNUSED_PARAMETER(type);
    UNUSED_PARAMETER(samples);
    UNUSED_PARAMETER(usage);
    UNUSED_PARAMETER(tiling);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_array(report_data, "vkGetPhysicalDeviceSparseImageFormatProperties", "pPropertyCount", "pProperties", pPropertyCount, pProperties, VK_TRUE, VK_FALSE, VK_FALSE);

    return skipCall;
}

static VkBool32 param_check_vkQueueBindSparse(
    debug_report_data*                          report_data,
    uint32_t                                    bindInfoCount,
    const VkBindSparseInfo*                     pBindInfo,
    VkFence                                     fence)
{
    UNUSED_PARAMETER(fence);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_struct_type_array(report_data, "vkQueueBindSparse", "bindInfoCount", "pBindInfo", "VK_STRUCTURE_TYPE_BIND_SPARSE_INFO", bindInfoCount, pBindInfo, VK_STRUCTURE_TYPE_BIND_SPARSE_INFO, VK_FALSE, VK_TRUE);

    return skipCall;
}

static VkBool32 param_check_vkCreateFence(
    debug_report_data*                          report_data,
    const VkFenceCreateInfo*                    pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkFence*                                    pFence)
{
    UNUSED_PARAMETER(pAllocator);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_struct_type(report_data, "vkCreateFence", "pCreateInfo", "VK_STRUCTURE_TYPE_FENCE_CREATE_INFO", pCreateInfo, VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, VK_TRUE);

    skipCall |= validate_required_pointer(report_data, "vkCreateFence", "pFence", pFence);

    return skipCall;
}

static VkBool32 param_check_vkResetFences(
    debug_report_data*                          report_data,
    uint32_t                                    fenceCount,
    const VkFence*                              pFences)
{
    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_array(report_data, "vkResetFences", "fenceCount", "pFences", fenceCount, pFences, VK_TRUE, VK_TRUE);

    return skipCall;
}

static VkBool32 param_check_vkWaitForFences(
    debug_report_data*                          report_data,
    uint32_t                                    fenceCount,
    const VkFence*                              pFences,
    VkBool32                                    waitAll,
    uint64_t                                    timeout)
{
    UNUSED_PARAMETER(waitAll);
    UNUSED_PARAMETER(timeout);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_array(report_data, "vkWaitForFences", "fenceCount", "pFences", fenceCount, pFences, VK_TRUE, VK_TRUE);

    return skipCall;
}

static VkBool32 param_check_vkCreateSemaphore(
    debug_report_data*                          report_data,
    const VkSemaphoreCreateInfo*                pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkSemaphore*                                pSemaphore)
{
    UNUSED_PARAMETER(pAllocator);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_struct_type(report_data, "vkCreateSemaphore", "pCreateInfo", "VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO", pCreateInfo, VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, VK_TRUE);

    skipCall |= validate_required_pointer(report_data, "vkCreateSemaphore", "pSemaphore", pSemaphore);

    return skipCall;
}

static VkBool32 param_check_vkCreateEvent(
    debug_report_data*                          report_data,
    const VkEventCreateInfo*                    pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkEvent*                                    pEvent)
{
    UNUSED_PARAMETER(pAllocator);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_struct_type(report_data, "vkCreateEvent", "pCreateInfo", "VK_STRUCTURE_TYPE_EVENT_CREATE_INFO", pCreateInfo, VK_STRUCTURE_TYPE_EVENT_CREATE_INFO, VK_TRUE);

    skipCall |= validate_required_pointer(report_data, "vkCreateEvent", "pEvent", pEvent);

    return skipCall;
}

static VkBool32 param_check_vkCreateQueryPool(
    debug_report_data*                          report_data,
    const VkQueryPoolCreateInfo*                pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkQueryPool*                                pQueryPool)
{
    UNUSED_PARAMETER(pAllocator);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_struct_type(report_data, "vkCreateQueryPool", "pCreateInfo", "VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO", pCreateInfo, VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO, VK_TRUE);

    skipCall |= validate_required_pointer(report_data, "vkCreateQueryPool", "pQueryPool", pQueryPool);

    return skipCall;
}

static VkBool32 param_check_vkGetQueryPoolResults(
    debug_report_data*                          report_data,
    VkQueryPool                                 queryPool,
    uint32_t                                    firstQuery,
    uint32_t                                    queryCount,
    size_t                                      dataSize,
    void*                                       pData,
    VkDeviceSize                                stride,
    VkQueryResultFlags                          flags)
{
    UNUSED_PARAMETER(queryPool);
    UNUSED_PARAMETER(firstQuery);
    UNUSED_PARAMETER(queryCount);
    UNUSED_PARAMETER(stride);
    UNUSED_PARAMETER(flags);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_array(report_data, "vkGetQueryPoolResults", "dataSize", "pData", dataSize, pData, VK_TRUE, VK_TRUE);

    return skipCall;
}

static VkBool32 param_check_vkCreateBuffer(
    debug_report_data*                          report_data,
    const VkBufferCreateInfo*                   pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkBuffer*                                   pBuffer)
{
    UNUSED_PARAMETER(pAllocator);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_struct_type(report_data, "vkCreateBuffer", "pCreateInfo", "VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO", pCreateInfo, VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, VK_TRUE);

    skipCall |= validate_required_pointer(report_data, "vkCreateBuffer", "pBuffer", pBuffer);

    return skipCall;
}

static VkBool32 param_check_vkCreateBufferView(
    debug_report_data*                          report_data,
    const VkBufferViewCreateInfo*               pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkBufferView*                               pView)
{
    UNUSED_PARAMETER(pAllocator);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_struct_type(report_data, "vkCreateBufferView", "pCreateInfo", "VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO", pCreateInfo, VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO, VK_TRUE);

    skipCall |= validate_required_pointer(report_data, "vkCreateBufferView", "pView", pView);

    return skipCall;
}

static VkBool32 param_check_vkCreateImage(
    debug_report_data*                          report_data,
    const VkImageCreateInfo*                    pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkImage*                                    pImage)
{
    UNUSED_PARAMETER(pAllocator);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_struct_type(report_data, "vkCreateImage", "pCreateInfo", "VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO", pCreateInfo, VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, VK_TRUE);

    skipCall |= validate_required_pointer(report_data, "vkCreateImage", "pImage", pImage);

    return skipCall;
}

static VkBool32 param_check_vkGetImageSubresourceLayout(
    debug_report_data*                          report_data,
    VkImage                                     image,
    const VkImageSubresource*                   pSubresource,
    VkSubresourceLayout*                        pLayout)
{
    UNUSED_PARAMETER(image);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_required_pointer(report_data, "vkGetImageSubresourceLayout", "pSubresource", pSubresource);

    skipCall |= validate_required_pointer(report_data, "vkGetImageSubresourceLayout", "pLayout", pLayout);

    return skipCall;
}

static VkBool32 param_check_vkCreateImageView(
    debug_report_data*                          report_data,
    const VkImageViewCreateInfo*                pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkImageView*                                pView)
{
    UNUSED_PARAMETER(pAllocator);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_struct_type(report_data, "vkCreateImageView", "pCreateInfo", "VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO", pCreateInfo, VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, VK_TRUE);

    skipCall |= validate_required_pointer(report_data, "vkCreateImageView", "pView", pView);

    return skipCall;
}

static VkBool32 param_check_vkCreateShaderModule(
    debug_report_data*                          report_data,
    const VkShaderModuleCreateInfo*             pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkShaderModule*                             pShaderModule)
{
    UNUSED_PARAMETER(pAllocator);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_struct_type(report_data, "vkCreateShaderModule", "pCreateInfo", "VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO", pCreateInfo, VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, VK_TRUE);

    skipCall |= validate_required_pointer(report_data, "vkCreateShaderModule", "pShaderModule", pShaderModule);

    return skipCall;
}

static VkBool32 param_check_vkCreatePipelineCache(
    debug_report_data*                          report_data,
    const VkPipelineCacheCreateInfo*            pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkPipelineCache*                            pPipelineCache)
{
    UNUSED_PARAMETER(pAllocator);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_struct_type(report_data, "vkCreatePipelineCache", "pCreateInfo", "VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO", pCreateInfo, VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO, VK_TRUE);

    skipCall |= validate_required_pointer(report_data, "vkCreatePipelineCache", "pPipelineCache", pPipelineCache);

    return skipCall;
}

static VkBool32 param_check_vkGetPipelineCacheData(
    debug_report_data*                          report_data,
    VkPipelineCache                             pipelineCache,
    size_t*                                     pDataSize,
    void*                                       pData)
{
    UNUSED_PARAMETER(pipelineCache);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_array(report_data, "vkGetPipelineCacheData", "pDataSize", "pData", pDataSize, pData, VK_TRUE, VK_FALSE, VK_FALSE);

    return skipCall;
}

static VkBool32 param_check_vkMergePipelineCaches(
    debug_report_data*                          report_data,
    VkPipelineCache                             dstCache,
    uint32_t                                    srcCacheCount,
    const VkPipelineCache*                      pSrcCaches)
{
    UNUSED_PARAMETER(dstCache);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_array(report_data, "vkMergePipelineCaches", "srcCacheCount", "pSrcCaches", srcCacheCount, pSrcCaches, VK_TRUE, VK_TRUE);

    return skipCall;
}

static VkBool32 param_check_vkCreateGraphicsPipelines(
    debug_report_data*                          report_data,
    VkPipelineCache                             pipelineCache,
    uint32_t                                    createInfoCount,
    const VkGraphicsPipelineCreateInfo*         pCreateInfos,
    const VkAllocationCallbacks*                pAllocator,
    VkPipeline*                                 pPipelines)
{
    UNUSED_PARAMETER(pipelineCache);
    UNUSED_PARAMETER(pAllocator);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_struct_type_array(report_data, "vkCreateGraphicsPipelines", "createInfoCount", "pCreateInfos", "VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO", createInfoCount, pCreateInfos, VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO, VK_TRUE, VK_TRUE);

    skipCall |= validate_array(report_data, "vkCreateGraphicsPipelines", "createInfoCount", "pPipelines", createInfoCount, pPipelines, VK_TRUE, VK_TRUE);

    return skipCall;
}

static VkBool32 param_check_vkCreateComputePipelines(
    debug_report_data*                          report_data,
    VkPipelineCache                             pipelineCache,
    uint32_t                                    createInfoCount,
    const VkComputePipelineCreateInfo*          pCreateInfos,
    const VkAllocationCallbacks*                pAllocator,
    VkPipeline*                                 pPipelines)
{
    UNUSED_PARAMETER(pipelineCache);
    UNUSED_PARAMETER(pAllocator);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_struct_type_array(report_data, "vkCreateComputePipelines", "createInfoCount", "pCreateInfos", "VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO", createInfoCount, pCreateInfos, VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO, VK_TRUE, VK_TRUE);

    skipCall |= validate_array(report_data, "vkCreateComputePipelines", "createInfoCount", "pPipelines", createInfoCount, pPipelines, VK_TRUE, VK_TRUE);

    return skipCall;
}

static VkBool32 param_check_vkCreatePipelineLayout(
    debug_report_data*                          report_data,
    const VkPipelineLayoutCreateInfo*           pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkPipelineLayout*                           pPipelineLayout)
{
    UNUSED_PARAMETER(pAllocator);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_struct_type(report_data, "vkCreatePipelineLayout", "pCreateInfo", "VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO", pCreateInfo, VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, VK_TRUE);

    skipCall |= validate_required_pointer(report_data, "vkCreatePipelineLayout", "pPipelineLayout", pPipelineLayout);

    return skipCall;
}

static VkBool32 param_check_vkCreateSampler(
    debug_report_data*                          report_data,
    const VkSamplerCreateInfo*                  pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkSampler*                                  pSampler)
{
    UNUSED_PARAMETER(pAllocator);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_struct_type(report_data, "vkCreateSampler", "pCreateInfo", "VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO", pCreateInfo, VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO, VK_TRUE);

    skipCall |= validate_required_pointer(report_data, "vkCreateSampler", "pSampler", pSampler);

    return skipCall;
}

static VkBool32 param_check_vkCreateDescriptorSetLayout(
    debug_report_data*                          report_data,
    const VkDescriptorSetLayoutCreateInfo*      pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkDescriptorSetLayout*                      pSetLayout)
{
    UNUSED_PARAMETER(pAllocator);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_struct_type(report_data, "vkCreateDescriptorSetLayout", "pCreateInfo", "VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO", pCreateInfo, VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, VK_TRUE);

    skipCall |= validate_required_pointer(report_data, "vkCreateDescriptorSetLayout", "pSetLayout", pSetLayout);

    return skipCall;
}

static VkBool32 param_check_vkCreateDescriptorPool(
    debug_report_data*                          report_data,
    const VkDescriptorPoolCreateInfo*           pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkDescriptorPool*                           pDescriptorPool)
{
    UNUSED_PARAMETER(pAllocator);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_struct_type(report_data, "vkCreateDescriptorPool", "pCreateInfo", "VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO", pCreateInfo, VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, VK_TRUE);

    skipCall |= validate_required_pointer(report_data, "vkCreateDescriptorPool", "pDescriptorPool", pDescriptorPool);

    return skipCall;
}

static VkBool32 param_check_vkAllocateDescriptorSets(
    debug_report_data*                          report_data,
    const VkDescriptorSetAllocateInfo*          pAllocateInfo,
    VkDescriptorSet*                            pDescriptorSets)
{
    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_struct_type(report_data, "vkAllocateDescriptorSets", "pAllocateInfo", "VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO", pAllocateInfo, VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, VK_TRUE);

    skipCall |= validate_array(report_data, "vkAllocateDescriptorSets", "pAllocateInfo->descriptorSetCount", "pDescriptorSets", pAllocateInfo->descriptorSetCount, pDescriptorSets, VK_TRUE, VK_TRUE);

    return skipCall;
}

static VkBool32 param_check_vkFreeDescriptorSets(
    debug_report_data*                          report_data,
    VkDescriptorPool                            descriptorPool,
    uint32_t                                    descriptorSetCount,
    const VkDescriptorSet*                      pDescriptorSets)
{
    UNUSED_PARAMETER(descriptorPool);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_array(report_data, "vkFreeDescriptorSets", "descriptorSetCount", "pDescriptorSets", descriptorSetCount, pDescriptorSets, VK_TRUE, VK_TRUE);

    return skipCall;
}

static VkBool32 param_check_vkUpdateDescriptorSets(
    debug_report_data*                          report_data,
    uint32_t                                    descriptorWriteCount,
    const VkWriteDescriptorSet*                 pDescriptorWrites,
    uint32_t                                    descriptorCopyCount,
    const VkCopyDescriptorSet*                  pDescriptorCopies)
{
    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_struct_type_array(report_data, "vkUpdateDescriptorSets", "descriptorWriteCount", "pDescriptorWrites", "VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET", descriptorWriteCount, pDescriptorWrites, VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, VK_FALSE, VK_TRUE);

    skipCall |= validate_struct_type_array(report_data, "vkUpdateDescriptorSets", "descriptorCopyCount", "pDescriptorCopies", "VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET", descriptorCopyCount, pDescriptorCopies, VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET, VK_FALSE, VK_TRUE);

    return skipCall;
}

static VkBool32 param_check_vkCreateFramebuffer(
    debug_report_data*                          report_data,
    const VkFramebufferCreateInfo*              pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkFramebuffer*                              pFramebuffer)
{
    UNUSED_PARAMETER(pAllocator);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_struct_type(report_data, "vkCreateFramebuffer", "pCreateInfo", "VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO", pCreateInfo, VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO, VK_TRUE);

    skipCall |= validate_required_pointer(report_data, "vkCreateFramebuffer", "pFramebuffer", pFramebuffer);

    return skipCall;
}

static VkBool32 param_check_vkCreateRenderPass(
    debug_report_data*                          report_data,
    const VkRenderPassCreateInfo*               pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkRenderPass*                               pRenderPass)
{
    UNUSED_PARAMETER(pAllocator);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_struct_type(report_data, "vkCreateRenderPass", "pCreateInfo", "VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO", pCreateInfo, VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO, VK_TRUE);

    skipCall |= validate_required_pointer(report_data, "vkCreateRenderPass", "pRenderPass", pRenderPass);

    return skipCall;
}

static VkBool32 param_check_vkGetRenderAreaGranularity(
    debug_report_data*                          report_data,
    VkRenderPass                                renderPass,
    VkExtent2D*                                 pGranularity)
{
    UNUSED_PARAMETER(renderPass);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_required_pointer(report_data, "vkGetRenderAreaGranularity", "pGranularity", pGranularity);

    return skipCall;
}

static VkBool32 param_check_vkCreateCommandPool(
    debug_report_data*                          report_data,
    const VkCommandPoolCreateInfo*              pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkCommandPool*                              pCommandPool)
{
    UNUSED_PARAMETER(pAllocator);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_struct_type(report_data, "vkCreateCommandPool", "pCreateInfo", "VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO", pCreateInfo, VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, VK_TRUE);

    skipCall |= validate_required_pointer(report_data, "vkCreateCommandPool", "pCommandPool", pCommandPool);

    return skipCall;
}

static VkBool32 param_check_vkAllocateCommandBuffers(
    debug_report_data*                          report_data,
    const VkCommandBufferAllocateInfo*          pAllocateInfo,
    VkCommandBuffer*                            pCommandBuffers)
{
    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_struct_type(report_data, "vkAllocateCommandBuffers", "pAllocateInfo", "VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO", pAllocateInfo, VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, VK_TRUE);

    skipCall |= validate_array(report_data, "vkAllocateCommandBuffers", "pAllocateInfo->commandBufferCount", "pCommandBuffers", pAllocateInfo->commandBufferCount, pCommandBuffers, VK_TRUE, VK_TRUE);

    return skipCall;
}

static VkBool32 param_check_vkFreeCommandBuffers(
    debug_report_data*                          report_data,
    VkCommandPool                               commandPool,
    uint32_t                                    commandBufferCount,
    const VkCommandBuffer*                      pCommandBuffers)
{
    UNUSED_PARAMETER(commandPool);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_array(report_data, "vkFreeCommandBuffers", "commandBufferCount", "pCommandBuffers", commandBufferCount, pCommandBuffers, VK_TRUE, VK_TRUE);

    return skipCall;
}

static VkBool32 param_check_vkBeginCommandBuffer(
    debug_report_data*                          report_data,
    const VkCommandBufferBeginInfo*             pBeginInfo)
{
    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_struct_type(report_data, "vkBeginCommandBuffer", "pBeginInfo", "VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO", pBeginInfo, VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, VK_TRUE);

    return skipCall;
}

static VkBool32 param_check_vkCmdSetViewport(
    debug_report_data*                          report_data,
    uint32_t                                    firstViewport,
    uint32_t                                    viewportCount,
    const VkViewport*                           pViewports)
{
    UNUSED_PARAMETER(firstViewport);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_array(report_data, "vkCmdSetViewport", "viewportCount", "pViewports", viewportCount, pViewports, VK_TRUE, VK_TRUE);

    return skipCall;
}

static VkBool32 param_check_vkCmdSetScissor(
    debug_report_data*                          report_data,
    uint32_t                                    firstScissor,
    uint32_t                                    scissorCount,
    const VkRect2D*                             pScissors)
{
    UNUSED_PARAMETER(firstScissor);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_array(report_data, "vkCmdSetScissor", "scissorCount", "pScissors", scissorCount, pScissors, VK_TRUE, VK_TRUE);

    return skipCall;
}

static VkBool32 param_check_vkCmdSetBlendConstants(
    debug_report_data*                          report_data,
    const float                                 blendConstants[4])
{
    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_required_pointer(report_data, "vkCmdSetBlendConstants", "blendConstants", blendConstants);

    return skipCall;
}

static VkBool32 param_check_vkCmdBindDescriptorSets(
    debug_report_data*                          report_data,
    VkPipelineBindPoint                         pipelineBindPoint,
    VkPipelineLayout                            layout,
    uint32_t                                    firstSet,
    uint32_t                                    descriptorSetCount,
    const VkDescriptorSet*                      pDescriptorSets,
    uint32_t                                    dynamicOffsetCount,
    const uint32_t*                             pDynamicOffsets)
{
    UNUSED_PARAMETER(pipelineBindPoint);
    UNUSED_PARAMETER(layout);
    UNUSED_PARAMETER(firstSet);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_array(report_data, "vkCmdBindDescriptorSets", "descriptorSetCount", "pDescriptorSets", descriptorSetCount, pDescriptorSets, VK_TRUE, VK_TRUE);

    skipCall |= validate_array(report_data, "vkCmdBindDescriptorSets", "dynamicOffsetCount", "pDynamicOffsets", dynamicOffsetCount, pDynamicOffsets, VK_FALSE, VK_TRUE);

    return skipCall;
}

static VkBool32 param_check_vkCmdBindVertexBuffers(
    debug_report_data*                          report_data,
    uint32_t                                    firstBinding,
    uint32_t                                    bindingCount,
    const VkBuffer*                             pBuffers,
    const VkDeviceSize*                         pOffsets)
{
    UNUSED_PARAMETER(firstBinding);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_array(report_data, "vkCmdBindVertexBuffers", "bindingCount", "pBuffers", bindingCount, pBuffers, VK_TRUE, VK_TRUE);

    skipCall |= validate_array(report_data, "vkCmdBindVertexBuffers", "bindingCount", "pOffsets", bindingCount, pOffsets, VK_TRUE, VK_TRUE);

    return skipCall;
}

static VkBool32 param_check_vkCmdCopyBuffer(
    debug_report_data*                          report_data,
    VkBuffer                                    srcBuffer,
    VkBuffer                                    dstBuffer,
    uint32_t                                    regionCount,
    const VkBufferCopy*                         pRegions)
{
    UNUSED_PARAMETER(srcBuffer);
    UNUSED_PARAMETER(dstBuffer);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_array(report_data, "vkCmdCopyBuffer", "regionCount", "pRegions", regionCount, pRegions, VK_TRUE, VK_TRUE);

    return skipCall;
}

static VkBool32 param_check_vkCmdCopyImage(
    debug_report_data*                          report_data,
    VkImage                                     srcImage,
    VkImageLayout                               srcImageLayout,
    VkImage                                     dstImage,
    VkImageLayout                               dstImageLayout,
    uint32_t                                    regionCount,
    const VkImageCopy*                          pRegions)
{
    UNUSED_PARAMETER(srcImage);
    UNUSED_PARAMETER(srcImageLayout);
    UNUSED_PARAMETER(dstImage);
    UNUSED_PARAMETER(dstImageLayout);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_array(report_data, "vkCmdCopyImage", "regionCount", "pRegions", regionCount, pRegions, VK_TRUE, VK_TRUE);

    return skipCall;
}

static VkBool32 param_check_vkCmdBlitImage(
    debug_report_data*                          report_data,
    VkImage                                     srcImage,
    VkImageLayout                               srcImageLayout,
    VkImage                                     dstImage,
    VkImageLayout                               dstImageLayout,
    uint32_t                                    regionCount,
    const VkImageBlit*                          pRegions,
    VkFilter                                    filter)
{
    UNUSED_PARAMETER(srcImage);
    UNUSED_PARAMETER(srcImageLayout);
    UNUSED_PARAMETER(dstImage);
    UNUSED_PARAMETER(dstImageLayout);
    UNUSED_PARAMETER(filter);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_array(report_data, "vkCmdBlitImage", "regionCount", "pRegions", regionCount, pRegions, VK_TRUE, VK_TRUE);

    return skipCall;
}

static VkBool32 param_check_vkCmdCopyBufferToImage(
    debug_report_data*                          report_data,
    VkBuffer                                    srcBuffer,
    VkImage                                     dstImage,
    VkImageLayout                               dstImageLayout,
    uint32_t                                    regionCount,
    const VkBufferImageCopy*                    pRegions)
{
    UNUSED_PARAMETER(srcBuffer);
    UNUSED_PARAMETER(dstImage);
    UNUSED_PARAMETER(dstImageLayout);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_array(report_data, "vkCmdCopyBufferToImage", "regionCount", "pRegions", regionCount, pRegions, VK_TRUE, VK_TRUE);

    return skipCall;
}

static VkBool32 param_check_vkCmdCopyImageToBuffer(
    debug_report_data*                          report_data,
    VkImage                                     srcImage,
    VkImageLayout                               srcImageLayout,
    VkBuffer                                    dstBuffer,
    uint32_t                                    regionCount,
    const VkBufferImageCopy*                    pRegions)
{
    UNUSED_PARAMETER(srcImage);
    UNUSED_PARAMETER(srcImageLayout);
    UNUSED_PARAMETER(dstBuffer);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_array(report_data, "vkCmdCopyImageToBuffer", "regionCount", "pRegions", regionCount, pRegions, VK_TRUE, VK_TRUE);

    return skipCall;
}

static VkBool32 param_check_vkCmdUpdateBuffer(
    debug_report_data*                          report_data,
    VkBuffer                                    dstBuffer,
    VkDeviceSize                                dstOffset,
    VkDeviceSize                                dataSize,
    const uint32_t*                             pData)
{
    UNUSED_PARAMETER(dstBuffer);
    UNUSED_PARAMETER(dstOffset);
    UNUSED_PARAMETER(dataSize);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_array(report_data, "vkCmdUpdateBuffer", "dataSize", "pData", dataSize, pData, VK_TRUE, VK_TRUE);

    return skipCall;
}

static VkBool32 param_check_vkCmdClearColorImage(
    debug_report_data*                          report_data,
    VkImage                                     image,
    VkImageLayout                               imageLayout,
    const VkClearColorValue*                    pColor,
    uint32_t                                    rangeCount,
    const VkImageSubresourceRange*              pRanges)
{
    UNUSED_PARAMETER(image);
    UNUSED_PARAMETER(imageLayout);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_required_pointer(report_data, "vkCmdClearColorImage", "pColor", pColor);

    skipCall |= validate_array(report_data, "vkCmdClearColorImage", "rangeCount", "pRanges", rangeCount, pRanges, VK_TRUE, VK_TRUE);

    return skipCall;
}

static VkBool32 param_check_vkCmdClearDepthStencilImage(
    debug_report_data*                          report_data,
    VkImage                                     image,
    VkImageLayout                               imageLayout,
    const VkClearDepthStencilValue*             pDepthStencil,
    uint32_t                                    rangeCount,
    const VkImageSubresourceRange*              pRanges)
{
    UNUSED_PARAMETER(image);
    UNUSED_PARAMETER(imageLayout);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_required_pointer(report_data, "vkCmdClearDepthStencilImage", "pDepthStencil", pDepthStencil);

    skipCall |= validate_array(report_data, "vkCmdClearDepthStencilImage", "rangeCount", "pRanges", rangeCount, pRanges, VK_TRUE, VK_TRUE);

    return skipCall;
}

static VkBool32 param_check_vkCmdClearAttachments(
    debug_report_data*                          report_data,
    uint32_t                                    attachmentCount,
    const VkClearAttachment*                    pAttachments,
    uint32_t                                    rectCount,
    const VkClearRect*                          pRects)
{
    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_array(report_data, "vkCmdClearAttachments", "attachmentCount", "pAttachments", attachmentCount, pAttachments, VK_TRUE, VK_TRUE);

    skipCall |= validate_array(report_data, "vkCmdClearAttachments", "rectCount", "pRects", rectCount, pRects, VK_TRUE, VK_TRUE);

    return skipCall;
}

static VkBool32 param_check_vkCmdResolveImage(
    debug_report_data*                          report_data,
    VkImage                                     srcImage,
    VkImageLayout                               srcImageLayout,
    VkImage                                     dstImage,
    VkImageLayout                               dstImageLayout,
    uint32_t                                    regionCount,
    const VkImageResolve*                       pRegions)
{
    UNUSED_PARAMETER(srcImage);
    UNUSED_PARAMETER(srcImageLayout);
    UNUSED_PARAMETER(dstImage);
    UNUSED_PARAMETER(dstImageLayout);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_array(report_data, "vkCmdResolveImage", "regionCount", "pRegions", regionCount, pRegions, VK_TRUE, VK_TRUE);

    return skipCall;
}

static VkBool32 param_check_vkCmdWaitEvents(
    debug_report_data*                          report_data,
    uint32_t                                    eventCount,
    const VkEvent*                              pEvents,
    VkPipelineStageFlags                        srcStageMask,
    VkPipelineStageFlags                        dstStageMask,
    uint32_t                                    memoryBarrierCount,
    const VkMemoryBarrier*                      pMemoryBarriers,
    uint32_t                                    bufferMemoryBarrierCount,
    const VkBufferMemoryBarrier*                pBufferMemoryBarriers,
    uint32_t                                    imageMemoryBarrierCount,
    const VkImageMemoryBarrier*                 pImageMemoryBarriers)
{
    UNUSED_PARAMETER(srcStageMask);
    UNUSED_PARAMETER(dstStageMask);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_array(report_data, "vkCmdWaitEvents", "eventCount", "pEvents", eventCount, pEvents, VK_TRUE, VK_TRUE);

    skipCall |= validate_struct_type_array(report_data, "vkCmdWaitEvents", "memoryBarrierCount", "pMemoryBarriers", "VK_STRUCTURE_TYPE_MEMORY_BARRIER", memoryBarrierCount, pMemoryBarriers, VK_STRUCTURE_TYPE_MEMORY_BARRIER, VK_FALSE, VK_TRUE);

    skipCall |= validate_struct_type_array(report_data, "vkCmdWaitEvents", "bufferMemoryBarrierCount", "pBufferMemoryBarriers", "VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER", bufferMemoryBarrierCount, pBufferMemoryBarriers, VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, VK_FALSE, VK_TRUE);

    skipCall |= validate_struct_type_array(report_data, "vkCmdWaitEvents", "imageMemoryBarrierCount", "pImageMemoryBarriers", "VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER", imageMemoryBarrierCount, pImageMemoryBarriers, VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, VK_FALSE, VK_TRUE);

    return skipCall;
}

static VkBool32 param_check_vkCmdPipelineBarrier(
    debug_report_data*                          report_data,
    VkPipelineStageFlags                        srcStageMask,
    VkPipelineStageFlags                        dstStageMask,
    VkDependencyFlags                           dependencyFlags,
    uint32_t                                    memoryBarrierCount,
    const VkMemoryBarrier*                      pMemoryBarriers,
    uint32_t                                    bufferMemoryBarrierCount,
    const VkBufferMemoryBarrier*                pBufferMemoryBarriers,
    uint32_t                                    imageMemoryBarrierCount,
    const VkImageMemoryBarrier*                 pImageMemoryBarriers)
{
    UNUSED_PARAMETER(srcStageMask);
    UNUSED_PARAMETER(dstStageMask);
    UNUSED_PARAMETER(dependencyFlags);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_struct_type_array(report_data, "vkCmdPipelineBarrier", "memoryBarrierCount", "pMemoryBarriers", "VK_STRUCTURE_TYPE_MEMORY_BARRIER", memoryBarrierCount, pMemoryBarriers, VK_STRUCTURE_TYPE_MEMORY_BARRIER, VK_FALSE, VK_TRUE);

    skipCall |= validate_struct_type_array(report_data, "vkCmdPipelineBarrier", "bufferMemoryBarrierCount", "pBufferMemoryBarriers", "VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER", bufferMemoryBarrierCount, pBufferMemoryBarriers, VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, VK_FALSE, VK_TRUE);

    skipCall |= validate_struct_type_array(report_data, "vkCmdPipelineBarrier", "imageMemoryBarrierCount", "pImageMemoryBarriers", "VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER", imageMemoryBarrierCount, pImageMemoryBarriers, VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, VK_FALSE, VK_TRUE);

    return skipCall;
}

static VkBool32 param_check_vkCmdPushConstants(
    debug_report_data*                          report_data,
    VkPipelineLayout                            layout,
    VkShaderStageFlags                          stageFlags,
    uint32_t                                    offset,
    uint32_t                                    size,
    const void*                                 pValues)
{
    UNUSED_PARAMETER(layout);
    UNUSED_PARAMETER(stageFlags);
    UNUSED_PARAMETER(offset);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_array(report_data, "vkCmdPushConstants", "size", "pValues", size, pValues, VK_TRUE, VK_TRUE);

    return skipCall;
}

static VkBool32 param_check_vkCmdBeginRenderPass(
    debug_report_data*                          report_data,
    const VkRenderPassBeginInfo*                pRenderPassBegin,
    VkSubpassContents                           contents)
{
    UNUSED_PARAMETER(contents);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_struct_type(report_data, "vkCmdBeginRenderPass", "pRenderPassBegin", "VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO", pRenderPassBegin, VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO, VK_TRUE);

    return skipCall;
}

static VkBool32 param_check_vkCmdExecuteCommands(
    debug_report_data*                          report_data,
    uint32_t                                    commandBufferCount,
    const VkCommandBuffer*                      pCommandBuffers)
{
    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_array(report_data, "vkCmdExecuteCommands", "commandBufferCount", "pCommandBuffers", commandBufferCount, pCommandBuffers, VK_TRUE, VK_TRUE);

    return skipCall;
}


static VkBool32 param_check_vkGetPhysicalDeviceSurfaceSupportKHR(
    debug_report_data*                          report_data,
    uint32_t                                    queueFamilyIndex,
    VkSurfaceKHR                                surface,
    VkBool32*                                   pSupported)
{
    UNUSED_PARAMETER(queueFamilyIndex);
    UNUSED_PARAMETER(surface);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_required_pointer(report_data, "vkGetPhysicalDeviceSurfaceSupportKHR", "pSupported", pSupported);

    return skipCall;
}

static VkBool32 param_check_vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
    debug_report_data*                          report_data,
    VkSurfaceKHR                                surface,
    VkSurfaceCapabilitiesKHR*                   pSurfaceCapabilities)
{
    UNUSED_PARAMETER(surface);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_required_pointer(report_data, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR", "pSurfaceCapabilities", pSurfaceCapabilities);

    return skipCall;
}

static VkBool32 param_check_vkGetPhysicalDeviceSurfaceFormatsKHR(
    debug_report_data*                          report_data,
    VkSurfaceKHR                                surface,
    uint32_t*                                   pSurfaceFormatCount,
    VkSurfaceFormatKHR*                         pSurfaceFormats)
{
    UNUSED_PARAMETER(surface);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_array(report_data, "vkGetPhysicalDeviceSurfaceFormatsKHR", "pSurfaceFormatCount", "pSurfaceFormats", pSurfaceFormatCount, pSurfaceFormats, VK_TRUE, VK_FALSE, VK_FALSE);

    return skipCall;
}

static VkBool32 param_check_vkGetPhysicalDeviceSurfacePresentModesKHR(
    debug_report_data*                          report_data,
    VkSurfaceKHR                                surface,
    uint32_t*                                   pPresentModeCount,
    VkPresentModeKHR*                           pPresentModes)
{
    UNUSED_PARAMETER(surface);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_array(report_data, "vkGetPhysicalDeviceSurfacePresentModesKHR", "pPresentModeCount", "pPresentModes", pPresentModeCount, pPresentModes, VK_TRUE, VK_FALSE, VK_FALSE);

    return skipCall;
}


static VkBool32 param_check_vkCreateSwapchainKHR(
    debug_report_data*                          report_data,
    const VkSwapchainCreateInfoKHR*             pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkSwapchainKHR*                             pSwapchain)
{
    UNUSED_PARAMETER(pAllocator);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_struct_type(report_data, "vkCreateSwapchainKHR", "pCreateInfo", "VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR", pCreateInfo, VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR, VK_TRUE);

    skipCall |= validate_required_pointer(report_data, "vkCreateSwapchainKHR", "pSwapchain", pSwapchain);

    return skipCall;
}

static VkBool32 param_check_vkGetSwapchainImagesKHR(
    debug_report_data*                          report_data,
    VkSwapchainKHR                              swapchain,
    uint32_t*                                   pSwapchainImageCount,
    VkImage*                                    pSwapchainImages)
{
    UNUSED_PARAMETER(swapchain);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_array(report_data, "vkGetSwapchainImagesKHR", "pSwapchainImageCount", "pSwapchainImages", pSwapchainImageCount, pSwapchainImages, VK_TRUE, VK_FALSE, VK_FALSE);

    return skipCall;
}

static VkBool32 param_check_vkAcquireNextImageKHR(
    debug_report_data*                          report_data,
    VkSwapchainKHR                              swapchain,
    uint64_t                                    timeout,
    VkSemaphore                                 semaphore,
    VkFence                                     fence,
    uint32_t*                                   pImageIndex)
{
    UNUSED_PARAMETER(swapchain);
    UNUSED_PARAMETER(timeout);
    UNUSED_PARAMETER(semaphore);
    UNUSED_PARAMETER(fence);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_required_pointer(report_data, "vkAcquireNextImageKHR", "pImageIndex", pImageIndex);

    return skipCall;
}

static VkBool32 param_check_vkQueuePresentKHR(
    debug_report_data*                          report_data,
    const VkPresentInfoKHR*                     pPresentInfo)
{
    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_struct_type(report_data, "vkQueuePresentKHR", "pPresentInfo", "VK_STRUCTURE_TYPE_PRESENT_INFO_KHR", pPresentInfo, VK_STRUCTURE_TYPE_PRESENT_INFO_KHR, VK_TRUE);

    return skipCall;
}


static VkBool32 param_check_vkGetPhysicalDeviceDisplayPropertiesKHR(
    debug_report_data*                          report_data,
    uint32_t*                                   pPropertyCount,
    VkDisplayPropertiesKHR*                     pProperties)
{
    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_array(report_data, "vkGetPhysicalDeviceDisplayPropertiesKHR", "pPropertyCount", "pProperties", pPropertyCount, pProperties, VK_TRUE, VK_FALSE, VK_FALSE);

    return skipCall;
}

static VkBool32 param_check_vkGetPhysicalDeviceDisplayPlanePropertiesKHR(
    debug_report_data*                          report_data,
    uint32_t*                                   pPropertyCount,
    VkDisplayPlanePropertiesKHR*                pProperties)
{
    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_array(report_data, "vkGetPhysicalDeviceDisplayPlanePropertiesKHR", "pPropertyCount", "pProperties", pPropertyCount, pProperties, VK_TRUE, VK_FALSE, VK_FALSE);

    return skipCall;
}

static VkBool32 param_check_vkGetDisplayPlaneSupportedDisplaysKHR(
    debug_report_data*                          report_data,
    uint32_t                                    planeIndex,
    uint32_t*                                   pDisplayCount,
    VkDisplayKHR*                               pDisplays)
{
    UNUSED_PARAMETER(planeIndex);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_array(report_data, "vkGetDisplayPlaneSupportedDisplaysKHR", "pDisplayCount", "pDisplays", pDisplayCount, pDisplays, VK_TRUE, VK_FALSE, VK_FALSE);

    return skipCall;
}

static VkBool32 param_check_vkGetDisplayModePropertiesKHR(
    debug_report_data*                          report_data,
    VkDisplayKHR                                display,
    uint32_t*                                   pPropertyCount,
    VkDisplayModePropertiesKHR*                 pProperties)
{
    UNUSED_PARAMETER(display);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_array(report_data, "vkGetDisplayModePropertiesKHR", "pPropertyCount", "pProperties", pPropertyCount, pProperties, VK_TRUE, VK_FALSE, VK_FALSE);

    return skipCall;
}

static VkBool32 param_check_vkCreateDisplayModeKHR(
    debug_report_data*                          report_data,
    VkDisplayKHR                                display,
    const VkDisplayModeCreateInfoKHR*           pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkDisplayModeKHR*                           pMode)
{
    UNUSED_PARAMETER(display);
    UNUSED_PARAMETER(pAllocator);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_struct_type(report_data, "vkCreateDisplayModeKHR", "pCreateInfo", "VK_STRUCTURE_TYPE_DISPLAY_MODE_CREATE_INFO_KHR", pCreateInfo, VK_STRUCTURE_TYPE_DISPLAY_MODE_CREATE_INFO_KHR, VK_TRUE);

    skipCall |= validate_required_pointer(report_data, "vkCreateDisplayModeKHR", "pMode", pMode);

    return skipCall;
}

static VkBool32 param_check_vkGetDisplayPlaneCapabilitiesKHR(
    debug_report_data*                          report_data,
    VkDisplayModeKHR                            mode,
    uint32_t                                    planeIndex,
    VkDisplayPlaneCapabilitiesKHR*              pCapabilities)
{
    UNUSED_PARAMETER(mode);
    UNUSED_PARAMETER(planeIndex);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_required_pointer(report_data, "vkGetDisplayPlaneCapabilitiesKHR", "pCapabilities", pCapabilities);

    return skipCall;
}

static VkBool32 param_check_vkCreateDisplayPlaneSurfaceKHR(
    debug_report_data*                          report_data,
    const VkDisplaySurfaceCreateInfoKHR*        pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkSurfaceKHR*                               pSurface)
{
    UNUSED_PARAMETER(pAllocator);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_struct_type(report_data, "vkCreateDisplayPlaneSurfaceKHR", "pCreateInfo", "VK_STRUCTURE_TYPE_DISPLAY_SURFACE_CREATE_INFO_KHR", pCreateInfo, VK_STRUCTURE_TYPE_DISPLAY_SURFACE_CREATE_INFO_KHR, VK_TRUE);

    skipCall |= validate_required_pointer(report_data, "vkCreateDisplayPlaneSurfaceKHR", "pSurface", pSurface);

    return skipCall;
}


static VkBool32 param_check_vkCreateSharedSwapchainsKHR(
    debug_report_data*                          report_data,
    uint32_t                                    swapchainCount,
    const VkSwapchainCreateInfoKHR*             pCreateInfos,
    const VkAllocationCallbacks*                pAllocator,
    VkSwapchainKHR*                             pSwapchains)
{
    UNUSED_PARAMETER(pAllocator);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_array(report_data, "vkCreateSharedSwapchainsKHR", "swapchainCount", "pCreateInfos", swapchainCount, pCreateInfos, VK_TRUE, VK_TRUE);

    skipCall |= validate_array(report_data, "vkCreateSharedSwapchainsKHR", "swapchainCount", "pSwapchains", swapchainCount, pSwapchains, VK_TRUE, VK_TRUE);

    return skipCall;
}


#ifdef VK_USE_PLATFORM_XLIB_KHR
static VkBool32 param_check_vkCreateXlibSurfaceKHR(
    debug_report_data*                          report_data,
    const VkXlibSurfaceCreateInfoKHR*           pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkSurfaceKHR*                               pSurface)
{
    UNUSED_PARAMETER(pAllocator);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_struct_type(report_data, "vkCreateXlibSurfaceKHR", "pCreateInfo", "VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR", pCreateInfo, VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR, VK_TRUE);

    skipCall |= validate_required_pointer(report_data, "vkCreateXlibSurfaceKHR", "pSurface", pSurface);

    return skipCall;
}

static VkBool32 param_check_vkGetPhysicalDeviceXlibPresentationSupportKHR(
    debug_report_data*                          report_data,
    uint32_t                                    queueFamilyIndex,
    Display*                                    dpy,
    VisualID                                    visualID)
{
    UNUSED_PARAMETER(queueFamilyIndex);
    UNUSED_PARAMETER(visualID);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_required_pointer(report_data, "vkGetPhysicalDeviceXlibPresentationSupportKHR", "dpy", dpy);

    return skipCall;
}
#endif /* VK_USE_PLATFORM_XLIB_KHR */

#ifdef VK_USE_PLATFORM_XCB_KHR
static VkBool32 param_check_vkCreateXcbSurfaceKHR(
    debug_report_data*                          report_data,
    const VkXcbSurfaceCreateInfoKHR*            pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkSurfaceKHR*                               pSurface)
{
    UNUSED_PARAMETER(pAllocator);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_struct_type(report_data, "vkCreateXcbSurfaceKHR", "pCreateInfo", "VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR", pCreateInfo, VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR, VK_TRUE);

    skipCall |= validate_required_pointer(report_data, "vkCreateXcbSurfaceKHR", "pSurface", pSurface);

    return skipCall;
}

static VkBool32 param_check_vkGetPhysicalDeviceXcbPresentationSupportKHR(
    debug_report_data*                          report_data,
    uint32_t                                    queueFamilyIndex,
    xcb_connection_t*                           connection,
    xcb_visualid_t                              visual_id)
{
    UNUSED_PARAMETER(queueFamilyIndex);
    UNUSED_PARAMETER(visual_id);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_required_pointer(report_data, "vkGetPhysicalDeviceXcbPresentationSupportKHR", "connection", connection);

    return skipCall;
}
#endif /* VK_USE_PLATFORM_XCB_KHR */

#ifdef VK_USE_PLATFORM_WAYLAND_KHR
static VkBool32 param_check_vkCreateWaylandSurfaceKHR(
    debug_report_data*                          report_data,
    const VkWaylandSurfaceCreateInfoKHR*        pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkSurfaceKHR*                               pSurface)
{
    UNUSED_PARAMETER(pAllocator);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_struct_type(report_data, "vkCreateWaylandSurfaceKHR", "pCreateInfo", "VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR", pCreateInfo, VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR, VK_TRUE);

    skipCall |= validate_required_pointer(report_data, "vkCreateWaylandSurfaceKHR", "pSurface", pSurface);

    return skipCall;
}

static VkBool32 param_check_vkGetPhysicalDeviceWaylandPresentationSupportKHR(
    debug_report_data*                          report_data,
    uint32_t                                    queueFamilyIndex,
    struct wl_display*                          display)
{
    UNUSED_PARAMETER(queueFamilyIndex);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_required_pointer(report_data, "vkGetPhysicalDeviceWaylandPresentationSupportKHR", "display", display);

    return skipCall;
}
#endif /* VK_USE_PLATFORM_WAYLAND_KHR */

#ifdef VK_USE_PLATFORM_MIR_KHR
static VkBool32 param_check_vkCreateMirSurfaceKHR(
    debug_report_data*                          report_data,
    const VkMirSurfaceCreateInfoKHR*            pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkSurfaceKHR*                               pSurface)
{
    UNUSED_PARAMETER(pAllocator);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_struct_type(report_data, "vkCreateMirSurfaceKHR", "pCreateInfo", "VK_STRUCTURE_TYPE_MIR_SURFACE_CREATE_INFO_KHR", pCreateInfo, VK_STRUCTURE_TYPE_MIR_SURFACE_CREATE_INFO_KHR, VK_TRUE);

    skipCall |= validate_required_pointer(report_data, "vkCreateMirSurfaceKHR", "pSurface", pSurface);

    return skipCall;
}

static VkBool32 param_check_vkGetPhysicalDeviceMirPresentationSupportKHR(
    debug_report_data*                          report_data,
    uint32_t                                    queueFamilyIndex,
    MirConnection*                              connection)
{
    UNUSED_PARAMETER(queueFamilyIndex);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_required_pointer(report_data, "vkGetPhysicalDeviceMirPresentationSupportKHR", "connection", connection);

    return skipCall;
}
#endif /* VK_USE_PLATFORM_MIR_KHR */

#ifdef VK_USE_PLATFORM_ANDROID_KHR
static VkBool32 param_check_vkCreateAndroidSurfaceKHR(
    debug_report_data*                          report_data,
    const VkAndroidSurfaceCreateInfoKHR*        pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkSurfaceKHR*                               pSurface)
{
    UNUSED_PARAMETER(pAllocator);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_struct_type(report_data, "vkCreateAndroidSurfaceKHR", "pCreateInfo", "VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR", pCreateInfo, VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR, VK_TRUE);

    skipCall |= validate_required_pointer(report_data, "vkCreateAndroidSurfaceKHR", "pSurface", pSurface);

    return skipCall;
}
#endif /* VK_USE_PLATFORM_ANDROID_KHR */

#ifdef VK_USE_PLATFORM_WIN32_KHR
static VkBool32 param_check_vkCreateWin32SurfaceKHR(
    debug_report_data*                          report_data,
    const VkWin32SurfaceCreateInfoKHR*          pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkSurfaceKHR*                               pSurface)
{
    UNUSED_PARAMETER(pAllocator);

    VkBool32 skipCall = VK_FALSE;

    skipCall |= validate_struct_type(report_data, "vkCreateWin32SurfaceKHR", "pCreateInfo", "VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR", pCreateInfo, VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR, VK_TRUE);

    skipCall |= validate_required_pointer(report_data, "vkCreateWin32SurfaceKHR", "pSurface", pSurface);

    return skipCall;
}
#endif /* VK_USE_PLATFORM_WIN32_KHR */




#endif
