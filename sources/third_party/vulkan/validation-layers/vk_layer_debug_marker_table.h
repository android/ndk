/* Copyright (c) 2015-2016 The Khronos Group Inc.
 * Copyright (c) 2015-2016 Valve Corporation
 * Copyright (c) 2015-2016 LunarG, Inc.
 * Copyright (c) 2015-2016 Google Inc.
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
 * Author: Michael Lentine <mlentine@google.com>
 * Author: Tobin Ehlis <tobin@lunarg.com>
 */

#pragma once

#include <cassert>
#include <unordered_map>

extern std::unordered_map<void *, VkLayerDebugMarkerDispatchTable *> tableDebugMarkerMap;
VkLayerDebugMarkerDispatchTable * initDebugMarkerTable(VkDevice dev);

// Map lookup must be thread safe
static inline VkLayerDebugMarkerDispatchTable *debug_marker_dispatch_table(void* object)
{
    VkLayerDebugMarkerDispatchTable *pDisp  = *(VkLayerDebugMarkerDispatchTable **) object;
    std::unordered_map<void *, VkLayerDebugMarkerDispatchTable *>::const_iterator it = tableDebugMarkerMap.find((void *) pDisp);
    assert(it != tableDebugMarkerMap.end() && "Not able to find debug marker dispatch entry");
    return it->second;
}

