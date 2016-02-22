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
 * Author: Tobin Ehlis <tobine@google.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "vulkan/vulkan.h"
#include "vk_loader_platform.h"

#include <vector>
#include <unordered_map>

#include "vulkan/vk_layer.h"
#include "vk_layer_config.h"
#include "vk_layer_table.h"
#include "vk_layer_data.h"
#include "vk_layer_logging.h"
#include "vk_layer_extension_utils.h"
#include "vk_safe_struct.h"

struct layer_data {
    bool wsi_enabled;

    layer_data() :
        wsi_enabled(false)
    {};
};

struct instExts {
    bool wsi_enabled;
    bool xlib_enabled;
    bool xcb_enabled;
    bool wayland_enabled;
    bool mir_enabled;
    bool android_enabled;
    bool win32_enabled;
};

static std::unordered_map<void*, struct instExts> instanceExtMap;
static std::unordered_map<void*, layer_data *>    layer_data_map;
static device_table_map                           unique_objects_device_table_map;
static instance_table_map                         unique_objects_instance_table_map;
// Structure to wrap returned non-dispatchable objects to guarantee they have unique handles
//  address of struct will be used as the unique handle
struct VkUniqueObject
{
    uint64_t actualObject;
};

// Handle CreateInstance
static void createInstanceRegisterExtensions(const VkInstanceCreateInfo* pCreateInfo, VkInstance instance)
{
    uint32_t i;
    VkLayerInstanceDispatchTable *pDisp = get_dispatch_table(unique_objects_instance_table_map, instance);
    PFN_vkGetInstanceProcAddr gpa = pDisp->GetInstanceProcAddr;
    pDisp->GetPhysicalDeviceSurfaceSupportKHR = (PFN_vkGetPhysicalDeviceSurfaceSupportKHR) gpa(instance, "vkGetPhysicalDeviceSurfaceSupportKHR");
    pDisp->GetPhysicalDeviceSurfaceCapabilitiesKHR = (PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR) gpa(instance, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
    pDisp->GetPhysicalDeviceSurfaceFormatsKHR = (PFN_vkGetPhysicalDeviceSurfaceFormatsKHR) gpa(instance, "vkGetPhysicalDeviceSurfaceFormatsKHR");
    pDisp->GetPhysicalDeviceSurfacePresentModesKHR = (PFN_vkGetPhysicalDeviceSurfacePresentModesKHR) gpa(instance, "vkGetPhysicalDeviceSurfacePresentModesKHR");
#ifdef VK_USE_PLATFORM_WIN32_KHR
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

    instanceExtMap[pDisp] = {};
    for (i = 0; i < pCreateInfo->enabledExtensionCount; i++) {
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_SURFACE_EXTENSION_NAME) == 0)
            instanceExtMap[pDisp].wsi_enabled = true;
#ifdef VK_USE_PLATFORM_XLIB_KHR
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_XLIB_SURFACE_EXTENSION_NAME) == 0)
            instanceExtMap[pDisp].xlib_enabled = true;
#endif
#ifdef VK_USE_PLATFORM_XCB_KHR
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_XCB_SURFACE_EXTENSION_NAME) == 0)
            instanceExtMap[pDisp].xcb_enabled = true;
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME) == 0)
            instanceExtMap[pDisp].wayland_enabled = true;
#endif
#ifdef VK_USE_PLATFORM_MIR_KHR
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_MIR_SURFACE_EXTENSION_NAME) == 0)
            instanceExtMap[pDisp].mir_enabled = true;
#endif
#ifdef VK_USE_PLATFORM_ANDROID_KHR
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_ANDROID_SURFACE_EXTENSION_NAME) == 0)
            instanceExtMap[pDisp].android_enabled = true;
#endif
#ifdef VK_USE_PLATFORM_WIN32_KHR
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_WIN32_SURFACE_EXTENSION_NAME) == 0)
            instanceExtMap[pDisp].win32_enabled = true;
#endif
    }
}

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

    initInstanceTable(*pInstance, fpGetInstanceProcAddr, unique_objects_instance_table_map);

    createInstanceRegisterExtensions(pCreateInfo, *pInstance);

    return result;
}

// Handle CreateDevice
static void createDeviceRegisterExtensions(const VkDeviceCreateInfo* pCreateInfo, VkDevice device)
{
    layer_data *my_device_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkLayerDispatchTable *pDisp = get_dispatch_table(unique_objects_device_table_map, device);
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
    }
}

VkResult
explicit_CreateDevice(
    VkPhysicalDevice         gpu,
    const VkDeviceCreateInfo *pCreateInfo,
    const VkAllocationCallbacks   *pAllocator,
    VkDevice                 *pDevice)
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

    // Setup layer's device dispatch table
    initDeviceTable(*pDevice, fpGetDeviceProcAddr, unique_objects_device_table_map);

    createDeviceRegisterExtensions(pCreateInfo, *pDevice);

    return result;
}

VkResult explicit_QueueSubmit(VkQueue queue, uint32_t submitCount, const VkSubmitInfo* pSubmits, VkFence fence)
{
// UNWRAP USES:
//  0 : fence,VkFence
    if (VK_NULL_HANDLE != fence) {
        fence = (VkFence)((VkUniqueObject*)fence)->actualObject;
    }
//  waitSemaphoreCount : pSubmits[submitCount]->pWaitSemaphores,VkSemaphore
    std::vector<VkSemaphore> original_pWaitSemaphores = {};
//  signalSemaphoreCount : pSubmits[submitCount]->pSignalSemaphores,VkSemaphore
    std::vector<VkSemaphore> original_pSignalSemaphores = {};
    if (pSubmits) {
        for (uint32_t index0=0; index0<submitCount; ++index0) {
            if (pSubmits[index0].pWaitSemaphores) {
                for (uint32_t index1=0; index1<pSubmits[index0].waitSemaphoreCount; ++index1) {
                    VkSemaphore** ppSemaphore = (VkSemaphore**)&(pSubmits[index0].pWaitSemaphores);
                    original_pWaitSemaphores.push_back(pSubmits[index0].pWaitSemaphores[index1]);
                    *(ppSemaphore[index1]) = (VkSemaphore)((VkUniqueObject*)pSubmits[index0].pWaitSemaphores[index1])->actualObject;
                }
            }
            if (pSubmits[index0].pSignalSemaphores) {
                for (uint32_t index1=0; index1<pSubmits[index0].signalSemaphoreCount; ++index1) {
                    VkSemaphore** ppSemaphore = (VkSemaphore**)&(pSubmits[index0].pSignalSemaphores);
                    original_pSignalSemaphores.push_back(pSubmits[index0].pSignalSemaphores[index1]);
                    *(ppSemaphore[index1]) = (VkSemaphore)((VkUniqueObject*)pSubmits[index0].pSignalSemaphores[index1])->actualObject;
                }
            }
        }
    }
    VkResult result = get_dispatch_table(unique_objects_device_table_map, queue)->QueueSubmit(queue, submitCount, pSubmits, fence);
    if (pSubmits) {
        for (uint32_t index0=0; index0<submitCount; ++index0) {
            if (pSubmits[index0].pWaitSemaphores) {
                for (uint32_t index1=0; index1<pSubmits[index0].waitSemaphoreCount; ++index1) {
                    VkSemaphore** ppSemaphore = (VkSemaphore**)&(pSubmits[index0].pWaitSemaphores);
                    *(ppSemaphore[index1]) = original_pWaitSemaphores[index1];
                }
            }
            if (pSubmits[index0].pSignalSemaphores) {
                for (uint32_t index1=0; index1<pSubmits[index0].signalSemaphoreCount; ++index1) {
                    VkSemaphore** ppSemaphore = (VkSemaphore**)&(pSubmits[index0].pSignalSemaphores);
                    *(ppSemaphore[index1]) = original_pSignalSemaphores[index1];
                }
            }
        }
    }
    return result;
}

VkResult explicit_QueueBindSparse(VkQueue queue, uint32_t bindInfoCount, const VkBindSparseInfo* pBindInfo, VkFence fence)
{
// UNWRAP USES:
//  0 : pBindInfo[bindInfoCount]->pBufferBinds[bufferBindCount]->buffer,VkBuffer, pBindInfo[bindInfoCount]->pBufferBinds[bufferBindCount]->pBinds[bindCount]->memory,VkDeviceMemory, pBindInfo[bindInfoCount]->pImageOpaqueBinds[imageOpaqueBindCount]->image,VkImage, pBindInfo[bindInfoCount]->pImageOpaqueBinds[imageOpaqueBindCount]->pBinds[bindCount]->memory,VkDeviceMemory, pBindInfo[bindInfoCount]->pImageBinds[imageBindCount]->image,VkImage, pBindInfo[bindInfoCount]->pImageBinds[imageBindCount]->pBinds[bindCount]->memory,VkDeviceMemory
    std::vector<VkBuffer> original_buffer = {};
    std::vector<VkDeviceMemory> original_memory1 = {};
    std::vector<VkImage> original_image1 = {};
    std::vector<VkDeviceMemory> original_memory2 = {};
    std::vector<VkImage> original_image2 = {};
    std::vector<VkDeviceMemory> original_memory3 = {};
    std::vector<VkSemaphore> original_pWaitSemaphores = {};
    std::vector<VkSemaphore> original_pSignalSemaphores = {};
    if (pBindInfo) {
        for (uint32_t index0=0; index0<bindInfoCount; ++index0) {
            if (pBindInfo[index0].pBufferBinds) {
                for (uint32_t index1=0; index1<pBindInfo[index0].bufferBindCount; ++index1) {
                    if (pBindInfo[index0].pBufferBinds[index1].buffer) {
                        VkBuffer* pBuffer = (VkBuffer*)&(pBindInfo[index0].pBufferBinds[index1].buffer);
                        original_buffer.push_back(pBindInfo[index0].pBufferBinds[index1].buffer);
                        *(pBuffer) = (VkBuffer)((VkUniqueObject*)pBindInfo[index0].pBufferBinds[index1].buffer)->actualObject;
                    }
                    if (pBindInfo[index0].pBufferBinds[index1].pBinds) {
                        for (uint32_t index2=0; index2<pBindInfo[index0].pBufferBinds[index1].bindCount; ++index2) {
                            if (pBindInfo[index0].pBufferBinds[index1].pBinds[index2].memory) {
                                VkDeviceMemory* pDeviceMemory = (VkDeviceMemory*)&(pBindInfo[index0].pBufferBinds[index1].pBinds[index2].memory);
                                original_memory1.push_back(pBindInfo[index0].pBufferBinds[index1].pBinds[index2].memory);
                                *(pDeviceMemory) = (VkDeviceMemory)((VkUniqueObject*)pBindInfo[index0].pBufferBinds[index1].pBinds[index2].memory)->actualObject;
                            }
                        }
                    }
                }
            }
            if (pBindInfo[index0].pImageOpaqueBinds) {
                for (uint32_t index1=0; index1<pBindInfo[index0].imageOpaqueBindCount; ++index1) {
                    if (pBindInfo[index0].pImageOpaqueBinds[index1].image) {
                        VkImage* pImage = (VkImage*)&(pBindInfo[index0].pImageOpaqueBinds[index1].image);
                        original_image1.push_back(pBindInfo[index0].pImageOpaqueBinds[index1].image);
                        *(pImage) = (VkImage)((VkUniqueObject*)pBindInfo[index0].pImageOpaqueBinds[index1].image)->actualObject;
                    }
                    if (pBindInfo[index0].pImageOpaqueBinds[index1].pBinds) {
                        for (uint32_t index2=0; index2<pBindInfo[index0].pImageOpaqueBinds[index1].bindCount; ++index2) {
                            if (pBindInfo[index0].pImageOpaqueBinds[index1].pBinds[index2].memory) {
                                VkDeviceMemory* pDeviceMemory = (VkDeviceMemory*)&(pBindInfo[index0].pImageOpaqueBinds[index1].pBinds[index2].memory);
                                original_memory2.push_back(pBindInfo[index0].pImageOpaqueBinds[index1].pBinds[index2].memory);
                                *(pDeviceMemory) = (VkDeviceMemory)((VkUniqueObject*)pBindInfo[index0].pImageOpaqueBinds[index1].pBinds[index2].memory)->actualObject;
                            }
                        }
                    }
                }
            }
            if (pBindInfo[index0].pImageBinds) {
                for (uint32_t index1=0; index1<pBindInfo[index0].imageBindCount; ++index1) {
                    if (pBindInfo[index0].pImageBinds[index1].image) {
                        VkImage* pImage = (VkImage*)&(pBindInfo[index0].pImageBinds[index1].image);
                        original_image2.push_back(pBindInfo[index0].pImageBinds[index1].image);
                        *(pImage) = (VkImage)((VkUniqueObject*)pBindInfo[index0].pImageBinds[index1].image)->actualObject;
                    }
                    if (pBindInfo[index0].pImageBinds[index1].pBinds) {
                        for (uint32_t index2=0; index2<pBindInfo[index0].pImageBinds[index1].bindCount; ++index2) {
                            if (pBindInfo[index0].pImageBinds[index1].pBinds[index2].memory) {
                                VkDeviceMemory* pDeviceMemory = (VkDeviceMemory*)&(pBindInfo[index0].pImageBinds[index1].pBinds[index2].memory);
                                original_memory3.push_back(pBindInfo[index0].pImageBinds[index1].pBinds[index2].memory);
                                *(pDeviceMemory) = (VkDeviceMemory)((VkUniqueObject*)pBindInfo[index0].pImageBinds[index1].pBinds[index2].memory)->actualObject;
                            }
                        }
                    }
                }
            }
            if (pBindInfo[index0].pWaitSemaphores) {
                for (uint32_t index1=0; index1<pBindInfo[index0].waitSemaphoreCount; ++index1) {
                    VkSemaphore** ppSemaphore = (VkSemaphore**)&(pBindInfo[index0].pWaitSemaphores);
                    original_pWaitSemaphores.push_back(pBindInfo[index0].pWaitSemaphores[index1]);
                    *(ppSemaphore[index1]) = (VkSemaphore)((VkUniqueObject*)pBindInfo[index0].pWaitSemaphores[index1])->actualObject;
                }
            }
            if (pBindInfo[index0].pSignalSemaphores) {
                for (uint32_t index1=0; index1<pBindInfo[index0].signalSemaphoreCount; ++index1) {
                    VkSemaphore** ppSemaphore = (VkSemaphore**)&(pBindInfo[index0].pSignalSemaphores);
                    original_pSignalSemaphores.push_back(pBindInfo[index0].pSignalSemaphores[index1]);
                    *(ppSemaphore[index1]) = (VkSemaphore)((VkUniqueObject*)pBindInfo[index0].pSignalSemaphores[index1])->actualObject;
                }
            }
        }
    }
    if (VK_NULL_HANDLE != fence) {
        fence = (VkFence)((VkUniqueObject*)fence)->actualObject;
    }
    VkResult result = get_dispatch_table(unique_objects_device_table_map, queue)->QueueBindSparse(queue, bindInfoCount, pBindInfo, fence);
    if (pBindInfo) {
        for (uint32_t index0=0; index0<bindInfoCount; ++index0) {
            if (pBindInfo[index0].pBufferBinds) {
                for (uint32_t index1=0; index1<pBindInfo[index0].bufferBindCount; ++index1) {
                    if (pBindInfo[index0].pBufferBinds[index1].buffer) {
                        VkBuffer* pBuffer = (VkBuffer*)&(pBindInfo[index0].pBufferBinds[index1].buffer);
                        *(pBuffer) = original_buffer[index1];
                    }
                    if (pBindInfo[index0].pBufferBinds[index1].pBinds) {
                        for (uint32_t index2=0; index2<pBindInfo[index0].pBufferBinds[index1].bindCount; ++index2) {
                            if (pBindInfo[index0].pBufferBinds[index1].pBinds[index2].memory) {
                                VkDeviceMemory* pDeviceMemory = (VkDeviceMemory*)&(pBindInfo[index0].pBufferBinds[index1].pBinds[index2].memory);
                                *(pDeviceMemory) = original_memory1[index2];
                            }
                        }
                    }
                }
            }
            if (pBindInfo[index0].pImageOpaqueBinds) {
                for (uint32_t index1=0; index1<pBindInfo[index0].imageOpaqueBindCount; ++index1) {
                    if (pBindInfo[index0].pImageOpaqueBinds[index1].image) {
                        VkImage* pImage = (VkImage*)&(pBindInfo[index0].pImageOpaqueBinds[index1].image);
                        *(pImage) = original_image1[index1];
                    }
                    if (pBindInfo[index0].pImageOpaqueBinds[index1].pBinds) {
                        for (uint32_t index2=0; index2<pBindInfo[index0].pImageOpaqueBinds[index1].bindCount; ++index2) {
                            if (pBindInfo[index0].pImageOpaqueBinds[index1].pBinds[index2].memory) {
                                VkDeviceMemory* pDeviceMemory = (VkDeviceMemory*)&(pBindInfo[index0].pImageOpaqueBinds[index1].pBinds[index2].memory);
                                *(pDeviceMemory) = original_memory2[index2];
                            }
                        }
                    }
                }
            }
            if (pBindInfo[index0].pImageBinds) {
                for (uint32_t index1=0; index1<pBindInfo[index0].imageBindCount; ++index1) {
                    if (pBindInfo[index0].pImageBinds[index1].image) {
                        VkImage* pImage = (VkImage*)&(pBindInfo[index0].pImageBinds[index1].image);
                        *(pImage) = original_image2[index1];
                    }
                    if (pBindInfo[index0].pImageBinds[index1].pBinds) {
                        for (uint32_t index2=0; index2<pBindInfo[index0].pImageBinds[index1].bindCount; ++index2) {
                            if (pBindInfo[index0].pImageBinds[index1].pBinds[index2].memory) {
                                VkDeviceMemory* pDeviceMemory = (VkDeviceMemory*)&(pBindInfo[index0].pImageBinds[index1].pBinds[index2].memory);
                                *(pDeviceMemory) = original_memory3[index2];
                            }
                        }
                    }
                }
            }
            if (pBindInfo[index0].pWaitSemaphores) {
                for (uint32_t index1=0; index1<pBindInfo[index0].waitSemaphoreCount; ++index1) {
                    VkSemaphore** ppSemaphore = (VkSemaphore**)&(pBindInfo[index0].pWaitSemaphores);
                    *(ppSemaphore[index1]) = original_pWaitSemaphores[index1];
                }
            }
            if (pBindInfo[index0].pSignalSemaphores) {
                for (uint32_t index1=0; index1<pBindInfo[index0].signalSemaphoreCount; ++index1) {
                    VkSemaphore** ppSemaphore = (VkSemaphore**)&(pBindInfo[index0].pSignalSemaphores);
                    *(ppSemaphore[index1]) = original_pSignalSemaphores[index1];
                }
            }
        }
    }
    return result;
}

VkResult explicit_CreateComputePipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkComputePipelineCreateInfo* pCreateInfos, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines)
{
// STRUCT USES:{'pipelineCache': 'VkPipelineCache', 'pCreateInfos[createInfoCount]': {'stage': {'module': 'VkShaderModule'}, 'layout': 'VkPipelineLayout', 'basePipelineHandle': 'VkPipeline'}}
//LOCAL DECLS:{'pCreateInfos': 'VkComputePipelineCreateInfo*'}
    safe_VkComputePipelineCreateInfo* local_pCreateInfos = NULL;
    if (pCreateInfos) {
        local_pCreateInfos = new safe_VkComputePipelineCreateInfo[createInfoCount];
        for (uint32_t idx0=0; idx0<createInfoCount; ++idx0) {
            local_pCreateInfos[idx0].initialize(&pCreateInfos[idx0]);
            if (pCreateInfos[idx0].basePipelineHandle) {
                local_pCreateInfos[idx0].basePipelineHandle = (VkPipeline)((VkUniqueObject*)pCreateInfos[idx0].basePipelineHandle)->actualObject;
            }
            if (pCreateInfos[idx0].layout) {
                local_pCreateInfos[idx0].layout = (VkPipelineLayout)((VkUniqueObject*)pCreateInfos[idx0].layout)->actualObject;
            }
            if (pCreateInfos[idx0].stage.module) {
                local_pCreateInfos[idx0].stage.module = (VkShaderModule)((VkUniqueObject*)pCreateInfos[idx0].stage.module)->actualObject;
            }
        }
    }
    if (pipelineCache) {
        pipelineCache = (VkPipelineCache)((VkUniqueObject*)pipelineCache)->actualObject;
    }
// CODEGEN : file /usr/local/google/home/tobine/vulkan_work/LoaderAndTools/vk-layer-generate.py line #1671
    VkResult result = get_dispatch_table(unique_objects_device_table_map, device)->CreateComputePipelines(device, pipelineCache, createInfoCount, (const VkComputePipelineCreateInfo*)local_pCreateInfos, pAllocator, pPipelines);
    if (local_pCreateInfos)
        delete[] local_pCreateInfos;
    if (VK_SUCCESS == result) {
        VkUniqueObject* pUO = NULL;
        for (uint32_t i=0; i<createInfoCount; ++i) {
            pUO = new VkUniqueObject();
            pUO->actualObject = (uint64_t)pPipelines[i];
            pPipelines[i] = (VkPipeline)pUO;
        }
    }
    return result;
}

VkResult explicit_CreateGraphicsPipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkGraphicsPipelineCreateInfo* pCreateInfos, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines)
{
// STRUCT USES:{'pipelineCache': 'VkPipelineCache', 'pCreateInfos[createInfoCount]': {'layout': 'VkPipelineLayout', 'pStages[stageCount]': {'module': 'VkShaderModule'}, 'renderPass': 'VkRenderPass', 'basePipelineHandle': 'VkPipeline'}}
//LOCAL DECLS:{'pCreateInfos': 'VkGraphicsPipelineCreateInfo*'}
    safe_VkGraphicsPipelineCreateInfo* local_pCreateInfos = NULL;
    if (pCreateInfos) {
        local_pCreateInfos = new safe_VkGraphicsPipelineCreateInfo[createInfoCount];
        for (uint32_t idx0=0; idx0<createInfoCount; ++idx0) {
            local_pCreateInfos[idx0].initialize(&pCreateInfos[idx0]);
            if (pCreateInfos[idx0].basePipelineHandle) {
                local_pCreateInfos[idx0].basePipelineHandle = (VkPipeline)((VkUniqueObject*)pCreateInfos[idx0].basePipelineHandle)->actualObject;
            }
            if (pCreateInfos[idx0].layout) {
                local_pCreateInfos[idx0].layout = (VkPipelineLayout)((VkUniqueObject*)pCreateInfos[idx0].layout)->actualObject;
            }
            if (pCreateInfos[idx0].pStages) {
                for (uint32_t idx1=0; idx1<pCreateInfos[idx0].stageCount; ++idx1) {
                    if (pCreateInfos[idx0].pStages[idx1].module) {
                        local_pCreateInfos[idx0].pStages[idx1].module = (VkShaderModule)((VkUniqueObject*)pCreateInfos[idx0].pStages[idx1].module)->actualObject;
                    }
                }
            }
            if (pCreateInfos[idx0].renderPass) {
                local_pCreateInfos[idx0].renderPass = (VkRenderPass)((VkUniqueObject*)pCreateInfos[idx0].renderPass)->actualObject;
            }
        }
    }
    if (pipelineCache) {
        pipelineCache = (VkPipelineCache)((VkUniqueObject*)pipelineCache)->actualObject;
    }
// CODEGEN : file /usr/local/google/home/tobine/vulkan_work/LoaderAndTools/vk-layer-generate.py line #1671
    VkResult result = get_dispatch_table(unique_objects_device_table_map, device)->CreateGraphicsPipelines(device, pipelineCache, createInfoCount, (const VkGraphicsPipelineCreateInfo*)local_pCreateInfos, pAllocator, pPipelines);
    if (local_pCreateInfos)
        delete[] local_pCreateInfos;
    if (VK_SUCCESS == result) {
        VkUniqueObject* pUO = NULL;
        for (uint32_t i=0; i<createInfoCount; ++i) {
            pUO = new VkUniqueObject();
            pUO->actualObject = (uint64_t)pPipelines[i];
            pPipelines[i] = (VkPipeline)pUO;
        }
    }
    return result;
}

VkResult explicit_GetSwapchainImagesKHR(VkDevice device, VkSwapchainKHR swapchain, uint32_t* pSwapchainImageCount, VkImage* pSwapchainImages)
{
// UNWRAP USES:
//  0 : swapchain,VkSwapchainKHR, pSwapchainImages,VkImage
    if (VK_NULL_HANDLE != swapchain) {
        swapchain = (VkSwapchainKHR)((VkUniqueObject*)swapchain)->actualObject;
    }
    VkResult result = get_dispatch_table(unique_objects_device_table_map, device)->GetSwapchainImagesKHR(device, swapchain, pSwapchainImageCount, pSwapchainImages);
    // TODO : Need to add corresponding code to delete these images
    if (VK_SUCCESS == result) {
        if ((*pSwapchainImageCount > 0) && pSwapchainImages) {
            std::vector<VkUniqueObject*> uniqueImages = {};
            for (uint32_t i=0; i<*pSwapchainImageCount; ++i) {
                uniqueImages.push_back(new VkUniqueObject());
                uniqueImages[i]->actualObject = (uint64_t)pSwapchainImages[i];
                pSwapchainImages[i] = (VkImage)uniqueImages[i];
            }
        }
    }
    return result;
}
