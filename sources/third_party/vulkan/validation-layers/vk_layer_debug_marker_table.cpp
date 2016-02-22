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
 */

#include <assert.h>
#include <unordered_map>
#include "vulkan/vk_debug_marker_layer.h"
std::unordered_map<void *, VkLayerDebugMarkerDispatchTable *> tableDebugMarkerMap;

/* Various dispatchable objects will use the same underlying dispatch table if they
 * are created from that "parent" object. Thus use pointer to dispatch table
 * as the key to these table maps.
 *    Instance -> PhysicalDevice
 *    Device -> CommandBuffer or Queue
 * If use the object themselves as key to map then implies Create entrypoints have to be intercepted
 * and a new key inserted into map */
VkLayerDebugMarkerDispatchTable * initDebugMarkerTable(VkDevice device)
{
    VkLayerDebugMarkerDispatchTable *pDebugMarkerTable;

    assert(device);
    VkLayerDispatchTable *pDisp = *(VkLayerDispatchTable **) device;

    std::unordered_map<void *, VkLayerDebugMarkerDispatchTable *>::const_iterator it = tableDebugMarkerMap.find((void *) pDisp);
    if (it == tableDebugMarkerMap.end())
    {
        pDebugMarkerTable = new VkLayerDebugMarkerDispatchTable;
        tableDebugMarkerMap[(void *) pDisp] = pDebugMarkerTable;
    } else
    {
        return it->second;
    }

    pDebugMarkerTable->CmdDbgMarkerBegin = (PFN_vkCmdDbgMarkerBegin) pDisp->GetDeviceProcAddr(device, "vkCmdDbgMarkerBegin");
    pDebugMarkerTable->CmdDbgMarkerEnd   = (PFN_vkCmdDbgMarkerEnd) pDisp->GetDeviceProcAddr(device, "vkCmdDbgMarkerEnd");
    pDebugMarkerTable->DbgSetObjectTag   = (PFN_vkDbgSetObjectTag) pDisp->GetDeviceProcAddr(device, "vkDbgSetObjectTag");
    pDebugMarkerTable->DbgSetObjectName  = (PFN_vkDbgSetObjectName) pDisp->GetDeviceProcAddr(device, "vkDbgSetObjectName");

    return pDebugMarkerTable;
}
