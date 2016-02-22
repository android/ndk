#ifndef __thread_check_h_
#define __thread_check_h_ 1

#ifdef __cplusplus
extern "C" {
#endif

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



VkResult VKAPI_CALL vkEnumeratePhysicalDevices(
    VkInstance                                  instance,
    uint32_t*                                   pPhysicalDeviceCount,
    VkPhysicalDevice*                           pPhysicalDevices)
{
    dispatch_key key = get_dispatch_key(instance);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerInstanceDispatchTable *pTable = my_data->instance_dispatch_table;
    VkResult result;
    startReadObject(my_data, instance);

    result = pTable->EnumeratePhysicalDevices(instance,pPhysicalDeviceCount,pPhysicalDevices);
    finishReadObject(my_data, instance);

    return result;
}

void VKAPI_CALL vkGetDeviceQueue(
    VkDevice                                    device,
    uint32_t                                    queueFamilyIndex,
    uint32_t                                    queueIndex,
    VkQueue*                                    pQueue)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startReadObject(my_data, device);

    pTable->GetDeviceQueue(device,queueFamilyIndex,queueIndex,pQueue);
    finishReadObject(my_data, device);

}

VkResult VKAPI_CALL vkQueueSubmit(
    VkQueue                                     queue,
    uint32_t                                    submitCount,
    const VkSubmitInfo*                         pSubmits,
    VkFence                                     fence)
{
    dispatch_key key = get_dispatch_key(queue);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    startWriteObject(my_data, queue);
    for (int index=0;index<submitCount;index++) {
        for(int index2=0;index2<pSubmits[index].waitSemaphoreCount;index2++)        startWriteObject(my_data, pSubmits[index].pWaitSemaphores[index2]);
        for(int index2=0;index2<pSubmits[index].signalSemaphoreCount;index2++)        startWriteObject(my_data, pSubmits[index].pSignalSemaphores[index2]);
    }
    startWriteObject(my_data, fence);
// Host access to queue must be externally synchronized
// Host access to pSubmits[].pWaitSemaphores[],pSubmits[].pSignalSemaphores[] must be externally synchronized
// Host access to fence must be externally synchronized

    result = pTable->QueueSubmit(queue,submitCount,pSubmits,fence);
    finishWriteObject(my_data, queue);
    for (int index=0;index<submitCount;index++) {
        for(int index2=0;index2<pSubmits[index].waitSemaphoreCount;index2++)        finishWriteObject(my_data, pSubmits[index].pWaitSemaphores[index2]);
        for(int index2=0;index2<pSubmits[index].signalSemaphoreCount;index2++)        finishWriteObject(my_data, pSubmits[index].pSignalSemaphores[index2]);
    }
    finishWriteObject(my_data, fence);
// Host access to queue must be externally synchronized
// Host access to pSubmits[].pWaitSemaphores[],pSubmits[].pSignalSemaphores[] must be externally synchronized
// Host access to fence must be externally synchronized

    return result;
}

VkResult VKAPI_CALL vkQueueWaitIdle(
    VkQueue                                     queue)
{
    dispatch_key key = get_dispatch_key(queue);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    startReadObject(my_data, queue);

    result = pTable->QueueWaitIdle(queue);
    finishReadObject(my_data, queue);

    return result;
}

VkResult VKAPI_CALL vkDeviceWaitIdle(
    VkDevice                                    device)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    startReadObject(my_data, device);
    // all sname:VkQueue objects created from pname:device must be externally synchronized between host accesses

    result = pTable->DeviceWaitIdle(device);
    finishReadObject(my_data, device);
    // all sname:VkQueue objects created from pname:device must be externally synchronized between host accesses

    return result;
}

VkResult VKAPI_CALL vkAllocateMemory(
    VkDevice                                    device,
    const VkMemoryAllocateInfo*                 pAllocateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkDeviceMemory*                             pMemory)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    startReadObject(my_data, device);

    result = pTable->AllocateMemory(device,pAllocateInfo,pAllocator,pMemory);
    finishReadObject(my_data, device);

    return result;
}

void VKAPI_CALL vkFreeMemory(
    VkDevice                                    device,
    VkDeviceMemory                              memory,
    const VkAllocationCallbacks*                pAllocator)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startReadObject(my_data, device);
    startWriteObject(my_data, memory);
// Host access to memory must be externally synchronized

    pTable->FreeMemory(device,memory,pAllocator);
    finishReadObject(my_data, device);
    finishWriteObject(my_data, memory);
// Host access to memory must be externally synchronized

}

VkResult VKAPI_CALL vkMapMemory(
    VkDevice                                    device,
    VkDeviceMemory                              memory,
    VkDeviceSize                                offset,
    VkDeviceSize                                size,
    VkMemoryMapFlags                            flags,
    void**                                      ppData)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    startReadObject(my_data, device);
    startWriteObject(my_data, memory);
// Host access to memory must be externally synchronized

    result = pTable->MapMemory(device,memory,offset,size,flags,ppData);
    finishReadObject(my_data, device);
    finishWriteObject(my_data, memory);
// Host access to memory must be externally synchronized

    return result;
}

void VKAPI_CALL vkUnmapMemory(
    VkDevice                                    device,
    VkDeviceMemory                              memory)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startReadObject(my_data, device);
    startWriteObject(my_data, memory);
// Host access to memory must be externally synchronized

    pTable->UnmapMemory(device,memory);
    finishReadObject(my_data, device);
    finishWriteObject(my_data, memory);
// Host access to memory must be externally synchronized

}

VkResult VKAPI_CALL vkFlushMappedMemoryRanges(
    VkDevice                                    device,
    uint32_t                                    memoryRangeCount,
    const VkMappedMemoryRange*                  pMemoryRanges)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    startReadObject(my_data, device);

    result = pTable->FlushMappedMemoryRanges(device,memoryRangeCount,pMemoryRanges);
    finishReadObject(my_data, device);

    return result;
}

VkResult VKAPI_CALL vkInvalidateMappedMemoryRanges(
    VkDevice                                    device,
    uint32_t                                    memoryRangeCount,
    const VkMappedMemoryRange*                  pMemoryRanges)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    startReadObject(my_data, device);

    result = pTable->InvalidateMappedMemoryRanges(device,memoryRangeCount,pMemoryRanges);
    finishReadObject(my_data, device);

    return result;
}

void VKAPI_CALL vkGetDeviceMemoryCommitment(
    VkDevice                                    device,
    VkDeviceMemory                              memory,
    VkDeviceSize*                               pCommittedMemoryInBytes)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startReadObject(my_data, device);
    startReadObject(my_data, memory);

    pTable->GetDeviceMemoryCommitment(device,memory,pCommittedMemoryInBytes);
    finishReadObject(my_data, device);
    finishReadObject(my_data, memory);

}

VkResult VKAPI_CALL vkBindBufferMemory(
    VkDevice                                    device,
    VkBuffer                                    buffer,
    VkDeviceMemory                              memory,
    VkDeviceSize                                memoryOffset)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    startReadObject(my_data, device);
    startWriteObject(my_data, buffer);
    startReadObject(my_data, memory);
// Host access to buffer must be externally synchronized

    result = pTable->BindBufferMemory(device,buffer,memory,memoryOffset);
    finishReadObject(my_data, device);
    finishWriteObject(my_data, buffer);
    finishReadObject(my_data, memory);
// Host access to buffer must be externally synchronized

    return result;
}

VkResult VKAPI_CALL vkBindImageMemory(
    VkDevice                                    device,
    VkImage                                     image,
    VkDeviceMemory                              memory,
    VkDeviceSize                                memoryOffset)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    startReadObject(my_data, device);
    startWriteObject(my_data, image);
    startReadObject(my_data, memory);
// Host access to image must be externally synchronized

    result = pTable->BindImageMemory(device,image,memory,memoryOffset);
    finishReadObject(my_data, device);
    finishWriteObject(my_data, image);
    finishReadObject(my_data, memory);
// Host access to image must be externally synchronized

    return result;
}

void VKAPI_CALL vkGetBufferMemoryRequirements(
    VkDevice                                    device,
    VkBuffer                                    buffer,
    VkMemoryRequirements*                       pMemoryRequirements)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startReadObject(my_data, device);
    startReadObject(my_data, buffer);

    pTable->GetBufferMemoryRequirements(device,buffer,pMemoryRequirements);
    finishReadObject(my_data, device);
    finishReadObject(my_data, buffer);

}

void VKAPI_CALL vkGetImageMemoryRequirements(
    VkDevice                                    device,
    VkImage                                     image,
    VkMemoryRequirements*                       pMemoryRequirements)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startReadObject(my_data, device);
    startReadObject(my_data, image);

    pTable->GetImageMemoryRequirements(device,image,pMemoryRequirements);
    finishReadObject(my_data, device);
    finishReadObject(my_data, image);

}

void VKAPI_CALL vkGetImageSparseMemoryRequirements(
    VkDevice                                    device,
    VkImage                                     image,
    uint32_t*                                   pSparseMemoryRequirementCount,
    VkSparseImageMemoryRequirements*            pSparseMemoryRequirements)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startReadObject(my_data, device);
    startReadObject(my_data, image);

    pTable->GetImageSparseMemoryRequirements(device,image,pSparseMemoryRequirementCount,pSparseMemoryRequirements);
    finishReadObject(my_data, device);
    finishReadObject(my_data, image);

}

VkResult VKAPI_CALL vkQueueBindSparse(
    VkQueue                                     queue,
    uint32_t                                    bindInfoCount,
    const VkBindSparseInfo*                     pBindInfo,
    VkFence                                     fence)
{
    dispatch_key key = get_dispatch_key(queue);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    startWriteObject(my_data, queue);
    for (int index=0;index<bindInfoCount;index++) {
        for(int index2=0;index2<pBindInfo[index].waitSemaphoreCount;index2++)        startWriteObject(my_data, pBindInfo[index].pWaitSemaphores[index2]);
        for(int index2=0;index2<pBindInfo[index].signalSemaphoreCount;index2++)        startWriteObject(my_data, pBindInfo[index].pSignalSemaphores[index2]);
        for(int index2=0;index2<pBindInfo[index].bufferBindCount;index2++)        startWriteObject(my_data, pBindInfo[index].pBufferBinds[index2].buffer);
        for(int index2=0;index2<pBindInfo[index].imageOpaqueBindCount;index2++)        startWriteObject(my_data, pBindInfo[index].pImageOpaqueBinds[index2].image);
        for(int index2=0;index2<pBindInfo[index].imageBindCount;index2++)        startWriteObject(my_data, pBindInfo[index].pImageBinds[index2].image);
    }
    startWriteObject(my_data, fence);
// Host access to queue must be externally synchronized
// Host access to pBindInfo[].pWaitSemaphores[],pBindInfo[].pSignalSemaphores[],pBindInfo[].pBufferBinds[].buffer,pBindInfo[].pImageOpaqueBinds[].image,pBindInfo[].pImageBinds[].image must be externally synchronized
// Host access to fence must be externally synchronized

    result = pTable->QueueBindSparse(queue,bindInfoCount,pBindInfo,fence);
    finishWriteObject(my_data, queue);
    for (int index=0;index<bindInfoCount;index++) {
        for(int index2=0;index2<pBindInfo[index].waitSemaphoreCount;index2++)        finishWriteObject(my_data, pBindInfo[index].pWaitSemaphores[index2]);
        for(int index2=0;index2<pBindInfo[index].signalSemaphoreCount;index2++)        finishWriteObject(my_data, pBindInfo[index].pSignalSemaphores[index2]);
        for(int index2=0;index2<pBindInfo[index].bufferBindCount;index2++)        finishWriteObject(my_data, pBindInfo[index].pBufferBinds[index2].buffer);
        for(int index2=0;index2<pBindInfo[index].imageOpaqueBindCount;index2++)        finishWriteObject(my_data, pBindInfo[index].pImageOpaqueBinds[index2].image);
        for(int index2=0;index2<pBindInfo[index].imageBindCount;index2++)        finishWriteObject(my_data, pBindInfo[index].pImageBinds[index2].image);
    }
    finishWriteObject(my_data, fence);
// Host access to queue must be externally synchronized
// Host access to pBindInfo[].pWaitSemaphores[],pBindInfo[].pSignalSemaphores[],pBindInfo[].pBufferBinds[].buffer,pBindInfo[].pImageOpaqueBinds[].image,pBindInfo[].pImageBinds[].image must be externally synchronized
// Host access to fence must be externally synchronized

    return result;
}

VkResult VKAPI_CALL vkCreateFence(
    VkDevice                                    device,
    const VkFenceCreateInfo*                    pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkFence*                                    pFence)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    startReadObject(my_data, device);

    result = pTable->CreateFence(device,pCreateInfo,pAllocator,pFence);
    finishReadObject(my_data, device);

    return result;
}

void VKAPI_CALL vkDestroyFence(
    VkDevice                                    device,
    VkFence                                     fence,
    const VkAllocationCallbacks*                pAllocator)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startReadObject(my_data, device);
    startWriteObject(my_data, fence);
// Host access to fence must be externally synchronized

    pTable->DestroyFence(device,fence,pAllocator);
    finishReadObject(my_data, device);
    finishWriteObject(my_data, fence);
// Host access to fence must be externally synchronized

}

VkResult VKAPI_CALL vkResetFences(
    VkDevice                                    device,
    uint32_t                                    fenceCount,
    const VkFence*                              pFences)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    startReadObject(my_data, device);
    for (int index=0;index<fenceCount;index++) {
        startWriteObject(my_data, pFences[index]);
    }
// Host access to each member of pFences must be externally synchronized

    result = pTable->ResetFences(device,fenceCount,pFences);
    finishReadObject(my_data, device);
    for (int index=0;index<fenceCount;index++) {
        finishWriteObject(my_data, pFences[index]);
    }
// Host access to each member of pFences must be externally synchronized

    return result;
}

VkResult VKAPI_CALL vkGetFenceStatus(
    VkDevice                                    device,
    VkFence                                     fence)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    startReadObject(my_data, device);
    startReadObject(my_data, fence);

    result = pTable->GetFenceStatus(device,fence);
    finishReadObject(my_data, device);
    finishReadObject(my_data, fence);

    return result;
}

VkResult VKAPI_CALL vkWaitForFences(
    VkDevice                                    device,
    uint32_t                                    fenceCount,
    const VkFence*                              pFences,
    VkBool32                                    waitAll,
    uint64_t                                    timeout)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    startReadObject(my_data, device);
    for (int index=0;index<fenceCount;index++) {
        startReadObject(my_data, pFences[index]);
    }

    result = pTable->WaitForFences(device,fenceCount,pFences,waitAll,timeout);
    finishReadObject(my_data, device);
    for (int index=0;index<fenceCount;index++) {
        finishReadObject(my_data, pFences[index]);
    }

    return result;
}

VkResult VKAPI_CALL vkCreateSemaphore(
    VkDevice                                    device,
    const VkSemaphoreCreateInfo*                pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkSemaphore*                                pSemaphore)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    startReadObject(my_data, device);

    result = pTable->CreateSemaphore(device,pCreateInfo,pAllocator,pSemaphore);
    finishReadObject(my_data, device);

    return result;
}

void VKAPI_CALL vkDestroySemaphore(
    VkDevice                                    device,
    VkSemaphore                                 semaphore,
    const VkAllocationCallbacks*                pAllocator)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startReadObject(my_data, device);
    startWriteObject(my_data, semaphore);
// Host access to semaphore must be externally synchronized

    pTable->DestroySemaphore(device,semaphore,pAllocator);
    finishReadObject(my_data, device);
    finishWriteObject(my_data, semaphore);
// Host access to semaphore must be externally synchronized

}

VkResult VKAPI_CALL vkCreateEvent(
    VkDevice                                    device,
    const VkEventCreateInfo*                    pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkEvent*                                    pEvent)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    startReadObject(my_data, device);

    result = pTable->CreateEvent(device,pCreateInfo,pAllocator,pEvent);
    finishReadObject(my_data, device);

    return result;
}

void VKAPI_CALL vkDestroyEvent(
    VkDevice                                    device,
    VkEvent                                     event,
    const VkAllocationCallbacks*                pAllocator)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startReadObject(my_data, device);
    startWriteObject(my_data, event);
// Host access to event must be externally synchronized

    pTable->DestroyEvent(device,event,pAllocator);
    finishReadObject(my_data, device);
    finishWriteObject(my_data, event);
// Host access to event must be externally synchronized

}

VkResult VKAPI_CALL vkGetEventStatus(
    VkDevice                                    device,
    VkEvent                                     event)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    startReadObject(my_data, device);
    startReadObject(my_data, event);

    result = pTable->GetEventStatus(device,event);
    finishReadObject(my_data, device);
    finishReadObject(my_data, event);

    return result;
}

VkResult VKAPI_CALL vkSetEvent(
    VkDevice                                    device,
    VkEvent                                     event)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    startReadObject(my_data, device);
    startWriteObject(my_data, event);
// Host access to event must be externally synchronized

    result = pTable->SetEvent(device,event);
    finishReadObject(my_data, device);
    finishWriteObject(my_data, event);
// Host access to event must be externally synchronized

    return result;
}

VkResult VKAPI_CALL vkResetEvent(
    VkDevice                                    device,
    VkEvent                                     event)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    startReadObject(my_data, device);
    startWriteObject(my_data, event);
// Host access to event must be externally synchronized

    result = pTable->ResetEvent(device,event);
    finishReadObject(my_data, device);
    finishWriteObject(my_data, event);
// Host access to event must be externally synchronized

    return result;
}

VkResult VKAPI_CALL vkCreateQueryPool(
    VkDevice                                    device,
    const VkQueryPoolCreateInfo*                pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkQueryPool*                                pQueryPool)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    startReadObject(my_data, device);

    result = pTable->CreateQueryPool(device,pCreateInfo,pAllocator,pQueryPool);
    finishReadObject(my_data, device);

    return result;
}

void VKAPI_CALL vkDestroyQueryPool(
    VkDevice                                    device,
    VkQueryPool                                 queryPool,
    const VkAllocationCallbacks*                pAllocator)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startReadObject(my_data, device);
    startWriteObject(my_data, queryPool);
// Host access to queryPool must be externally synchronized

    pTable->DestroyQueryPool(device,queryPool,pAllocator);
    finishReadObject(my_data, device);
    finishWriteObject(my_data, queryPool);
// Host access to queryPool must be externally synchronized

}

VkResult VKAPI_CALL vkGetQueryPoolResults(
    VkDevice                                    device,
    VkQueryPool                                 queryPool,
    uint32_t                                    firstQuery,
    uint32_t                                    queryCount,
    size_t                                      dataSize,
    void*                                       pData,
    VkDeviceSize                                stride,
    VkQueryResultFlags                          flags)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    startReadObject(my_data, device);
    startReadObject(my_data, queryPool);

    result = pTable->GetQueryPoolResults(device,queryPool,firstQuery,queryCount,dataSize,pData,stride,flags);
    finishReadObject(my_data, device);
    finishReadObject(my_data, queryPool);

    return result;
}

VkResult VKAPI_CALL vkCreateBuffer(
    VkDevice                                    device,
    const VkBufferCreateInfo*                   pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkBuffer*                                   pBuffer)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    startReadObject(my_data, device);

    result = pTable->CreateBuffer(device,pCreateInfo,pAllocator,pBuffer);
    finishReadObject(my_data, device);

    return result;
}

void VKAPI_CALL vkDestroyBuffer(
    VkDevice                                    device,
    VkBuffer                                    buffer,
    const VkAllocationCallbacks*                pAllocator)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startReadObject(my_data, device);
    startWriteObject(my_data, buffer);
// Host access to buffer must be externally synchronized

    pTable->DestroyBuffer(device,buffer,pAllocator);
    finishReadObject(my_data, device);
    finishWriteObject(my_data, buffer);
// Host access to buffer must be externally synchronized

}

VkResult VKAPI_CALL vkCreateBufferView(
    VkDevice                                    device,
    const VkBufferViewCreateInfo*               pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkBufferView*                               pView)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    startReadObject(my_data, device);

    result = pTable->CreateBufferView(device,pCreateInfo,pAllocator,pView);
    finishReadObject(my_data, device);

    return result;
}

void VKAPI_CALL vkDestroyBufferView(
    VkDevice                                    device,
    VkBufferView                                bufferView,
    const VkAllocationCallbacks*                pAllocator)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startReadObject(my_data, device);
    startWriteObject(my_data, bufferView);
// Host access to bufferView must be externally synchronized

    pTable->DestroyBufferView(device,bufferView,pAllocator);
    finishReadObject(my_data, device);
    finishWriteObject(my_data, bufferView);
// Host access to bufferView must be externally synchronized

}

VkResult VKAPI_CALL vkCreateImage(
    VkDevice                                    device,
    const VkImageCreateInfo*                    pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkImage*                                    pImage)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    startReadObject(my_data, device);

    result = pTable->CreateImage(device,pCreateInfo,pAllocator,pImage);
    finishReadObject(my_data, device);

    return result;
}

void VKAPI_CALL vkDestroyImage(
    VkDevice                                    device,
    VkImage                                     image,
    const VkAllocationCallbacks*                pAllocator)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startReadObject(my_data, device);
    startWriteObject(my_data, image);
// Host access to image must be externally synchronized

    pTable->DestroyImage(device,image,pAllocator);
    finishReadObject(my_data, device);
    finishWriteObject(my_data, image);
// Host access to image must be externally synchronized

}

void VKAPI_CALL vkGetImageSubresourceLayout(
    VkDevice                                    device,
    VkImage                                     image,
    const VkImageSubresource*                   pSubresource,
    VkSubresourceLayout*                        pLayout)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startReadObject(my_data, device);
    startReadObject(my_data, image);

    pTable->GetImageSubresourceLayout(device,image,pSubresource,pLayout);
    finishReadObject(my_data, device);
    finishReadObject(my_data, image);

}

VkResult VKAPI_CALL vkCreateImageView(
    VkDevice                                    device,
    const VkImageViewCreateInfo*                pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkImageView*                                pView)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    startReadObject(my_data, device);

    result = pTable->CreateImageView(device,pCreateInfo,pAllocator,pView);
    finishReadObject(my_data, device);

    return result;
}

void VKAPI_CALL vkDestroyImageView(
    VkDevice                                    device,
    VkImageView                                 imageView,
    const VkAllocationCallbacks*                pAllocator)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startReadObject(my_data, device);
    startWriteObject(my_data, imageView);
// Host access to imageView must be externally synchronized

    pTable->DestroyImageView(device,imageView,pAllocator);
    finishReadObject(my_data, device);
    finishWriteObject(my_data, imageView);
// Host access to imageView must be externally synchronized

}

VkResult VKAPI_CALL vkCreateShaderModule(
    VkDevice                                    device,
    const VkShaderModuleCreateInfo*             pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkShaderModule*                             pShaderModule)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    startReadObject(my_data, device);

    result = pTable->CreateShaderModule(device,pCreateInfo,pAllocator,pShaderModule);
    finishReadObject(my_data, device);

    return result;
}

void VKAPI_CALL vkDestroyShaderModule(
    VkDevice                                    device,
    VkShaderModule                              shaderModule,
    const VkAllocationCallbacks*                pAllocator)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startReadObject(my_data, device);
    startWriteObject(my_data, shaderModule);
// Host access to shaderModule must be externally synchronized

    pTable->DestroyShaderModule(device,shaderModule,pAllocator);
    finishReadObject(my_data, device);
    finishWriteObject(my_data, shaderModule);
// Host access to shaderModule must be externally synchronized

}

VkResult VKAPI_CALL vkCreatePipelineCache(
    VkDevice                                    device,
    const VkPipelineCacheCreateInfo*            pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkPipelineCache*                            pPipelineCache)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    startReadObject(my_data, device);

    result = pTable->CreatePipelineCache(device,pCreateInfo,pAllocator,pPipelineCache);
    finishReadObject(my_data, device);

    return result;
}

void VKAPI_CALL vkDestroyPipelineCache(
    VkDevice                                    device,
    VkPipelineCache                             pipelineCache,
    const VkAllocationCallbacks*                pAllocator)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startReadObject(my_data, device);
    startWriteObject(my_data, pipelineCache);
// Host access to pipelineCache must be externally synchronized

    pTable->DestroyPipelineCache(device,pipelineCache,pAllocator);
    finishReadObject(my_data, device);
    finishWriteObject(my_data, pipelineCache);
// Host access to pipelineCache must be externally synchronized

}

VkResult VKAPI_CALL vkGetPipelineCacheData(
    VkDevice                                    device,
    VkPipelineCache                             pipelineCache,
    size_t*                                     pDataSize,
    void*                                       pData)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    startReadObject(my_data, device);
    startReadObject(my_data, pipelineCache);

    result = pTable->GetPipelineCacheData(device,pipelineCache,pDataSize,pData);
    finishReadObject(my_data, device);
    finishReadObject(my_data, pipelineCache);

    return result;
}

VkResult VKAPI_CALL vkMergePipelineCaches(
    VkDevice                                    device,
    VkPipelineCache                             dstCache,
    uint32_t                                    srcCacheCount,
    const VkPipelineCache*                      pSrcCaches)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    startReadObject(my_data, device);
    startWriteObject(my_data, dstCache);
    for (int index=0;index<srcCacheCount;index++) {
        startReadObject(my_data, pSrcCaches[index]);
    }
// Host access to dstCache must be externally synchronized

    result = pTable->MergePipelineCaches(device,dstCache,srcCacheCount,pSrcCaches);
    finishReadObject(my_data, device);
    finishWriteObject(my_data, dstCache);
    for (int index=0;index<srcCacheCount;index++) {
        finishReadObject(my_data, pSrcCaches[index]);
    }
// Host access to dstCache must be externally synchronized

    return result;
}

VkResult VKAPI_CALL vkCreateGraphicsPipelines(
    VkDevice                                    device,
    VkPipelineCache                             pipelineCache,
    uint32_t                                    createInfoCount,
    const VkGraphicsPipelineCreateInfo*         pCreateInfos,
    const VkAllocationCallbacks*                pAllocator,
    VkPipeline*                                 pPipelines)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    startReadObject(my_data, device);
    startReadObject(my_data, pipelineCache);

    result = pTable->CreateGraphicsPipelines(device,pipelineCache,createInfoCount,pCreateInfos,pAllocator,pPipelines);
    finishReadObject(my_data, device);
    finishReadObject(my_data, pipelineCache);

    return result;
}

VkResult VKAPI_CALL vkCreateComputePipelines(
    VkDevice                                    device,
    VkPipelineCache                             pipelineCache,
    uint32_t                                    createInfoCount,
    const VkComputePipelineCreateInfo*          pCreateInfos,
    const VkAllocationCallbacks*                pAllocator,
    VkPipeline*                                 pPipelines)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    startReadObject(my_data, device);
    startReadObject(my_data, pipelineCache);

    result = pTable->CreateComputePipelines(device,pipelineCache,createInfoCount,pCreateInfos,pAllocator,pPipelines);
    finishReadObject(my_data, device);
    finishReadObject(my_data, pipelineCache);

    return result;
}

void VKAPI_CALL vkDestroyPipeline(
    VkDevice                                    device,
    VkPipeline                                  pipeline,
    const VkAllocationCallbacks*                pAllocator)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startReadObject(my_data, device);
    startWriteObject(my_data, pipeline);
// Host access to pipeline must be externally synchronized

    pTable->DestroyPipeline(device,pipeline,pAllocator);
    finishReadObject(my_data, device);
    finishWriteObject(my_data, pipeline);
// Host access to pipeline must be externally synchronized

}

VkResult VKAPI_CALL vkCreatePipelineLayout(
    VkDevice                                    device,
    const VkPipelineLayoutCreateInfo*           pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkPipelineLayout*                           pPipelineLayout)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    startReadObject(my_data, device);

    result = pTable->CreatePipelineLayout(device,pCreateInfo,pAllocator,pPipelineLayout);
    finishReadObject(my_data, device);

    return result;
}

void VKAPI_CALL vkDestroyPipelineLayout(
    VkDevice                                    device,
    VkPipelineLayout                            pipelineLayout,
    const VkAllocationCallbacks*                pAllocator)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startReadObject(my_data, device);
    startWriteObject(my_data, pipelineLayout);
// Host access to pipelineLayout must be externally synchronized

    pTable->DestroyPipelineLayout(device,pipelineLayout,pAllocator);
    finishReadObject(my_data, device);
    finishWriteObject(my_data, pipelineLayout);
// Host access to pipelineLayout must be externally synchronized

}

VkResult VKAPI_CALL vkCreateSampler(
    VkDevice                                    device,
    const VkSamplerCreateInfo*                  pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkSampler*                                  pSampler)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    startReadObject(my_data, device);

    result = pTable->CreateSampler(device,pCreateInfo,pAllocator,pSampler);
    finishReadObject(my_data, device);

    return result;
}

void VKAPI_CALL vkDestroySampler(
    VkDevice                                    device,
    VkSampler                                   sampler,
    const VkAllocationCallbacks*                pAllocator)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startReadObject(my_data, device);
    startWriteObject(my_data, sampler);
// Host access to sampler must be externally synchronized

    pTable->DestroySampler(device,sampler,pAllocator);
    finishReadObject(my_data, device);
    finishWriteObject(my_data, sampler);
// Host access to sampler must be externally synchronized

}

VkResult VKAPI_CALL vkCreateDescriptorSetLayout(
    VkDevice                                    device,
    const VkDescriptorSetLayoutCreateInfo*      pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkDescriptorSetLayout*                      pSetLayout)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    startReadObject(my_data, device);

    result = pTable->CreateDescriptorSetLayout(device,pCreateInfo,pAllocator,pSetLayout);
    finishReadObject(my_data, device);

    return result;
}

void VKAPI_CALL vkDestroyDescriptorSetLayout(
    VkDevice                                    device,
    VkDescriptorSetLayout                       descriptorSetLayout,
    const VkAllocationCallbacks*                pAllocator)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startReadObject(my_data, device);
    startWriteObject(my_data, descriptorSetLayout);
// Host access to descriptorSetLayout must be externally synchronized

    pTable->DestroyDescriptorSetLayout(device,descriptorSetLayout,pAllocator);
    finishReadObject(my_data, device);
    finishWriteObject(my_data, descriptorSetLayout);
// Host access to descriptorSetLayout must be externally synchronized

}

VkResult VKAPI_CALL vkCreateDescriptorPool(
    VkDevice                                    device,
    const VkDescriptorPoolCreateInfo*           pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkDescriptorPool*                           pDescriptorPool)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    startReadObject(my_data, device);

    result = pTable->CreateDescriptorPool(device,pCreateInfo,pAllocator,pDescriptorPool);
    finishReadObject(my_data, device);

    return result;
}

void VKAPI_CALL vkDestroyDescriptorPool(
    VkDevice                                    device,
    VkDescriptorPool                            descriptorPool,
    const VkAllocationCallbacks*                pAllocator)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startReadObject(my_data, device);
    startWriteObject(my_data, descriptorPool);
// Host access to descriptorPool must be externally synchronized

    pTable->DestroyDescriptorPool(device,descriptorPool,pAllocator);
    finishReadObject(my_data, device);
    finishWriteObject(my_data, descriptorPool);
// Host access to descriptorPool must be externally synchronized

}

VkResult VKAPI_CALL vkResetDescriptorPool(
    VkDevice                                    device,
    VkDescriptorPool                            descriptorPool,
    VkDescriptorPoolResetFlags                  flags)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    startReadObject(my_data, device);
    startWriteObject(my_data, descriptorPool);
// Host access to descriptorPool must be externally synchronized
    // any sname:VkDescriptorSet objects allocated from pname:descriptorPool must be externally synchronized between host accesses

    result = pTable->ResetDescriptorPool(device,descriptorPool,flags);
    finishReadObject(my_data, device);
    finishWriteObject(my_data, descriptorPool);
// Host access to descriptorPool must be externally synchronized
    // any sname:VkDescriptorSet objects allocated from pname:descriptorPool must be externally synchronized between host accesses

    return result;
}

VkResult VKAPI_CALL vkAllocateDescriptorSets(
    VkDevice                                    device,
    const VkDescriptorSetAllocateInfo*          pAllocateInfo,
    VkDescriptorSet*                            pDescriptorSets)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    startReadObject(my_data, device);
    startWriteObject(my_data, pAllocateInfo->descriptorPool);
// Host access to pAllocateInfo->descriptorPool must be externally synchronized

    result = pTable->AllocateDescriptorSets(device,pAllocateInfo,pDescriptorSets);
    finishReadObject(my_data, device);
    finishWriteObject(my_data, pAllocateInfo->descriptorPool);
// Host access to pAllocateInfo->descriptorPool must be externally synchronized

    return result;
}

VkResult VKAPI_CALL vkFreeDescriptorSets(
    VkDevice                                    device,
    VkDescriptorPool                            descriptorPool,
    uint32_t                                    descriptorSetCount,
    const VkDescriptorSet*                      pDescriptorSets)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    startReadObject(my_data, device);
    startWriteObject(my_data, descriptorPool);
    for (int index=0;index<descriptorSetCount;index++) {
        startWriteObject(my_data, pDescriptorSets[index]);
    }
// Host access to descriptorPool must be externally synchronized
// Host access to each member of pDescriptorSets must be externally synchronized

    result = pTable->FreeDescriptorSets(device,descriptorPool,descriptorSetCount,pDescriptorSets);
    finishReadObject(my_data, device);
    finishWriteObject(my_data, descriptorPool);
    for (int index=0;index<descriptorSetCount;index++) {
        finishWriteObject(my_data, pDescriptorSets[index]);
    }
// Host access to descriptorPool must be externally synchronized
// Host access to each member of pDescriptorSets must be externally synchronized

    return result;
}

void VKAPI_CALL vkUpdateDescriptorSets(
    VkDevice                                    device,
    uint32_t                                    descriptorWriteCount,
    const VkWriteDescriptorSet*                 pDescriptorWrites,
    uint32_t                                    descriptorCopyCount,
    const VkCopyDescriptorSet*                  pDescriptorCopies)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startReadObject(my_data, device);
    for (int index=0;index<descriptorWriteCount;index++) {
        startWriteObject(my_data, pDescriptorWrites[index].dstSet);
    }
    for (int index=0;index<descriptorCopyCount;index++) {
        startWriteObject(my_data, pDescriptorCopies[index].dstSet);
    }
// Host access to pDescriptorWrites[].dstSet must be externally synchronized
// Host access to pDescriptorCopies[].dstSet must be externally synchronized

    pTable->UpdateDescriptorSets(device,descriptorWriteCount,pDescriptorWrites,descriptorCopyCount,pDescriptorCopies);
    finishReadObject(my_data, device);
    for (int index=0;index<descriptorWriteCount;index++) {
        finishWriteObject(my_data, pDescriptorWrites[index].dstSet);
    }
    for (int index=0;index<descriptorCopyCount;index++) {
        finishWriteObject(my_data, pDescriptorCopies[index].dstSet);
    }
// Host access to pDescriptorWrites[].dstSet must be externally synchronized
// Host access to pDescriptorCopies[].dstSet must be externally synchronized

}

VkResult VKAPI_CALL vkCreateFramebuffer(
    VkDevice                                    device,
    const VkFramebufferCreateInfo*              pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkFramebuffer*                              pFramebuffer)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    startReadObject(my_data, device);

    result = pTable->CreateFramebuffer(device,pCreateInfo,pAllocator,pFramebuffer);
    finishReadObject(my_data, device);

    return result;
}

void VKAPI_CALL vkDestroyFramebuffer(
    VkDevice                                    device,
    VkFramebuffer                               framebuffer,
    const VkAllocationCallbacks*                pAllocator)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startReadObject(my_data, device);
    startWriteObject(my_data, framebuffer);
// Host access to framebuffer must be externally synchronized

    pTable->DestroyFramebuffer(device,framebuffer,pAllocator);
    finishReadObject(my_data, device);
    finishWriteObject(my_data, framebuffer);
// Host access to framebuffer must be externally synchronized

}

VkResult VKAPI_CALL vkCreateRenderPass(
    VkDevice                                    device,
    const VkRenderPassCreateInfo*               pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkRenderPass*                               pRenderPass)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    startReadObject(my_data, device);

    result = pTable->CreateRenderPass(device,pCreateInfo,pAllocator,pRenderPass);
    finishReadObject(my_data, device);

    return result;
}

void VKAPI_CALL vkDestroyRenderPass(
    VkDevice                                    device,
    VkRenderPass                                renderPass,
    const VkAllocationCallbacks*                pAllocator)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startReadObject(my_data, device);
    startWriteObject(my_data, renderPass);
// Host access to renderPass must be externally synchronized

    pTable->DestroyRenderPass(device,renderPass,pAllocator);
    finishReadObject(my_data, device);
    finishWriteObject(my_data, renderPass);
// Host access to renderPass must be externally synchronized

}

void VKAPI_CALL vkGetRenderAreaGranularity(
    VkDevice                                    device,
    VkRenderPass                                renderPass,
    VkExtent2D*                                 pGranularity)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startReadObject(my_data, device);
    startReadObject(my_data, renderPass);

    pTable->GetRenderAreaGranularity(device,renderPass,pGranularity);
    finishReadObject(my_data, device);
    finishReadObject(my_data, renderPass);

}

VkResult VKAPI_CALL vkCreateCommandPool(
    VkDevice                                    device,
    const VkCommandPoolCreateInfo*              pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkCommandPool*                              pCommandPool)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    startReadObject(my_data, device);

    result = pTable->CreateCommandPool(device,pCreateInfo,pAllocator,pCommandPool);
    finishReadObject(my_data, device);

    return result;
}

void VKAPI_CALL vkDestroyCommandPool(
    VkDevice                                    device,
    VkCommandPool                               commandPool,
    const VkAllocationCallbacks*                pAllocator)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startReadObject(my_data, device);
    startWriteObject(my_data, commandPool);
// Host access to commandPool must be externally synchronized

    pTable->DestroyCommandPool(device,commandPool,pAllocator);
    finishReadObject(my_data, device);
    finishWriteObject(my_data, commandPool);
// Host access to commandPool must be externally synchronized

}

VkResult VKAPI_CALL vkResetCommandPool(
    VkDevice                                    device,
    VkCommandPool                               commandPool,
    VkCommandPoolResetFlags                     flags)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    startReadObject(my_data, device);
    startWriteObject(my_data, commandPool);
// Host access to commandPool must be externally synchronized

    result = pTable->ResetCommandPool(device,commandPool,flags);
    finishReadObject(my_data, device);
    finishWriteObject(my_data, commandPool);
// Host access to commandPool must be externally synchronized

    return result;
}

VkResult VKAPI_CALL vkBeginCommandBuffer(
    VkCommandBuffer                             commandBuffer,
    const VkCommandBufferBeginInfo*             pBeginInfo)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    startWriteObject(my_data, commandBuffer);
// Host access to commandBuffer must be externally synchronized

    result = pTable->BeginCommandBuffer(commandBuffer,pBeginInfo);
    finishWriteObject(my_data, commandBuffer);
// Host access to commandBuffer must be externally synchronized

    return result;
}

VkResult VKAPI_CALL vkEndCommandBuffer(
    VkCommandBuffer                             commandBuffer)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    startWriteObject(my_data, commandBuffer);
// Host access to commandBuffer must be externally synchronized

    result = pTable->EndCommandBuffer(commandBuffer);
    finishWriteObject(my_data, commandBuffer);
// Host access to commandBuffer must be externally synchronized

    return result;
}

VkResult VKAPI_CALL vkResetCommandBuffer(
    VkCommandBuffer                             commandBuffer,
    VkCommandBufferResetFlags                   flags)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    VkResult result;
    startWriteObject(my_data, commandBuffer);
// Host access to commandBuffer must be externally synchronized

    result = pTable->ResetCommandBuffer(commandBuffer,flags);
    finishWriteObject(my_data, commandBuffer);
// Host access to commandBuffer must be externally synchronized

    return result;
}

void VKAPI_CALL vkCmdBindPipeline(
    VkCommandBuffer                             commandBuffer,
    VkPipelineBindPoint                         pipelineBindPoint,
    VkPipeline                                  pipeline)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startWriteObject(my_data, commandBuffer);
    startReadObject(my_data, pipeline);
// Host access to commandBuffer must be externally synchronized

    pTable->CmdBindPipeline(commandBuffer,pipelineBindPoint,pipeline);
    finishWriteObject(my_data, commandBuffer);
    finishReadObject(my_data, pipeline);
// Host access to commandBuffer must be externally synchronized

}

void VKAPI_CALL vkCmdSetViewport(
    VkCommandBuffer                             commandBuffer,
    uint32_t                                    firstViewport,
    uint32_t                                    viewportCount,
    const VkViewport*                           pViewports)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startWriteObject(my_data, commandBuffer);
// Host access to commandBuffer must be externally synchronized

    pTable->CmdSetViewport(commandBuffer,firstViewport,viewportCount,pViewports);
    finishWriteObject(my_data, commandBuffer);
// Host access to commandBuffer must be externally synchronized

}

void VKAPI_CALL vkCmdSetScissor(
    VkCommandBuffer                             commandBuffer,
    uint32_t                                    firstScissor,
    uint32_t                                    scissorCount,
    const VkRect2D*                             pScissors)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startWriteObject(my_data, commandBuffer);
// Host access to commandBuffer must be externally synchronized

    pTable->CmdSetScissor(commandBuffer,firstScissor,scissorCount,pScissors);
    finishWriteObject(my_data, commandBuffer);
// Host access to commandBuffer must be externally synchronized

}

void VKAPI_CALL vkCmdSetLineWidth(
    VkCommandBuffer                             commandBuffer,
    float                                       lineWidth)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startWriteObject(my_data, commandBuffer);
// Host access to commandBuffer must be externally synchronized

    pTable->CmdSetLineWidth(commandBuffer,lineWidth);
    finishWriteObject(my_data, commandBuffer);
// Host access to commandBuffer must be externally synchronized

}

void VKAPI_CALL vkCmdSetDepthBias(
    VkCommandBuffer                             commandBuffer,
    float                                       depthBiasConstantFactor,
    float                                       depthBiasClamp,
    float                                       depthBiasSlopeFactor)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startWriteObject(my_data, commandBuffer);
// Host access to commandBuffer must be externally synchronized

    pTable->CmdSetDepthBias(commandBuffer,depthBiasConstantFactor,depthBiasClamp,depthBiasSlopeFactor);
    finishWriteObject(my_data, commandBuffer);
// Host access to commandBuffer must be externally synchronized

}

void VKAPI_CALL vkCmdSetBlendConstants(
    VkCommandBuffer                             commandBuffer,
    const float                                 blendConstants[4])
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startWriteObject(my_data, commandBuffer);
// Host access to commandBuffer must be externally synchronized

    pTable->CmdSetBlendConstants(commandBuffer,blendConstants);
    finishWriteObject(my_data, commandBuffer);
// Host access to commandBuffer must be externally synchronized

}

void VKAPI_CALL vkCmdSetDepthBounds(
    VkCommandBuffer                             commandBuffer,
    float                                       minDepthBounds,
    float                                       maxDepthBounds)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startWriteObject(my_data, commandBuffer);
// Host access to commandBuffer must be externally synchronized

    pTable->CmdSetDepthBounds(commandBuffer,minDepthBounds,maxDepthBounds);
    finishWriteObject(my_data, commandBuffer);
// Host access to commandBuffer must be externally synchronized

}

void VKAPI_CALL vkCmdSetStencilCompareMask(
    VkCommandBuffer                             commandBuffer,
    VkStencilFaceFlags                          faceMask,
    uint32_t                                    compareMask)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startWriteObject(my_data, commandBuffer);
// Host access to commandBuffer must be externally synchronized

    pTable->CmdSetStencilCompareMask(commandBuffer,faceMask,compareMask);
    finishWriteObject(my_data, commandBuffer);
// Host access to commandBuffer must be externally synchronized

}

void VKAPI_CALL vkCmdSetStencilWriteMask(
    VkCommandBuffer                             commandBuffer,
    VkStencilFaceFlags                          faceMask,
    uint32_t                                    writeMask)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startWriteObject(my_data, commandBuffer);
// Host access to commandBuffer must be externally synchronized

    pTable->CmdSetStencilWriteMask(commandBuffer,faceMask,writeMask);
    finishWriteObject(my_data, commandBuffer);
// Host access to commandBuffer must be externally synchronized

}

void VKAPI_CALL vkCmdSetStencilReference(
    VkCommandBuffer                             commandBuffer,
    VkStencilFaceFlags                          faceMask,
    uint32_t                                    reference)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startWriteObject(my_data, commandBuffer);
// Host access to commandBuffer must be externally synchronized

    pTable->CmdSetStencilReference(commandBuffer,faceMask,reference);
    finishWriteObject(my_data, commandBuffer);
// Host access to commandBuffer must be externally synchronized

}

void VKAPI_CALL vkCmdBindDescriptorSets(
    VkCommandBuffer                             commandBuffer,
    VkPipelineBindPoint                         pipelineBindPoint,
    VkPipelineLayout                            layout,
    uint32_t                                    firstSet,
    uint32_t                                    descriptorSetCount,
    const VkDescriptorSet*                      pDescriptorSets,
    uint32_t                                    dynamicOffsetCount,
    const uint32_t*                             pDynamicOffsets)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startWriteObject(my_data, commandBuffer);
    startReadObject(my_data, layout);
// Host access to commandBuffer must be externally synchronized

    pTable->CmdBindDescriptorSets(commandBuffer,pipelineBindPoint,layout,firstSet,descriptorSetCount,pDescriptorSets,dynamicOffsetCount,pDynamicOffsets);
    finishWriteObject(my_data, commandBuffer);
    finishReadObject(my_data, layout);
// Host access to commandBuffer must be externally synchronized

}

void VKAPI_CALL vkCmdBindIndexBuffer(
    VkCommandBuffer                             commandBuffer,
    VkBuffer                                    buffer,
    VkDeviceSize                                offset,
    VkIndexType                                 indexType)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startWriteObject(my_data, commandBuffer);
    startReadObject(my_data, buffer);
// Host access to commandBuffer must be externally synchronized

    pTable->CmdBindIndexBuffer(commandBuffer,buffer,offset,indexType);
    finishWriteObject(my_data, commandBuffer);
    finishReadObject(my_data, buffer);
// Host access to commandBuffer must be externally synchronized

}

void VKAPI_CALL vkCmdBindVertexBuffers(
    VkCommandBuffer                             commandBuffer,
    uint32_t                                    firstBinding,
    uint32_t                                    bindingCount,
    const VkBuffer*                             pBuffers,
    const VkDeviceSize*                         pOffsets)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startWriteObject(my_data, commandBuffer);
    for (int index=0;index<bindingCount;index++) {
        startReadObject(my_data, pBuffers[index]);
    }
// Host access to commandBuffer must be externally synchronized

    pTable->CmdBindVertexBuffers(commandBuffer,firstBinding,bindingCount,pBuffers,pOffsets);
    finishWriteObject(my_data, commandBuffer);
    for (int index=0;index<bindingCount;index++) {
        finishReadObject(my_data, pBuffers[index]);
    }
// Host access to commandBuffer must be externally synchronized

}

void VKAPI_CALL vkCmdDraw(
    VkCommandBuffer                             commandBuffer,
    uint32_t                                    vertexCount,
    uint32_t                                    instanceCount,
    uint32_t                                    firstVertex,
    uint32_t                                    firstInstance)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startWriteObject(my_data, commandBuffer);
// Host access to commandBuffer must be externally synchronized

    pTable->CmdDraw(commandBuffer,vertexCount,instanceCount,firstVertex,firstInstance);
    finishWriteObject(my_data, commandBuffer);
// Host access to commandBuffer must be externally synchronized

}

void VKAPI_CALL vkCmdDrawIndexed(
    VkCommandBuffer                             commandBuffer,
    uint32_t                                    indexCount,
    uint32_t                                    instanceCount,
    uint32_t                                    firstIndex,
    int32_t                                     vertexOffset,
    uint32_t                                    firstInstance)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startWriteObject(my_data, commandBuffer);
// Host access to commandBuffer must be externally synchronized

    pTable->CmdDrawIndexed(commandBuffer,indexCount,instanceCount,firstIndex,vertexOffset,firstInstance);
    finishWriteObject(my_data, commandBuffer);
// Host access to commandBuffer must be externally synchronized

}

void VKAPI_CALL vkCmdDrawIndirect(
    VkCommandBuffer                             commandBuffer,
    VkBuffer                                    buffer,
    VkDeviceSize                                offset,
    uint32_t                                    drawCount,
    uint32_t                                    stride)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startWriteObject(my_data, commandBuffer);
    startReadObject(my_data, buffer);
// Host access to commandBuffer must be externally synchronized

    pTable->CmdDrawIndirect(commandBuffer,buffer,offset,drawCount,stride);
    finishWriteObject(my_data, commandBuffer);
    finishReadObject(my_data, buffer);
// Host access to commandBuffer must be externally synchronized

}

void VKAPI_CALL vkCmdDrawIndexedIndirect(
    VkCommandBuffer                             commandBuffer,
    VkBuffer                                    buffer,
    VkDeviceSize                                offset,
    uint32_t                                    drawCount,
    uint32_t                                    stride)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startWriteObject(my_data, commandBuffer);
    startReadObject(my_data, buffer);
// Host access to commandBuffer must be externally synchronized

    pTable->CmdDrawIndexedIndirect(commandBuffer,buffer,offset,drawCount,stride);
    finishWriteObject(my_data, commandBuffer);
    finishReadObject(my_data, buffer);
// Host access to commandBuffer must be externally synchronized

}

void VKAPI_CALL vkCmdDispatch(
    VkCommandBuffer                             commandBuffer,
    uint32_t                                    x,
    uint32_t                                    y,
    uint32_t                                    z)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startWriteObject(my_data, commandBuffer);
// Host access to commandBuffer must be externally synchronized

    pTable->CmdDispatch(commandBuffer,x,y,z);
    finishWriteObject(my_data, commandBuffer);
// Host access to commandBuffer must be externally synchronized

}

void VKAPI_CALL vkCmdDispatchIndirect(
    VkCommandBuffer                             commandBuffer,
    VkBuffer                                    buffer,
    VkDeviceSize                                offset)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startWriteObject(my_data, commandBuffer);
    startReadObject(my_data, buffer);
// Host access to commandBuffer must be externally synchronized

    pTable->CmdDispatchIndirect(commandBuffer,buffer,offset);
    finishWriteObject(my_data, commandBuffer);
    finishReadObject(my_data, buffer);
// Host access to commandBuffer must be externally synchronized

}

void VKAPI_CALL vkCmdCopyBuffer(
    VkCommandBuffer                             commandBuffer,
    VkBuffer                                    srcBuffer,
    VkBuffer                                    dstBuffer,
    uint32_t                                    regionCount,
    const VkBufferCopy*                         pRegions)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startWriteObject(my_data, commandBuffer);
    startReadObject(my_data, srcBuffer);
    startReadObject(my_data, dstBuffer);
// Host access to commandBuffer must be externally synchronized

    pTable->CmdCopyBuffer(commandBuffer,srcBuffer,dstBuffer,regionCount,pRegions);
    finishWriteObject(my_data, commandBuffer);
    finishReadObject(my_data, srcBuffer);
    finishReadObject(my_data, dstBuffer);
// Host access to commandBuffer must be externally synchronized

}

void VKAPI_CALL vkCmdCopyImage(
    VkCommandBuffer                             commandBuffer,
    VkImage                                     srcImage,
    VkImageLayout                               srcImageLayout,
    VkImage                                     dstImage,
    VkImageLayout                               dstImageLayout,
    uint32_t                                    regionCount,
    const VkImageCopy*                          pRegions)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startWriteObject(my_data, commandBuffer);
    startReadObject(my_data, srcImage);
    startReadObject(my_data, dstImage);
// Host access to commandBuffer must be externally synchronized

    pTable->CmdCopyImage(commandBuffer,srcImage,srcImageLayout,dstImage,dstImageLayout,regionCount,pRegions);
    finishWriteObject(my_data, commandBuffer);
    finishReadObject(my_data, srcImage);
    finishReadObject(my_data, dstImage);
// Host access to commandBuffer must be externally synchronized

}

void VKAPI_CALL vkCmdBlitImage(
    VkCommandBuffer                             commandBuffer,
    VkImage                                     srcImage,
    VkImageLayout                               srcImageLayout,
    VkImage                                     dstImage,
    VkImageLayout                               dstImageLayout,
    uint32_t                                    regionCount,
    const VkImageBlit*                          pRegions,
    VkFilter                                    filter)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startWriteObject(my_data, commandBuffer);
    startReadObject(my_data, srcImage);
    startReadObject(my_data, dstImage);
// Host access to commandBuffer must be externally synchronized

    pTable->CmdBlitImage(commandBuffer,srcImage,srcImageLayout,dstImage,dstImageLayout,regionCount,pRegions,filter);
    finishWriteObject(my_data, commandBuffer);
    finishReadObject(my_data, srcImage);
    finishReadObject(my_data, dstImage);
// Host access to commandBuffer must be externally synchronized

}

void VKAPI_CALL vkCmdCopyBufferToImage(
    VkCommandBuffer                             commandBuffer,
    VkBuffer                                    srcBuffer,
    VkImage                                     dstImage,
    VkImageLayout                               dstImageLayout,
    uint32_t                                    regionCount,
    const VkBufferImageCopy*                    pRegions)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startWriteObject(my_data, commandBuffer);
    startReadObject(my_data, srcBuffer);
    startReadObject(my_data, dstImage);
// Host access to commandBuffer must be externally synchronized

    pTable->CmdCopyBufferToImage(commandBuffer,srcBuffer,dstImage,dstImageLayout,regionCount,pRegions);
    finishWriteObject(my_data, commandBuffer);
    finishReadObject(my_data, srcBuffer);
    finishReadObject(my_data, dstImage);
// Host access to commandBuffer must be externally synchronized

}

void VKAPI_CALL vkCmdCopyImageToBuffer(
    VkCommandBuffer                             commandBuffer,
    VkImage                                     srcImage,
    VkImageLayout                               srcImageLayout,
    VkBuffer                                    dstBuffer,
    uint32_t                                    regionCount,
    const VkBufferImageCopy*                    pRegions)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startWriteObject(my_data, commandBuffer);
    startReadObject(my_data, srcImage);
    startReadObject(my_data, dstBuffer);
// Host access to commandBuffer must be externally synchronized

    pTable->CmdCopyImageToBuffer(commandBuffer,srcImage,srcImageLayout,dstBuffer,regionCount,pRegions);
    finishWriteObject(my_data, commandBuffer);
    finishReadObject(my_data, srcImage);
    finishReadObject(my_data, dstBuffer);
// Host access to commandBuffer must be externally synchronized

}

void VKAPI_CALL vkCmdUpdateBuffer(
    VkCommandBuffer                             commandBuffer,
    VkBuffer                                    dstBuffer,
    VkDeviceSize                                dstOffset,
    VkDeviceSize                                dataSize,
    const uint32_t*                             pData)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startWriteObject(my_data, commandBuffer);
    startReadObject(my_data, dstBuffer);
// Host access to commandBuffer must be externally synchronized

    pTable->CmdUpdateBuffer(commandBuffer,dstBuffer,dstOffset,dataSize,pData);
    finishWriteObject(my_data, commandBuffer);
    finishReadObject(my_data, dstBuffer);
// Host access to commandBuffer must be externally synchronized

}

void VKAPI_CALL vkCmdFillBuffer(
    VkCommandBuffer                             commandBuffer,
    VkBuffer                                    dstBuffer,
    VkDeviceSize                                dstOffset,
    VkDeviceSize                                size,
    uint32_t                                    data)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startWriteObject(my_data, commandBuffer);
    startReadObject(my_data, dstBuffer);
// Host access to commandBuffer must be externally synchronized

    pTable->CmdFillBuffer(commandBuffer,dstBuffer,dstOffset,size,data);
    finishWriteObject(my_data, commandBuffer);
    finishReadObject(my_data, dstBuffer);
// Host access to commandBuffer must be externally synchronized

}

void VKAPI_CALL vkCmdClearColorImage(
    VkCommandBuffer                             commandBuffer,
    VkImage                                     image,
    VkImageLayout                               imageLayout,
    const VkClearColorValue*                    pColor,
    uint32_t                                    rangeCount,
    const VkImageSubresourceRange*              pRanges)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startWriteObject(my_data, commandBuffer);
    startReadObject(my_data, image);
// Host access to commandBuffer must be externally synchronized

    pTable->CmdClearColorImage(commandBuffer,image,imageLayout,pColor,rangeCount,pRanges);
    finishWriteObject(my_data, commandBuffer);
    finishReadObject(my_data, image);
// Host access to commandBuffer must be externally synchronized

}

void VKAPI_CALL vkCmdClearDepthStencilImage(
    VkCommandBuffer                             commandBuffer,
    VkImage                                     image,
    VkImageLayout                               imageLayout,
    const VkClearDepthStencilValue*             pDepthStencil,
    uint32_t                                    rangeCount,
    const VkImageSubresourceRange*              pRanges)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startWriteObject(my_data, commandBuffer);
    startReadObject(my_data, image);
// Host access to commandBuffer must be externally synchronized

    pTable->CmdClearDepthStencilImage(commandBuffer,image,imageLayout,pDepthStencil,rangeCount,pRanges);
    finishWriteObject(my_data, commandBuffer);
    finishReadObject(my_data, image);
// Host access to commandBuffer must be externally synchronized

}

void VKAPI_CALL vkCmdClearAttachments(
    VkCommandBuffer                             commandBuffer,
    uint32_t                                    attachmentCount,
    const VkClearAttachment*                    pAttachments,
    uint32_t                                    rectCount,
    const VkClearRect*                          pRects)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startWriteObject(my_data, commandBuffer);
// Host access to commandBuffer must be externally synchronized

    pTable->CmdClearAttachments(commandBuffer,attachmentCount,pAttachments,rectCount,pRects);
    finishWriteObject(my_data, commandBuffer);
// Host access to commandBuffer must be externally synchronized

}

void VKAPI_CALL vkCmdResolveImage(
    VkCommandBuffer                             commandBuffer,
    VkImage                                     srcImage,
    VkImageLayout                               srcImageLayout,
    VkImage                                     dstImage,
    VkImageLayout                               dstImageLayout,
    uint32_t                                    regionCount,
    const VkImageResolve*                       pRegions)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startWriteObject(my_data, commandBuffer);
    startReadObject(my_data, srcImage);
    startReadObject(my_data, dstImage);
// Host access to commandBuffer must be externally synchronized

    pTable->CmdResolveImage(commandBuffer,srcImage,srcImageLayout,dstImage,dstImageLayout,regionCount,pRegions);
    finishWriteObject(my_data, commandBuffer);
    finishReadObject(my_data, srcImage);
    finishReadObject(my_data, dstImage);
// Host access to commandBuffer must be externally synchronized

}

void VKAPI_CALL vkCmdSetEvent(
    VkCommandBuffer                             commandBuffer,
    VkEvent                                     event,
    VkPipelineStageFlags                        stageMask)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startWriteObject(my_data, commandBuffer);
    startReadObject(my_data, event);
// Host access to commandBuffer must be externally synchronized

    pTable->CmdSetEvent(commandBuffer,event,stageMask);
    finishWriteObject(my_data, commandBuffer);
    finishReadObject(my_data, event);
// Host access to commandBuffer must be externally synchronized

}

void VKAPI_CALL vkCmdResetEvent(
    VkCommandBuffer                             commandBuffer,
    VkEvent                                     event,
    VkPipelineStageFlags                        stageMask)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startWriteObject(my_data, commandBuffer);
    startReadObject(my_data, event);
// Host access to commandBuffer must be externally synchronized

    pTable->CmdResetEvent(commandBuffer,event,stageMask);
    finishWriteObject(my_data, commandBuffer);
    finishReadObject(my_data, event);
// Host access to commandBuffer must be externally synchronized

}

void VKAPI_CALL vkCmdWaitEvents(
    VkCommandBuffer                             commandBuffer,
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
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startWriteObject(my_data, commandBuffer);
    for (int index=0;index<eventCount;index++) {
        startReadObject(my_data, pEvents[index]);
    }
// Host access to commandBuffer must be externally synchronized

    pTable->CmdWaitEvents(commandBuffer,eventCount,pEvents,srcStageMask,dstStageMask,memoryBarrierCount,pMemoryBarriers,bufferMemoryBarrierCount,pBufferMemoryBarriers,imageMemoryBarrierCount,pImageMemoryBarriers);
    finishWriteObject(my_data, commandBuffer);
    for (int index=0;index<eventCount;index++) {
        finishReadObject(my_data, pEvents[index]);
    }
// Host access to commandBuffer must be externally synchronized

}

void VKAPI_CALL vkCmdPipelineBarrier(
    VkCommandBuffer                             commandBuffer,
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
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startWriteObject(my_data, commandBuffer);
// Host access to commandBuffer must be externally synchronized

    pTable->CmdPipelineBarrier(commandBuffer,srcStageMask,dstStageMask,dependencyFlags,memoryBarrierCount,pMemoryBarriers,bufferMemoryBarrierCount,pBufferMemoryBarriers,imageMemoryBarrierCount,pImageMemoryBarriers);
    finishWriteObject(my_data, commandBuffer);
// Host access to commandBuffer must be externally synchronized

}

void VKAPI_CALL vkCmdBeginQuery(
    VkCommandBuffer                             commandBuffer,
    VkQueryPool                                 queryPool,
    uint32_t                                    query,
    VkQueryControlFlags                         flags)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startWriteObject(my_data, commandBuffer);
    startReadObject(my_data, queryPool);
// Host access to commandBuffer must be externally synchronized

    pTable->CmdBeginQuery(commandBuffer,queryPool,query,flags);
    finishWriteObject(my_data, commandBuffer);
    finishReadObject(my_data, queryPool);
// Host access to commandBuffer must be externally synchronized

}

void VKAPI_CALL vkCmdEndQuery(
    VkCommandBuffer                             commandBuffer,
    VkQueryPool                                 queryPool,
    uint32_t                                    query)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startWriteObject(my_data, commandBuffer);
    startReadObject(my_data, queryPool);
// Host access to commandBuffer must be externally synchronized

    pTable->CmdEndQuery(commandBuffer,queryPool,query);
    finishWriteObject(my_data, commandBuffer);
    finishReadObject(my_data, queryPool);
// Host access to commandBuffer must be externally synchronized

}

void VKAPI_CALL vkCmdResetQueryPool(
    VkCommandBuffer                             commandBuffer,
    VkQueryPool                                 queryPool,
    uint32_t                                    firstQuery,
    uint32_t                                    queryCount)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startWriteObject(my_data, commandBuffer);
    startReadObject(my_data, queryPool);
// Host access to commandBuffer must be externally synchronized

    pTable->CmdResetQueryPool(commandBuffer,queryPool,firstQuery,queryCount);
    finishWriteObject(my_data, commandBuffer);
    finishReadObject(my_data, queryPool);
// Host access to commandBuffer must be externally synchronized

}

void VKAPI_CALL vkCmdWriteTimestamp(
    VkCommandBuffer                             commandBuffer,
    VkPipelineStageFlagBits                     pipelineStage,
    VkQueryPool                                 queryPool,
    uint32_t                                    query)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startWriteObject(my_data, commandBuffer);
    startReadObject(my_data, queryPool);
// Host access to commandBuffer must be externally synchronized

    pTable->CmdWriteTimestamp(commandBuffer,pipelineStage,queryPool,query);
    finishWriteObject(my_data, commandBuffer);
    finishReadObject(my_data, queryPool);
// Host access to commandBuffer must be externally synchronized

}

void VKAPI_CALL vkCmdCopyQueryPoolResults(
    VkCommandBuffer                             commandBuffer,
    VkQueryPool                                 queryPool,
    uint32_t                                    firstQuery,
    uint32_t                                    queryCount,
    VkBuffer                                    dstBuffer,
    VkDeviceSize                                dstOffset,
    VkDeviceSize                                stride,
    VkQueryResultFlags                          flags)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startWriteObject(my_data, commandBuffer);
    startReadObject(my_data, queryPool);
    startReadObject(my_data, dstBuffer);
// Host access to commandBuffer must be externally synchronized

    pTable->CmdCopyQueryPoolResults(commandBuffer,queryPool,firstQuery,queryCount,dstBuffer,dstOffset,stride,flags);
    finishWriteObject(my_data, commandBuffer);
    finishReadObject(my_data, queryPool);
    finishReadObject(my_data, dstBuffer);
// Host access to commandBuffer must be externally synchronized

}

void VKAPI_CALL vkCmdPushConstants(
    VkCommandBuffer                             commandBuffer,
    VkPipelineLayout                            layout,
    VkShaderStageFlags                          stageFlags,
    uint32_t                                    offset,
    uint32_t                                    size,
    const void*                                 pValues)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startWriteObject(my_data, commandBuffer);
    startReadObject(my_data, layout);
// Host access to commandBuffer must be externally synchronized

    pTable->CmdPushConstants(commandBuffer,layout,stageFlags,offset,size,pValues);
    finishWriteObject(my_data, commandBuffer);
    finishReadObject(my_data, layout);
// Host access to commandBuffer must be externally synchronized

}

void VKAPI_CALL vkCmdBeginRenderPass(
    VkCommandBuffer                             commandBuffer,
    const VkRenderPassBeginInfo*                pRenderPassBegin,
    VkSubpassContents                           contents)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startWriteObject(my_data, commandBuffer);
// Host access to commandBuffer must be externally synchronized

    pTable->CmdBeginRenderPass(commandBuffer,pRenderPassBegin,contents);
    finishWriteObject(my_data, commandBuffer);
// Host access to commandBuffer must be externally synchronized

}

void VKAPI_CALL vkCmdNextSubpass(
    VkCommandBuffer                             commandBuffer,
    VkSubpassContents                           contents)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startWriteObject(my_data, commandBuffer);
// Host access to commandBuffer must be externally synchronized

    pTable->CmdNextSubpass(commandBuffer,contents);
    finishWriteObject(my_data, commandBuffer);
// Host access to commandBuffer must be externally synchronized

}

void VKAPI_CALL vkCmdEndRenderPass(
    VkCommandBuffer                             commandBuffer)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startWriteObject(my_data, commandBuffer);
// Host access to commandBuffer must be externally synchronized

    pTable->CmdEndRenderPass(commandBuffer);
    finishWriteObject(my_data, commandBuffer);
// Host access to commandBuffer must be externally synchronized

}

void VKAPI_CALL vkCmdExecuteCommands(
    VkCommandBuffer                             commandBuffer,
    uint32_t                                    commandBufferCount,
    const VkCommandBuffer*                      pCommandBuffers)
{
    dispatch_key key = get_dispatch_key(commandBuffer);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    startWriteObject(my_data, commandBuffer);
    for (int index=0;index<commandBufferCount;index++) {
        startReadObject(my_data, pCommandBuffers[index]);
    }
// Host access to commandBuffer must be externally synchronized

    pTable->CmdExecuteCommands(commandBuffer,commandBufferCount,pCommandBuffers);
    finishWriteObject(my_data, commandBuffer);
    for (int index=0;index<commandBufferCount;index++) {
        finishReadObject(my_data, pCommandBuffers[index]);
    }
// Host access to commandBuffer must be externally synchronized

}

// TODO - not wrapping KHR function vkDestroySurfaceKHR
// TODO - not wrapping KHR function vkGetPhysicalDeviceSurfaceSupportKHR
// TODO - not wrapping KHR function vkGetPhysicalDeviceSurfaceCapabilitiesKHR
// TODO - not wrapping KHR function vkGetPhysicalDeviceSurfaceFormatsKHR
// TODO - not wrapping KHR function vkGetPhysicalDeviceSurfacePresentModesKHR

// TODO - not wrapping KHR function vkCreateSwapchainKHR
// TODO - not wrapping KHR function vkDestroySwapchainKHR
// TODO - not wrapping KHR function vkGetSwapchainImagesKHR
// TODO - not wrapping KHR function vkAcquireNextImageKHR
// TODO - not wrapping KHR function vkQueuePresentKHR

// TODO - not wrapping KHR function vkGetPhysicalDeviceDisplayPropertiesKHR
// TODO - not wrapping KHR function vkGetPhysicalDeviceDisplayPlanePropertiesKHR
// TODO - not wrapping KHR function vkGetDisplayPlaneSupportedDisplaysKHR
// TODO - not wrapping KHR function vkGetDisplayModePropertiesKHR
// TODO - not wrapping KHR function vkCreateDisplayModeKHR
// TODO - not wrapping KHR function vkGetDisplayPlaneCapabilitiesKHR
// TODO - not wrapping KHR function vkCreateDisplayPlaneSurfaceKHR

// TODO - not wrapping KHR function vkCreateSharedSwapchainsKHR

#ifdef VK_USE_PLATFORM_XLIB_KHR
// TODO - not wrapping KHR function vkCreateXlibSurfaceKHR
// TODO - not wrapping KHR function vkGetPhysicalDeviceXlibPresentationSupportKHR
#endif /* VK_USE_PLATFORM_XLIB_KHR */

#ifdef VK_USE_PLATFORM_XCB_KHR
// TODO - not wrapping KHR function vkCreateXcbSurfaceKHR
// TODO - not wrapping KHR function vkGetPhysicalDeviceXcbPresentationSupportKHR
#endif /* VK_USE_PLATFORM_XCB_KHR */

#ifdef VK_USE_PLATFORM_WAYLAND_KHR
// TODO - not wrapping KHR function vkCreateWaylandSurfaceKHR
// TODO - not wrapping KHR function vkGetPhysicalDeviceWaylandPresentationSupportKHR
#endif /* VK_USE_PLATFORM_WAYLAND_KHR */

#ifdef VK_USE_PLATFORM_MIR_KHR
// TODO - not wrapping KHR function vkCreateMirSurfaceKHR
// TODO - not wrapping KHR function vkGetPhysicalDeviceMirPresentationSupportKHR
#endif /* VK_USE_PLATFORM_MIR_KHR */

#ifdef VK_USE_PLATFORM_ANDROID_KHR
// TODO - not wrapping KHR function vkCreateAndroidSurfaceKHR
#endif /* VK_USE_PLATFORM_ANDROID_KHR */

#ifdef VK_USE_PLATFORM_WIN32_KHR
// TODO - not wrapping KHR function vkCreateWin32SurfaceKHR
// TODO - not wrapping KHR function vkGetPhysicalDeviceWin32PresentationSupportKHR
#endif /* VK_USE_PLATFORM_WIN32_KHR */


void VKAPI_CALL vkDebugReportMessageEXT(
    VkInstance                                  instance,
    VkDebugReportFlagsEXT                       flags,
    VkDebugReportObjectTypeEXT                  objectType,
    uint64_t                                    object,
    size_t                                      location,
    int32_t                                     messageCode,
    const char*                                 pLayerPrefix,
    const char*                                 pMessage)
{
    dispatch_key key = get_dispatch_key(instance);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerInstanceDispatchTable *pTable = my_data->instance_dispatch_table;
    startReadObject(my_data, instance);

    pTable->DebugReportMessageEXT(instance,flags,objectType,object,location,messageCode,pLayerPrefix,pMessage);
    finishReadObject(my_data, instance);

}

// intercepts
struct { const char* name; PFN_vkVoidFunction pFunc;} procmap[] = {
    {"vkCreateInstance", reinterpret_cast<PFN_vkVoidFunction>(vkCreateInstance)},
    {"vkDestroyInstance", reinterpret_cast<PFN_vkVoidFunction>(vkDestroyInstance)},
    {"vkEnumeratePhysicalDevices", reinterpret_cast<PFN_vkVoidFunction>(vkEnumeratePhysicalDevices)},
    {"vkGetInstanceProcAddr", reinterpret_cast<PFN_vkVoidFunction>(vkGetInstanceProcAddr)},
    {"vkGetDeviceProcAddr", reinterpret_cast<PFN_vkVoidFunction>(vkGetDeviceProcAddr)},
    {"vkCreateDevice", reinterpret_cast<PFN_vkVoidFunction>(vkCreateDevice)},
    {"vkDestroyDevice", reinterpret_cast<PFN_vkVoidFunction>(vkDestroyDevice)},
    {"vkEnumerateInstanceExtensionProperties", reinterpret_cast<PFN_vkVoidFunction>(vkEnumerateInstanceExtensionProperties)},
    {"vkEnumerateInstanceLayerProperties", reinterpret_cast<PFN_vkVoidFunction>(vkEnumerateInstanceLayerProperties)},
    {"vkGetDeviceQueue", reinterpret_cast<PFN_vkVoidFunction>(vkGetDeviceQueue)},
    {"vkQueueSubmit", reinterpret_cast<PFN_vkVoidFunction>(vkQueueSubmit)},
    {"vkQueueWaitIdle", reinterpret_cast<PFN_vkVoidFunction>(vkQueueWaitIdle)},
    {"vkDeviceWaitIdle", reinterpret_cast<PFN_vkVoidFunction>(vkDeviceWaitIdle)},
    {"vkAllocateMemory", reinterpret_cast<PFN_vkVoidFunction>(vkAllocateMemory)},
    {"vkFreeMemory", reinterpret_cast<PFN_vkVoidFunction>(vkFreeMemory)},
    {"vkMapMemory", reinterpret_cast<PFN_vkVoidFunction>(vkMapMemory)},
    {"vkUnmapMemory", reinterpret_cast<PFN_vkVoidFunction>(vkUnmapMemory)},
    {"vkFlushMappedMemoryRanges", reinterpret_cast<PFN_vkVoidFunction>(vkFlushMappedMemoryRanges)},
    {"vkInvalidateMappedMemoryRanges", reinterpret_cast<PFN_vkVoidFunction>(vkInvalidateMappedMemoryRanges)},
    {"vkGetDeviceMemoryCommitment", reinterpret_cast<PFN_vkVoidFunction>(vkGetDeviceMemoryCommitment)},
    {"vkBindBufferMemory", reinterpret_cast<PFN_vkVoidFunction>(vkBindBufferMemory)},
    {"vkBindImageMemory", reinterpret_cast<PFN_vkVoidFunction>(vkBindImageMemory)},
    {"vkGetBufferMemoryRequirements", reinterpret_cast<PFN_vkVoidFunction>(vkGetBufferMemoryRequirements)},
    {"vkGetImageMemoryRequirements", reinterpret_cast<PFN_vkVoidFunction>(vkGetImageMemoryRequirements)},
    {"vkGetImageSparseMemoryRequirements", reinterpret_cast<PFN_vkVoidFunction>(vkGetImageSparseMemoryRequirements)},
    {"vkQueueBindSparse", reinterpret_cast<PFN_vkVoidFunction>(vkQueueBindSparse)},
    {"vkCreateFence", reinterpret_cast<PFN_vkVoidFunction>(vkCreateFence)},
    {"vkDestroyFence", reinterpret_cast<PFN_vkVoidFunction>(vkDestroyFence)},
    {"vkResetFences", reinterpret_cast<PFN_vkVoidFunction>(vkResetFences)},
    {"vkGetFenceStatus", reinterpret_cast<PFN_vkVoidFunction>(vkGetFenceStatus)},
    {"vkWaitForFences", reinterpret_cast<PFN_vkVoidFunction>(vkWaitForFences)},
    {"vkCreateSemaphore", reinterpret_cast<PFN_vkVoidFunction>(vkCreateSemaphore)},
    {"vkDestroySemaphore", reinterpret_cast<PFN_vkVoidFunction>(vkDestroySemaphore)},
    {"vkCreateEvent", reinterpret_cast<PFN_vkVoidFunction>(vkCreateEvent)},
    {"vkDestroyEvent", reinterpret_cast<PFN_vkVoidFunction>(vkDestroyEvent)},
    {"vkGetEventStatus", reinterpret_cast<PFN_vkVoidFunction>(vkGetEventStatus)},
    {"vkSetEvent", reinterpret_cast<PFN_vkVoidFunction>(vkSetEvent)},
    {"vkResetEvent", reinterpret_cast<PFN_vkVoidFunction>(vkResetEvent)},
    {"vkCreateQueryPool", reinterpret_cast<PFN_vkVoidFunction>(vkCreateQueryPool)},
    {"vkDestroyQueryPool", reinterpret_cast<PFN_vkVoidFunction>(vkDestroyQueryPool)},
    {"vkGetQueryPoolResults", reinterpret_cast<PFN_vkVoidFunction>(vkGetQueryPoolResults)},
    {"vkCreateBuffer", reinterpret_cast<PFN_vkVoidFunction>(vkCreateBuffer)},
    {"vkDestroyBuffer", reinterpret_cast<PFN_vkVoidFunction>(vkDestroyBuffer)},
    {"vkCreateBufferView", reinterpret_cast<PFN_vkVoidFunction>(vkCreateBufferView)},
    {"vkDestroyBufferView", reinterpret_cast<PFN_vkVoidFunction>(vkDestroyBufferView)},
    {"vkCreateImage", reinterpret_cast<PFN_vkVoidFunction>(vkCreateImage)},
    {"vkDestroyImage", reinterpret_cast<PFN_vkVoidFunction>(vkDestroyImage)},
    {"vkGetImageSubresourceLayout", reinterpret_cast<PFN_vkVoidFunction>(vkGetImageSubresourceLayout)},
    {"vkCreateImageView", reinterpret_cast<PFN_vkVoidFunction>(vkCreateImageView)},
    {"vkDestroyImageView", reinterpret_cast<PFN_vkVoidFunction>(vkDestroyImageView)},
    {"vkCreateShaderModule", reinterpret_cast<PFN_vkVoidFunction>(vkCreateShaderModule)},
    {"vkDestroyShaderModule", reinterpret_cast<PFN_vkVoidFunction>(vkDestroyShaderModule)},
    {"vkCreatePipelineCache", reinterpret_cast<PFN_vkVoidFunction>(vkCreatePipelineCache)},
    {"vkDestroyPipelineCache", reinterpret_cast<PFN_vkVoidFunction>(vkDestroyPipelineCache)},
    {"vkGetPipelineCacheData", reinterpret_cast<PFN_vkVoidFunction>(vkGetPipelineCacheData)},
    {"vkMergePipelineCaches", reinterpret_cast<PFN_vkVoidFunction>(vkMergePipelineCaches)},
    {"vkCreateGraphicsPipelines", reinterpret_cast<PFN_vkVoidFunction>(vkCreateGraphicsPipelines)},
    {"vkCreateComputePipelines", reinterpret_cast<PFN_vkVoidFunction>(vkCreateComputePipelines)},
    {"vkDestroyPipeline", reinterpret_cast<PFN_vkVoidFunction>(vkDestroyPipeline)},
    {"vkCreatePipelineLayout", reinterpret_cast<PFN_vkVoidFunction>(vkCreatePipelineLayout)},
    {"vkDestroyPipelineLayout", reinterpret_cast<PFN_vkVoidFunction>(vkDestroyPipelineLayout)},
    {"vkCreateSampler", reinterpret_cast<PFN_vkVoidFunction>(vkCreateSampler)},
    {"vkDestroySampler", reinterpret_cast<PFN_vkVoidFunction>(vkDestroySampler)},
    {"vkCreateDescriptorSetLayout", reinterpret_cast<PFN_vkVoidFunction>(vkCreateDescriptorSetLayout)},
    {"vkDestroyDescriptorSetLayout", reinterpret_cast<PFN_vkVoidFunction>(vkDestroyDescriptorSetLayout)},
    {"vkCreateDescriptorPool", reinterpret_cast<PFN_vkVoidFunction>(vkCreateDescriptorPool)},
    {"vkDestroyDescriptorPool", reinterpret_cast<PFN_vkVoidFunction>(vkDestroyDescriptorPool)},
    {"vkResetDescriptorPool", reinterpret_cast<PFN_vkVoidFunction>(vkResetDescriptorPool)},
    {"vkAllocateDescriptorSets", reinterpret_cast<PFN_vkVoidFunction>(vkAllocateDescriptorSets)},
    {"vkFreeDescriptorSets", reinterpret_cast<PFN_vkVoidFunction>(vkFreeDescriptorSets)},
    {"vkUpdateDescriptorSets", reinterpret_cast<PFN_vkVoidFunction>(vkUpdateDescriptorSets)},
    {"vkCreateFramebuffer", reinterpret_cast<PFN_vkVoidFunction>(vkCreateFramebuffer)},
    {"vkDestroyFramebuffer", reinterpret_cast<PFN_vkVoidFunction>(vkDestroyFramebuffer)},
    {"vkCreateRenderPass", reinterpret_cast<PFN_vkVoidFunction>(vkCreateRenderPass)},
    {"vkDestroyRenderPass", reinterpret_cast<PFN_vkVoidFunction>(vkDestroyRenderPass)},
    {"vkGetRenderAreaGranularity", reinterpret_cast<PFN_vkVoidFunction>(vkGetRenderAreaGranularity)},
    {"vkCreateCommandPool", reinterpret_cast<PFN_vkVoidFunction>(vkCreateCommandPool)},
    {"vkDestroyCommandPool", reinterpret_cast<PFN_vkVoidFunction>(vkDestroyCommandPool)},
    {"vkResetCommandPool", reinterpret_cast<PFN_vkVoidFunction>(vkResetCommandPool)},
    {"vkAllocateCommandBuffers", reinterpret_cast<PFN_vkVoidFunction>(vkAllocateCommandBuffers)},
    {"vkFreeCommandBuffers", reinterpret_cast<PFN_vkVoidFunction>(vkFreeCommandBuffers)},
    {"vkBeginCommandBuffer", reinterpret_cast<PFN_vkVoidFunction>(vkBeginCommandBuffer)},
    {"vkEndCommandBuffer", reinterpret_cast<PFN_vkVoidFunction>(vkEndCommandBuffer)},
    {"vkResetCommandBuffer", reinterpret_cast<PFN_vkVoidFunction>(vkResetCommandBuffer)},
    {"vkCmdBindPipeline", reinterpret_cast<PFN_vkVoidFunction>(vkCmdBindPipeline)},
    {"vkCmdSetViewport", reinterpret_cast<PFN_vkVoidFunction>(vkCmdSetViewport)},
    {"vkCmdSetScissor", reinterpret_cast<PFN_vkVoidFunction>(vkCmdSetScissor)},
    {"vkCmdSetLineWidth", reinterpret_cast<PFN_vkVoidFunction>(vkCmdSetLineWidth)},
    {"vkCmdSetDepthBias", reinterpret_cast<PFN_vkVoidFunction>(vkCmdSetDepthBias)},
    {"vkCmdSetBlendConstants", reinterpret_cast<PFN_vkVoidFunction>(vkCmdSetBlendConstants)},
    {"vkCmdSetDepthBounds", reinterpret_cast<PFN_vkVoidFunction>(vkCmdSetDepthBounds)},
    {"vkCmdSetStencilCompareMask", reinterpret_cast<PFN_vkVoidFunction>(vkCmdSetStencilCompareMask)},
    {"vkCmdSetStencilWriteMask", reinterpret_cast<PFN_vkVoidFunction>(vkCmdSetStencilWriteMask)},
    {"vkCmdSetStencilReference", reinterpret_cast<PFN_vkVoidFunction>(vkCmdSetStencilReference)},
    {"vkCmdBindDescriptorSets", reinterpret_cast<PFN_vkVoidFunction>(vkCmdBindDescriptorSets)},
    {"vkCmdBindIndexBuffer", reinterpret_cast<PFN_vkVoidFunction>(vkCmdBindIndexBuffer)},
    {"vkCmdBindVertexBuffers", reinterpret_cast<PFN_vkVoidFunction>(vkCmdBindVertexBuffers)},
    {"vkCmdDraw", reinterpret_cast<PFN_vkVoidFunction>(vkCmdDraw)},
    {"vkCmdDrawIndexed", reinterpret_cast<PFN_vkVoidFunction>(vkCmdDrawIndexed)},
    {"vkCmdDrawIndirect", reinterpret_cast<PFN_vkVoidFunction>(vkCmdDrawIndirect)},
    {"vkCmdDrawIndexedIndirect", reinterpret_cast<PFN_vkVoidFunction>(vkCmdDrawIndexedIndirect)},
    {"vkCmdDispatch", reinterpret_cast<PFN_vkVoidFunction>(vkCmdDispatch)},
    {"vkCmdDispatchIndirect", reinterpret_cast<PFN_vkVoidFunction>(vkCmdDispatchIndirect)},
    {"vkCmdCopyBuffer", reinterpret_cast<PFN_vkVoidFunction>(vkCmdCopyBuffer)},
    {"vkCmdCopyImage", reinterpret_cast<PFN_vkVoidFunction>(vkCmdCopyImage)},
    {"vkCmdBlitImage", reinterpret_cast<PFN_vkVoidFunction>(vkCmdBlitImage)},
    {"vkCmdCopyBufferToImage", reinterpret_cast<PFN_vkVoidFunction>(vkCmdCopyBufferToImage)},
    {"vkCmdCopyImageToBuffer", reinterpret_cast<PFN_vkVoidFunction>(vkCmdCopyImageToBuffer)},
    {"vkCmdUpdateBuffer", reinterpret_cast<PFN_vkVoidFunction>(vkCmdUpdateBuffer)},
    {"vkCmdFillBuffer", reinterpret_cast<PFN_vkVoidFunction>(vkCmdFillBuffer)},
    {"vkCmdClearColorImage", reinterpret_cast<PFN_vkVoidFunction>(vkCmdClearColorImage)},
    {"vkCmdClearDepthStencilImage", reinterpret_cast<PFN_vkVoidFunction>(vkCmdClearDepthStencilImage)},
    {"vkCmdClearAttachments", reinterpret_cast<PFN_vkVoidFunction>(vkCmdClearAttachments)},
    {"vkCmdResolveImage", reinterpret_cast<PFN_vkVoidFunction>(vkCmdResolveImage)},
    {"vkCmdSetEvent", reinterpret_cast<PFN_vkVoidFunction>(vkCmdSetEvent)},
    {"vkCmdResetEvent", reinterpret_cast<PFN_vkVoidFunction>(vkCmdResetEvent)},
    {"vkCmdWaitEvents", reinterpret_cast<PFN_vkVoidFunction>(vkCmdWaitEvents)},
    {"vkCmdPipelineBarrier", reinterpret_cast<PFN_vkVoidFunction>(vkCmdPipelineBarrier)},
    {"vkCmdBeginQuery", reinterpret_cast<PFN_vkVoidFunction>(vkCmdBeginQuery)},
    {"vkCmdEndQuery", reinterpret_cast<PFN_vkVoidFunction>(vkCmdEndQuery)},
    {"vkCmdResetQueryPool", reinterpret_cast<PFN_vkVoidFunction>(vkCmdResetQueryPool)},
    {"vkCmdWriteTimestamp", reinterpret_cast<PFN_vkVoidFunction>(vkCmdWriteTimestamp)},
    {"vkCmdCopyQueryPoolResults", reinterpret_cast<PFN_vkVoidFunction>(vkCmdCopyQueryPoolResults)},
    {"vkCmdPushConstants", reinterpret_cast<PFN_vkVoidFunction>(vkCmdPushConstants)},
    {"vkCmdBeginRenderPass", reinterpret_cast<PFN_vkVoidFunction>(vkCmdBeginRenderPass)},
    {"vkCmdNextSubpass", reinterpret_cast<PFN_vkVoidFunction>(vkCmdNextSubpass)},
    {"vkCmdEndRenderPass", reinterpret_cast<PFN_vkVoidFunction>(vkCmdEndRenderPass)},
    {"vkCmdExecuteCommands", reinterpret_cast<PFN_vkVoidFunction>(vkCmdExecuteCommands)},
    {"vkCreateDebugReportCallbackEXT", reinterpret_cast<PFN_vkVoidFunction>(vkCreateDebugReportCallbackEXT)},
    {"vkDestroyDebugReportCallbackEXT", reinterpret_cast<PFN_vkVoidFunction>(vkDestroyDebugReportCallbackEXT)},
    {"vkDebugReportMessageEXT", reinterpret_cast<PFN_vkVoidFunction>(vkDebugReportMessageEXT)},
};


#ifdef __cplusplus
}
#endif

#endif
