/* THIS FILE IS GENERATED.  DO NOT EDIT. */

/*
 * Vulkan
 *
 * Copyright (c) 2015-2016 The Khronos Group Inc.
 * Copyright (c) 2015-2016 Valve Corporation.
 * Copyright (c) 2015-2016 LunarG, Inc.
 * Copyright (c) 2015-2016 Google Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Materials"),
 * to deal in the Materials without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Materials, and to permit persons to whom the
 * Materials is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
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
 * Author: Courtney Goeltzenleuchter <courtney@LunarG.com>
 * Author: Tobin Ehlis <tobin@lunarg.com>
 */
//#includes, #defines, globals and such...
#include "vk_safe_struct.h"
safe_VkApplicationInfo::safe_VkApplicationInfo(const VkApplicationInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	pApplicationName(pInStruct->pApplicationName),
	applicationVersion(pInStruct->applicationVersion),
	pEngineName(pInStruct->pEngineName),
	engineVersion(pInStruct->engineVersion),
	apiVersion(pInStruct->apiVersion)
{
}

safe_VkApplicationInfo::safe_VkApplicationInfo() {}

safe_VkApplicationInfo::~safe_VkApplicationInfo()
{
}

void safe_VkApplicationInfo::initialize(const VkApplicationInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    pApplicationName = pInStruct->pApplicationName;
    applicationVersion = pInStruct->applicationVersion;
    pEngineName = pInStruct->pEngineName;
    engineVersion = pInStruct->engineVersion;
    apiVersion = pInStruct->apiVersion;
}

safe_VkInstanceCreateInfo::safe_VkInstanceCreateInfo(const VkInstanceCreateInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	enabledLayerCount(pInStruct->enabledLayerCount),
	ppEnabledLayerNames(pInStruct->ppEnabledLayerNames),
	enabledExtensionCount(pInStruct->enabledExtensionCount),
	ppEnabledExtensionNames(pInStruct->ppEnabledExtensionNames)
{
    if (pInStruct->pApplicationInfo)
        pApplicationInfo = new safe_VkApplicationInfo(pInStruct->pApplicationInfo);
    else
        pApplicationInfo = NULL;
}

safe_VkInstanceCreateInfo::safe_VkInstanceCreateInfo() {}

safe_VkInstanceCreateInfo::~safe_VkInstanceCreateInfo()
{
    if (pApplicationInfo)
        delete pApplicationInfo;
}

void safe_VkInstanceCreateInfo::initialize(const VkInstanceCreateInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    enabledLayerCount = pInStruct->enabledLayerCount;
    ppEnabledLayerNames = pInStruct->ppEnabledLayerNames;
    enabledExtensionCount = pInStruct->enabledExtensionCount;
    ppEnabledExtensionNames = pInStruct->ppEnabledExtensionNames;
    if (pInStruct->pApplicationInfo)
        pApplicationInfo = new safe_VkApplicationInfo(pInStruct->pApplicationInfo);
    else
        pApplicationInfo = NULL;
}

safe_VkAllocationCallbacks::safe_VkAllocationCallbacks(const VkAllocationCallbacks* pInStruct) : 
	pUserData(pInStruct->pUserData),
	pfnAllocation(pInStruct->pfnAllocation),
	pfnReallocation(pInStruct->pfnReallocation),
	pfnFree(pInStruct->pfnFree),
	pfnInternalAllocation(pInStruct->pfnInternalAllocation),
	pfnInternalFree(pInStruct->pfnInternalFree)
{
}

safe_VkAllocationCallbacks::safe_VkAllocationCallbacks() {}

safe_VkAllocationCallbacks::~safe_VkAllocationCallbacks()
{
}

void safe_VkAllocationCallbacks::initialize(const VkAllocationCallbacks* pInStruct)
{
    pUserData = pInStruct->pUserData;
    pfnAllocation = pInStruct->pfnAllocation;
    pfnReallocation = pInStruct->pfnReallocation;
    pfnFree = pInStruct->pfnFree;
    pfnInternalAllocation = pInStruct->pfnInternalAllocation;
    pfnInternalFree = pInStruct->pfnInternalFree;
}

safe_VkDeviceQueueCreateInfo::safe_VkDeviceQueueCreateInfo(const VkDeviceQueueCreateInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	queueFamilyIndex(pInStruct->queueFamilyIndex),
	queueCount(pInStruct->queueCount),
	pQueuePriorities(pInStruct->pQueuePriorities)
{
}

safe_VkDeviceQueueCreateInfo::safe_VkDeviceQueueCreateInfo() {}

safe_VkDeviceQueueCreateInfo::~safe_VkDeviceQueueCreateInfo()
{
}

void safe_VkDeviceQueueCreateInfo::initialize(const VkDeviceQueueCreateInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    queueFamilyIndex = pInStruct->queueFamilyIndex;
    queueCount = pInStruct->queueCount;
    pQueuePriorities = pInStruct->pQueuePriorities;
}

safe_VkDeviceCreateInfo::safe_VkDeviceCreateInfo(const VkDeviceCreateInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	queueCreateInfoCount(pInStruct->queueCreateInfoCount),
	pQueueCreateInfos(NULL),
	enabledLayerCount(pInStruct->enabledLayerCount),
	ppEnabledLayerNames(pInStruct->ppEnabledLayerNames),
	enabledExtensionCount(pInStruct->enabledExtensionCount),
	ppEnabledExtensionNames(pInStruct->ppEnabledExtensionNames),
	pEnabledFeatures(pInStruct->pEnabledFeatures)
{
    if (queueCreateInfoCount && pInStruct->pQueueCreateInfos) {
        pQueueCreateInfos = new safe_VkDeviceQueueCreateInfo[queueCreateInfoCount];
        for (uint32_t i=0; i<queueCreateInfoCount; ++i) {
            pQueueCreateInfos[i].initialize(&pInStruct->pQueueCreateInfos[i]);
        }
    }
}

safe_VkDeviceCreateInfo::safe_VkDeviceCreateInfo() {}

safe_VkDeviceCreateInfo::~safe_VkDeviceCreateInfo()
{
    if (pQueueCreateInfos)
        delete[] pQueueCreateInfos;
}

void safe_VkDeviceCreateInfo::initialize(const VkDeviceCreateInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    queueCreateInfoCount = pInStruct->queueCreateInfoCount;
    pQueueCreateInfos = NULL;
    enabledLayerCount = pInStruct->enabledLayerCount;
    ppEnabledLayerNames = pInStruct->ppEnabledLayerNames;
    enabledExtensionCount = pInStruct->enabledExtensionCount;
    ppEnabledExtensionNames = pInStruct->ppEnabledExtensionNames;
    pEnabledFeatures = pInStruct->pEnabledFeatures;
    if (queueCreateInfoCount && pInStruct->pQueueCreateInfos) {
        pQueueCreateInfos = new safe_VkDeviceQueueCreateInfo[queueCreateInfoCount];
        for (uint32_t i=0; i<queueCreateInfoCount; ++i) {
            pQueueCreateInfos[i].initialize(&pInStruct->pQueueCreateInfos[i]);
        }
    }
}

safe_VkSubmitInfo::safe_VkSubmitInfo(const VkSubmitInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	waitSemaphoreCount(pInStruct->waitSemaphoreCount),
	pWaitSemaphores(NULL),
	pWaitDstStageMask(pInStruct->pWaitDstStageMask),
	commandBufferCount(pInStruct->commandBufferCount),
	pCommandBuffers(pInStruct->pCommandBuffers),
	signalSemaphoreCount(pInStruct->signalSemaphoreCount),
	pSignalSemaphores(NULL)
{
    if (waitSemaphoreCount && pInStruct->pWaitSemaphores) {
        pWaitSemaphores = new VkSemaphore[waitSemaphoreCount];
        for (uint32_t i=0; i<waitSemaphoreCount; ++i) {
            pWaitSemaphores[i] = pInStruct->pWaitSemaphores[i];
        }
    }
    if (signalSemaphoreCount && pInStruct->pSignalSemaphores) {
        pSignalSemaphores = new VkSemaphore[signalSemaphoreCount];
        for (uint32_t i=0; i<signalSemaphoreCount; ++i) {
            pSignalSemaphores[i] = pInStruct->pSignalSemaphores[i];
        }
    }
}

safe_VkSubmitInfo::safe_VkSubmitInfo() {}

safe_VkSubmitInfo::~safe_VkSubmitInfo()
{
    if (pWaitSemaphores)
        delete[] pWaitSemaphores;
    if (pSignalSemaphores)
        delete[] pSignalSemaphores;
}

void safe_VkSubmitInfo::initialize(const VkSubmitInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    waitSemaphoreCount = pInStruct->waitSemaphoreCount;
    pWaitSemaphores = NULL;
    pWaitDstStageMask = pInStruct->pWaitDstStageMask;
    commandBufferCount = pInStruct->commandBufferCount;
    pCommandBuffers = pInStruct->pCommandBuffers;
    signalSemaphoreCount = pInStruct->signalSemaphoreCount;
    pSignalSemaphores = NULL;
    if (waitSemaphoreCount && pInStruct->pWaitSemaphores) {
        pWaitSemaphores = new VkSemaphore[waitSemaphoreCount];
        for (uint32_t i=0; i<waitSemaphoreCount; ++i) {
            pWaitSemaphores[i] = pInStruct->pWaitSemaphores[i];
        }
    }
    if (signalSemaphoreCount && pInStruct->pSignalSemaphores) {
        pSignalSemaphores = new VkSemaphore[signalSemaphoreCount];
        for (uint32_t i=0; i<signalSemaphoreCount; ++i) {
            pSignalSemaphores[i] = pInStruct->pSignalSemaphores[i];
        }
    }
}

safe_VkMemoryAllocateInfo::safe_VkMemoryAllocateInfo(const VkMemoryAllocateInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	allocationSize(pInStruct->allocationSize),
	memoryTypeIndex(pInStruct->memoryTypeIndex)
{
}

safe_VkMemoryAllocateInfo::safe_VkMemoryAllocateInfo() {}

safe_VkMemoryAllocateInfo::~safe_VkMemoryAllocateInfo()
{
}

void safe_VkMemoryAllocateInfo::initialize(const VkMemoryAllocateInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    allocationSize = pInStruct->allocationSize;
    memoryTypeIndex = pInStruct->memoryTypeIndex;
}

safe_VkMappedMemoryRange::safe_VkMappedMemoryRange(const VkMappedMemoryRange* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	memory(pInStruct->memory),
	offset(pInStruct->offset),
	size(pInStruct->size)
{
}

safe_VkMappedMemoryRange::safe_VkMappedMemoryRange() {}

safe_VkMappedMemoryRange::~safe_VkMappedMemoryRange()
{
}

void safe_VkMappedMemoryRange::initialize(const VkMappedMemoryRange* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    memory = pInStruct->memory;
    offset = pInStruct->offset;
    size = pInStruct->size;
}

safe_VkSparseBufferMemoryBindInfo::safe_VkSparseBufferMemoryBindInfo(const VkSparseBufferMemoryBindInfo* pInStruct) : 
	buffer(pInStruct->buffer),
	bindCount(pInStruct->bindCount),
	pBinds(NULL)
{
    if (bindCount && pInStruct->pBinds) {
        pBinds = new VkSparseMemoryBind[bindCount];
        for (uint32_t i=0; i<bindCount; ++i) {
            pBinds[i] = pInStruct->pBinds[i];
        }
    }
}

safe_VkSparseBufferMemoryBindInfo::safe_VkSparseBufferMemoryBindInfo() {}

safe_VkSparseBufferMemoryBindInfo::~safe_VkSparseBufferMemoryBindInfo()
{
    if (pBinds)
        delete[] pBinds;
}

void safe_VkSparseBufferMemoryBindInfo::initialize(const VkSparseBufferMemoryBindInfo* pInStruct)
{
    buffer = pInStruct->buffer;
    bindCount = pInStruct->bindCount;
    pBinds = NULL;
    if (bindCount && pInStruct->pBinds) {
        pBinds = new VkSparseMemoryBind[bindCount];
        for (uint32_t i=0; i<bindCount; ++i) {
            pBinds[i] = pInStruct->pBinds[i];
        }
    }
}

safe_VkSparseImageOpaqueMemoryBindInfo::safe_VkSparseImageOpaqueMemoryBindInfo(const VkSparseImageOpaqueMemoryBindInfo* pInStruct) : 
	image(pInStruct->image),
	bindCount(pInStruct->bindCount),
	pBinds(NULL)
{
    if (bindCount && pInStruct->pBinds) {
        pBinds = new VkSparseMemoryBind[bindCount];
        for (uint32_t i=0; i<bindCount; ++i) {
            pBinds[i] = pInStruct->pBinds[i];
        }
    }
}

safe_VkSparseImageOpaqueMemoryBindInfo::safe_VkSparseImageOpaqueMemoryBindInfo() {}

safe_VkSparseImageOpaqueMemoryBindInfo::~safe_VkSparseImageOpaqueMemoryBindInfo()
{
    if (pBinds)
        delete[] pBinds;
}

void safe_VkSparseImageOpaqueMemoryBindInfo::initialize(const VkSparseImageOpaqueMemoryBindInfo* pInStruct)
{
    image = pInStruct->image;
    bindCount = pInStruct->bindCount;
    pBinds = NULL;
    if (bindCount && pInStruct->pBinds) {
        pBinds = new VkSparseMemoryBind[bindCount];
        for (uint32_t i=0; i<bindCount; ++i) {
            pBinds[i] = pInStruct->pBinds[i];
        }
    }
}

safe_VkSparseImageMemoryBindInfo::safe_VkSparseImageMemoryBindInfo(const VkSparseImageMemoryBindInfo* pInStruct) : 
	image(pInStruct->image),
	bindCount(pInStruct->bindCount),
	pBinds(NULL)
{
    if (bindCount && pInStruct->pBinds) {
        pBinds = new VkSparseImageMemoryBind[bindCount];
        for (uint32_t i=0; i<bindCount; ++i) {
            pBinds[i] = pInStruct->pBinds[i];
        }
    }
}

safe_VkSparseImageMemoryBindInfo::safe_VkSparseImageMemoryBindInfo() {}

safe_VkSparseImageMemoryBindInfo::~safe_VkSparseImageMemoryBindInfo()
{
    if (pBinds)
        delete[] pBinds;
}

void safe_VkSparseImageMemoryBindInfo::initialize(const VkSparseImageMemoryBindInfo* pInStruct)
{
    image = pInStruct->image;
    bindCount = pInStruct->bindCount;
    pBinds = NULL;
    if (bindCount && pInStruct->pBinds) {
        pBinds = new VkSparseImageMemoryBind[bindCount];
        for (uint32_t i=0; i<bindCount; ++i) {
            pBinds[i] = pInStruct->pBinds[i];
        }
    }
}

safe_VkBindSparseInfo::safe_VkBindSparseInfo(const VkBindSparseInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	waitSemaphoreCount(pInStruct->waitSemaphoreCount),
	pWaitSemaphores(NULL),
	bufferBindCount(pInStruct->bufferBindCount),
	pBufferBinds(NULL),
	imageOpaqueBindCount(pInStruct->imageOpaqueBindCount),
	pImageOpaqueBinds(NULL),
	imageBindCount(pInStruct->imageBindCount),
	pImageBinds(NULL),
	signalSemaphoreCount(pInStruct->signalSemaphoreCount),
	pSignalSemaphores(NULL)
{
    if (waitSemaphoreCount && pInStruct->pWaitSemaphores) {
        pWaitSemaphores = new VkSemaphore[waitSemaphoreCount];
        for (uint32_t i=0; i<waitSemaphoreCount; ++i) {
            pWaitSemaphores[i] = pInStruct->pWaitSemaphores[i];
        }
    }
    if (bufferBindCount && pInStruct->pBufferBinds) {
        pBufferBinds = new safe_VkSparseBufferMemoryBindInfo[bufferBindCount];
        for (uint32_t i=0; i<bufferBindCount; ++i) {
            pBufferBinds[i].initialize(&pInStruct->pBufferBinds[i]);
        }
    }
    if (imageOpaqueBindCount && pInStruct->pImageOpaqueBinds) {
        pImageOpaqueBinds = new safe_VkSparseImageOpaqueMemoryBindInfo[imageOpaqueBindCount];
        for (uint32_t i=0; i<imageOpaqueBindCount; ++i) {
            pImageOpaqueBinds[i].initialize(&pInStruct->pImageOpaqueBinds[i]);
        }
    }
    if (imageBindCount && pInStruct->pImageBinds) {
        pImageBinds = new safe_VkSparseImageMemoryBindInfo[imageBindCount];
        for (uint32_t i=0; i<imageBindCount; ++i) {
            pImageBinds[i].initialize(&pInStruct->pImageBinds[i]);
        }
    }
    if (signalSemaphoreCount && pInStruct->pSignalSemaphores) {
        pSignalSemaphores = new VkSemaphore[signalSemaphoreCount];
        for (uint32_t i=0; i<signalSemaphoreCount; ++i) {
            pSignalSemaphores[i] = pInStruct->pSignalSemaphores[i];
        }
    }
}

safe_VkBindSparseInfo::safe_VkBindSparseInfo() {}

safe_VkBindSparseInfo::~safe_VkBindSparseInfo()
{
    if (pWaitSemaphores)
        delete[] pWaitSemaphores;
    if (pBufferBinds)
        delete[] pBufferBinds;
    if (pImageOpaqueBinds)
        delete[] pImageOpaqueBinds;
    if (pImageBinds)
        delete[] pImageBinds;
    if (pSignalSemaphores)
        delete[] pSignalSemaphores;
}

void safe_VkBindSparseInfo::initialize(const VkBindSparseInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    waitSemaphoreCount = pInStruct->waitSemaphoreCount;
    pWaitSemaphores = NULL;
    bufferBindCount = pInStruct->bufferBindCount;
    pBufferBinds = NULL;
    imageOpaqueBindCount = pInStruct->imageOpaqueBindCount;
    pImageOpaqueBinds = NULL;
    imageBindCount = pInStruct->imageBindCount;
    pImageBinds = NULL;
    signalSemaphoreCount = pInStruct->signalSemaphoreCount;
    pSignalSemaphores = NULL;
    if (waitSemaphoreCount && pInStruct->pWaitSemaphores) {
        pWaitSemaphores = new VkSemaphore[waitSemaphoreCount];
        for (uint32_t i=0; i<waitSemaphoreCount; ++i) {
            pWaitSemaphores[i] = pInStruct->pWaitSemaphores[i];
        }
    }
    if (bufferBindCount && pInStruct->pBufferBinds) {
        pBufferBinds = new safe_VkSparseBufferMemoryBindInfo[bufferBindCount];
        for (uint32_t i=0; i<bufferBindCount; ++i) {
            pBufferBinds[i].initialize(&pInStruct->pBufferBinds[i]);
        }
    }
    if (imageOpaqueBindCount && pInStruct->pImageOpaqueBinds) {
        pImageOpaqueBinds = new safe_VkSparseImageOpaqueMemoryBindInfo[imageOpaqueBindCount];
        for (uint32_t i=0; i<imageOpaqueBindCount; ++i) {
            pImageOpaqueBinds[i].initialize(&pInStruct->pImageOpaqueBinds[i]);
        }
    }
    if (imageBindCount && pInStruct->pImageBinds) {
        pImageBinds = new safe_VkSparseImageMemoryBindInfo[imageBindCount];
        for (uint32_t i=0; i<imageBindCount; ++i) {
            pImageBinds[i].initialize(&pInStruct->pImageBinds[i]);
        }
    }
    if (signalSemaphoreCount && pInStruct->pSignalSemaphores) {
        pSignalSemaphores = new VkSemaphore[signalSemaphoreCount];
        for (uint32_t i=0; i<signalSemaphoreCount; ++i) {
            pSignalSemaphores[i] = pInStruct->pSignalSemaphores[i];
        }
    }
}

safe_VkFenceCreateInfo::safe_VkFenceCreateInfo(const VkFenceCreateInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags)
{
}

safe_VkFenceCreateInfo::safe_VkFenceCreateInfo() {}

safe_VkFenceCreateInfo::~safe_VkFenceCreateInfo()
{
}

void safe_VkFenceCreateInfo::initialize(const VkFenceCreateInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
}

safe_VkSemaphoreCreateInfo::safe_VkSemaphoreCreateInfo(const VkSemaphoreCreateInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags)
{
}

safe_VkSemaphoreCreateInfo::safe_VkSemaphoreCreateInfo() {}

safe_VkSemaphoreCreateInfo::~safe_VkSemaphoreCreateInfo()
{
}

void safe_VkSemaphoreCreateInfo::initialize(const VkSemaphoreCreateInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
}

safe_VkEventCreateInfo::safe_VkEventCreateInfo(const VkEventCreateInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags)
{
}

safe_VkEventCreateInfo::safe_VkEventCreateInfo() {}

safe_VkEventCreateInfo::~safe_VkEventCreateInfo()
{
}

void safe_VkEventCreateInfo::initialize(const VkEventCreateInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
}

safe_VkQueryPoolCreateInfo::safe_VkQueryPoolCreateInfo(const VkQueryPoolCreateInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	queryType(pInStruct->queryType),
	queryCount(pInStruct->queryCount),
	pipelineStatistics(pInStruct->pipelineStatistics)
{
}

safe_VkQueryPoolCreateInfo::safe_VkQueryPoolCreateInfo() {}

safe_VkQueryPoolCreateInfo::~safe_VkQueryPoolCreateInfo()
{
}

void safe_VkQueryPoolCreateInfo::initialize(const VkQueryPoolCreateInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    queryType = pInStruct->queryType;
    queryCount = pInStruct->queryCount;
    pipelineStatistics = pInStruct->pipelineStatistics;
}

safe_VkBufferCreateInfo::safe_VkBufferCreateInfo(const VkBufferCreateInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	size(pInStruct->size),
	usage(pInStruct->usage),
	sharingMode(pInStruct->sharingMode),
	queueFamilyIndexCount(pInStruct->queueFamilyIndexCount),
	pQueueFamilyIndices(pInStruct->pQueueFamilyIndices)
{
}

safe_VkBufferCreateInfo::safe_VkBufferCreateInfo() {}

safe_VkBufferCreateInfo::~safe_VkBufferCreateInfo()
{
}

void safe_VkBufferCreateInfo::initialize(const VkBufferCreateInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    size = pInStruct->size;
    usage = pInStruct->usage;
    sharingMode = pInStruct->sharingMode;
    queueFamilyIndexCount = pInStruct->queueFamilyIndexCount;
    pQueueFamilyIndices = pInStruct->pQueueFamilyIndices;
}

safe_VkBufferViewCreateInfo::safe_VkBufferViewCreateInfo(const VkBufferViewCreateInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	buffer(pInStruct->buffer),
	format(pInStruct->format),
	offset(pInStruct->offset),
	range(pInStruct->range)
{
}

safe_VkBufferViewCreateInfo::safe_VkBufferViewCreateInfo() {}

safe_VkBufferViewCreateInfo::~safe_VkBufferViewCreateInfo()
{
}

void safe_VkBufferViewCreateInfo::initialize(const VkBufferViewCreateInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    buffer = pInStruct->buffer;
    format = pInStruct->format;
    offset = pInStruct->offset;
    range = pInStruct->range;
}

safe_VkImageCreateInfo::safe_VkImageCreateInfo(const VkImageCreateInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	imageType(pInStruct->imageType),
	format(pInStruct->format),
	extent(pInStruct->extent),
	mipLevels(pInStruct->mipLevels),
	arrayLayers(pInStruct->arrayLayers),
	samples(pInStruct->samples),
	tiling(pInStruct->tiling),
	usage(pInStruct->usage),
	sharingMode(pInStruct->sharingMode),
	queueFamilyIndexCount(pInStruct->queueFamilyIndexCount),
	pQueueFamilyIndices(pInStruct->pQueueFamilyIndices),
	initialLayout(pInStruct->initialLayout)
{
}

safe_VkImageCreateInfo::safe_VkImageCreateInfo() {}

safe_VkImageCreateInfo::~safe_VkImageCreateInfo()
{
}

void safe_VkImageCreateInfo::initialize(const VkImageCreateInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    imageType = pInStruct->imageType;
    format = pInStruct->format;
    extent = pInStruct->extent;
    mipLevels = pInStruct->mipLevels;
    arrayLayers = pInStruct->arrayLayers;
    samples = pInStruct->samples;
    tiling = pInStruct->tiling;
    usage = pInStruct->usage;
    sharingMode = pInStruct->sharingMode;
    queueFamilyIndexCount = pInStruct->queueFamilyIndexCount;
    pQueueFamilyIndices = pInStruct->pQueueFamilyIndices;
    initialLayout = pInStruct->initialLayout;
}

safe_VkImageViewCreateInfo::safe_VkImageViewCreateInfo(const VkImageViewCreateInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	image(pInStruct->image),
	viewType(pInStruct->viewType),
	format(pInStruct->format),
	components(pInStruct->components),
	subresourceRange(pInStruct->subresourceRange)
{
}

safe_VkImageViewCreateInfo::safe_VkImageViewCreateInfo() {}

safe_VkImageViewCreateInfo::~safe_VkImageViewCreateInfo()
{
}

void safe_VkImageViewCreateInfo::initialize(const VkImageViewCreateInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    image = pInStruct->image;
    viewType = pInStruct->viewType;
    format = pInStruct->format;
    components = pInStruct->components;
    subresourceRange = pInStruct->subresourceRange;
}

safe_VkShaderModuleCreateInfo::safe_VkShaderModuleCreateInfo(const VkShaderModuleCreateInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	codeSize(pInStruct->codeSize),
	pCode(pInStruct->pCode)
{
}

safe_VkShaderModuleCreateInfo::safe_VkShaderModuleCreateInfo() {}

safe_VkShaderModuleCreateInfo::~safe_VkShaderModuleCreateInfo()
{
}

void safe_VkShaderModuleCreateInfo::initialize(const VkShaderModuleCreateInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    codeSize = pInStruct->codeSize;
    pCode = pInStruct->pCode;
}

safe_VkPipelineCacheCreateInfo::safe_VkPipelineCacheCreateInfo(const VkPipelineCacheCreateInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	initialDataSize(pInStruct->initialDataSize),
	pInitialData(pInStruct->pInitialData)
{
}

safe_VkPipelineCacheCreateInfo::safe_VkPipelineCacheCreateInfo() {}

safe_VkPipelineCacheCreateInfo::~safe_VkPipelineCacheCreateInfo()
{
}

void safe_VkPipelineCacheCreateInfo::initialize(const VkPipelineCacheCreateInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    initialDataSize = pInStruct->initialDataSize;
    pInitialData = pInStruct->pInitialData;
}

safe_VkSpecializationInfo::safe_VkSpecializationInfo(const VkSpecializationInfo* pInStruct) : 
	mapEntryCount(pInStruct->mapEntryCount),
	pMapEntries(pInStruct->pMapEntries),
	dataSize(pInStruct->dataSize),
	pData(pInStruct->pData)
{
}

safe_VkSpecializationInfo::safe_VkSpecializationInfo() {}

safe_VkSpecializationInfo::~safe_VkSpecializationInfo()
{
}

void safe_VkSpecializationInfo::initialize(const VkSpecializationInfo* pInStruct)
{
    mapEntryCount = pInStruct->mapEntryCount;
    pMapEntries = pInStruct->pMapEntries;
    dataSize = pInStruct->dataSize;
    pData = pInStruct->pData;
}

safe_VkPipelineShaderStageCreateInfo::safe_VkPipelineShaderStageCreateInfo(const VkPipelineShaderStageCreateInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	stage(pInStruct->stage),
	module(pInStruct->module),
	pName(pInStruct->pName)
{
    if (pInStruct->pSpecializationInfo)
        pSpecializationInfo = new safe_VkSpecializationInfo(pInStruct->pSpecializationInfo);
    else
        pSpecializationInfo = NULL;
}

safe_VkPipelineShaderStageCreateInfo::safe_VkPipelineShaderStageCreateInfo() {}

safe_VkPipelineShaderStageCreateInfo::~safe_VkPipelineShaderStageCreateInfo()
{
    if (pSpecializationInfo)
        delete pSpecializationInfo;
}

void safe_VkPipelineShaderStageCreateInfo::initialize(const VkPipelineShaderStageCreateInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    stage = pInStruct->stage;
    module = pInStruct->module;
    pName = pInStruct->pName;
    if (pInStruct->pSpecializationInfo)
        pSpecializationInfo = new safe_VkSpecializationInfo(pInStruct->pSpecializationInfo);
    else
        pSpecializationInfo = NULL;
}

safe_VkPipelineVertexInputStateCreateInfo::safe_VkPipelineVertexInputStateCreateInfo(const VkPipelineVertexInputStateCreateInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	vertexBindingDescriptionCount(pInStruct->vertexBindingDescriptionCount),
	pVertexBindingDescriptions(pInStruct->pVertexBindingDescriptions),
	vertexAttributeDescriptionCount(pInStruct->vertexAttributeDescriptionCount),
	pVertexAttributeDescriptions(pInStruct->pVertexAttributeDescriptions)
{
}

safe_VkPipelineVertexInputStateCreateInfo::safe_VkPipelineVertexInputStateCreateInfo() {}

safe_VkPipelineVertexInputStateCreateInfo::~safe_VkPipelineVertexInputStateCreateInfo()
{
}

void safe_VkPipelineVertexInputStateCreateInfo::initialize(const VkPipelineVertexInputStateCreateInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    vertexBindingDescriptionCount = pInStruct->vertexBindingDescriptionCount;
    pVertexBindingDescriptions = pInStruct->pVertexBindingDescriptions;
    vertexAttributeDescriptionCount = pInStruct->vertexAttributeDescriptionCount;
    pVertexAttributeDescriptions = pInStruct->pVertexAttributeDescriptions;
}

safe_VkPipelineInputAssemblyStateCreateInfo::safe_VkPipelineInputAssemblyStateCreateInfo(const VkPipelineInputAssemblyStateCreateInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	topology(pInStruct->topology),
	primitiveRestartEnable(pInStruct->primitiveRestartEnable)
{
}

safe_VkPipelineInputAssemblyStateCreateInfo::safe_VkPipelineInputAssemblyStateCreateInfo() {}

safe_VkPipelineInputAssemblyStateCreateInfo::~safe_VkPipelineInputAssemblyStateCreateInfo()
{
}

void safe_VkPipelineInputAssemblyStateCreateInfo::initialize(const VkPipelineInputAssemblyStateCreateInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    topology = pInStruct->topology;
    primitiveRestartEnable = pInStruct->primitiveRestartEnable;
}

safe_VkPipelineTessellationStateCreateInfo::safe_VkPipelineTessellationStateCreateInfo(const VkPipelineTessellationStateCreateInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	patchControlPoints(pInStruct->patchControlPoints)
{
}

safe_VkPipelineTessellationStateCreateInfo::safe_VkPipelineTessellationStateCreateInfo() {}

safe_VkPipelineTessellationStateCreateInfo::~safe_VkPipelineTessellationStateCreateInfo()
{
}

void safe_VkPipelineTessellationStateCreateInfo::initialize(const VkPipelineTessellationStateCreateInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    patchControlPoints = pInStruct->patchControlPoints;
}

safe_VkPipelineViewportStateCreateInfo::safe_VkPipelineViewportStateCreateInfo(const VkPipelineViewportStateCreateInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	viewportCount(pInStruct->viewportCount),
	pViewports(pInStruct->pViewports),
	scissorCount(pInStruct->scissorCount),
	pScissors(pInStruct->pScissors)
{
}

safe_VkPipelineViewportStateCreateInfo::safe_VkPipelineViewportStateCreateInfo() {}

safe_VkPipelineViewportStateCreateInfo::~safe_VkPipelineViewportStateCreateInfo()
{
}

void safe_VkPipelineViewportStateCreateInfo::initialize(const VkPipelineViewportStateCreateInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    viewportCount = pInStruct->viewportCount;
    pViewports = pInStruct->pViewports;
    scissorCount = pInStruct->scissorCount;
    pScissors = pInStruct->pScissors;
}

safe_VkPipelineRasterizationStateCreateInfo::safe_VkPipelineRasterizationStateCreateInfo(const VkPipelineRasterizationStateCreateInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	depthClampEnable(pInStruct->depthClampEnable),
	rasterizerDiscardEnable(pInStruct->rasterizerDiscardEnable),
	polygonMode(pInStruct->polygonMode),
	cullMode(pInStruct->cullMode),
	frontFace(pInStruct->frontFace),
	depthBiasEnable(pInStruct->depthBiasEnable),
	depthBiasConstantFactor(pInStruct->depthBiasConstantFactor),
	depthBiasClamp(pInStruct->depthBiasClamp),
	depthBiasSlopeFactor(pInStruct->depthBiasSlopeFactor),
	lineWidth(pInStruct->lineWidth)
{
}

safe_VkPipelineRasterizationStateCreateInfo::safe_VkPipelineRasterizationStateCreateInfo() {}

safe_VkPipelineRasterizationStateCreateInfo::~safe_VkPipelineRasterizationStateCreateInfo()
{
}

void safe_VkPipelineRasterizationStateCreateInfo::initialize(const VkPipelineRasterizationStateCreateInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    depthClampEnable = pInStruct->depthClampEnable;
    rasterizerDiscardEnable = pInStruct->rasterizerDiscardEnable;
    polygonMode = pInStruct->polygonMode;
    cullMode = pInStruct->cullMode;
    frontFace = pInStruct->frontFace;
    depthBiasEnable = pInStruct->depthBiasEnable;
    depthBiasConstantFactor = pInStruct->depthBiasConstantFactor;
    depthBiasClamp = pInStruct->depthBiasClamp;
    depthBiasSlopeFactor = pInStruct->depthBiasSlopeFactor;
    lineWidth = pInStruct->lineWidth;
}

safe_VkPipelineMultisampleStateCreateInfo::safe_VkPipelineMultisampleStateCreateInfo(const VkPipelineMultisampleStateCreateInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	rasterizationSamples(pInStruct->rasterizationSamples),
	sampleShadingEnable(pInStruct->sampleShadingEnable),
	minSampleShading(pInStruct->minSampleShading),
	pSampleMask(pInStruct->pSampleMask),
	alphaToCoverageEnable(pInStruct->alphaToCoverageEnable),
	alphaToOneEnable(pInStruct->alphaToOneEnable)
{
}

safe_VkPipelineMultisampleStateCreateInfo::safe_VkPipelineMultisampleStateCreateInfo() {}

safe_VkPipelineMultisampleStateCreateInfo::~safe_VkPipelineMultisampleStateCreateInfo()
{
}

void safe_VkPipelineMultisampleStateCreateInfo::initialize(const VkPipelineMultisampleStateCreateInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    rasterizationSamples = pInStruct->rasterizationSamples;
    sampleShadingEnable = pInStruct->sampleShadingEnable;
    minSampleShading = pInStruct->minSampleShading;
    pSampleMask = pInStruct->pSampleMask;
    alphaToCoverageEnable = pInStruct->alphaToCoverageEnable;
    alphaToOneEnable = pInStruct->alphaToOneEnable;
}

safe_VkPipelineDepthStencilStateCreateInfo::safe_VkPipelineDepthStencilStateCreateInfo(const VkPipelineDepthStencilStateCreateInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	depthTestEnable(pInStruct->depthTestEnable),
	depthWriteEnable(pInStruct->depthWriteEnable),
	depthCompareOp(pInStruct->depthCompareOp),
	depthBoundsTestEnable(pInStruct->depthBoundsTestEnable),
	stencilTestEnable(pInStruct->stencilTestEnable),
	front(pInStruct->front),
	back(pInStruct->back),
	minDepthBounds(pInStruct->minDepthBounds),
	maxDepthBounds(pInStruct->maxDepthBounds)
{
}

safe_VkPipelineDepthStencilStateCreateInfo::safe_VkPipelineDepthStencilStateCreateInfo() {}

safe_VkPipelineDepthStencilStateCreateInfo::~safe_VkPipelineDepthStencilStateCreateInfo()
{
}

void safe_VkPipelineDepthStencilStateCreateInfo::initialize(const VkPipelineDepthStencilStateCreateInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    depthTestEnable = pInStruct->depthTestEnable;
    depthWriteEnable = pInStruct->depthWriteEnable;
    depthCompareOp = pInStruct->depthCompareOp;
    depthBoundsTestEnable = pInStruct->depthBoundsTestEnable;
    stencilTestEnable = pInStruct->stencilTestEnable;
    front = pInStruct->front;
    back = pInStruct->back;
    minDepthBounds = pInStruct->minDepthBounds;
    maxDepthBounds = pInStruct->maxDepthBounds;
}

safe_VkPipelineDynamicStateCreateInfo::safe_VkPipelineDynamicStateCreateInfo(const VkPipelineDynamicStateCreateInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	dynamicStateCount(pInStruct->dynamicStateCount),
	pDynamicStates(pInStruct->pDynamicStates)
{
}

safe_VkPipelineDynamicStateCreateInfo::safe_VkPipelineDynamicStateCreateInfo() {}

safe_VkPipelineDynamicStateCreateInfo::~safe_VkPipelineDynamicStateCreateInfo()
{
}

void safe_VkPipelineDynamicStateCreateInfo::initialize(const VkPipelineDynamicStateCreateInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    dynamicStateCount = pInStruct->dynamicStateCount;
    pDynamicStates = pInStruct->pDynamicStates;
}

safe_VkGraphicsPipelineCreateInfo::safe_VkGraphicsPipelineCreateInfo(const VkGraphicsPipelineCreateInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	stageCount(pInStruct->stageCount),
	pStages(NULL),
	pColorBlendState(pInStruct->pColorBlendState),
	layout(pInStruct->layout),
	renderPass(pInStruct->renderPass),
	subpass(pInStruct->subpass),
	basePipelineHandle(pInStruct->basePipelineHandle),
	basePipelineIndex(pInStruct->basePipelineIndex)
{
    if (stageCount && pInStruct->pStages) {
        pStages = new safe_VkPipelineShaderStageCreateInfo[stageCount];
        for (uint32_t i=0; i<stageCount; ++i) {
            pStages[i].initialize(&pInStruct->pStages[i]);
        }
    }
    if (pInStruct->pVertexInputState)
        pVertexInputState = new safe_VkPipelineVertexInputStateCreateInfo(pInStruct->pVertexInputState);
    else
        pVertexInputState = NULL;
    if (pInStruct->pInputAssemblyState)
        pInputAssemblyState = new safe_VkPipelineInputAssemblyStateCreateInfo(pInStruct->pInputAssemblyState);
    else
        pInputAssemblyState = NULL;
    if (pInStruct->pTessellationState)
        pTessellationState = new safe_VkPipelineTessellationStateCreateInfo(pInStruct->pTessellationState);
    else
        pTessellationState = NULL;
    if (pInStruct->pViewportState)
        pViewportState = new safe_VkPipelineViewportStateCreateInfo(pInStruct->pViewportState);
    else
        pViewportState = NULL;
    if (pInStruct->pRasterizationState)
        pRasterizationState = new safe_VkPipelineRasterizationStateCreateInfo(pInStruct->pRasterizationState);
    else
        pRasterizationState = NULL;
    if (pInStruct->pMultisampleState)
        pMultisampleState = new safe_VkPipelineMultisampleStateCreateInfo(pInStruct->pMultisampleState);
    else
        pMultisampleState = NULL;
    if (pInStruct->pDepthStencilState)
        pDepthStencilState = new safe_VkPipelineDepthStencilStateCreateInfo(pInStruct->pDepthStencilState);
    else
        pDepthStencilState = NULL;
    if (pInStruct->pDynamicState)
        pDynamicState = new safe_VkPipelineDynamicStateCreateInfo(pInStruct->pDynamicState);
    else
        pDynamicState = NULL;
}

safe_VkGraphicsPipelineCreateInfo::safe_VkGraphicsPipelineCreateInfo() {}

safe_VkGraphicsPipelineCreateInfo::~safe_VkGraphicsPipelineCreateInfo()
{
    if (pStages)
        delete[] pStages;
    if (pVertexInputState)
        delete pVertexInputState;
    if (pInputAssemblyState)
        delete pInputAssemblyState;
    if (pTessellationState)
        delete pTessellationState;
    if (pViewportState)
        delete pViewportState;
    if (pRasterizationState)
        delete pRasterizationState;
    if (pMultisampleState)
        delete pMultisampleState;
    if (pDepthStencilState)
        delete pDepthStencilState;
    if (pDynamicState)
        delete pDynamicState;
}

void safe_VkGraphicsPipelineCreateInfo::initialize(const VkGraphicsPipelineCreateInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    stageCount = pInStruct->stageCount;
    pStages = NULL;
    pColorBlendState = pInStruct->pColorBlendState;
    layout = pInStruct->layout;
    renderPass = pInStruct->renderPass;
    subpass = pInStruct->subpass;
    basePipelineHandle = pInStruct->basePipelineHandle;
    basePipelineIndex = pInStruct->basePipelineIndex;
    if (stageCount && pInStruct->pStages) {
        pStages = new safe_VkPipelineShaderStageCreateInfo[stageCount];
        for (uint32_t i=0; i<stageCount; ++i) {
            pStages[i].initialize(&pInStruct->pStages[i]);
        }
    }
    if (pInStruct->pVertexInputState)
        pVertexInputState = new safe_VkPipelineVertexInputStateCreateInfo(pInStruct->pVertexInputState);
    else
        pVertexInputState = NULL;
    if (pInStruct->pInputAssemblyState)
        pInputAssemblyState = new safe_VkPipelineInputAssemblyStateCreateInfo(pInStruct->pInputAssemblyState);
    else
        pInputAssemblyState = NULL;
    if (pInStruct->pTessellationState)
        pTessellationState = new safe_VkPipelineTessellationStateCreateInfo(pInStruct->pTessellationState);
    else
        pTessellationState = NULL;
    if (pInStruct->pViewportState)
        pViewportState = new safe_VkPipelineViewportStateCreateInfo(pInStruct->pViewportState);
    else
        pViewportState = NULL;
    if (pInStruct->pRasterizationState)
        pRasterizationState = new safe_VkPipelineRasterizationStateCreateInfo(pInStruct->pRasterizationState);
    else
        pRasterizationState = NULL;
    if (pInStruct->pMultisampleState)
        pMultisampleState = new safe_VkPipelineMultisampleStateCreateInfo(pInStruct->pMultisampleState);
    else
        pMultisampleState = NULL;
    if (pInStruct->pDepthStencilState)
        pDepthStencilState = new safe_VkPipelineDepthStencilStateCreateInfo(pInStruct->pDepthStencilState);
    else
        pDepthStencilState = NULL;
    if (pInStruct->pDynamicState)
        pDynamicState = new safe_VkPipelineDynamicStateCreateInfo(pInStruct->pDynamicState);
    else
        pDynamicState = NULL;
}

safe_VkComputePipelineCreateInfo::safe_VkComputePipelineCreateInfo(const VkComputePipelineCreateInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	stage(&pInStruct->stage),
	layout(pInStruct->layout),
	basePipelineHandle(pInStruct->basePipelineHandle),
	basePipelineIndex(pInStruct->basePipelineIndex)
{
}

safe_VkComputePipelineCreateInfo::safe_VkComputePipelineCreateInfo() {}

safe_VkComputePipelineCreateInfo::~safe_VkComputePipelineCreateInfo()
{
}

void safe_VkComputePipelineCreateInfo::initialize(const VkComputePipelineCreateInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
        stage.initialize(&pInStruct->stage);
    layout = pInStruct->layout;
    basePipelineHandle = pInStruct->basePipelineHandle;
    basePipelineIndex = pInStruct->basePipelineIndex;
}

safe_VkPipelineLayoutCreateInfo::safe_VkPipelineLayoutCreateInfo(const VkPipelineLayoutCreateInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	setLayoutCount(pInStruct->setLayoutCount),
	pSetLayouts(NULL),
	pushConstantRangeCount(pInStruct->pushConstantRangeCount),
	pPushConstantRanges(pInStruct->pPushConstantRanges)
{
    if (setLayoutCount && pInStruct->pSetLayouts) {
        pSetLayouts = new VkDescriptorSetLayout[setLayoutCount];
        for (uint32_t i=0; i<setLayoutCount; ++i) {
            pSetLayouts[i] = pInStruct->pSetLayouts[i];
        }
    }
}

safe_VkPipelineLayoutCreateInfo::safe_VkPipelineLayoutCreateInfo() {}

safe_VkPipelineLayoutCreateInfo::~safe_VkPipelineLayoutCreateInfo()
{
    if (pSetLayouts)
        delete[] pSetLayouts;
}

void safe_VkPipelineLayoutCreateInfo::initialize(const VkPipelineLayoutCreateInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    setLayoutCount = pInStruct->setLayoutCount;
    pSetLayouts = NULL;
    pushConstantRangeCount = pInStruct->pushConstantRangeCount;
    pPushConstantRanges = pInStruct->pPushConstantRanges;
    if (setLayoutCount && pInStruct->pSetLayouts) {
        pSetLayouts = new VkDescriptorSetLayout[setLayoutCount];
        for (uint32_t i=0; i<setLayoutCount; ++i) {
            pSetLayouts[i] = pInStruct->pSetLayouts[i];
        }
    }
}

safe_VkSamplerCreateInfo::safe_VkSamplerCreateInfo(const VkSamplerCreateInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	magFilter(pInStruct->magFilter),
	minFilter(pInStruct->minFilter),
	mipmapMode(pInStruct->mipmapMode),
	addressModeU(pInStruct->addressModeU),
	addressModeV(pInStruct->addressModeV),
	addressModeW(pInStruct->addressModeW),
	mipLodBias(pInStruct->mipLodBias),
	anisotropyEnable(pInStruct->anisotropyEnable),
	maxAnisotropy(pInStruct->maxAnisotropy),
	compareEnable(pInStruct->compareEnable),
	compareOp(pInStruct->compareOp),
	minLod(pInStruct->minLod),
	maxLod(pInStruct->maxLod),
	borderColor(pInStruct->borderColor),
	unnormalizedCoordinates(pInStruct->unnormalizedCoordinates)
{
}

safe_VkSamplerCreateInfo::safe_VkSamplerCreateInfo() {}

safe_VkSamplerCreateInfo::~safe_VkSamplerCreateInfo()
{
}

void safe_VkSamplerCreateInfo::initialize(const VkSamplerCreateInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    magFilter = pInStruct->magFilter;
    minFilter = pInStruct->minFilter;
    mipmapMode = pInStruct->mipmapMode;
    addressModeU = pInStruct->addressModeU;
    addressModeV = pInStruct->addressModeV;
    addressModeW = pInStruct->addressModeW;
    mipLodBias = pInStruct->mipLodBias;
    anisotropyEnable = pInStruct->anisotropyEnable;
    maxAnisotropy = pInStruct->maxAnisotropy;
    compareEnable = pInStruct->compareEnable;
    compareOp = pInStruct->compareOp;
    minLod = pInStruct->minLod;
    maxLod = pInStruct->maxLod;
    borderColor = pInStruct->borderColor;
    unnormalizedCoordinates = pInStruct->unnormalizedCoordinates;
}

safe_VkDescriptorSetLayoutBinding::safe_VkDescriptorSetLayoutBinding(const VkDescriptorSetLayoutBinding* pInStruct) : 
	binding(pInStruct->binding),
	descriptorType(pInStruct->descriptorType),
	descriptorCount(pInStruct->descriptorCount),
	stageFlags(pInStruct->stageFlags),
	pImmutableSamplers(NULL)
{
    if (descriptorCount && pInStruct->pImmutableSamplers) {
        pImmutableSamplers = new VkSampler[descriptorCount];
        for (uint32_t i=0; i<descriptorCount; ++i) {
            pImmutableSamplers[i] = pInStruct->pImmutableSamplers[i];
        }
    }
}

safe_VkDescriptorSetLayoutBinding::safe_VkDescriptorSetLayoutBinding() {}

safe_VkDescriptorSetLayoutBinding::~safe_VkDescriptorSetLayoutBinding()
{
    if (pImmutableSamplers)
        delete[] pImmutableSamplers;
}

void safe_VkDescriptorSetLayoutBinding::initialize(const VkDescriptorSetLayoutBinding* pInStruct)
{
    binding = pInStruct->binding;
    descriptorType = pInStruct->descriptorType;
    descriptorCount = pInStruct->descriptorCount;
    stageFlags = pInStruct->stageFlags;
    pImmutableSamplers = NULL;
    if (descriptorCount && pInStruct->pImmutableSamplers) {
        pImmutableSamplers = new VkSampler[descriptorCount];
        for (uint32_t i=0; i<descriptorCount; ++i) {
            pImmutableSamplers[i] = pInStruct->pImmutableSamplers[i];
        }
    }
}

safe_VkDescriptorSetLayoutCreateInfo::safe_VkDescriptorSetLayoutCreateInfo(const VkDescriptorSetLayoutCreateInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	bindingCount(pInStruct->bindingCount),
	pBindings(NULL)
{
    if (bindingCount && pInStruct->pBindings) {
        pBindings = new safe_VkDescriptorSetLayoutBinding[bindingCount];
        for (uint32_t i=0; i<bindingCount; ++i) {
            pBindings[i].initialize(&pInStruct->pBindings[i]);
        }
    }
}

safe_VkDescriptorSetLayoutCreateInfo::safe_VkDescriptorSetLayoutCreateInfo() {}

safe_VkDescriptorSetLayoutCreateInfo::~safe_VkDescriptorSetLayoutCreateInfo()
{
    if (pBindings)
        delete[] pBindings;
}

void safe_VkDescriptorSetLayoutCreateInfo::initialize(const VkDescriptorSetLayoutCreateInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    bindingCount = pInStruct->bindingCount;
    pBindings = NULL;
    if (bindingCount && pInStruct->pBindings) {
        pBindings = new safe_VkDescriptorSetLayoutBinding[bindingCount];
        for (uint32_t i=0; i<bindingCount; ++i) {
            pBindings[i].initialize(&pInStruct->pBindings[i]);
        }
    }
}

safe_VkDescriptorPoolCreateInfo::safe_VkDescriptorPoolCreateInfo(const VkDescriptorPoolCreateInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	maxSets(pInStruct->maxSets),
	poolSizeCount(pInStruct->poolSizeCount),
	pPoolSizes(pInStruct->pPoolSizes)
{
}

safe_VkDescriptorPoolCreateInfo::safe_VkDescriptorPoolCreateInfo() {}

safe_VkDescriptorPoolCreateInfo::~safe_VkDescriptorPoolCreateInfo()
{
}

void safe_VkDescriptorPoolCreateInfo::initialize(const VkDescriptorPoolCreateInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    maxSets = pInStruct->maxSets;
    poolSizeCount = pInStruct->poolSizeCount;
    pPoolSizes = pInStruct->pPoolSizes;
}

safe_VkDescriptorSetAllocateInfo::safe_VkDescriptorSetAllocateInfo(const VkDescriptorSetAllocateInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	descriptorPool(pInStruct->descriptorPool),
	descriptorSetCount(pInStruct->descriptorSetCount),
	pSetLayouts(NULL)
{
    if (descriptorSetCount && pInStruct->pSetLayouts) {
        pSetLayouts = new VkDescriptorSetLayout[descriptorSetCount];
        for (uint32_t i=0; i<descriptorSetCount; ++i) {
            pSetLayouts[i] = pInStruct->pSetLayouts[i];
        }
    }
}

safe_VkDescriptorSetAllocateInfo::safe_VkDescriptorSetAllocateInfo() {}

safe_VkDescriptorSetAllocateInfo::~safe_VkDescriptorSetAllocateInfo()
{
    if (pSetLayouts)
        delete[] pSetLayouts;
}

void safe_VkDescriptorSetAllocateInfo::initialize(const VkDescriptorSetAllocateInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    descriptorPool = pInStruct->descriptorPool;
    descriptorSetCount = pInStruct->descriptorSetCount;
    pSetLayouts = NULL;
    if (descriptorSetCount && pInStruct->pSetLayouts) {
        pSetLayouts = new VkDescriptorSetLayout[descriptorSetCount];
        for (uint32_t i=0; i<descriptorSetCount; ++i) {
            pSetLayouts[i] = pInStruct->pSetLayouts[i];
        }
    }
}

safe_VkWriteDescriptorSet::safe_VkWriteDescriptorSet(const VkWriteDescriptorSet* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	dstSet(pInStruct->dstSet),
	dstBinding(pInStruct->dstBinding),
	dstArrayElement(pInStruct->dstArrayElement),
	descriptorCount(pInStruct->descriptorCount),
	descriptorType(pInStruct->descriptorType),
	pImageInfo(NULL),
	pBufferInfo(NULL),
	pTexelBufferView(NULL)
{
    switch (descriptorType) {
        case VK_DESCRIPTOR_TYPE_SAMPLER:
        case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
        case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
        case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
        case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
        if (descriptorCount && pInStruct->pImageInfo) {
            pImageInfo = new VkDescriptorImageInfo[descriptorCount];
            for (uint32_t i=0; i<descriptorCount; ++i) {
                pImageInfo[i] = pInStruct->pImageInfo[i];
            }
        }
        break;
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
        if (descriptorCount && pInStruct->pBufferInfo) {
            pBufferInfo = new VkDescriptorBufferInfo[descriptorCount];
            for (uint32_t i=0; i<descriptorCount; ++i) {
                pBufferInfo[i] = pInStruct->pBufferInfo[i];
            }
        }
        break;
        case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
        case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
        if (descriptorCount && pInStruct->pTexelBufferView) {
            pTexelBufferView = new VkBufferView[descriptorCount];
            for (uint32_t i=0; i<descriptorCount; ++i) {
                pTexelBufferView[i] = pInStruct->pTexelBufferView[i];
            }
        }
        break;
        default:
        break;
    }
}

safe_VkWriteDescriptorSet::safe_VkWriteDescriptorSet() {}

safe_VkWriteDescriptorSet::~safe_VkWriteDescriptorSet()
{
    if (pImageInfo)
        delete[] pImageInfo;
    if (pBufferInfo)
        delete[] pBufferInfo;
    if (pTexelBufferView)
        delete[] pTexelBufferView;
}

void safe_VkWriteDescriptorSet::initialize(const VkWriteDescriptorSet* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    dstSet = pInStruct->dstSet;
    dstBinding = pInStruct->dstBinding;
    dstArrayElement = pInStruct->dstArrayElement;
    descriptorCount = pInStruct->descriptorCount;
    descriptorType = pInStruct->descriptorType;
    pImageInfo = NULL;
    pBufferInfo = NULL;
    pTexelBufferView = NULL;
    switch (descriptorType) {
        case VK_DESCRIPTOR_TYPE_SAMPLER:
        case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
        case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
        case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
        case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
        if (descriptorCount && pInStruct->pImageInfo) {
            pImageInfo = new VkDescriptorImageInfo[descriptorCount];
            for (uint32_t i=0; i<descriptorCount; ++i) {
                pImageInfo[i] = pInStruct->pImageInfo[i];
            }
        }
        break;
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
        if (descriptorCount && pInStruct->pBufferInfo) {
            pBufferInfo = new VkDescriptorBufferInfo[descriptorCount];
            for (uint32_t i=0; i<descriptorCount; ++i) {
                pBufferInfo[i] = pInStruct->pBufferInfo[i];
            }
        }
        break;
        case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
        case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
        if (descriptorCount && pInStruct->pTexelBufferView) {
            pTexelBufferView = new VkBufferView[descriptorCount];
            for (uint32_t i=0; i<descriptorCount; ++i) {
                pTexelBufferView[i] = pInStruct->pTexelBufferView[i];
            }
        }
        break;
        default:
        break;
    }
}

safe_VkCopyDescriptorSet::safe_VkCopyDescriptorSet(const VkCopyDescriptorSet* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	srcSet(pInStruct->srcSet),
	srcBinding(pInStruct->srcBinding),
	srcArrayElement(pInStruct->srcArrayElement),
	dstSet(pInStruct->dstSet),
	dstBinding(pInStruct->dstBinding),
	dstArrayElement(pInStruct->dstArrayElement),
	descriptorCount(pInStruct->descriptorCount)
{
}

safe_VkCopyDescriptorSet::safe_VkCopyDescriptorSet() {}

safe_VkCopyDescriptorSet::~safe_VkCopyDescriptorSet()
{
}

void safe_VkCopyDescriptorSet::initialize(const VkCopyDescriptorSet* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    srcSet = pInStruct->srcSet;
    srcBinding = pInStruct->srcBinding;
    srcArrayElement = pInStruct->srcArrayElement;
    dstSet = pInStruct->dstSet;
    dstBinding = pInStruct->dstBinding;
    dstArrayElement = pInStruct->dstArrayElement;
    descriptorCount = pInStruct->descriptorCount;
}

safe_VkFramebufferCreateInfo::safe_VkFramebufferCreateInfo(const VkFramebufferCreateInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	renderPass(pInStruct->renderPass),
	attachmentCount(pInStruct->attachmentCount),
	pAttachments(NULL),
	width(pInStruct->width),
	height(pInStruct->height),
	layers(pInStruct->layers)
{
    if (attachmentCount && pInStruct->pAttachments) {
        pAttachments = new VkImageView[attachmentCount];
        for (uint32_t i=0; i<attachmentCount; ++i) {
            pAttachments[i] = pInStruct->pAttachments[i];
        }
    }
}

safe_VkFramebufferCreateInfo::safe_VkFramebufferCreateInfo() {}

safe_VkFramebufferCreateInfo::~safe_VkFramebufferCreateInfo()
{
    if (pAttachments)
        delete[] pAttachments;
}

void safe_VkFramebufferCreateInfo::initialize(const VkFramebufferCreateInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    renderPass = pInStruct->renderPass;
    attachmentCount = pInStruct->attachmentCount;
    pAttachments = NULL;
    width = pInStruct->width;
    height = pInStruct->height;
    layers = pInStruct->layers;
    if (attachmentCount && pInStruct->pAttachments) {
        pAttachments = new VkImageView[attachmentCount];
        for (uint32_t i=0; i<attachmentCount; ++i) {
            pAttachments[i] = pInStruct->pAttachments[i];
        }
    }
}

safe_VkSubpassDescription::safe_VkSubpassDescription(const VkSubpassDescription* pInStruct) : 
	flags(pInStruct->flags),
	pipelineBindPoint(pInStruct->pipelineBindPoint),
	inputAttachmentCount(pInStruct->inputAttachmentCount),
	pInputAttachments(pInStruct->pInputAttachments),
	colorAttachmentCount(pInStruct->colorAttachmentCount),
	pColorAttachments(pInStruct->pColorAttachments),
	pResolveAttachments(pInStruct->pResolveAttachments),
	pDepthStencilAttachment(pInStruct->pDepthStencilAttachment),
	preserveAttachmentCount(pInStruct->preserveAttachmentCount),
	pPreserveAttachments(pInStruct->pPreserveAttachments)
{
}

safe_VkSubpassDescription::safe_VkSubpassDescription() {}

safe_VkSubpassDescription::~safe_VkSubpassDescription()
{
}

void safe_VkSubpassDescription::initialize(const VkSubpassDescription* pInStruct)
{
    flags = pInStruct->flags;
    pipelineBindPoint = pInStruct->pipelineBindPoint;
    inputAttachmentCount = pInStruct->inputAttachmentCount;
    pInputAttachments = pInStruct->pInputAttachments;
    colorAttachmentCount = pInStruct->colorAttachmentCount;
    pColorAttachments = pInStruct->pColorAttachments;
    pResolveAttachments = pInStruct->pResolveAttachments;
    pDepthStencilAttachment = pInStruct->pDepthStencilAttachment;
    preserveAttachmentCount = pInStruct->preserveAttachmentCount;
    pPreserveAttachments = pInStruct->pPreserveAttachments;
}

safe_VkRenderPassCreateInfo::safe_VkRenderPassCreateInfo(const VkRenderPassCreateInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	attachmentCount(pInStruct->attachmentCount),
	pAttachments(pInStruct->pAttachments),
	subpassCount(pInStruct->subpassCount),
	pSubpasses(NULL),
	dependencyCount(pInStruct->dependencyCount),
	pDependencies(pInStruct->pDependencies)
{
    if (subpassCount && pInStruct->pSubpasses) {
        pSubpasses = new safe_VkSubpassDescription[subpassCount];
        for (uint32_t i=0; i<subpassCount; ++i) {
            pSubpasses[i].initialize(&pInStruct->pSubpasses[i]);
        }
    }
}

safe_VkRenderPassCreateInfo::safe_VkRenderPassCreateInfo() {}

safe_VkRenderPassCreateInfo::~safe_VkRenderPassCreateInfo()
{
    if (pSubpasses)
        delete[] pSubpasses;
}

void safe_VkRenderPassCreateInfo::initialize(const VkRenderPassCreateInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    attachmentCount = pInStruct->attachmentCount;
    pAttachments = pInStruct->pAttachments;
    subpassCount = pInStruct->subpassCount;
    pSubpasses = NULL;
    dependencyCount = pInStruct->dependencyCount;
    pDependencies = pInStruct->pDependencies;
    if (subpassCount && pInStruct->pSubpasses) {
        pSubpasses = new safe_VkSubpassDescription[subpassCount];
        for (uint32_t i=0; i<subpassCount; ++i) {
            pSubpasses[i].initialize(&pInStruct->pSubpasses[i]);
        }
    }
}

safe_VkCommandPoolCreateInfo::safe_VkCommandPoolCreateInfo(const VkCommandPoolCreateInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	queueFamilyIndex(pInStruct->queueFamilyIndex)
{
}

safe_VkCommandPoolCreateInfo::safe_VkCommandPoolCreateInfo() {}

safe_VkCommandPoolCreateInfo::~safe_VkCommandPoolCreateInfo()
{
}

void safe_VkCommandPoolCreateInfo::initialize(const VkCommandPoolCreateInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    queueFamilyIndex = pInStruct->queueFamilyIndex;
}

safe_VkCommandBufferAllocateInfo::safe_VkCommandBufferAllocateInfo(const VkCommandBufferAllocateInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	commandPool(pInStruct->commandPool),
	level(pInStruct->level),
	commandBufferCount(pInStruct->commandBufferCount)
{
}

safe_VkCommandBufferAllocateInfo::safe_VkCommandBufferAllocateInfo() {}

safe_VkCommandBufferAllocateInfo::~safe_VkCommandBufferAllocateInfo()
{
}

void safe_VkCommandBufferAllocateInfo::initialize(const VkCommandBufferAllocateInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    commandPool = pInStruct->commandPool;
    level = pInStruct->level;
    commandBufferCount = pInStruct->commandBufferCount;
}

safe_VkCommandBufferInheritanceInfo::safe_VkCommandBufferInheritanceInfo(const VkCommandBufferInheritanceInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	renderPass(pInStruct->renderPass),
	subpass(pInStruct->subpass),
	framebuffer(pInStruct->framebuffer),
	occlusionQueryEnable(pInStruct->occlusionQueryEnable),
	queryFlags(pInStruct->queryFlags),
	pipelineStatistics(pInStruct->pipelineStatistics)
{
}

safe_VkCommandBufferInheritanceInfo::safe_VkCommandBufferInheritanceInfo() {}

safe_VkCommandBufferInheritanceInfo::~safe_VkCommandBufferInheritanceInfo()
{
}

void safe_VkCommandBufferInheritanceInfo::initialize(const VkCommandBufferInheritanceInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    renderPass = pInStruct->renderPass;
    subpass = pInStruct->subpass;
    framebuffer = pInStruct->framebuffer;
    occlusionQueryEnable = pInStruct->occlusionQueryEnable;
    queryFlags = pInStruct->queryFlags;
    pipelineStatistics = pInStruct->pipelineStatistics;
}

safe_VkCommandBufferBeginInfo::safe_VkCommandBufferBeginInfo(const VkCommandBufferBeginInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags)
{
    if (pInStruct->pInheritanceInfo)
        pInheritanceInfo = new safe_VkCommandBufferInheritanceInfo(pInStruct->pInheritanceInfo);
    else
        pInheritanceInfo = NULL;
}

safe_VkCommandBufferBeginInfo::safe_VkCommandBufferBeginInfo() {}

safe_VkCommandBufferBeginInfo::~safe_VkCommandBufferBeginInfo()
{
    if (pInheritanceInfo)
        delete pInheritanceInfo;
}

void safe_VkCommandBufferBeginInfo::initialize(const VkCommandBufferBeginInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    if (pInStruct->pInheritanceInfo)
        pInheritanceInfo = new safe_VkCommandBufferInheritanceInfo(pInStruct->pInheritanceInfo);
    else
        pInheritanceInfo = NULL;
}

safe_VkMemoryBarrier::safe_VkMemoryBarrier(const VkMemoryBarrier* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	srcAccessMask(pInStruct->srcAccessMask),
	dstAccessMask(pInStruct->dstAccessMask)
{
}

safe_VkMemoryBarrier::safe_VkMemoryBarrier() {}

safe_VkMemoryBarrier::~safe_VkMemoryBarrier()
{
}

void safe_VkMemoryBarrier::initialize(const VkMemoryBarrier* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    srcAccessMask = pInStruct->srcAccessMask;
    dstAccessMask = pInStruct->dstAccessMask;
}

safe_VkBufferMemoryBarrier::safe_VkBufferMemoryBarrier(const VkBufferMemoryBarrier* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	srcAccessMask(pInStruct->srcAccessMask),
	dstAccessMask(pInStruct->dstAccessMask),
	srcQueueFamilyIndex(pInStruct->srcQueueFamilyIndex),
	dstQueueFamilyIndex(pInStruct->dstQueueFamilyIndex),
	buffer(pInStruct->buffer),
	offset(pInStruct->offset),
	size(pInStruct->size)
{
}

safe_VkBufferMemoryBarrier::safe_VkBufferMemoryBarrier() {}

safe_VkBufferMemoryBarrier::~safe_VkBufferMemoryBarrier()
{
}

void safe_VkBufferMemoryBarrier::initialize(const VkBufferMemoryBarrier* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    srcAccessMask = pInStruct->srcAccessMask;
    dstAccessMask = pInStruct->dstAccessMask;
    srcQueueFamilyIndex = pInStruct->srcQueueFamilyIndex;
    dstQueueFamilyIndex = pInStruct->dstQueueFamilyIndex;
    buffer = pInStruct->buffer;
    offset = pInStruct->offset;
    size = pInStruct->size;
}

safe_VkImageMemoryBarrier::safe_VkImageMemoryBarrier(const VkImageMemoryBarrier* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	srcAccessMask(pInStruct->srcAccessMask),
	dstAccessMask(pInStruct->dstAccessMask),
	oldLayout(pInStruct->oldLayout),
	newLayout(pInStruct->newLayout),
	srcQueueFamilyIndex(pInStruct->srcQueueFamilyIndex),
	dstQueueFamilyIndex(pInStruct->dstQueueFamilyIndex),
	image(pInStruct->image),
	subresourceRange(pInStruct->subresourceRange)
{
}

safe_VkImageMemoryBarrier::safe_VkImageMemoryBarrier() {}

safe_VkImageMemoryBarrier::~safe_VkImageMemoryBarrier()
{
}

void safe_VkImageMemoryBarrier::initialize(const VkImageMemoryBarrier* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    srcAccessMask = pInStruct->srcAccessMask;
    dstAccessMask = pInStruct->dstAccessMask;
    oldLayout = pInStruct->oldLayout;
    newLayout = pInStruct->newLayout;
    srcQueueFamilyIndex = pInStruct->srcQueueFamilyIndex;
    dstQueueFamilyIndex = pInStruct->dstQueueFamilyIndex;
    image = pInStruct->image;
    subresourceRange = pInStruct->subresourceRange;
}

safe_VkRenderPassBeginInfo::safe_VkRenderPassBeginInfo(const VkRenderPassBeginInfo* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	renderPass(pInStruct->renderPass),
	framebuffer(pInStruct->framebuffer),
	renderArea(pInStruct->renderArea),
	clearValueCount(pInStruct->clearValueCount),
	pClearValues(pInStruct->pClearValues)
{
}

safe_VkRenderPassBeginInfo::safe_VkRenderPassBeginInfo() {}

safe_VkRenderPassBeginInfo::~safe_VkRenderPassBeginInfo()
{
}

void safe_VkRenderPassBeginInfo::initialize(const VkRenderPassBeginInfo* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    renderPass = pInStruct->renderPass;
    framebuffer = pInStruct->framebuffer;
    renderArea = pInStruct->renderArea;
    clearValueCount = pInStruct->clearValueCount;
    pClearValues = pInStruct->pClearValues;
}

safe_VkSwapchainCreateInfoKHR::safe_VkSwapchainCreateInfoKHR(const VkSwapchainCreateInfoKHR* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	surface(pInStruct->surface),
	minImageCount(pInStruct->minImageCount),
	imageFormat(pInStruct->imageFormat),
	imageColorSpace(pInStruct->imageColorSpace),
	imageExtent(pInStruct->imageExtent),
	imageArrayLayers(pInStruct->imageArrayLayers),
	imageUsage(pInStruct->imageUsage),
	imageSharingMode(pInStruct->imageSharingMode),
	queueFamilyIndexCount(pInStruct->queueFamilyIndexCount),
	pQueueFamilyIndices(pInStruct->pQueueFamilyIndices),
	preTransform(pInStruct->preTransform),
	compositeAlpha(pInStruct->compositeAlpha),
	presentMode(pInStruct->presentMode),
	clipped(pInStruct->clipped),
	oldSwapchain(pInStruct->oldSwapchain)
{
}

safe_VkSwapchainCreateInfoKHR::safe_VkSwapchainCreateInfoKHR() {}

safe_VkSwapchainCreateInfoKHR::~safe_VkSwapchainCreateInfoKHR()
{
}

void safe_VkSwapchainCreateInfoKHR::initialize(const VkSwapchainCreateInfoKHR* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    surface = pInStruct->surface;
    minImageCount = pInStruct->minImageCount;
    imageFormat = pInStruct->imageFormat;
    imageColorSpace = pInStruct->imageColorSpace;
    imageExtent = pInStruct->imageExtent;
    imageArrayLayers = pInStruct->imageArrayLayers;
    imageUsage = pInStruct->imageUsage;
    imageSharingMode = pInStruct->imageSharingMode;
    queueFamilyIndexCount = pInStruct->queueFamilyIndexCount;
    pQueueFamilyIndices = pInStruct->pQueueFamilyIndices;
    preTransform = pInStruct->preTransform;
    compositeAlpha = pInStruct->compositeAlpha;
    presentMode = pInStruct->presentMode;
    clipped = pInStruct->clipped;
    oldSwapchain = pInStruct->oldSwapchain;
}

safe_VkPresentInfoKHR::safe_VkPresentInfoKHR(const VkPresentInfoKHR* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	waitSemaphoreCount(pInStruct->waitSemaphoreCount),
	pWaitSemaphores(NULL),
	swapchainCount(pInStruct->swapchainCount),
	pSwapchains(NULL),
	pImageIndices(pInStruct->pImageIndices),
	pResults(pInStruct->pResults)
{
    if (waitSemaphoreCount && pInStruct->pWaitSemaphores) {
        pWaitSemaphores = new VkSemaphore[waitSemaphoreCount];
        for (uint32_t i=0; i<waitSemaphoreCount; ++i) {
            pWaitSemaphores[i] = pInStruct->pWaitSemaphores[i];
        }
    }
    if (swapchainCount && pInStruct->pSwapchains) {
        pSwapchains = new VkSwapchainKHR[swapchainCount];
        for (uint32_t i=0; i<swapchainCount; ++i) {
            pSwapchains[i] = pInStruct->pSwapchains[i];
        }
    }
}

safe_VkPresentInfoKHR::safe_VkPresentInfoKHR() {}

safe_VkPresentInfoKHR::~safe_VkPresentInfoKHR()
{
    if (pWaitSemaphores)
        delete[] pWaitSemaphores;
    if (pSwapchains)
        delete[] pSwapchains;
}

void safe_VkPresentInfoKHR::initialize(const VkPresentInfoKHR* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    waitSemaphoreCount = pInStruct->waitSemaphoreCount;
    pWaitSemaphores = NULL;
    swapchainCount = pInStruct->swapchainCount;
    pSwapchains = NULL;
    pImageIndices = pInStruct->pImageIndices;
    pResults = pInStruct->pResults;
    if (waitSemaphoreCount && pInStruct->pWaitSemaphores) {
        pWaitSemaphores = new VkSemaphore[waitSemaphoreCount];
        for (uint32_t i=0; i<waitSemaphoreCount; ++i) {
            pWaitSemaphores[i] = pInStruct->pWaitSemaphores[i];
        }
    }
    if (swapchainCount && pInStruct->pSwapchains) {
        pSwapchains = new VkSwapchainKHR[swapchainCount];
        for (uint32_t i=0; i<swapchainCount; ++i) {
            pSwapchains[i] = pInStruct->pSwapchains[i];
        }
    }
}

safe_VkDisplayPropertiesKHR::safe_VkDisplayPropertiesKHR(const VkDisplayPropertiesKHR* pInStruct) : 
	display(pInStruct->display),
	displayName(pInStruct->displayName),
	physicalDimensions(pInStruct->physicalDimensions),
	physicalResolution(pInStruct->physicalResolution),
	supportedTransforms(pInStruct->supportedTransforms),
	planeReorderPossible(pInStruct->planeReorderPossible),
	persistentContent(pInStruct->persistentContent)
{
}

safe_VkDisplayPropertiesKHR::safe_VkDisplayPropertiesKHR() {}

safe_VkDisplayPropertiesKHR::~safe_VkDisplayPropertiesKHR()
{
}

void safe_VkDisplayPropertiesKHR::initialize(const VkDisplayPropertiesKHR* pInStruct)
{
    display = pInStruct->display;
    displayName = pInStruct->displayName;
    physicalDimensions = pInStruct->physicalDimensions;
    physicalResolution = pInStruct->physicalResolution;
    supportedTransforms = pInStruct->supportedTransforms;
    planeReorderPossible = pInStruct->planeReorderPossible;
    persistentContent = pInStruct->persistentContent;
}

safe_VkDisplayModeCreateInfoKHR::safe_VkDisplayModeCreateInfoKHR(const VkDisplayModeCreateInfoKHR* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	parameters(pInStruct->parameters)
{
}

safe_VkDisplayModeCreateInfoKHR::safe_VkDisplayModeCreateInfoKHR() {}

safe_VkDisplayModeCreateInfoKHR::~safe_VkDisplayModeCreateInfoKHR()
{
}

void safe_VkDisplayModeCreateInfoKHR::initialize(const VkDisplayModeCreateInfoKHR* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    parameters = pInStruct->parameters;
}

safe_VkDisplaySurfaceCreateInfoKHR::safe_VkDisplaySurfaceCreateInfoKHR(const VkDisplaySurfaceCreateInfoKHR* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	displayMode(pInStruct->displayMode),
	planeIndex(pInStruct->planeIndex),
	planeStackIndex(pInStruct->planeStackIndex),
	transform(pInStruct->transform),
	globalAlpha(pInStruct->globalAlpha),
	alphaMode(pInStruct->alphaMode),
	imageExtent(pInStruct->imageExtent)
{
}

safe_VkDisplaySurfaceCreateInfoKHR::safe_VkDisplaySurfaceCreateInfoKHR() {}

safe_VkDisplaySurfaceCreateInfoKHR::~safe_VkDisplaySurfaceCreateInfoKHR()
{
}

void safe_VkDisplaySurfaceCreateInfoKHR::initialize(const VkDisplaySurfaceCreateInfoKHR* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    displayMode = pInStruct->displayMode;
    planeIndex = pInStruct->planeIndex;
    planeStackIndex = pInStruct->planeStackIndex;
    transform = pInStruct->transform;
    globalAlpha = pInStruct->globalAlpha;
    alphaMode = pInStruct->alphaMode;
    imageExtent = pInStruct->imageExtent;
}

safe_VkDisplayPresentInfoKHR::safe_VkDisplayPresentInfoKHR(const VkDisplayPresentInfoKHR* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	srcRect(pInStruct->srcRect),
	dstRect(pInStruct->dstRect),
	persistent(pInStruct->persistent)
{
}

safe_VkDisplayPresentInfoKHR::safe_VkDisplayPresentInfoKHR() {}

safe_VkDisplayPresentInfoKHR::~safe_VkDisplayPresentInfoKHR()
{
}

void safe_VkDisplayPresentInfoKHR::initialize(const VkDisplayPresentInfoKHR* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    srcRect = pInStruct->srcRect;
    dstRect = pInStruct->dstRect;
    persistent = pInStruct->persistent;
}
#ifdef VK_USE_PLATFORM_XLIB_KHR

safe_VkXlibSurfaceCreateInfoKHR::safe_VkXlibSurfaceCreateInfoKHR(const VkXlibSurfaceCreateInfoKHR* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	dpy(pInStruct->dpy),
	window(pInStruct->window)
{
}

safe_VkXlibSurfaceCreateInfoKHR::safe_VkXlibSurfaceCreateInfoKHR() {}

safe_VkXlibSurfaceCreateInfoKHR::~safe_VkXlibSurfaceCreateInfoKHR()
{
}

void safe_VkXlibSurfaceCreateInfoKHR::initialize(const VkXlibSurfaceCreateInfoKHR* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    dpy = pInStruct->dpy;
    window = pInStruct->window;
}
#endif
#ifdef VK_USE_PLATFORM_XCB_KHR

safe_VkXcbSurfaceCreateInfoKHR::safe_VkXcbSurfaceCreateInfoKHR(const VkXcbSurfaceCreateInfoKHR* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	connection(pInStruct->connection),
	window(pInStruct->window)
{
}

safe_VkXcbSurfaceCreateInfoKHR::safe_VkXcbSurfaceCreateInfoKHR() {}

safe_VkXcbSurfaceCreateInfoKHR::~safe_VkXcbSurfaceCreateInfoKHR()
{
}

void safe_VkXcbSurfaceCreateInfoKHR::initialize(const VkXcbSurfaceCreateInfoKHR* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    connection = pInStruct->connection;
    window = pInStruct->window;
}
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR

safe_VkWaylandSurfaceCreateInfoKHR::safe_VkWaylandSurfaceCreateInfoKHR(const VkWaylandSurfaceCreateInfoKHR* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	display(pInStruct->display),
	surface(pInStruct->surface)
{
}

safe_VkWaylandSurfaceCreateInfoKHR::safe_VkWaylandSurfaceCreateInfoKHR() {}

safe_VkWaylandSurfaceCreateInfoKHR::~safe_VkWaylandSurfaceCreateInfoKHR()
{
}

void safe_VkWaylandSurfaceCreateInfoKHR::initialize(const VkWaylandSurfaceCreateInfoKHR* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    display = pInStruct->display;
    surface = pInStruct->surface;
}
#endif
#ifdef VK_USE_PLATFORM_MIR_KHR

safe_VkMirSurfaceCreateInfoKHR::safe_VkMirSurfaceCreateInfoKHR(const VkMirSurfaceCreateInfoKHR* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	connection(pInStruct->connection),
	mirSurface(pInStruct->mirSurface)
{
}

safe_VkMirSurfaceCreateInfoKHR::safe_VkMirSurfaceCreateInfoKHR() {}

safe_VkMirSurfaceCreateInfoKHR::~safe_VkMirSurfaceCreateInfoKHR()
{
}

void safe_VkMirSurfaceCreateInfoKHR::initialize(const VkMirSurfaceCreateInfoKHR* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    connection = pInStruct->connection;
    mirSurface = pInStruct->mirSurface;
}
#endif
#ifdef VK_USE_PLATFORM_ANDROID_KHR

safe_VkAndroidSurfaceCreateInfoKHR::safe_VkAndroidSurfaceCreateInfoKHR(const VkAndroidSurfaceCreateInfoKHR* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	window(pInStruct->window)
{
}

safe_VkAndroidSurfaceCreateInfoKHR::safe_VkAndroidSurfaceCreateInfoKHR() {}

safe_VkAndroidSurfaceCreateInfoKHR::~safe_VkAndroidSurfaceCreateInfoKHR()
{
}

void safe_VkAndroidSurfaceCreateInfoKHR::initialize(const VkAndroidSurfaceCreateInfoKHR* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    window = pInStruct->window;
}
#endif
#ifdef VK_USE_PLATFORM_WIN32_KHR

safe_VkWin32SurfaceCreateInfoKHR::safe_VkWin32SurfaceCreateInfoKHR(const VkWin32SurfaceCreateInfoKHR* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	hinstance(pInStruct->hinstance),
	hwnd(pInStruct->hwnd)
{
}

safe_VkWin32SurfaceCreateInfoKHR::safe_VkWin32SurfaceCreateInfoKHR() {}

safe_VkWin32SurfaceCreateInfoKHR::~safe_VkWin32SurfaceCreateInfoKHR()
{
}

void safe_VkWin32SurfaceCreateInfoKHR::initialize(const VkWin32SurfaceCreateInfoKHR* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    hinstance = pInStruct->hinstance;
    hwnd = pInStruct->hwnd;
}
#endif

safe_VkDebugReportCallbackCreateInfoEXT::safe_VkDebugReportCallbackCreateInfoEXT(const VkDebugReportCallbackCreateInfoEXT* pInStruct) : 
	sType(pInStruct->sType),
	pNext(pInStruct->pNext),
	flags(pInStruct->flags),
	pfnCallback(pInStruct->pfnCallback),
	pUserData(pInStruct->pUserData)
{
}

safe_VkDebugReportCallbackCreateInfoEXT::safe_VkDebugReportCallbackCreateInfoEXT() {}

safe_VkDebugReportCallbackCreateInfoEXT::~safe_VkDebugReportCallbackCreateInfoEXT()
{
}

void safe_VkDebugReportCallbackCreateInfoEXT::initialize(const VkDebugReportCallbackCreateInfoEXT* pInStruct)
{
    sType = pInStruct->sType;
    pNext = pInStruct->pNext;
    flags = pInStruct->flags;
    pfnCallback = pInStruct->pfnCallback;
    pUserData = pInStruct->pUserData;
}