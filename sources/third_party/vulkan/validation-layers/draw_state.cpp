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
 * Author: Cody Northrop <cnorthrop@google.com>
 * Author: Michael Lentine <mlentine@google.com>
 * Author: Tobin Ehlis <tobine@google.com>
 * Author: Chia-I Wu <olv@google.com>
 * Author: Chris Forbes <chrisf@ijw.co.nz>
 * Author: Mark Lobodzinski <mark@lunarg.com>
 * Author: Ian Elliott <ianelliott@google.com>
 */

// Allow use of STL min and max functions in Windows
#define NOMINMAX

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <string>
#include <iostream>
#include <algorithm>
#include <list>
#include <spirv.hpp>
#include <set>

#include "vk_loader_platform.h"
#include "vk_dispatch_table_helper.h"
#include "vk_struct_string_helper_cpp.h"
#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wwrite-strings"
#endif
#if defined(__GNUC__)
#pragma GCC diagnostic warning "-Wwrite-strings"
#endif
#include "vk_struct_size_helper.h"
#include "draw_state.h"
#include "vk_layer_config.h"
#include "vulkan/vk_debug_marker_layer.h"
#include "vk_layer_table.h"
#include "vk_layer_debug_marker_table.h"
#include "vk_layer_data.h"
#include "vk_layer_logging.h"
#include "vk_layer_extension_utils.h"
#include "vk_layer_utils.h"

using std::unordered_map;
using std::unordered_set;

// Track command pools and their command buffers
struct CMD_POOL_INFO {
    VkCommandPoolCreateFlags    createFlags;
    uint32_t queueFamilyIndex;
    list<VkCommandBuffer>       commandBuffers; // list container of cmd buffers allocated from this pool
};

struct devExts {
    VkBool32 debug_marker_enabled;
    VkBool32 wsi_enabled;
    unordered_map<VkSwapchainKHR, SWAPCHAIN_NODE*> swapchainMap;
    unordered_map<VkImage, VkSwapchainKHR> imageToSwapchainMap;
};

// fwd decls
struct shader_module;
struct render_pass;

struct layer_data {
    debug_report_data* report_data;
    std::vector<VkDebugReportCallbackEXT> logging_callback;
    VkLayerDispatchTable* device_dispatch_table;
    VkLayerInstanceDispatchTable* instance_dispatch_table;
    devExts device_extensions;
    vector<VkQueue> queues; // all queues under given device
    // Global set of all cmdBuffers that are inFlight on this device
    unordered_set<VkCommandBuffer> globalInFlightCmdBuffers;
    // Layer specific data
    unordered_map<VkSampler,             unique_ptr<SAMPLER_NODE>>           sampleMap;
    unordered_map<VkImageView,           unique_ptr<VkImageViewCreateInfo>>  imageViewMap;
    unordered_map<VkImage,               unique_ptr<VkImageCreateInfo>>      imageMap;
    unordered_map<VkBufferView,          unique_ptr<VkBufferViewCreateInfo>> bufferViewMap;
    unordered_map<VkBuffer,              BUFFER_NODE>                        bufferMap;
    unordered_map<VkPipeline,            PIPELINE_NODE*>                     pipelineMap;
    unordered_map<VkCommandPool,         CMD_POOL_INFO>                      commandPoolMap;
    unordered_map<VkDescriptorPool,      DESCRIPTOR_POOL_NODE*>              descriptorPoolMap;
    unordered_map<VkDescriptorSet,       SET_NODE*>                          setMap;
    unordered_map<VkDescriptorSetLayout, LAYOUT_NODE*>                       descriptorSetLayoutMap;
    unordered_map<VkPipelineLayout,      PIPELINE_LAYOUT_NODE>               pipelineLayoutMap;
    unordered_map<VkDeviceMemory,        VkImage>                            memImageMap;
    unordered_map<VkFence,               FENCE_NODE>                         fenceMap;
    unordered_map<VkQueue,               QUEUE_NODE>                         queueMap;
    unordered_map<VkEvent,               EVENT_NODE>                         eventMap;
    unordered_map<QueryObject,           bool>                               queryToStateMap;
    unordered_map<VkQueryPool, QUERY_POOL_NODE> queryPoolMap;
    unordered_map<VkSemaphore, SEMAPHORE_NODE> semaphoreMap;
    unordered_map<void*,                 GLOBAL_CB_NODE*>                    commandBufferMap;
    unordered_map<VkFramebuffer,         VkFramebufferCreateInfo*>           frameBufferMap;
    unordered_map<VkImage, vector<ImageSubresourcePair>> imageSubresourceMap;
    unordered_map<ImageSubresourcePair, IMAGE_NODE> imageLayoutMap;
    unordered_map<VkRenderPass,          RENDER_PASS_NODE*>                  renderPassMap;
    unordered_map<VkShaderModule,        shader_module*>                     shaderModuleMap;
    // Current render pass
    VkRenderPassBeginInfo                renderPassBeginInfo;
    uint32_t                             currentSubpass;

    // Device specific data
    PHYS_DEV_PROPERTIES_NODE             physDevProperties;

    layer_data() :
        report_data(nullptr),
        device_dispatch_table(nullptr),
        instance_dispatch_table(nullptr),
        device_extensions()
    {};
};

// Code imported from ShaderChecker
static void
build_def_index(shader_module *);

// A forward iterator over spirv instructions. Provides easy access to len, opcode, and content words
// without the caller needing to care too much about the physical SPIRV module layout.
struct spirv_inst_iter {
    std::vector<uint32_t>::const_iterator zero;
    std::vector<uint32_t>::const_iterator it;

    uint32_t len() { return *it >> 16; }
    uint32_t opcode() { return *it & 0x0ffffu; }
    uint32_t const & word(unsigned n) { return it[n]; }
    uint32_t offset() { return (uint32_t)(it - zero); }

    spirv_inst_iter() {}

    spirv_inst_iter(std::vector<uint32_t>::const_iterator zero,
        std::vector<uint32_t>::const_iterator it) : zero(zero), it(it) {}

    bool operator== (spirv_inst_iter const & other) {
        return it == other.it;
    }

    bool operator!= (spirv_inst_iter const & other) {
        return it != other.it;
    }

    spirv_inst_iter operator++ (int) {  /* x++ */
        spirv_inst_iter ii = *this;
        it += len();
        return ii;
    }

    spirv_inst_iter operator++ () { /* ++x; */
        it += len();
        return *this;
    }

    /* The iterator and the value are the same thing. */
    spirv_inst_iter & operator* () { return *this; }
    spirv_inst_iter const & operator* () const { return *this; }
};

struct shader_module {
    /* the spirv image itself */
    vector<uint32_t> words;
    /* a mapping of <id> to the first word of its def. this is useful because walking type
     * trees, constant expressions, etc requires jumping all over the instruction stream.
     */
    unordered_map<unsigned, unsigned> def_index;

    shader_module(VkShaderModuleCreateInfo const *pCreateInfo) :
        words((uint32_t *)pCreateInfo->pCode, (uint32_t *)pCreateInfo->pCode + pCreateInfo->codeSize / sizeof(uint32_t)),
        def_index() {

        build_def_index(this);
    }

    /* expose begin() / end() to enable range-based for */
    spirv_inst_iter begin() const { return spirv_inst_iter(words.begin(), words.begin() + 5); }        /* first insn */
    spirv_inst_iter end() const { return spirv_inst_iter(words.begin(), words.end()); }          /* just past last insn */
    /* given an offset into the module, produce an iterator there. */
    spirv_inst_iter at(unsigned offset) const { return spirv_inst_iter(words.begin(), words.begin() + offset); }

    /* gets an iterator to the definition of an id */
    spirv_inst_iter get_def(unsigned id) const {
        auto it = def_index.find(id);
        if (it == def_index.end()) {
            return end();
        }
        return at(it->second);
    }
};

// TODO : Do we need to guard access to layer_data_map w/ lock?
static unordered_map<void*, layer_data*> layer_data_map;

// TODO : This can be much smarter, using separate locks for separate global data
static int globalLockInitialized = 0;
static loader_platform_thread_mutex globalLock;
#define MAX_TID 513
static loader_platform_thread_id g_tidMapping[MAX_TID] = {0};
static uint32_t g_maxTID = 0;

template layer_data *get_my_data_ptr<layer_data>(
        void *data_key,
        std::unordered_map<void *, layer_data *> &data_map);

// Map actual TID to an index value and return that index
//  This keeps TIDs in range from 0-MAX_TID and simplifies compares between runs
static uint32_t getTIDIndex() {
    loader_platform_thread_id tid = loader_platform_get_thread_id();
    for (uint32_t i = 0; i < g_maxTID; i++) {
        if (tid == g_tidMapping[i])
            return i;
    }
    // Don't yet have mapping, set it and return newly set index
    uint32_t retVal = (uint32_t) g_maxTID;
    g_tidMapping[g_maxTID++] = tid;
    assert(g_maxTID < MAX_TID);
    return retVal;
}

// Return a string representation of CMD_TYPE enum
static string cmdTypeToString(CMD_TYPE cmd)
{
    switch (cmd)
    {
        case CMD_BINDPIPELINE:
            return "CMD_BINDPIPELINE";
        case CMD_BINDPIPELINEDELTA:
            return "CMD_BINDPIPELINEDELTA";
        case CMD_SETVIEWPORTSTATE:
            return "CMD_SETVIEWPORTSTATE";
        case CMD_SETLINEWIDTHSTATE:
            return "CMD_SETLINEWIDTHSTATE";
        case CMD_SETDEPTHBIASSTATE:
            return "CMD_SETDEPTHBIASSTATE";
        case CMD_SETBLENDSTATE:
            return "CMD_SETBLENDSTATE";
        case CMD_SETDEPTHBOUNDSSTATE:
            return "CMD_SETDEPTHBOUNDSSTATE";
        case CMD_SETSTENCILREADMASKSTATE:
            return "CMD_SETSTENCILREADMASKSTATE";
        case CMD_SETSTENCILWRITEMASKSTATE:
            return "CMD_SETSTENCILWRITEMASKSTATE";
        case CMD_SETSTENCILREFERENCESTATE:
            return "CMD_SETSTENCILREFERENCESTATE";
        case CMD_BINDDESCRIPTORSETS:
            return "CMD_BINDDESCRIPTORSETS";
        case CMD_BINDINDEXBUFFER:
            return "CMD_BINDINDEXBUFFER";
        case CMD_BINDVERTEXBUFFER:
            return "CMD_BINDVERTEXBUFFER";
        case CMD_DRAW:
            return "CMD_DRAW";
        case CMD_DRAWINDEXED:
            return "CMD_DRAWINDEXED";
        case CMD_DRAWINDIRECT:
            return "CMD_DRAWINDIRECT";
        case CMD_DRAWINDEXEDINDIRECT:
            return "CMD_DRAWINDEXEDINDIRECT";
        case CMD_DISPATCH:
            return "CMD_DISPATCH";
        case CMD_DISPATCHINDIRECT:
            return "CMD_DISPATCHINDIRECT";
        case CMD_COPYBUFFER:
            return "CMD_COPYBUFFER";
        case CMD_COPYIMAGE:
            return "CMD_COPYIMAGE";
        case CMD_BLITIMAGE:
            return "CMD_BLITIMAGE";
        case CMD_COPYBUFFERTOIMAGE:
            return "CMD_COPYBUFFERTOIMAGE";
        case CMD_COPYIMAGETOBUFFER:
            return "CMD_COPYIMAGETOBUFFER";
        case CMD_CLONEIMAGEDATA:
            return "CMD_CLONEIMAGEDATA";
        case CMD_UPDATEBUFFER:
            return "CMD_UPDATEBUFFER";
        case CMD_FILLBUFFER:
            return "CMD_FILLBUFFER";
        case CMD_CLEARCOLORIMAGE:
            return "CMD_CLEARCOLORIMAGE";
        case CMD_CLEARATTACHMENTS:
            return "CMD_CLEARCOLORATTACHMENT";
        case CMD_CLEARDEPTHSTENCILIMAGE:
            return "CMD_CLEARDEPTHSTENCILIMAGE";
        case CMD_RESOLVEIMAGE:
            return "CMD_RESOLVEIMAGE";
        case CMD_SETEVENT:
            return "CMD_SETEVENT";
        case CMD_RESETEVENT:
            return "CMD_RESETEVENT";
        case CMD_WAITEVENTS:
            return "CMD_WAITEVENTS";
        case CMD_PIPELINEBARRIER:
            return "CMD_PIPELINEBARRIER";
        case CMD_BEGINQUERY:
            return "CMD_BEGINQUERY";
        case CMD_ENDQUERY:
            return "CMD_ENDQUERY";
        case CMD_RESETQUERYPOOL:
            return "CMD_RESETQUERYPOOL";
        case CMD_COPYQUERYPOOLRESULTS:
            return "CMD_COPYQUERYPOOLRESULTS";
        case CMD_WRITETIMESTAMP:
            return "CMD_WRITETIMESTAMP";
        case CMD_INITATOMICCOUNTERS:
            return "CMD_INITATOMICCOUNTERS";
        case CMD_LOADATOMICCOUNTERS:
            return "CMD_LOADATOMICCOUNTERS";
        case CMD_SAVEATOMICCOUNTERS:
            return "CMD_SAVEATOMICCOUNTERS";
        case CMD_BEGINRENDERPASS:
            return "CMD_BEGINRENDERPASS";
        case CMD_ENDRENDERPASS:
            return "CMD_ENDRENDERPASS";
        case CMD_DBGMARKERBEGIN:
            return "CMD_DBGMARKERBEGIN";
        case CMD_DBGMARKEREND:
            return "CMD_DBGMARKEREND";
        default:
            return "UNKNOWN";
    }
}

// SPIRV utility functions
static void
build_def_index(shader_module *module)
{
    for (auto insn : *module) {
        switch (insn.opcode()) {
        /* Types */
        case spv::OpTypeVoid:
        case spv::OpTypeBool:
        case spv::OpTypeInt:
        case spv::OpTypeFloat:
        case spv::OpTypeVector:
        case spv::OpTypeMatrix:
        case spv::OpTypeImage:
        case spv::OpTypeSampler:
        case spv::OpTypeSampledImage:
        case spv::OpTypeArray:
        case spv::OpTypeRuntimeArray:
        case spv::OpTypeStruct:
        case spv::OpTypeOpaque:
        case spv::OpTypePointer:
        case spv::OpTypeFunction:
        case spv::OpTypeEvent:
        case spv::OpTypeDeviceEvent:
        case spv::OpTypeReserveId:
        case spv::OpTypeQueue:
        case spv::OpTypePipe:
            module->def_index[insn.word(1)] = insn.offset();
            break;

        /* Fixed constants */
        case spv::OpConstantTrue:
        case spv::OpConstantFalse:
        case spv::OpConstant:
        case spv::OpConstantComposite:
        case spv::OpConstantSampler:
        case spv::OpConstantNull:
            module->def_index[insn.word(2)] = insn.offset();
            break;

        /* Specialization constants */
        case spv::OpSpecConstantTrue:
        case spv::OpSpecConstantFalse:
        case spv::OpSpecConstant:
        case spv::OpSpecConstantComposite:
        case spv::OpSpecConstantOp:
            module->def_index[insn.word(2)] = insn.offset();
            break;

        /* Variables */
        case spv::OpVariable:
            module->def_index[insn.word(2)] = insn.offset();
            break;

        /* Functions */
        case spv::OpFunction:
            module->def_index[insn.word(2)] = insn.offset();
            break;

        default:
            /* We don't care about any other defs for now. */
            break;
        }
    }
}


static spirv_inst_iter
find_entrypoint(shader_module *src, char const *name, VkShaderStageFlagBits stageBits)
{
    for (auto insn : *src) {
        if (insn.opcode() == spv::OpEntryPoint) {
            auto entrypointName = (char const *) &insn.word(3);
            auto entrypointStageBits = 1u << insn.word(1);

            if (!strcmp(entrypointName, name) && (entrypointStageBits & stageBits)) {
                return insn;
            }
        }
    }

    return src->end();
}


bool
shader_is_spirv(VkShaderModuleCreateInfo const *pCreateInfo)
{
    uint32_t *words = (uint32_t *)pCreateInfo->pCode;
    size_t sizeInWords = pCreateInfo->codeSize / sizeof(uint32_t);

    /* Just validate that the header makes sense. */
    return sizeInWords >= 5 && words[0] == spv::MagicNumber && words[1] == spv::Version;
}

static char const *
storage_class_name(unsigned sc)
{
    switch (sc) {
    case spv::StorageClassInput: return "input";
    case spv::StorageClassOutput: return "output";
    case spv::StorageClassUniformConstant: return "const uniform";
    case spv::StorageClassUniform: return "uniform";
    case spv::StorageClassWorkgroup: return "workgroup local";
    case spv::StorageClassCrossWorkgroup: return "workgroup global";
    case spv::StorageClassPrivate: return "private global";
    case spv::StorageClassFunction: return "function";
    case spv::StorageClassGeneric: return "generic";
    case spv::StorageClassAtomicCounter: return "atomic counter";
    case spv::StorageClassImage: return "image";
    default: return "unknown";
    }
}

/* get the value of an integral constant */
unsigned
get_constant_value(shader_module const *src, unsigned id)
{
    auto value = src->get_def(id);
    assert(value != src->end());

    if (value.opcode() != spv::OpConstant) {
        /* TODO: Either ensure that the specialization transform is already performed on a module we're
            considering here, OR -- specialize on the fly now.
            */
        return 1;
    }

    return value.word(3);
}

/* returns ptr to null terminator */
static char *
describe_type(char *dst, shader_module const *src, unsigned type)
{
    auto insn = src->get_def(type);
    assert(insn != src->end());

    switch (insn.opcode()) {
        case spv::OpTypeBool:
            return dst + sprintf(dst, "bool");
        case spv::OpTypeInt:
            return dst + sprintf(dst, "%cint%d", insn.word(3) ? 's' : 'u', insn.word(2));
        case spv::OpTypeFloat:
            return dst + sprintf(dst, "float%d", insn.word(2));
        case spv::OpTypeVector:
            dst += sprintf(dst, "vec%d of ", insn.word(3));
            return describe_type(dst, src, insn.word(2));
        case spv::OpTypeMatrix:
            dst += sprintf(dst, "mat%d of ", insn.word(3));
            return describe_type(dst, src, insn.word(2));
        case spv::OpTypeArray:
            dst += sprintf(dst, "arr[%d] of ", get_constant_value(src, insn.word(3)));
            return describe_type(dst, src, insn.word(2));
        case spv::OpTypePointer:
            dst += sprintf(dst, "ptr to %s ", storage_class_name(insn.word(2)));
            return describe_type(dst, src, insn.word(3));
        case spv::OpTypeStruct:
            {
                dst += sprintf(dst, "struct of (");
                for (unsigned i = 2; i < insn.len(); i++) {
                    dst = describe_type(dst, src, insn.word(i));
                    dst += sprintf(dst, i == insn.len()-1 ? ")" : ", ");
                }
                return dst;
            }
        case spv::OpTypeSampler:
            return dst + sprintf(dst, "sampler");
        default:
            return dst + sprintf(dst, "oddtype");
    }
}

static bool
types_match(shader_module const *a, shader_module const *b, unsigned a_type, unsigned b_type, bool b_arrayed)
{
    /* walk two type trees together, and complain about differences */
    auto a_insn = a->get_def(a_type);
    auto b_insn = b->get_def(b_type);
    assert(a_insn != a->end());
    assert(b_insn != b->end());

    if (b_arrayed && b_insn.opcode() == spv::OpTypeArray) {
        /* we probably just found the extra level of arrayness in b_type: compare the type inside it to a_type */
        return types_match(a, b, a_type, b_insn.word(2), false);
    }

    if (a_insn.opcode() != b_insn.opcode()) {
        return false;
    }

    switch (a_insn.opcode()) {
        /* if b_arrayed and we hit a leaf type, then we can't match -- there's nowhere for the extra OpTypeArray to be! */
        case spv::OpTypeBool:
            return true && !b_arrayed;
        case spv::OpTypeInt:
            /* match on width, signedness */
            return a_insn.word(2) == b_insn.word(2) && a_insn.word(3) == b_insn.word(3) && !b_arrayed;
        case spv::OpTypeFloat:
            /* match on width */
            return a_insn.word(2) == b_insn.word(2) && !b_arrayed;
        case spv::OpTypeVector:
        case spv::OpTypeMatrix:
            /* match on element type, count. these all have the same layout. we don't get here if
             * b_arrayed -- that is handled above. */
            return !b_arrayed &&
                types_match(a, b, a_insn.word(2), b_insn.word(2), b_arrayed) &&
                a_insn.word(3) == b_insn.word(3);
        case spv::OpTypeArray:
            /* match on element type, count. these all have the same layout. we don't get here if
             * b_arrayed. This differs from vector & matrix types in that the array size is the id of a constant instruction,
             * not a literal within OpTypeArray */
            return !b_arrayed &&
                types_match(a, b, a_insn.word(2), b_insn.word(2), b_arrayed) &&
                get_constant_value(a, a_insn.word(3)) == get_constant_value(b, b_insn.word(3));
        case spv::OpTypeStruct:
            /* match on all element types */
            {
                if (b_arrayed) {
                    /* for the purposes of matching different levels of arrayness, structs are leaves. */
                    return false;
                }

                if (a_insn.len() != b_insn.len()) {
                    return false;   /* structs cannot match if member counts differ */
                }

                for (unsigned i = 2; i < a_insn.len(); i++) {
                    if (!types_match(a, b, a_insn.word(i), b_insn.word(i), b_arrayed)) {
                        return false;
                    }
                }

                return true;
            }
        case spv::OpTypePointer:
            /* match on pointee type. storage class is expected to differ */
            return types_match(a, b, a_insn.word(3), b_insn.word(3), b_arrayed);

        default:
            /* remaining types are CLisms, or may not appear in the interfaces we
             * are interested in. Just claim no match.
             */
            return false;

    }
}

static int
value_or_default(std::unordered_map<unsigned, unsigned> const &map, unsigned id, int def)
{
    auto it = map.find(id);
    if (it == map.end())
        return def;
    else
        return it->second;
}


static unsigned
get_locations_consumed_by_type(shader_module const *src, unsigned type, bool strip_array_level)
{
    auto insn = src->get_def(type);
    assert(insn != src->end());

    switch (insn.opcode()) {
        case spv::OpTypePointer:
            /* see through the ptr -- this is only ever at the toplevel for graphics shaders;
             * we're never actually passing pointers around. */
            return get_locations_consumed_by_type(src, insn.word(3), strip_array_level);
        case spv::OpTypeArray:
            if (strip_array_level) {
                return get_locations_consumed_by_type(src, insn.word(2), false);
            }
            else {
                return get_constant_value(src, insn.word(3)) * get_locations_consumed_by_type(src, insn.word(2), false);
            }
        case spv::OpTypeMatrix:
            /* num locations is the dimension * element size */
            return insn.word(3) * get_locations_consumed_by_type(src, insn.word(2), false);
        default:
            /* everything else is just 1. */
            return 1;

        /* TODO: extend to handle 64bit scalar types, whose vectors may need
         * multiple locations. */
    }
}


typedef std::pair<unsigned, unsigned> location_t;
typedef std::pair<unsigned, unsigned> descriptor_slot_t;


struct interface_var {
    uint32_t id;
    uint32_t type_id;
    uint32_t offset;
    /* TODO: collect the name, too? Isn't required to be present. */
};


static void
collect_interface_block_members(layer_data *my_data, VkDevice dev,
                                shader_module const *src,
                                std::map<location_t, interface_var> &out,
                                std::unordered_map<unsigned, unsigned> const &blocks,
                                bool is_array_of_verts,
                                uint32_t id,
                                uint32_t type_id)
{
    /* Walk down the type_id presented, trying to determine whether it's actually an interface block. */
    auto type = src->get_def(type_id);

    while (true) {

        if (type.opcode() == spv::OpTypePointer) {
            type = src->get_def(type.word(3));
        }
        else if (type.opcode() == spv::OpTypeArray && is_array_of_verts) {
            type = src->get_def(type.word(2));
            is_array_of_verts = false;
        }
        else if (type.opcode() == spv::OpTypeStruct) {
            if (blocks.find(type.word(1)) == blocks.end()) {
                /* This isn't an interface block. */
                return;
            }
            else {
                /* We have found the correct type. Walk its members. */
                break;
            }
        }
        else {
            /* not an interface block */
            return;
        }
    }

    std::unordered_map<unsigned, unsigned> member_components;

    /* Walk all the OpMemberDecorate for type's result id -- first pass, collect components. */
    for (auto insn : *src) {
        if (insn.opcode() == spv::OpMemberDecorate && insn.word(1) == type.word(1)) {
            unsigned member_index = insn.word(2);

            if (insn.word(3) == spv::DecorationComponent) {
                unsigned component = insn.word(4);
                member_components[member_index] = component;
            }
        }
    }

    /* Second pass -- produce the output, from Location decorations */
    for (auto insn : *src) {
        if (insn.opcode() == spv::OpMemberDecorate && insn.word(1) == type.word(1)) {
            unsigned member_index = insn.word(2);
            unsigned member_type_id = type.word(2 + member_index);

            if (insn.word(3) == spv::DecorationLocation) {
                unsigned location = insn.word(4);
                unsigned num_locations = get_locations_consumed_by_type(src, member_type_id, false);
                auto component_it = member_components.find(member_index);
                unsigned component = component_it == member_components.end() ? 0 : component_it->second;

                for (unsigned int offset = 0; offset < num_locations; offset++) {
                    interface_var v;
                    v.id = id;
                    /* TODO: member index in interface_var too? */
                    v.type_id = member_type_id;
                    v.offset = offset;
                    out[std::make_pair(location + offset, component)] = v;
                }
            }
        }
    }
}

static void
collect_interface_by_location(layer_data *my_data, VkDevice dev,
                              shader_module const *src,
                              spirv_inst_iter entrypoint,
                              spv::StorageClass sinterface,
                              std::map<location_t, interface_var> &out,
                              bool is_array_of_verts)
{
    std::unordered_map<unsigned, unsigned> var_locations;
    std::unordered_map<unsigned, unsigned> var_builtins;
    std::unordered_map<unsigned, unsigned> var_components;
    std::unordered_map<unsigned, unsigned> blocks;

    for (auto insn : *src) {

        /* We consider two interface models: SSO rendezvous-by-location, and
         * builtins. Complain about anything that fits neither model.
         */
        if (insn.opcode() == spv::OpDecorate) {
            if (insn.word(2) == spv::DecorationLocation) {
                var_locations[insn.word(1)] = insn.word(3);
            }

            if (insn.word(2) == spv::DecorationBuiltIn) {
                var_builtins[insn.word(1)] = insn.word(3);
            }

            if (insn.word(2) == spv::DecorationComponent) {
                var_components[insn.word(1)] = insn.word(3);
            }

            if (insn.word(2) == spv::DecorationBlock) {
                blocks[insn.word(1)] = 1;
            }
        }
    }

        /* TODO: handle grouped decorations */
        /* TODO: handle index=1 dual source outputs from FS -- two vars will
         * have the same location, and we DONT want to clobber. */

    /* find the end of the entrypoint's name string. additional zero bytes follow the actual null
       terminator, to fill out the rest of the word - so we only need to look at the last byte in
       the word to determine which word contains the terminator. */
    auto word = 3;
    while (entrypoint.word(word) & 0xff000000u) {
        ++word;
    }
    ++word;

    for (; word < entrypoint.len(); word++) {
        auto insn = src->get_def(entrypoint.word(word));
        assert(insn != src->end());
        assert(insn.opcode() == spv::OpVariable);

        if (insn.word(3) == sinterface) {
            unsigned id = insn.word(2);
            unsigned type = insn.word(1);

            int location = value_or_default(var_locations, id, -1);
            int builtin = value_or_default(var_builtins, id, -1);
            unsigned component = value_or_default(var_components, id, 0);    /* unspecified is OK, is 0 */

            /* All variables and interface block members in the Input or Output storage classes
             * must be decorated with either a builtin or an explicit location.
             *
             * TODO: integrate the interface block support here. For now, don't complain --
             * a valid SPIRV module will only hit this path for the interface block case, as the
             * individual members of the type are decorated, rather than variable declarations.
             */

            if (location != -1) {
                /* A user-defined interface variable, with a location. Where a variable
                 * occupied multiple locations, emit one result for each. */
                unsigned num_locations = get_locations_consumed_by_type(src, type,
                        is_array_of_verts);
                for (unsigned int offset = 0; offset < num_locations; offset++) {
                    interface_var v;
                    v.id = id;
                    v.type_id = type;
                    v.offset = offset;
                    out[std::make_pair(location + offset, component)] = v;
                }
            }
            else if (builtin == -1) {
                /* An interface block instance */
                collect_interface_block_members(my_data, dev, src, out,
                                                blocks, is_array_of_verts, id, type);
            }
        }
    }
}

static void
collect_interface_by_descriptor_slot(layer_data *my_data, VkDevice dev,
                              shader_module const *src, spv::StorageClass sinterface,
                              std::unordered_set<uint32_t> const &accessible_ids,
                              std::map<descriptor_slot_t, interface_var> &out)
{

    std::unordered_map<unsigned, unsigned> var_sets;
    std::unordered_map<unsigned, unsigned> var_bindings;

    for (auto insn : *src) {
        /* All variables in the Uniform or UniformConstant storage classes are required to be decorated with both
         * DecorationDescriptorSet and DecorationBinding.
         */
        if (insn.opcode() == spv::OpDecorate) {
            if (insn.word(2) == spv::DecorationDescriptorSet) {
                var_sets[insn.word(1)] = insn.word(3);
            }

            if (insn.word(2) == spv::DecorationBinding) {
                var_bindings[insn.word(1)] = insn.word(3);
            }
        }
    }

    for (auto id : accessible_ids) {
        auto insn = src->get_def(id);
        assert(insn != src->end());

        if (insn.opcode() == spv::OpVariable &&
                (insn.word(3) == spv::StorageClassUniform ||
                 insn.word(3) == spv::StorageClassUniformConstant)) {
            unsigned set = value_or_default(var_sets, insn.word(2), 0);
            unsigned binding = value_or_default(var_bindings, insn.word(2), 0);

            auto existing_it = out.find(std::make_pair(set, binding));
            if (existing_it != out.end()) {
                /* conflict within spv image */
                log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, /*dev*/0, __LINE__,
                        SHADER_CHECKER_INCONSISTENT_SPIRV, "SC",
                        "var %d (type %d) in %s interface in descriptor slot (%u,%u) conflicts with existing definition",
                        insn.word(2), insn.word(1), storage_class_name(sinterface),
                        existing_it->first.first, existing_it->first.second);
            }

            interface_var v;
            v.id = insn.word(2);
            v.type_id = insn.word(1);
            out[std::make_pair(set, binding)] = v;
        }
    }
}

static bool
validate_interface_between_stages(layer_data *my_data, VkDevice dev,
                                  shader_module const *producer, spirv_inst_iter producer_entrypoint, char const *producer_name,
                                  shader_module const *consumer, spirv_inst_iter consumer_entrypoint, char const *consumer_name,
                                  bool consumer_arrayed_input)
{
    std::map<location_t, interface_var> outputs;
    std::map<location_t, interface_var> inputs;

    bool pass = true;

    collect_interface_by_location(my_data, dev, producer, producer_entrypoint, spv::StorageClassOutput, outputs, false);
    collect_interface_by_location(my_data, dev, consumer, consumer_entrypoint, spv::StorageClassInput, inputs, consumer_arrayed_input);

    auto a_it = outputs.begin();
    auto b_it = inputs.begin();

    /* maps sorted by key (location); walk them together to find mismatches */
    while ((outputs.size() > 0 && a_it != outputs.end()) || ( inputs.size() && b_it != inputs.end())) {
        bool a_at_end = outputs.size() == 0 || a_it == outputs.end();
        bool b_at_end = inputs.size() == 0  || b_it == inputs.end();
        auto a_first = a_at_end ? std::make_pair(0u, 0u) : a_it->first;
        auto b_first = b_at_end ? std::make_pair(0u, 0u) : b_it->first;

        if (b_at_end || ((!a_at_end) && (a_first < b_first))) {
            if (log_msg(my_data->report_data, VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, /*dev*/0, __LINE__, SHADER_CHECKER_OUTPUT_NOT_CONSUMED, "SC",
                    "%s writes to output location %u.%u which is not consumed by %s", producer_name, a_first.first, a_first.second, consumer_name)) {
                pass = false;
            }
            a_it++;
        }
        else if (a_at_end || a_first > b_first) {
            if (log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, /*dev*/0, __LINE__, SHADER_CHECKER_INPUT_NOT_PRODUCED, "SC",
                    "%s consumes input location %u.%u which is not written by %s", consumer_name, b_first.first, b_first.second, producer_name)) {
                pass = false;
            }
            b_it++;
        }
        else {
            if (types_match(producer, consumer, a_it->second.type_id, b_it->second.type_id, consumer_arrayed_input)) {
                /* OK! */
            }
            else {
                char producer_type[1024];
                char consumer_type[1024];
                describe_type(producer_type, producer, a_it->second.type_id);
                describe_type(consumer_type, consumer, b_it->second.type_id);

                if (log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, /*dev*/0, __LINE__, SHADER_CHECKER_INTERFACE_TYPE_MISMATCH, "SC",
                        "Type mismatch on location %u.%u: '%s' vs '%s'", a_first.first, a_first.second, producer_type, consumer_type)) {
                pass = false;
                }
            }
            a_it++;
            b_it++;
        }
    }

    return pass;
}

enum FORMAT_TYPE {
    FORMAT_TYPE_UNDEFINED,
    FORMAT_TYPE_FLOAT,  /* UNORM, SNORM, FLOAT, USCALED, SSCALED, SRGB -- anything we consider float in the shader */
    FORMAT_TYPE_SINT,
    FORMAT_TYPE_UINT,
};

static unsigned
get_format_type(VkFormat fmt) {
    switch (fmt) {
    case VK_FORMAT_UNDEFINED:
        return FORMAT_TYPE_UNDEFINED;
    case VK_FORMAT_R8_SINT:
    case VK_FORMAT_R8G8_SINT:
    case VK_FORMAT_R8G8B8_SINT:
    case VK_FORMAT_R8G8B8A8_SINT:
    case VK_FORMAT_R16_SINT:
    case VK_FORMAT_R16G16_SINT:
    case VK_FORMAT_R16G16B16_SINT:
    case VK_FORMAT_R16G16B16A16_SINT:
    case VK_FORMAT_R32_SINT:
    case VK_FORMAT_R32G32_SINT:
    case VK_FORMAT_R32G32B32_SINT:
    case VK_FORMAT_R32G32B32A32_SINT:
    case VK_FORMAT_B8G8R8_SINT:
    case VK_FORMAT_B8G8R8A8_SINT:
    case VK_FORMAT_A2B10G10R10_SINT_PACK32:
    case VK_FORMAT_A2R10G10B10_SINT_PACK32:
        return FORMAT_TYPE_SINT;
    case VK_FORMAT_R8_UINT:
    case VK_FORMAT_R8G8_UINT:
    case VK_FORMAT_R8G8B8_UINT:
    case VK_FORMAT_R8G8B8A8_UINT:
    case VK_FORMAT_R16_UINT:
    case VK_FORMAT_R16G16_UINT:
    case VK_FORMAT_R16G16B16_UINT:
    case VK_FORMAT_R16G16B16A16_UINT:
    case VK_FORMAT_R32_UINT:
    case VK_FORMAT_R32G32_UINT:
    case VK_FORMAT_R32G32B32_UINT:
    case VK_FORMAT_R32G32B32A32_UINT:
    case VK_FORMAT_B8G8R8_UINT:
    case VK_FORMAT_B8G8R8A8_UINT:
    case VK_FORMAT_A2B10G10R10_UINT_PACK32:
    case VK_FORMAT_A2R10G10B10_UINT_PACK32:
        return FORMAT_TYPE_UINT;
    default:
        return FORMAT_TYPE_FLOAT;
    }
}

/* characterizes a SPIR-V type appearing in an interface to a FF stage,
 * for comparison to a VkFormat's characterization above. */
static unsigned
get_fundamental_type(shader_module const *src, unsigned type)
{
    auto insn = src->get_def(type);
    assert(insn != src->end());

    switch (insn.opcode()) {
        case spv::OpTypeInt:
            return insn.word(3) ? FORMAT_TYPE_SINT : FORMAT_TYPE_UINT;
        case spv::OpTypeFloat:
            return FORMAT_TYPE_FLOAT;
        case spv::OpTypeVector:
            return get_fundamental_type(src, insn.word(2));
        case spv::OpTypeMatrix:
            return get_fundamental_type(src, insn.word(2));
        case spv::OpTypeArray:
            return get_fundamental_type(src, insn.word(2));
        case spv::OpTypePointer:
            return get_fundamental_type(src, insn.word(3));
        default:
            return FORMAT_TYPE_UNDEFINED;
    }
}

static bool
validate_vi_consistency(layer_data *my_data, VkDevice dev, VkPipelineVertexInputStateCreateInfo const *vi)
{
    /* walk the binding descriptions, which describe the step rate and stride of each vertex buffer.
     * each binding should be specified only once.
     */
    std::unordered_map<uint32_t, VkVertexInputBindingDescription const *> bindings;
    bool pass = true;

    for (unsigned i = 0; i < vi->vertexBindingDescriptionCount; i++) {
        auto desc = &vi->pVertexBindingDescriptions[i];
        auto & binding = bindings[desc->binding];
        if (binding) {
            if (log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, /*dev*/0, __LINE__, SHADER_CHECKER_INCONSISTENT_VI, "SC",
                    "Duplicate vertex input binding descriptions for binding %d", desc->binding)) {
                pass = false;
            }
        }
        else {
            binding = desc;
        }
    }

    return pass;
}

static bool
validate_vi_against_vs_inputs(layer_data *my_data, VkDevice dev, VkPipelineVertexInputStateCreateInfo const *vi, shader_module const *vs, spirv_inst_iter entrypoint)
{
    std::map<location_t, interface_var> inputs;
    bool pass = true;

    collect_interface_by_location(my_data, dev, vs, entrypoint, spv::StorageClassInput, inputs, false);

    /* Build index by location */
    std::map<uint32_t, VkVertexInputAttributeDescription const *> attribs;
    if (vi) {
        for (unsigned i = 0; i < vi->vertexAttributeDescriptionCount; i++)
            attribs[vi->pVertexAttributeDescriptions[i].location] = &vi->pVertexAttributeDescriptions[i];
    }

    auto it_a = attribs.begin();
    auto it_b = inputs.begin();

    while ((attribs.size() > 0 && it_a != attribs.end()) || (inputs.size() > 0 && it_b != inputs.end())) {
        bool a_at_end = attribs.size() == 0 || it_a == attribs.end();
        bool b_at_end = inputs.size() == 0  || it_b == inputs.end();
        auto a_first = a_at_end ? 0 : it_a->first;
        auto b_first = b_at_end ? 0 : it_b->first.first;
        if (!a_at_end && (b_at_end || a_first < b_first)) {
            if (log_msg(my_data->report_data, VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, /*dev*/0, __LINE__, SHADER_CHECKER_OUTPUT_NOT_CONSUMED, "SC",
                    "Vertex attribute at location %d not consumed by VS", a_first)) {
                pass = false;
            }
            it_a++;
        }
        else if (!b_at_end && (a_at_end || b_first < a_first)) {
            if (log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, /*dev*/0, __LINE__, SHADER_CHECKER_INPUT_NOT_PRODUCED, "SC",
                    "VS consumes input at location %d but not provided", b_first)) {
                pass = false;
            }
            it_b++;
        }
        else {
            unsigned attrib_type = get_format_type(it_a->second->format);
            unsigned input_type = get_fundamental_type(vs, it_b->second.type_id);

            /* type checking */
            if (attrib_type != FORMAT_TYPE_UNDEFINED && input_type != FORMAT_TYPE_UNDEFINED && attrib_type != input_type) {
                char vs_type[1024];
                describe_type(vs_type, vs, it_b->second.type_id);
                if (log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, /*dev*/0, __LINE__, SHADER_CHECKER_INTERFACE_TYPE_MISMATCH, "SC",
                        "Attribute type of `%s` at location %d does not match VS input type of `%s`",
                        string_VkFormat(it_a->second->format), a_first, vs_type)) {
                    pass = false;
                }
            }

            /* OK! */
            it_a++;
            it_b++;
        }
    }

    return pass;
}

static bool
validate_fs_outputs_against_render_pass(layer_data *my_data, VkDevice dev, shader_module const *fs, spirv_inst_iter entrypoint, RENDER_PASS_NODE const *rp, uint32_t subpass)
{
    const std::vector<VkFormat> &color_formats = rp->subpassColorFormats[subpass];
    std::map<location_t, interface_var> outputs;
    bool pass = true;

    /* TODO: dual source blend index (spv::DecIndex, zero if not provided) */

    collect_interface_by_location(my_data, dev, fs, entrypoint, spv::StorageClassOutput, outputs, false);

    auto it = outputs.begin();
    uint32_t attachment = 0;

    /* Walk attachment list and outputs together -- this is a little overpowered since attachments
     * are currently dense, but the parallel with matching between shader stages is nice.
     */

    while ((outputs.size() > 0 && it != outputs.end()) || attachment < color_formats.size()) {
        if (attachment == color_formats.size() || ( it != outputs.end() && it->first.first < attachment)) {
            if (log_msg(my_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, /*dev*/0, __LINE__, SHADER_CHECKER_OUTPUT_NOT_CONSUMED, "SC",
                    "FS writes to output location %d with no matching attachment", it->first.first)) {
                pass = false;
            }
            it++;
        }
        else if (it == outputs.end() || it->first.first > attachment) {
            if (log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, /*dev*/0, __LINE__, SHADER_CHECKER_INPUT_NOT_PRODUCED, "SC",
                    "Attachment %d not written by FS", attachment)) {
                pass = false;
            }
            attachment++;
        }
        else {
            unsigned output_type = get_fundamental_type(fs, it->second.type_id);
            unsigned att_type = get_format_type(color_formats[attachment]);

            /* type checking */
            if (att_type != FORMAT_TYPE_UNDEFINED && output_type != FORMAT_TYPE_UNDEFINED && att_type != output_type) {
                char fs_type[1024];
                describe_type(fs_type, fs, it->second.type_id);
                if (log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, /*dev*/0, __LINE__, SHADER_CHECKER_INTERFACE_TYPE_MISMATCH, "SC",
                        "Attachment %d of type `%s` does not match FS output type of `%s`",
                        attachment, string_VkFormat(color_formats[attachment]), fs_type)) {
                    pass = false;
                }
            }

            /* OK! */
            it++;
            attachment++;
        }
    }

    return pass;
}


/* For some analyses, we need to know about all ids referenced by the static call tree of a particular
 * entrypoint. This is important for identifying the set of shader resources actually used by an entrypoint,
 * for example.
 * Note: we only explore parts of the image which might actually contain ids we care about for the above analyses.
 *  - NOT the shader input/output interfaces.
 *
 * TODO: The set of interesting opcodes here was determined by eyeballing the SPIRV spec. It might be worth
 * converting parts of this to be generated from the machine-readable spec instead.
 */
static void
mark_accessible_ids(shader_module const *src, spirv_inst_iter entrypoint, std::unordered_set<uint32_t> &ids)
{
    std::unordered_set<uint32_t> worklist;
    worklist.insert(entrypoint.word(2));

    while (!worklist.empty()) {
        auto id_iter = worklist.begin();
        auto id = *id_iter;
        worklist.erase(id_iter);

        auto insn = src->get_def(id);
        if (insn == src->end()) {
            /* id is something we didnt collect in build_def_index. that's OK -- we'll stumble
             * across all kinds of things here that we may not care about. */
            continue;
        }

        /* try to add to the output set */
        if (!ids.insert(id).second) {
            continue;       /* if we already saw this id, we don't want to walk it again. */
        }

        switch (insn.opcode()) {
        case spv::OpFunction:
            /* scan whole body of the function, enlisting anything interesting */
            while (++insn, insn.opcode() != spv::OpFunctionEnd) {
                switch (insn.opcode()) {
                case spv::OpLoad:
                case spv::OpAtomicLoad:
                case spv::OpAtomicExchange:
                case spv::OpAtomicCompareExchange:
                case spv::OpAtomicCompareExchangeWeak:
                case spv::OpAtomicIIncrement:
                case spv::OpAtomicIDecrement:
                case spv::OpAtomicIAdd:
                case spv::OpAtomicISub:
                case spv::OpAtomicSMin:
                case spv::OpAtomicUMin:
                case spv::OpAtomicSMax:
                case spv::OpAtomicUMax:
                case spv::OpAtomicAnd:
                case spv::OpAtomicOr:
                case spv::OpAtomicXor:
                    worklist.insert(insn.word(3));  /* ptr */
                    break;
                case spv::OpStore:
                case spv::OpAtomicStore:
                    worklist.insert(insn.word(1));  /* ptr */
                    break;
                case spv::OpAccessChain:
                case spv::OpInBoundsAccessChain:
                    worklist.insert(insn.word(3));  /* base ptr */
                    break;
                case spv::OpSampledImage:
                case spv::OpImageSampleImplicitLod:
                case spv::OpImageSampleExplicitLod:
                case spv::OpImageSampleDrefImplicitLod:
                case spv::OpImageSampleDrefExplicitLod:
                case spv::OpImageSampleProjImplicitLod:
                case spv::OpImageSampleProjExplicitLod:
                case spv::OpImageSampleProjDrefImplicitLod:
                case spv::OpImageSampleProjDrefExplicitLod:
                case spv::OpImageFetch:
                case spv::OpImageGather:
                case spv::OpImageDrefGather:
                case spv::OpImageRead:
                case spv::OpImage:
                case spv::OpImageQueryFormat:
                case spv::OpImageQueryOrder:
                case spv::OpImageQuerySizeLod:
                case spv::OpImageQuerySize:
                case spv::OpImageQueryLod:
                case spv::OpImageQueryLevels:
                case spv::OpImageQuerySamples:
                case spv::OpImageSparseSampleImplicitLod:
                case spv::OpImageSparseSampleExplicitLod:
                case spv::OpImageSparseSampleDrefImplicitLod:
                case spv::OpImageSparseSampleDrefExplicitLod:
                case spv::OpImageSparseSampleProjImplicitLod:
                case spv::OpImageSparseSampleProjExplicitLod:
                case spv::OpImageSparseSampleProjDrefImplicitLod:
                case spv::OpImageSparseSampleProjDrefExplicitLod:
                case spv::OpImageSparseFetch:
                case spv::OpImageSparseGather:
                case spv::OpImageSparseDrefGather:
                case spv::OpImageTexelPointer:
                    worklist.insert(insn.word(3));  /* image or sampled image */
                    break;
                case spv::OpImageWrite:
                    worklist.insert(insn.word(1));  /* image -- different operand order to above */
                    break;
                case spv::OpFunctionCall:
                    for (auto i = 3; i < insn.len(); i++) {
                        worklist.insert(insn.word(i));  /* fn itself, and all args */
                    }
                    break;

                case spv::OpExtInst:
                    for (auto i = 5; i < insn.len(); i++) {
                        worklist.insert(insn.word(i));  /* operands to ext inst */
                    }
                    break;
                }
            }
            break;
        }
    }
}


struct shader_stage_attributes {
    char const * const name;
    bool arrayed_input;
};


static shader_stage_attributes
shader_stage_attribs[] = {
    { "vertex shader", false },
    { "tessellation control shader", true },
    { "tessellation evaluation shader", false },
    { "geometry shader", true },
    { "fragment shader", false },
};

// For given pipelineLayout verify that the setLayout at slot.first
//  has the requested binding at slot.second
static bool
has_descriptor_binding(layer_data* my_data,
                       vector<VkDescriptorSetLayout>* pipelineLayout,
                       descriptor_slot_t slot)
{
    if (!pipelineLayout)
        return false;

    if (slot.first >= pipelineLayout->size())
        return false;

    const auto &bindingMap = my_data->descriptorSetLayoutMap[(*pipelineLayout)[slot.first]]
                   ->bindingToIndexMap;

    return (bindingMap.find(slot.second) != bindingMap.end());
}

static uint32_t get_shader_stage_id(VkShaderStageFlagBits stage)
{
    uint32_t bit_pos = u_ffs(stage);
    return bit_pos-1;
}

// Block of code at start here for managing/tracking Pipeline state that this layer cares about

static uint64_t g_drawCount[NUM_DRAW_TYPES] = {0, 0, 0, 0};

// TODO : Should be tracking lastBound per commandBuffer and when draws occur, report based on that cmd buffer lastBound
//   Then need to synchronize the accesses based on cmd buffer so that if I'm reading state on one cmd buffer, updates
//   to that same cmd buffer by separate thread are not changing state from underneath us
// Track the last cmd buffer touched by this thread

// prototype
static GLOBAL_CB_NODE* getCBNode(layer_data*, const VkCommandBuffer);

static VkBool32 hasDrawCmd(GLOBAL_CB_NODE* pCB)
{
    for (uint32_t i=0; i<NUM_DRAW_TYPES; i++) {
        if (pCB->drawCount[i])
            return VK_TRUE;
    }
    return VK_FALSE;
}

// Check object status for selected flag state
static VkBool32 validate_status(layer_data* my_data, GLOBAL_CB_NODE* pNode, CBStatusFlags enable_mask, CBStatusFlags status_mask, CBStatusFlags status_flag, VkFlags msg_flags, DRAW_STATE_ERROR error_code, const char* fail_msg)
{
    // If non-zero enable mask is present, check it against status but if enable_mask
    //  is 0 then no enable required so we should always just check status
    if ((!enable_mask) || (enable_mask & pNode->status)) {
        if ((pNode->status & status_mask) != status_flag) {
            // TODO : How to pass dispatchable objects as srcObject? Here src obj should be cmd buffer
            return log_msg(my_data->report_data, msg_flags, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, 0, __LINE__, error_code, "DS",
                    "CB object %#" PRIxLEAST64 ": %s", (uint64_t)(pNode->commandBuffer), fail_msg);
        }
    }
    return VK_FALSE;
}

// Retrieve pipeline node ptr for given pipeline object
static PIPELINE_NODE* getPipeline(layer_data* my_data, const VkPipeline pipeline)
{
    if (my_data->pipelineMap.find(pipeline) == my_data->pipelineMap.end()) {
        return NULL;
    }
    return my_data->pipelineMap[pipeline];
}

// Return VK_TRUE if for a given PSO, the given state enum is dynamic, else return VK_FALSE
static VkBool32 isDynamic(const PIPELINE_NODE* pPipeline, const VkDynamicState state)
{
    if (pPipeline && pPipeline->graphicsPipelineCI.pDynamicState) {
        for (uint32_t i=0; i<pPipeline->graphicsPipelineCI.pDynamicState->dynamicStateCount; i++) {
            if (state == pPipeline->graphicsPipelineCI.pDynamicState->pDynamicStates[i])
                return VK_TRUE;
        }
    }
    return VK_FALSE;
}

// Validate state stored as flags at time of draw call
static VkBool32 validate_draw_state_flags(layer_data* my_data, GLOBAL_CB_NODE* pCB, VkBool32 indexedDraw) {
    VkBool32 result;
    result = validate_status(my_data, pCB, CBSTATUS_NONE, CBSTATUS_VIEWPORT_SET, CBSTATUS_VIEWPORT_SET, VK_DEBUG_REPORT_ERROR_BIT_EXT, DRAWSTATE_VIEWPORT_NOT_BOUND, "Dynamic viewport state not set for this command buffer");
    result |= validate_status(my_data, pCB, CBSTATUS_NONE, CBSTATUS_SCISSOR_SET, CBSTATUS_SCISSOR_SET, VK_DEBUG_REPORT_ERROR_BIT_EXT, DRAWSTATE_SCISSOR_NOT_BOUND, "Dynamic scissor state not set for this command buffer");
    result |= validate_status(my_data, pCB, CBSTATUS_NONE, CBSTATUS_LINE_WIDTH_SET, CBSTATUS_LINE_WIDTH_SET, VK_DEBUG_REPORT_ERROR_BIT_EXT, DRAWSTATE_LINE_WIDTH_NOT_BOUND, "Dynamic line width state not set for this command buffer");
    result |= validate_status(my_data, pCB, CBSTATUS_NONE, CBSTATUS_DEPTH_BIAS_SET, CBSTATUS_DEPTH_BIAS_SET, VK_DEBUG_REPORT_ERROR_BIT_EXT, DRAWSTATE_DEPTH_BIAS_NOT_BOUND, "Dynamic depth bias state not set for this command buffer");
    result |= validate_status(my_data, pCB, CBSTATUS_COLOR_BLEND_WRITE_ENABLE, CBSTATUS_BLEND_SET, CBSTATUS_BLEND_SET, VK_DEBUG_REPORT_ERROR_BIT_EXT, DRAWSTATE_BLEND_NOT_BOUND, "Dynamic blend object state not set for this command buffer");
    result |= validate_status(my_data, pCB, CBSTATUS_DEPTH_WRITE_ENABLE, CBSTATUS_DEPTH_BOUNDS_SET, CBSTATUS_DEPTH_BOUNDS_SET, VK_DEBUG_REPORT_ERROR_BIT_EXT, DRAWSTATE_DEPTH_BOUNDS_NOT_BOUND, "Dynamic depth bounds state not set for this command buffer");
    result |= validate_status(my_data, pCB, CBSTATUS_STENCIL_TEST_ENABLE, CBSTATUS_STENCIL_READ_MASK_SET, CBSTATUS_STENCIL_READ_MASK_SET, VK_DEBUG_REPORT_ERROR_BIT_EXT, DRAWSTATE_STENCIL_NOT_BOUND, "Dynamic stencil read mask state not set for this command buffer");
    result |= validate_status(my_data, pCB, CBSTATUS_STENCIL_TEST_ENABLE, CBSTATUS_STENCIL_WRITE_MASK_SET, CBSTATUS_STENCIL_WRITE_MASK_SET, VK_DEBUG_REPORT_ERROR_BIT_EXT, DRAWSTATE_STENCIL_NOT_BOUND, "Dynamic stencil write mask state not set for this command buffer");
    result |= validate_status(my_data, pCB, CBSTATUS_STENCIL_TEST_ENABLE, CBSTATUS_STENCIL_REFERENCE_SET, CBSTATUS_STENCIL_REFERENCE_SET, VK_DEBUG_REPORT_ERROR_BIT_EXT, DRAWSTATE_STENCIL_NOT_BOUND, "Dynamic stencil reference state not set for this command buffer");
    if (indexedDraw)
        result |= validate_status(my_data, pCB, CBSTATUS_NONE, CBSTATUS_INDEX_BUFFER_BOUND, CBSTATUS_INDEX_BUFFER_BOUND, VK_DEBUG_REPORT_ERROR_BIT_EXT, DRAWSTATE_INDEX_BUFFER_NOT_BOUND, "Index buffer object not bound to this command buffer when Indexed Draw attempted");
    return result;
}

// Verify attachment reference compatibility according to spec
//  If one array is larger, treat missing elements of shorter array as VK_ATTACHMENT_UNUSED & other array much match this
//  If both AttachmentReference arrays have requested index, check their corresponding AttachementDescriptions
//   to make sure that format and samples counts match.
//  If not, they are not compatible.
static bool attachment_references_compatible(const uint32_t index, const VkAttachmentReference* pPrimary, const uint32_t primaryCount, const VkAttachmentDescription* pPrimaryAttachments, 
                                            const VkAttachmentReference* pSecondary, const uint32_t secondaryCount, const VkAttachmentDescription* pSecondaryAttachments)
{
    if (index >= primaryCount) { // Check secondary as if primary is VK_ATTACHMENT_UNUSED
        if (VK_ATTACHMENT_UNUSED != pSecondary[index].attachment)
            return false;
    } else if (index >= secondaryCount) { // Check primary as if secondary is VK_ATTACHMENT_UNUSED
        if (VK_ATTACHMENT_UNUSED != pPrimary[index].attachment)
            return false;
    } else { // format and sample count must match
        if ((pPrimaryAttachments[pPrimary[index].attachment].format == pSecondaryAttachments[pSecondary[index].attachment].format) &&
            (pPrimaryAttachments[pPrimary[index].attachment].samples == pSecondaryAttachments[pSecondary[index].attachment].samples))
            return true;
    }
    // Format and sample counts didn't match
    return false;
}

// For give primary and secondary RenderPass objects, verify that they're compatible
static bool verify_renderpass_compatibility(layer_data* my_data, const VkRenderPass primaryRP, const VkRenderPass secondaryRP, string& errorMsg)
{
    stringstream errorStr;
    if (my_data->renderPassMap.find(primaryRP) == my_data->renderPassMap.end()) {
        errorStr << "invalid VkRenderPass (" << primaryRP << ")";
        errorMsg = errorStr.str();
        return false;
    } else if (my_data->renderPassMap.find(secondaryRP) == my_data->renderPassMap.end()) {
        errorStr << "invalid VkRenderPass (" << secondaryRP << ")";
        errorMsg = errorStr.str();
        return false;
    }
    // Trivial pass case is exact same RP
    if (primaryRP == secondaryRP) {
        return true;
    }
    const VkRenderPassCreateInfo* primaryRPCI = my_data->renderPassMap[primaryRP]->pCreateInfo;
    const VkRenderPassCreateInfo* secondaryRPCI = my_data->renderPassMap[secondaryRP]->pCreateInfo;
    if (primaryRPCI->subpassCount != secondaryRPCI->subpassCount) {
        errorStr << "RenderPass for primary cmdBuffer has " << primaryRPCI->subpassCount << " subpasses but renderPass for secondary cmdBuffer has " << secondaryRPCI->subpassCount << " subpasses.";
        errorMsg = errorStr.str();
        return false;
    }
    uint32_t spIndex = 0;
    for (spIndex = 0; spIndex < primaryRPCI->subpassCount; ++spIndex) {
        // For each subpass, verify that corresponding color, input, resolve & depth/stencil attachment references are compatible
        uint32_t primaryColorCount = primaryRPCI->pSubpasses[spIndex].colorAttachmentCount;
        uint32_t secondaryColorCount = secondaryRPCI->pSubpasses[spIndex].colorAttachmentCount;
        uint32_t colorMax = std::max(primaryColorCount, secondaryColorCount);
        for (uint32_t cIdx = 0; cIdx < colorMax; ++cIdx) {
            if (!attachment_references_compatible(cIdx, primaryRPCI->pSubpasses[spIndex].pColorAttachments, primaryColorCount, primaryRPCI->pAttachments,
                                                 secondaryRPCI->pSubpasses[spIndex].pColorAttachments, secondaryColorCount, secondaryRPCI->pAttachments)) {
                errorStr << "color attachments at index " << cIdx << " of subpass index " << spIndex << " are not compatible.";
                errorMsg = errorStr.str();
                return false;
            } else if (!attachment_references_compatible(cIdx, primaryRPCI->pSubpasses[spIndex].pResolveAttachments, primaryColorCount, primaryRPCI->pAttachments,
                                                        secondaryRPCI->pSubpasses[spIndex].pResolveAttachments, secondaryColorCount, secondaryRPCI->pAttachments)) {
                errorStr << "resolve attachments at index " << cIdx << " of subpass index " << spIndex << " are not compatible.";
                errorMsg = errorStr.str();
                return false;
            } else if (!attachment_references_compatible(cIdx, primaryRPCI->pSubpasses[spIndex].pDepthStencilAttachment, primaryColorCount, primaryRPCI->pAttachments,
                                                        secondaryRPCI->pSubpasses[spIndex].pDepthStencilAttachment, secondaryColorCount, secondaryRPCI->pAttachments)) {
                errorStr << "depth/stencil attachments at index " << cIdx << " of subpass index " << spIndex << " are not compatible.";
                errorMsg = errorStr.str();
                return false;
            }
        }
        uint32_t primaryInputCount = primaryRPCI->pSubpasses[spIndex].inputAttachmentCount;
        uint32_t secondaryInputCount = secondaryRPCI->pSubpasses[spIndex].inputAttachmentCount;
        uint32_t inputMax = std::max(primaryInputCount, secondaryInputCount);
        for (uint32_t i = 0; i < inputMax; ++i) {
            if (!attachment_references_compatible(i, primaryRPCI->pSubpasses[spIndex].pInputAttachments, primaryColorCount, primaryRPCI->pAttachments,
                                                 secondaryRPCI->pSubpasses[spIndex].pInputAttachments, secondaryColorCount, secondaryRPCI->pAttachments)) {
                errorStr << "input attachments at index " << i << " of subpass index " << spIndex << " are not compatible.";
                errorMsg = errorStr.str();
                return false;
            }
        }
    }
    return true;
}

// For give SET_NODE, verify that its Set is compatible w/ the setLayout corresponding to pipelineLayout[layoutIndex]
static bool verify_set_layout_compatibility(layer_data* my_data, const SET_NODE* pSet, const VkPipelineLayout layout, const uint32_t layoutIndex, string& errorMsg)
{
    stringstream errorStr;
    if (my_data->pipelineLayoutMap.find(layout) == my_data->pipelineLayoutMap.end()) {
        errorStr << "invalid VkPipelineLayout (" << layout << ")";
        errorMsg = errorStr.str();
        return false;
    }
    PIPELINE_LAYOUT_NODE pl = my_data->pipelineLayoutMap[layout];
    if (layoutIndex >= pl.descriptorSetLayouts.size()) {
        errorStr << "VkPipelineLayout (" << layout << ") only contains " << pl.descriptorSetLayouts.size() << " setLayouts corresponding to sets 0-" << pl.descriptorSetLayouts.size()-1 << ", but you're attempting to bind set to index " << layoutIndex;
        errorMsg = errorStr.str();
        return false;
    }
    // Get the specific setLayout from PipelineLayout that overlaps this set
    LAYOUT_NODE* pLayoutNode = my_data->descriptorSetLayoutMap[pl.descriptorSetLayouts[layoutIndex]];
    if (pLayoutNode->layout == pSet->pLayout->layout) { // trivial pass case
        return true;
    }
    size_t descriptorCount = pLayoutNode->descriptorTypes.size();
    if (descriptorCount != pSet->pLayout->descriptorTypes.size()) {
        errorStr << "setLayout " << layoutIndex << " from pipelineLayout " << layout << " has " << descriptorCount << " descriptors, but corresponding set being bound has " << pSet->pLayout->descriptorTypes.size() << " descriptors.";
        errorMsg = errorStr.str();
        return false; // trivial fail case
    }
    // Now need to check set against corresponding pipelineLayout to verify compatibility
    for (size_t i=0; i<descriptorCount; ++i) {
        // Need to verify that layouts are identically defined
        //  TODO : Is below sufficient? Making sure that types & stageFlags match per descriptor
        //    do we also need to check immutable samplers?
        if (pLayoutNode->descriptorTypes[i] != pSet->pLayout->descriptorTypes[i]) {
            errorStr << "descriptor " << i << " for descriptorSet being bound is type '" << string_VkDescriptorType(pSet->pLayout->descriptorTypes[i]) << "' but corresponding descriptor from pipelineLayout is type '" << string_VkDescriptorType(pLayoutNode->descriptorTypes[i]) << "'";
            errorMsg = errorStr.str();
            return false;
        }
        if (pLayoutNode->stageFlags[i] != pSet->pLayout->stageFlags[i]) {
            errorStr << "stageFlags " << i << " for descriptorSet being bound is " << pSet->pLayout->stageFlags[i] << "' but corresponding descriptor from pipelineLayout has stageFlags " << pLayoutNode->stageFlags[i];
            errorMsg = errorStr.str();
            return false;
        }
    }
    return true;
}


// Validate that data for each specialization entry is fully contained within the buffer.
static VkBool32
validate_specialization_offsets(layer_data *my_data, VkPipelineShaderStageCreateInfo const *info)
{
    VkBool32 pass = VK_TRUE;

    VkSpecializationInfo const *spec = info->pSpecializationInfo;

    if (spec) {
        for (auto i = 0u; i < spec->mapEntryCount; i++) {
            if (spec->pMapEntries[i].offset + spec->pMapEntries[i].size > spec->dataSize) {
                if (log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                    /*dev*/0, __LINE__, SHADER_CHECKER_BAD_SPECIALIZATION, "SC",
                    "Specialization entry %u (for constant id %u) references memory outside provided "
                    "specialization data (bytes %u.."
                    PRINTF_SIZE_T_SPECIFIER "; " PRINTF_SIZE_T_SPECIFIER " bytes provided)",
                    i, spec->pMapEntries[i].constantID,
                    spec->pMapEntries[i].offset,
                    spec->pMapEntries[i].offset + spec->pMapEntries[i].size - 1,
                    spec->dataSize)) {

                    pass = VK_FALSE;
                }
            }
        }
    }

    return pass;
}


// Validate that the shaders used by the given pipeline
//  As a side effect this function also records the sets that are actually used by the pipeline
static VkBool32
validate_pipeline_shaders(layer_data *my_data, VkDevice dev, PIPELINE_NODE* pPipeline)
{
    VkGraphicsPipelineCreateInfo const *pCreateInfo = &pPipeline->graphicsPipelineCI;
    /* We seem to allow pipeline stages to be specified out of order, so collect and identify them
     * before trying to do anything more: */
    int vertex_stage = get_shader_stage_id(VK_SHADER_STAGE_VERTEX_BIT);
    int fragment_stage = get_shader_stage_id(VK_SHADER_STAGE_FRAGMENT_BIT);

    shader_module *shaders[5];
    memset(shaders, 0, sizeof(shaders));
    spirv_inst_iter entrypoints[5];
    memset(entrypoints, 0, sizeof(entrypoints));
    RENDER_PASS_NODE const *rp = 0;
    VkPipelineVertexInputStateCreateInfo const *vi = 0;
    VkBool32 pass = VK_TRUE;

    for (uint32_t i = 0; i < pCreateInfo->stageCount; i++) {
        VkPipelineShaderStageCreateInfo const *pStage = &pCreateInfo->pStages[i];
        if (pStage->sType == VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO) {

            if ((pStage->stage & (VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
                                  | VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT)) == 0) {
                if (log_msg(my_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, /*dev*/0, __LINE__, SHADER_CHECKER_UNKNOWN_STAGE, "SC",
                        "Unknown shader stage %d", pStage->stage)) {
                    pass = VK_FALSE;
                }
            }
            else {
                pass = validate_specialization_offsets(my_data, pStage) && pass;

                auto stage_id = get_shader_stage_id(pStage->stage);
                shader_module *module = my_data->shaderModuleMap[pStage->module];
                shaders[stage_id] = module;

                /* find the entrypoint */
                entrypoints[stage_id] = find_entrypoint(module, pStage->pName, pStage->stage);
                if (entrypoints[stage_id] == module->end()) {
                    if (log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, /*dev*/0, __LINE__, SHADER_CHECKER_MISSING_ENTRYPOINT, "SC",
                        "No entrypoint found named `%s` for stages %u", pStage->pName, pStage->stage)) {
                        pass = VK_FALSE;
                    }
                }

                /* mark accessible ids */
                std::unordered_set<uint32_t> accessible_ids;
                mark_accessible_ids(module, entrypoints[stage_id], accessible_ids);

                /* validate descriptor set layout against what the entrypoint actually uses */
                std::map<descriptor_slot_t, interface_var> descriptor_uses;
                collect_interface_by_descriptor_slot(my_data, dev, module, spv::StorageClassUniform,
                        accessible_ids,
                        descriptor_uses);

                auto layouts = pCreateInfo->layout != VK_NULL_HANDLE ?
                    &(my_data->pipelineLayoutMap[pCreateInfo->layout].descriptorSetLayouts) : nullptr;

                for (auto it = descriptor_uses.begin(); it != descriptor_uses.end(); it++) {
                    // As a side-effect of this function, capture which sets are used by the pipeline
                    pPipeline->active_sets.insert(it->first.first);

                    /* find the matching binding */
                    auto found = has_descriptor_binding(my_data, layouts, it->first);

                    if (!found) {
                        char type_name[1024];
                        describe_type(type_name, module, it->second.type_id);
                        if (log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, /*dev*/0, __LINE__,
                                SHADER_CHECKER_MISSING_DESCRIPTOR, "SC",
                                "Shader uses descriptor slot %u.%u (used as type `%s`) but not declared in pipeline layout",
                                it->first.first, it->first.second, type_name)) {
                            pass = VK_FALSE;
                        }
                    }
                }
            }
        }
    }

    if (pCreateInfo->renderPass != VK_NULL_HANDLE)
        rp = my_data->renderPassMap[pCreateInfo->renderPass];

    vi = pCreateInfo->pVertexInputState;

    if (vi) {
        pass = validate_vi_consistency(my_data, dev, vi) && pass;
    }

    if (shaders[vertex_stage]) {
        pass = validate_vi_against_vs_inputs(my_data, dev, vi, shaders[vertex_stage], entrypoints[vertex_stage]) && pass;
    }

    /* TODO: enforce rules about present combinations of shaders */
    int producer = get_shader_stage_id(VK_SHADER_STAGE_VERTEX_BIT);
    int consumer = get_shader_stage_id(VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT);

    while (!shaders[producer] && producer != fragment_stage) {
        producer++;
        consumer++;
    }

    for (; producer != fragment_stage && consumer <= fragment_stage; consumer++) {
        assert(shaders[producer]);
        if (shaders[consumer]) {
            pass = validate_interface_between_stages(my_data, dev,
                                                     shaders[producer], entrypoints[producer],
                                                     shader_stage_attribs[producer].name,
                                                     shaders[consumer], entrypoints[consumer],
                                                     shader_stage_attribs[consumer].name,
                                                     shader_stage_attribs[consumer].arrayed_input) && pass;

            producer = consumer;
        }
    }

    if (shaders[fragment_stage] && rp) {
        pass = validate_fs_outputs_against_render_pass(my_data, dev, shaders[fragment_stage],
            entrypoints[fragment_stage], rp, pCreateInfo->subpass) && pass;
    }

    return pass;
}

// Return Set node ptr for specified set or else NULL
static SET_NODE* getSetNode(layer_data* my_data, const VkDescriptorSet set)
{
    if (my_data->setMap.find(set) == my_data->setMap.end()) {
        return NULL;
    }
    return my_data->setMap[set];
}
// For the given command buffer, verify that for each set set in activeSetNodes
//  that any dynamic descriptor in that set has a valid dynamic offset bound.
//  To be valid, the dynamic offset combined with the offet and range from its
//  descriptor update must not overflow the size of its buffer being updated
static VkBool32 validate_dynamic_offsets(layer_data* my_data, const GLOBAL_CB_NODE* pCB, const vector<SET_NODE*> activeSetNodes)
{
    VkBool32 result = VK_FALSE;

    VkWriteDescriptorSet* pWDS = NULL;
    uint32_t dynOffsetIndex = 0;
    VkDeviceSize bufferSize = 0;
    for (auto set_node : activeSetNodes) {
        for (uint32_t i = 0; i < set_node->descriptorCount; ++i) {
            if (set_node->ppDescriptors[i] != NULL) {
                switch (set_node->ppDescriptors[i]->sType) {
                case VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET:
                    pWDS = (VkWriteDescriptorSet *)set_node->ppDescriptors[i];
                    if ((pWDS->descriptorType ==
                         VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC) ||
                        (pWDS->descriptorType ==
                         VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC)) {
                        for (uint32_t j = 0; j < pWDS->descriptorCount; ++j) {
                            bufferSize =
                                my_data->bufferMap[pWDS->pBufferInfo[j].buffer]
                                    .create_info->size;
                            if (pWDS->pBufferInfo[j].range == VK_WHOLE_SIZE) {
                                if ((pCB->dynamicOffsets[dynOffsetIndex] +
                                     pWDS->pBufferInfo[j].offset) > bufferSize) {
                                    result |= log_msg(
                                        my_data->report_data,
                                        VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                        VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT,
                                        (uint64_t)set_node->set, __LINE__,
                                        DRAWSTATE_DYNAMIC_OFFSET_OVERFLOW, "DS",
                                        "VkDescriptorSet (%#" PRIxLEAST64
                                        ") bound as set #%u has range of "
                                        "VK_WHOLE_SIZE but dynamic offset %u "
                                        "combined with offet %#" PRIxLEAST64
                                        " oversteps its buffer (%#" PRIxLEAST64
                                        ") which has a size of %#" PRIxLEAST64 ".",
                                        (uint64_t)set_node->set, i,
                                        pCB->dynamicOffsets[dynOffsetIndex],
                                        pWDS->pBufferInfo[j].offset,
                                        (uint64_t)pWDS->pBufferInfo[j].buffer,
                                        bufferSize);
                                }
                            } else if ((pCB->dynamicOffsets[dynOffsetIndex] +
                                        pWDS->pBufferInfo[j].offset +
                                        pWDS->pBufferInfo[j].range) > bufferSize) {
                                result |= log_msg(
                                    my_data->report_data,
                                    VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                    VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT,
                                    (uint64_t)set_node->set, __LINE__,
                                    DRAWSTATE_DYNAMIC_OFFSET_OVERFLOW, "DS",
                                    "VkDescriptorSet (%#" PRIxLEAST64
                                    ") bound as set #%u has dynamic offset %u. "
                                    "Combined with offet %#" PRIxLEAST64
                                    " and range %#" PRIxLEAST64
                                    " from its update, this oversteps its buffer "
                                    "(%#" PRIxLEAST64
                                    ") which has a size of %#" PRIxLEAST64 ".",
                                    (uint64_t)set_node->set, i,
                                    pCB->dynamicOffsets[dynOffsetIndex],
                                    pWDS->pBufferInfo[j].offset,
                                    pWDS->pBufferInfo[j].range,
                                    (uint64_t)pWDS->pBufferInfo[j].buffer,
                                    bufferSize);
                            } else if ((pCB->dynamicOffsets[dynOffsetIndex] +
                                        pWDS->pBufferInfo[j].offset +
                                        pWDS->pBufferInfo[j].range) >
                                       bufferSize) {
                              result |= log_msg(
                                  my_data->report_data,
                                  VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                  VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT,
                                  (uint64_t)set_node->set, __LINE__,
                                  DRAWSTATE_DYNAMIC_OFFSET_OVERFLOW, "DS",
                                  "VkDescriptorSet (%#" PRIxLEAST64
                                  ") bound as set #%u has dynamic offset %u. "
                                  "Combined with offet %#" PRIxLEAST64
                                  " and range %#" PRIxLEAST64
                                  " from its update, this oversteps its buffer "
                                  "(%#" PRIxLEAST64
                                  ") which has a size of %#" PRIxLEAST64 ".",
                                  (uint64_t)set_node->set, i,
                                  pCB->dynamicOffsets[dynOffsetIndex],
                                  pWDS->pBufferInfo[j].offset,
                                  pWDS->pBufferInfo[j].range,
                                  (uint64_t)pWDS->pBufferInfo[j].buffer,
                                  bufferSize);
                            }
                            dynOffsetIndex++;
                            i += j; // Advance i to end of this set of descriptors (++i at end of for loop will move 1 index past last of these descriptors)
                        }
                    }
                    break;
                default: // Currently only shadowing Write update nodes so shouldn't get here
                    assert(0);
                    continue;
                }
            }
        }
    }
    return result;
}

// Validate overall state at the time of a draw call
static VkBool32 validate_draw_state(layer_data* my_data, GLOBAL_CB_NODE* pCB, VkBool32 indexedDraw) {
    // First check flag states
    VkBool32 result = validate_draw_state_flags(my_data, pCB, indexedDraw);
    PIPELINE_NODE* pPipe = getPipeline(my_data, pCB->lastBoundPipeline);
    // Now complete other state checks
    // TODO : Currently only performing next check if *something* was bound (non-zero last bound)
    //  There is probably a better way to gate when this check happens, and to know if something *should* have been bound
    //  We should have that check separately and then gate this check based on that check
    if (pPipe) {
        if (pCB->lastBoundPipelineLayout) {
            string errorString;
            // Need a vector (vs. std::set) of active Sets for dynamicOffset validation in case same set bound w/ different offsets
            vector<SET_NODE*> activeSetNodes;
            for (auto setIndex : pPipe->active_sets) {
                // If valid set is not bound throw an error
                if ((pCB->boundDescriptorSets.size() <= setIndex) || (!pCB->boundDescriptorSets[setIndex])) {
                    result |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT) 0, 0, __LINE__, DRAWSTATE_DESCRIPTOR_SET_NOT_BOUND, "DS",
                            "VkPipeline %#" PRIxLEAST64 " uses set #%u but that set is not bound.", (uint64_t)pPipe->pipeline, setIndex);
                } else if (!verify_set_layout_compatibility(my_data, my_data->setMap[pCB->boundDescriptorSets[setIndex]], pPipe->graphicsPipelineCI.layout, setIndex, errorString)) {
                    // Set is bound but not compatible w/ overlapping pipelineLayout from PSO
                    VkDescriptorSet setHandle = my_data->setMap[pCB->boundDescriptorSets[setIndex]]->set;
                    result |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT, (uint64_t)setHandle, __LINE__, DRAWSTATE_PIPELINE_LAYOUTS_INCOMPATIBLE, "DS",
                        "VkDescriptorSet (%#" PRIxLEAST64 ") bound as set #%u is not compatible with overlapping VkPipelineLayout %#" PRIxLEAST64 " due to: %s",
                            (uint64_t)setHandle, setIndex, (uint64_t)pPipe->graphicsPipelineCI.layout, errorString.c_str());
                } else { // Valid set is bound and layout compatible, validate that it's updated and verify any dynamic offsets
                    // Pull the set node
                    SET_NODE* pSet = my_data->setMap[pCB->boundDescriptorSets[setIndex]];
                    // Save vector of all active sets to verify dynamicOffsets below
                    activeSetNodes.push_back(pSet);
                    // Make sure set has been updated
                    if (!pSet->pUpdateStructs) {
                        result |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT, (uint64_t) pSet->set, __LINE__, DRAWSTATE_DESCRIPTOR_SET_NOT_UPDATED, "DS",
                            "DS %#" PRIxLEAST64 " bound but it was never updated. It is now being used to draw so this will result in undefined behavior.", (uint64_t) pSet->set);
                    }
                }
            }
            // For each dynamic descriptor, make sure dynamic offset doesn't overstep buffer
            if (!pCB->dynamicOffsets.empty())
                result |= validate_dynamic_offsets(my_data, pCB, activeSetNodes);
        }
        // Verify Vtx binding
        if (pPipe->vtxBindingCount > 0) {
            VkPipelineVertexInputStateCreateInfo *vtxInCI = &pPipe->vertexInputCI;
            for (uint32_t i = 0; i < vtxInCI->vertexBindingDescriptionCount; i++) {
                if ((pCB->currentDrawData.buffers.size() < (i+1)) || (pCB->currentDrawData.buffers[i] == VK_NULL_HANDLE)) {
                    result |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT) 0, 0, __LINE__, DRAWSTATE_VTX_INDEX_OUT_OF_BOUNDS, "DS",
                        "The Pipeline State Object (%#" PRIxLEAST64 ") expects that this Command Buffer's vertex binding Index %d should be set via vkCmdBindVertexBuffers.",
                        (uint64_t)pCB->lastBoundPipeline, i);

                }
            }
        } else {
            if (!pCB->currentDrawData.buffers.empty()) {
                result |= log_msg(my_data->report_data, VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT, (VkDebugReportObjectTypeEXT) 0, 0, __LINE__, DRAWSTATE_VTX_INDEX_OUT_OF_BOUNDS,
                    "DS", "Vertex buffers are bound to command buffer (%#" PRIxLEAST64 ") but no vertex buffers are attached to this Pipeline State Object (%#" PRIxLEAST64 ").",
                    (uint64_t)pCB->commandBuffer, (uint64_t)pCB->lastBoundPipeline);
            }
        }
        // If Viewport or scissors are dynamic, verify that dynamic count matches PSO count
        VkBool32 dynViewport = isDynamic(pPipe, VK_DYNAMIC_STATE_VIEWPORT);
        VkBool32 dynScissor = isDynamic(pPipe, VK_DYNAMIC_STATE_SCISSOR);
        if (dynViewport) {
            if (pCB->viewports.size() != pPipe->graphicsPipelineCI.pViewportState->viewportCount) {
                result |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT) 0, 0, __LINE__, DRAWSTATE_VIEWPORT_SCISSOR_MISMATCH, "DS",
                    "Dynamic viewportCount from vkCmdSetViewport() is " PRINTF_SIZE_T_SPECIFIER ", but PSO viewportCount is %u. These counts must match.", pCB->viewports.size(), pPipe->graphicsPipelineCI.pViewportState->viewportCount);
            }
        }
        if (dynScissor) {
            if (pCB->scissors.size() != pPipe->graphicsPipelineCI.pViewportState->scissorCount) {
                result |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT) 0, 0, __LINE__, DRAWSTATE_VIEWPORT_SCISSOR_MISMATCH, "DS",
                    "Dynamic scissorCount from vkCmdSetScissor() is " PRINTF_SIZE_T_SPECIFIER ", but PSO scissorCount is %u. These counts must match.", pCB->scissors.size(), pPipe->graphicsPipelineCI.pViewportState->scissorCount);
            }
        }
    }
    return result;
}

// Verify that create state for a pipeline is valid
static VkBool32 verifyPipelineCreateState(layer_data* my_data, const VkDevice device, PIPELINE_NODE* pPipeline)
{
    VkBool32 skipCall = VK_FALSE;

    if (pPipeline->graphicsPipelineCI.pColorBlendState != NULL) {
        if (!my_data->physDevProperties.features.independentBlend) {
            VkPipelineColorBlendAttachmentState *pAttachments = pPipeline->pAttachments;
            for (uint i = 1 ; i < pPipeline->attachmentCount ; i++) {
                if ((pAttachments[0].blendEnable != pAttachments[i].blendEnable) ||
                    (pAttachments[0].srcColorBlendFactor != pAttachments[i].srcColorBlendFactor) ||
                    (pAttachments[0].dstColorBlendFactor != pAttachments[i].dstColorBlendFactor) ||
                    (pAttachments[0].colorBlendOp != pAttachments[i].colorBlendOp) ||
                    (pAttachments[0].srcAlphaBlendFactor != pAttachments[i].srcAlphaBlendFactor) ||
                    (pAttachments[0].dstAlphaBlendFactor != pAttachments[i].dstAlphaBlendFactor) ||
                    (pAttachments[0].alphaBlendOp != pAttachments[i].alphaBlendOp) ||
                    (pAttachments[0].colorWriteMask != pAttachments[i].colorWriteMask)) {
                    skipCall |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                        (VkDebugReportObjectTypeEXT) 0, 0, __LINE__, DRAWSTATE_INDEPENDENT_BLEND, "DS",
                                        "Invalid Pipeline CreateInfo: If independent blend feature not enabled, all elements of pAttachments must be identical");
                }
            }
        }
        if (!my_data->physDevProperties.features.logicOp &&
            (pPipeline->graphicsPipelineCI.pColorBlendState->logicOpEnable != VK_FALSE)) {
            skipCall |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                (VkDebugReportObjectTypeEXT) 0, 0, __LINE__, DRAWSTATE_DISABLED_LOGIC_OP, "DS",
                                "Invalid Pipeline CreateInfo: If logic operations feature not enabled, logicOpEnable must be VK_FALSE");
        }
        if ((pPipeline->graphicsPipelineCI.pColorBlendState->logicOpEnable == VK_TRUE) &&
            ((pPipeline->graphicsPipelineCI.pColorBlendState->logicOp < VK_LOGIC_OP_CLEAR) ||
            (pPipeline->graphicsPipelineCI.pColorBlendState->logicOp > VK_LOGIC_OP_SET))) {
            skipCall |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                (VkDebugReportObjectTypeEXT) 0, 0, __LINE__, DRAWSTATE_INVALID_LOGIC_OP, "DS",
                                "Invalid Pipeline CreateInfo: If logicOpEnable is VK_TRUE, logicOp must be a valid VkLogicOp value");
        }
    }

    if (!validate_pipeline_shaders(my_data, device, pPipeline)) {
        skipCall = VK_TRUE;
    }
    // VS is required
    if (!(pPipeline->active_shaders & VK_SHADER_STAGE_VERTEX_BIT)) {
        skipCall |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT) 0, 0, __LINE__, DRAWSTATE_INVALID_PIPELINE_CREATE_STATE, "DS",
                "Invalid Pipeline CreateInfo State: Vtx Shader required");
    }
    // Either both or neither TC/TE shaders should be defined
    if (((pPipeline->active_shaders & VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT) == 0) !=
         ((pPipeline->active_shaders & VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT) == 0) ) {
        skipCall |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT) 0, 0, __LINE__, DRAWSTATE_INVALID_PIPELINE_CREATE_STATE, "DS",
                "Invalid Pipeline CreateInfo State: TE and TC shaders must be included or excluded as a pair");
    }
    // Compute shaders should be specified independent of Gfx shaders
    if ((pPipeline->active_shaders & VK_SHADER_STAGE_COMPUTE_BIT) &&
        (pPipeline->active_shaders & (VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT |
                                     VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT | VK_SHADER_STAGE_GEOMETRY_BIT |
                                     VK_SHADER_STAGE_FRAGMENT_BIT))) {
        skipCall |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT) 0, 0, __LINE__, DRAWSTATE_INVALID_PIPELINE_CREATE_STATE, "DS",
                "Invalid Pipeline CreateInfo State: Do not specify Compute Shader for Gfx Pipeline");
    }
    // VK_PRIMITIVE_TOPOLOGY_PATCH_LIST primitive topology is only valid for tessellation pipelines.
    // Mismatching primitive topology and tessellation fails graphics pipeline creation.
    if (pPipeline->active_shaders & (VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT) &&
        (pPipeline->iaStateCI.topology != VK_PRIMITIVE_TOPOLOGY_PATCH_LIST)) {
        skipCall |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT) 0, 0, __LINE__, DRAWSTATE_INVALID_PIPELINE_CREATE_STATE, "DS",
                "Invalid Pipeline CreateInfo State: VK_PRIMITIVE_TOPOLOGY_PATCH_LIST must be set as IA topology for tessellation pipelines");
    }
    if (pPipeline->iaStateCI.topology == VK_PRIMITIVE_TOPOLOGY_PATCH_LIST) {
        if (~pPipeline->active_shaders & VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT) {
            skipCall |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT) 0, 0, __LINE__, DRAWSTATE_INVALID_PIPELINE_CREATE_STATE, "DS",
                "Invalid Pipeline CreateInfo State: VK_PRIMITIVE_TOPOLOGY_PATCH_LIST primitive topology is only valid for tessellation pipelines");
        }
        if (!pPipeline->tessStateCI.patchControlPoints || (pPipeline->tessStateCI.patchControlPoints > 32)) {
            skipCall |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT) 0, 0, __LINE__, DRAWSTATE_INVALID_PIPELINE_CREATE_STATE, "DS",
                    "Invalid Pipeline CreateInfo State: VK_PRIMITIVE_TOPOLOGY_PATCH_LIST primitive topology used with patchControlPoints value %u."
                    " patchControlPoints should be >0 and <=32.", pPipeline->tessStateCI.patchControlPoints);
        }
    }
    // Viewport state must be included and viewport and scissor counts should always match
    // NOTE : Even if these are flagged as dynamic, counts need to be set correctly for shader compiler
    if (!pPipeline->graphicsPipelineCI.pViewportState) {
        skipCall |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT) 0, 0, __LINE__, DRAWSTATE_VIEWPORT_SCISSOR_MISMATCH, "DS",
            "Gfx Pipeline pViewportState is null. Even if viewport and scissors are dynamic PSO must include viewportCount and scissorCount in pViewportState.");
    } else if (pPipeline->graphicsPipelineCI.pViewportState->scissorCount != pPipeline->graphicsPipelineCI.pViewportState->viewportCount) {
        skipCall |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT) 0, 0, __LINE__, DRAWSTATE_VIEWPORT_SCISSOR_MISMATCH, "DS",
                "Gfx Pipeline viewport count (%u) must match scissor count (%u).", pPipeline->vpStateCI.viewportCount, pPipeline->vpStateCI.scissorCount);
    } else {
        // If viewport or scissor are not dynamic, then verify that data is appropriate for count
        VkBool32 dynViewport = isDynamic(pPipeline, VK_DYNAMIC_STATE_VIEWPORT);
        VkBool32 dynScissor = isDynamic(pPipeline, VK_DYNAMIC_STATE_SCISSOR);
        if (!dynViewport) {
            if (pPipeline->graphicsPipelineCI.pViewportState->viewportCount && !pPipeline->graphicsPipelineCI.pViewportState->pViewports) {
                skipCall |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT) 0, 0, __LINE__, DRAWSTATE_VIEWPORT_SCISSOR_MISMATCH, "DS",
                    "Gfx Pipeline viewportCount is %u, but pViewports is NULL. For non-zero viewportCount, you must either include pViewports data, or include viewport in pDynamicState and set it with vkCmdSetViewport().", pPipeline->graphicsPipelineCI.pViewportState->viewportCount);
            }
        }
        if (!dynScissor) {
            if (pPipeline->graphicsPipelineCI.pViewportState->scissorCount && !pPipeline->graphicsPipelineCI.pViewportState->pScissors) {
                skipCall |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT) 0, 0, __LINE__, DRAWSTATE_VIEWPORT_SCISSOR_MISMATCH, "DS",
                    "Gfx Pipeline scissorCount is %u, but pScissors is NULL. For non-zero scissorCount, you must either include pScissors data, or include scissor in pDynamicState and set it with vkCmdSetScissor().", pPipeline->graphicsPipelineCI.pViewportState->scissorCount);
            }
        }
    }
    return skipCall;
}

// Init the pipeline mapping info based on pipeline create info LL tree
//  Threading note : Calls to this function should wrapped in mutex
// TODO : this should really just be in the constructor for PIPELINE_NODE
static PIPELINE_NODE* initGraphicsPipeline(layer_data* dev_data, const VkGraphicsPipelineCreateInfo* pCreateInfo, PIPELINE_NODE* pBasePipeline)
{
    PIPELINE_NODE* pPipeline = new PIPELINE_NODE;

    if (pBasePipeline) {
        *pPipeline = *pBasePipeline;
    }

    // First init create info
    memcpy(&pPipeline->graphicsPipelineCI, pCreateInfo, sizeof(VkGraphicsPipelineCreateInfo));

    size_t bufferSize = 0;
    const VkPipelineVertexInputStateCreateInfo* pVICI = NULL;
    const VkPipelineColorBlendStateCreateInfo*  pCBCI = NULL;

    for (uint32_t i = 0; i < pCreateInfo->stageCount; i++) {
        const VkPipelineShaderStageCreateInfo *pPSSCI = &pCreateInfo->pStages[i];

        switch (pPSSCI->stage) {
            case VK_SHADER_STAGE_VERTEX_BIT:
                memcpy(&pPipeline->vsCI, pPSSCI, sizeof(VkPipelineShaderStageCreateInfo));
                pPipeline->active_shaders |= VK_SHADER_STAGE_VERTEX_BIT;
                break;
            case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
                memcpy(&pPipeline->tcsCI, pPSSCI, sizeof(VkPipelineShaderStageCreateInfo));
                pPipeline->active_shaders |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
                break;
            case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
                memcpy(&pPipeline->tesCI, pPSSCI, sizeof(VkPipelineShaderStageCreateInfo));
                pPipeline->active_shaders |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
                break;
            case VK_SHADER_STAGE_GEOMETRY_BIT:
                memcpy(&pPipeline->gsCI, pPSSCI, sizeof(VkPipelineShaderStageCreateInfo));
                pPipeline->active_shaders |= VK_SHADER_STAGE_GEOMETRY_BIT;
                break;
            case VK_SHADER_STAGE_FRAGMENT_BIT:
                memcpy(&pPipeline->fsCI, pPSSCI, sizeof(VkPipelineShaderStageCreateInfo));
                pPipeline->active_shaders |= VK_SHADER_STAGE_FRAGMENT_BIT;
                break;
            case VK_SHADER_STAGE_COMPUTE_BIT:
                // TODO : Flag error, CS is specified through VkComputePipelineCreateInfo
                pPipeline->active_shaders |= VK_SHADER_STAGE_COMPUTE_BIT;
                break;
            default:
                // TODO : Flag error
                break;
        }
    }
    // Copy over GraphicsPipelineCreateInfo structure embedded pointers
    if (pCreateInfo->stageCount != 0) {
        pPipeline->graphicsPipelineCI.pStages = new VkPipelineShaderStageCreateInfo[pCreateInfo->stageCount];
        bufferSize =  pCreateInfo->stageCount * sizeof(VkPipelineShaderStageCreateInfo);
        memcpy((void*)pPipeline->graphicsPipelineCI.pStages, pCreateInfo->pStages, bufferSize);
    }
    if (pCreateInfo->pVertexInputState != NULL) {
        memcpy((void*)&pPipeline->vertexInputCI, pCreateInfo->pVertexInputState , sizeof(VkPipelineVertexInputStateCreateInfo));
        // Copy embedded ptrs
        pVICI = pCreateInfo->pVertexInputState;
        pPipeline->vtxBindingCount = pVICI->vertexBindingDescriptionCount;
        if (pPipeline->vtxBindingCount) {
            pPipeline->pVertexBindingDescriptions = new VkVertexInputBindingDescription[pPipeline->vtxBindingCount];
            bufferSize = pPipeline->vtxBindingCount * sizeof(VkVertexInputBindingDescription);
            memcpy((void*)pPipeline->pVertexBindingDescriptions, pVICI->pVertexBindingDescriptions, bufferSize);
        }
        pPipeline->vtxAttributeCount = pVICI->vertexAttributeDescriptionCount;
        if (pPipeline->vtxAttributeCount) {
            pPipeline->pVertexAttributeDescriptions = new VkVertexInputAttributeDescription[pPipeline->vtxAttributeCount];
            bufferSize = pPipeline->vtxAttributeCount * sizeof(VkVertexInputAttributeDescription);
            memcpy((void*)pPipeline->pVertexAttributeDescriptions, pVICI->pVertexAttributeDescriptions, bufferSize);
        }
        pPipeline->graphicsPipelineCI.pVertexInputState = &pPipeline->vertexInputCI;
    }
    if (pCreateInfo->pInputAssemblyState != NULL) {
        memcpy((void*)&pPipeline->iaStateCI, pCreateInfo->pInputAssemblyState, sizeof(VkPipelineInputAssemblyStateCreateInfo));
        pPipeline->graphicsPipelineCI.pInputAssemblyState = &pPipeline->iaStateCI;
    }
    if (pCreateInfo->pTessellationState != NULL) {
        memcpy((void*)&pPipeline->tessStateCI, pCreateInfo->pTessellationState, sizeof(VkPipelineTessellationStateCreateInfo));
        pPipeline->graphicsPipelineCI.pTessellationState = &pPipeline->tessStateCI;
    }
    if (pCreateInfo->pViewportState != NULL) {
        memcpy((void*)&pPipeline->vpStateCI, pCreateInfo->pViewportState, sizeof(VkPipelineViewportStateCreateInfo));
        pPipeline->graphicsPipelineCI.pViewportState = &pPipeline->vpStateCI;
    }
    if (pCreateInfo->pRasterizationState != NULL) {
        memcpy((void*)&pPipeline->rsStateCI, pCreateInfo->pRasterizationState, sizeof(VkPipelineRasterizationStateCreateInfo));
        pPipeline->graphicsPipelineCI.pRasterizationState = &pPipeline->rsStateCI;
    }
    if (pCreateInfo->pMultisampleState != NULL) {
        memcpy((void*)&pPipeline->msStateCI, pCreateInfo->pMultisampleState, sizeof(VkPipelineMultisampleStateCreateInfo));
        pPipeline->graphicsPipelineCI.pMultisampleState = &pPipeline->msStateCI;
    }
    if (pCreateInfo->pDepthStencilState != NULL) {
        memcpy((void*)&pPipeline->dsStateCI, pCreateInfo->pDepthStencilState, sizeof(VkPipelineDepthStencilStateCreateInfo));
        pPipeline->graphicsPipelineCI.pDepthStencilState = &pPipeline->dsStateCI;
    }
    if (pCreateInfo->pColorBlendState != NULL) {
        memcpy((void*)&pPipeline->cbStateCI, pCreateInfo->pColorBlendState, sizeof(VkPipelineColorBlendStateCreateInfo));
        // Copy embedded ptrs
        pCBCI = pCreateInfo->pColorBlendState;
        pPipeline->attachmentCount = pCBCI->attachmentCount;
        if (pPipeline->attachmentCount) {
            pPipeline->pAttachments = new VkPipelineColorBlendAttachmentState[pPipeline->attachmentCount];
            bufferSize = pPipeline->attachmentCount * sizeof(VkPipelineColorBlendAttachmentState);
            memcpy((void*)pPipeline->pAttachments, pCBCI->pAttachments, bufferSize);
        }
        pPipeline->graphicsPipelineCI.pColorBlendState = &pPipeline->cbStateCI;
    }
    if (pCreateInfo->pDynamicState != NULL) {
        memcpy((void*)&pPipeline->dynStateCI, pCreateInfo->pDynamicState, sizeof(VkPipelineDynamicStateCreateInfo));
        if (pPipeline->dynStateCI.dynamicStateCount) {
            pPipeline->dynStateCI.pDynamicStates = new VkDynamicState[pPipeline->dynStateCI.dynamicStateCount];
            bufferSize = pPipeline->dynStateCI.dynamicStateCount * sizeof(VkDynamicState);
            memcpy((void*)pPipeline->dynStateCI.pDynamicStates, pCreateInfo->pDynamicState->pDynamicStates, bufferSize);
        }
        pPipeline->graphicsPipelineCI.pDynamicState = &pPipeline->dynStateCI;
    }
    pPipeline->active_sets.clear();
    return pPipeline;
}

// Free the Pipeline nodes
static void deletePipelines(layer_data* my_data)
{
    if (my_data->pipelineMap.size() <= 0)
        return;
    for (auto ii=my_data->pipelineMap.begin(); ii!=my_data->pipelineMap.end(); ++ii) {
        if ((*ii).second->graphicsPipelineCI.stageCount != 0) {
            delete[] (*ii).second->graphicsPipelineCI.pStages;
        }
        if ((*ii).second->pVertexBindingDescriptions) {
            delete[] (*ii).second->pVertexBindingDescriptions;
        }
        if ((*ii).second->pVertexAttributeDescriptions) {
            delete[] (*ii).second->pVertexAttributeDescriptions;
        }
        if ((*ii).second->pAttachments) {
            delete[] (*ii).second->pAttachments;
        }
        if ((*ii).second->dynStateCI.dynamicStateCount != 0) {
            delete[] (*ii).second->dynStateCI.pDynamicStates;
        }
        delete (*ii).second;
    }
    my_data->pipelineMap.clear();
}

// For given pipeline, return number of MSAA samples, or one if MSAA disabled
static VkSampleCountFlagBits getNumSamples(layer_data* my_data, const VkPipeline pipeline)
{
    PIPELINE_NODE* pPipe = my_data->pipelineMap[pipeline];
    if (VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO == pPipe->msStateCI.sType) {
        return pPipe->msStateCI.rasterizationSamples;
    }
    return VK_SAMPLE_COUNT_1_BIT;
}

// Validate state related to the PSO
static VkBool32 validatePipelineState(layer_data* my_data, const GLOBAL_CB_NODE* pCB, const VkPipelineBindPoint pipelineBindPoint, const VkPipeline pipeline)
{
    if (VK_PIPELINE_BIND_POINT_GRAPHICS == pipelineBindPoint) {
        // Verify that any MSAA request in PSO matches sample# in bound FB
        VkSampleCountFlagBits psoNumSamples = getNumSamples(my_data, pipeline);
        if (pCB->activeRenderPass) {
            const VkRenderPassCreateInfo* pRPCI = my_data->renderPassMap[pCB->activeRenderPass]->pCreateInfo;
            const VkSubpassDescription* pSD = &pRPCI->pSubpasses[pCB->activeSubpass];
            VkSampleCountFlagBits subpassNumSamples = (VkSampleCountFlagBits) 0;
            uint32_t i;

            for (i = 0; i < pSD->colorAttachmentCount; i++) {
                VkSampleCountFlagBits samples;

                if (pSD->pColorAttachments[i].attachment == VK_ATTACHMENT_UNUSED)
                    continue;

                samples = pRPCI->pAttachments[pSD->pColorAttachments[i].attachment].samples;
                if (subpassNumSamples == (VkSampleCountFlagBits) 0) {
                    subpassNumSamples = samples;
                } else if (subpassNumSamples != samples) {
                    subpassNumSamples = (VkSampleCountFlagBits) -1;
                    break;
                }
            }
            if (pSD->pDepthStencilAttachment && pSD->pDepthStencilAttachment->attachment != VK_ATTACHMENT_UNUSED) {
                const VkSampleCountFlagBits samples = pRPCI->pAttachments[pSD->pDepthStencilAttachment->attachment].samples;
                if (subpassNumSamples == (VkSampleCountFlagBits) 0)
                    subpassNumSamples = samples;
                else if (subpassNumSamples != samples)
                    subpassNumSamples = (VkSampleCountFlagBits) -1;
            }

            if (psoNumSamples != subpassNumSamples) {
                return log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT, (uint64_t) pipeline, __LINE__, DRAWSTATE_NUM_SAMPLES_MISMATCH, "DS",
                        "Num samples mismatch! Binding PSO (%#" PRIxLEAST64 ") with %u samples while current RenderPass (%#" PRIxLEAST64 ") w/ %u samples!",
                        (uint64_t) pipeline, psoNumSamples, (uint64_t) pCB->activeRenderPass, subpassNumSamples);
            }
        } else {
            // TODO : I believe it's an error if we reach this point and don't have an activeRenderPass
            //   Verify and flag error as appropriate
        }
        // TODO : Add more checks here
    } else {
        // TODO : Validate non-gfx pipeline updates
    }
    return VK_FALSE;
}

// Block of code at start here specifically for managing/tracking DSs

// Return Pool node ptr for specified pool or else NULL
static DESCRIPTOR_POOL_NODE* getPoolNode(layer_data* my_data, const VkDescriptorPool pool)
{
    if (my_data->descriptorPoolMap.find(pool) == my_data->descriptorPoolMap.end()) {
        return NULL;
    }
    return my_data->descriptorPoolMap[pool];
}

static LAYOUT_NODE* getLayoutNode(layer_data* my_data, const VkDescriptorSetLayout layout) {
    if (my_data->descriptorSetLayoutMap.find(layout) == my_data->descriptorSetLayoutMap.end()) {
        return NULL;
    }
    return my_data->descriptorSetLayoutMap[layout];
}

// Return VK_FALSE if update struct is of valid type, otherwise flag error and return code from callback
static VkBool32 validUpdateStruct(layer_data* my_data, const VkDevice device, const GENERIC_HEADER* pUpdateStruct)
{
    switch (pUpdateStruct->sType)
    {
        case VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET:
        case VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET:
            return VK_FALSE;
        default:
            return log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT) 0, 0, __LINE__, DRAWSTATE_INVALID_UPDATE_STRUCT, "DS",
                    "Unexpected UPDATE struct of type %s (value %u) in vkUpdateDescriptors() struct tree", string_VkStructureType(pUpdateStruct->sType), pUpdateStruct->sType);
    }
}

// Set count for given update struct in the last parameter
// Return value of skipCall, which is only VK_TRUE if error occurs and callback signals execution to cease
static uint32_t getUpdateCount(layer_data* my_data, const VkDevice device, const GENERIC_HEADER* pUpdateStruct)
{
    switch (pUpdateStruct->sType)
    {
        case VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET:
            return ((VkWriteDescriptorSet*)pUpdateStruct)->descriptorCount;
        case VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET:
            // TODO : Need to understand this case better and make sure code is correct
            return ((VkCopyDescriptorSet*)pUpdateStruct)->descriptorCount;
        default:
            return 0;
    }
    return 0;
}

// For given Layout Node and binding, return index where that binding begins
static uint32_t getBindingStartIndex(const LAYOUT_NODE* pLayout, const uint32_t binding)
{
    uint32_t offsetIndex = 0;
    for (uint32_t i = 0; i < pLayout->createInfo.bindingCount; i++) {
        if (pLayout->createInfo.pBindings[i].binding == binding)
            break;
        offsetIndex += pLayout->createInfo.pBindings[i].descriptorCount;
    }
    return offsetIndex;
}

// For given layout node and binding, return last index that is updated
static uint32_t getBindingEndIndex(const LAYOUT_NODE* pLayout, const uint32_t binding)
{
    uint32_t offsetIndex = 0;
    for (uint32_t i = 0; i <  pLayout->createInfo.bindingCount; i++) {
        offsetIndex += pLayout->createInfo.pBindings[i].descriptorCount;
        if (pLayout->createInfo.pBindings[i].binding == binding)
            break;
    }
    return offsetIndex-1;
}

// For given layout and update, return the first overall index of the layout that is updated
static uint32_t getUpdateStartIndex(layer_data* my_data, const VkDevice device, const LAYOUT_NODE* pLayout, const uint32_t binding, const uint32_t arrayIndex, const GENERIC_HEADER* pUpdateStruct)
{
    return getBindingStartIndex(pLayout, binding)+arrayIndex;
}

// For given layout and update, return the last overall index of the layout that is updated
static uint32_t getUpdateEndIndex(layer_data* my_data, const VkDevice device, const LAYOUT_NODE* pLayout, const uint32_t binding, const uint32_t arrayIndex, const GENERIC_HEADER* pUpdateStruct)
{
    uint32_t count = getUpdateCount(my_data, device, pUpdateStruct);
    return getBindingStartIndex(pLayout, binding)+arrayIndex+count-1;
}

// Verify that the descriptor type in the update struct matches what's expected by the layout
static VkBool32 validateUpdateConsistency(layer_data* my_data, const VkDevice device, const LAYOUT_NODE* pLayout, const GENERIC_HEADER* pUpdateStruct, uint32_t startIndex, uint32_t endIndex)
{
    // First get actual type of update
    VkBool32 skipCall = VK_FALSE;
    VkDescriptorType actualType;
    uint32_t i = 0;
    switch (pUpdateStruct->sType)
    {
        case VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET:
            actualType = ((VkWriteDescriptorSet*)pUpdateStruct)->descriptorType;
            break;
        case VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET:
            /* no need to validate */
            return VK_FALSE;
            break;
        default:
            skipCall |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT) 0, 0, __LINE__, DRAWSTATE_INVALID_UPDATE_STRUCT, "DS",
                    "Unexpected UPDATE struct of type %s (value %u) in vkUpdateDescriptors() struct tree", string_VkStructureType(pUpdateStruct->sType), pUpdateStruct->sType);
    }
    if (VK_FALSE == skipCall) {
        // Set first stageFlags as reference and verify that all other updates match it
        VkShaderStageFlags refStageFlags = pLayout->stageFlags[startIndex];
        for (i = startIndex; i <= endIndex; i++) {
            if (pLayout->descriptorTypes[i] != actualType) {
                skipCall |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT) 0, 0, __LINE__, DRAWSTATE_DESCRIPTOR_TYPE_MISMATCH, "DS",
                    "Write descriptor update has descriptor type %s that does not match overlapping binding descriptor type of %s!",
                    string_VkDescriptorType(actualType), string_VkDescriptorType(pLayout->descriptorTypes[i]));
            }
            if (pLayout->stageFlags[i] != refStageFlags) {
                skipCall |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT) 0, 0, __LINE__, DRAWSTATE_DESCRIPTOR_STAGEFLAGS_MISMATCH, "DS",
                    "Write descriptor update has stageFlags %x that do not match overlapping binding descriptor stageFlags of %x!",
                    refStageFlags, pLayout->stageFlags[i]);
            }
        }
    }
    return skipCall;
}

// Determine the update type, allocate a new struct of that type, shadow the given pUpdate
//   struct into the pNewNode param. Return VK_TRUE if error condition encountered and callback signals early exit.
// NOTE : Calls to this function should be wrapped in mutex
static VkBool32 shadowUpdateNode(layer_data* my_data, const VkDevice device, GENERIC_HEADER* pUpdate, GENERIC_HEADER** pNewNode)
{
    VkBool32 skipCall = VK_FALSE;
    VkWriteDescriptorSet* pWDS = NULL;
    VkCopyDescriptorSet* pCDS = NULL;
    switch (pUpdate->sType)
    {
        case VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET:
            pWDS = new VkWriteDescriptorSet;
            *pNewNode = (GENERIC_HEADER*)pWDS;
            memcpy(pWDS, pUpdate, sizeof(VkWriteDescriptorSet));

            switch (pWDS->descriptorType) {
            case VK_DESCRIPTOR_TYPE_SAMPLER:
            case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
            case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
            case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
                {
                    VkDescriptorImageInfo *info = new VkDescriptorImageInfo[pWDS->descriptorCount];
                    memcpy(info, pWDS->pImageInfo, pWDS->descriptorCount * sizeof(VkDescriptorImageInfo));
                    pWDS->pImageInfo = info;
                }
                break;
            case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
            case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
                {
                    VkBufferView *info = new VkBufferView[pWDS->descriptorCount];
                    memcpy(info, pWDS->pTexelBufferView, pWDS->descriptorCount * sizeof(VkBufferView));
                    pWDS->pTexelBufferView = info;
                }
                break;
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
            case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
            case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
                {
                    VkDescriptorBufferInfo *info = new VkDescriptorBufferInfo[pWDS->descriptorCount];
                    memcpy(info, pWDS->pBufferInfo, pWDS->descriptorCount * sizeof(VkDescriptorBufferInfo));
                    pWDS->pBufferInfo = info;
                }
                break;
            default:
                return VK_ERROR_VALIDATION_FAILED_EXT;
                break;
            }
            break;
        case VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET:
            pCDS = new VkCopyDescriptorSet;
            *pNewNode = (GENERIC_HEADER*)pCDS;
            memcpy(pCDS, pUpdate, sizeof(VkCopyDescriptorSet));
            break;
        default:
            if (log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT) 0, 0, __LINE__, DRAWSTATE_INVALID_UPDATE_STRUCT, "DS",
                    "Unexpected UPDATE struct of type %s (value %u) in vkUpdateDescriptors() struct tree", string_VkStructureType(pUpdate->sType), pUpdate->sType))
                return VK_TRUE;
    }
    // Make sure that pNext for the end of shadow copy is NULL
    (*pNewNode)->pNext = NULL;
    return skipCall;
}

// Verify that given sampler is valid
static VkBool32 validateSampler(const layer_data* my_data, const VkSampler* pSampler, const VkBool32 immutable)
{
    VkBool32 skipCall = VK_FALSE;
    auto sampIt = my_data->sampleMap.find(*pSampler);
    if (sampIt == my_data->sampleMap.end()) {
        if (!immutable) {
            skipCall |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT, (uint64_t) *pSampler, __LINE__, DRAWSTATE_SAMPLER_DESCRIPTOR_ERROR, "DS",
                "vkUpdateDescriptorSets: Attempt to update descriptor with invalid sampler %#" PRIxLEAST64, (uint64_t) *pSampler);
        } else { // immutable
            skipCall |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT, (uint64_t) *pSampler, __LINE__, DRAWSTATE_SAMPLER_DESCRIPTOR_ERROR, "DS",
                "vkUpdateDescriptorSets: Attempt to update descriptor whose binding has an invalid immutable sampler %#" PRIxLEAST64, (uint64_t) *pSampler);
        }
    } else {
        // TODO : Any further checks we want to do on the sampler?
    }
    return skipCall;
}

// Set the layout on the global level
void SetLayout(layer_data *my_data, ImageSubresourcePair imgpair,
               const VkImageLayout &layout) {
    VkImage &image = imgpair.image;
    // TODO (mlentine): Maybe set format if new? Not used atm.
    my_data->imageLayoutMap[imgpair].layout = layout;
    // TODO (mlentine): Maybe make vector a set?
    auto subresource =
        std::find(my_data->imageSubresourceMap[image].begin(),
                  my_data->imageSubresourceMap[image].end(), imgpair);
    if (subresource == my_data->imageSubresourceMap[image].end()) {
        my_data->imageSubresourceMap[image].push_back(imgpair);
    }
}

void SetLayout(layer_data *my_data, VkImage image,
               const VkImageLayout &layout) {
    ImageSubresourcePair imgpair = {image, false, VkImageSubresource()};
    SetLayout(my_data, imgpair, layout);
}

void SetLayout(layer_data *my_data, VkImage image, VkImageSubresource range,
               const VkImageLayout &layout) {
    ImageSubresourcePair imgpair = {image, true, range};
    SetLayout(my_data, imgpair, layout);
}

// Set the layout on the cmdbuf level
void SetLayout(GLOBAL_CB_NODE *pCB, VkImage image, ImageSubresourcePair imgpair,
               const IMAGE_CMD_BUF_NODE &node) {
    pCB->imageLayoutMap[imgpair] = node;
    // TODO (mlentine): Maybe make vector a set?
    auto subresource =
        std::find(pCB->imageSubresourceMap[image].begin(),
                  pCB->imageSubresourceMap[image].end(), imgpair);
    if (subresource == pCB->imageSubresourceMap[image].end()) {
        pCB->imageSubresourceMap[image].push_back(imgpair);
    }
}

void SetLayout(GLOBAL_CB_NODE *pCB, VkImage image, ImageSubresourcePair imgpair,
               const VkImageLayout &layout) {
    pCB->imageLayoutMap[imgpair].layout = layout;
    // TODO (mlentine): Maybe make vector a set?
    assert(std::find(pCB->imageSubresourceMap[image].begin(),
                     pCB->imageSubresourceMap[image].end(),
                     imgpair) != pCB->imageSubresourceMap[image].end());
}

void SetLayout(GLOBAL_CB_NODE *pCB, VkImage image,
               const IMAGE_CMD_BUF_NODE &node) {
    ImageSubresourcePair imgpair = {image, false, VkImageSubresource()};
    SetLayout(pCB, image, imgpair, node);
}

void SetLayout(GLOBAL_CB_NODE *pCB, VkImage image, VkImageSubresource range,
               const IMAGE_CMD_BUF_NODE &node) {
    ImageSubresourcePair imgpair = {image, true, range};
    SetLayout(pCB, image, imgpair, node);
}

void SetLayout(GLOBAL_CB_NODE *pCB, VkImage image,
               const VkImageLayout &layout) {
    ImageSubresourcePair imgpair = {image, false, VkImageSubresource()};
    SetLayout(pCB, image, imgpair, layout);
}

void SetLayout(GLOBAL_CB_NODE *pCB, VkImage image, VkImageSubresource range,
               const VkImageLayout &layout) {
    ImageSubresourcePair imgpair = {image, true, range};
    SetLayout(pCB, image, imgpair, layout);
}

void SetLayout(const layer_data *dev_data, GLOBAL_CB_NODE *pCB,
               VkImageView imageView, const VkImageLayout &layout) {
    auto image_view_data = dev_data->imageViewMap.find(imageView);
    assert(image_view_data != dev_data->imageViewMap.end());
    const VkImage &image = image_view_data->second->image;
    const VkImageSubresourceRange &subRange =
        image_view_data->second->subresourceRange;
    // TODO: Do not iterate over every possibility - consolidate where possible
    for (uint32_t j = 0; j < subRange.levelCount; j++) {
        uint32_t level = subRange.baseMipLevel + j;
        for (uint32_t k = 0; k < subRange.layerCount; k++) {
            uint32_t layer = subRange.baseArrayLayer + k;
            VkImageSubresource sub = {subRange.aspectMask, level, layer};
            SetLayout(pCB, image, sub, layout);
        }
    }
}

// find layout(s) on the cmd buf level
bool FindLayout(const GLOBAL_CB_NODE *pCB, VkImage image,
                VkImageSubresource range, IMAGE_CMD_BUF_NODE &node) {
    ImageSubresourcePair imgpair = {image, true, range};
    auto imgsubIt = pCB->imageLayoutMap.find(imgpair);
    if (imgsubIt == pCB->imageLayoutMap.end()) {
        imgpair = {image, false, VkImageSubresource()};
        imgsubIt = pCB->imageLayoutMap.find(imgpair);
        if (imgsubIt == pCB->imageLayoutMap.end())
            return false;
    }
    node = imgsubIt->second;
    return true;
}

// find layout(s) on the global level
bool FindLayout(const layer_data *my_data, ImageSubresourcePair imgpair,
                VkImageLayout &layout) {
    auto imgsubIt = my_data->imageLayoutMap.find(imgpair);
    if (imgsubIt == my_data->imageLayoutMap.end()) {
        imgpair = {imgpair.image, false, VkImageSubresource()};
        imgsubIt = my_data->imageLayoutMap.find(imgpair);
        if(imgsubIt == my_data->imageLayoutMap.end()) return false;
    }
    layout = imgsubIt->second.layout;
    return true;
}

bool FindLayout(const layer_data *my_data, VkImage image,
                VkImageSubresource range, VkImageLayout &layout) {
    ImageSubresourcePair imgpair = {image, true, range};
    return FindLayout(my_data, imgpair, layout);
}

bool FindLayouts(const layer_data *my_data, VkImage image,
                 std::vector<VkImageLayout> &layouts) {
    auto sub_data = my_data->imageSubresourceMap.find(image);
    if (sub_data == my_data->imageSubresourceMap.end())
        return false;
    auto imgIt = my_data->imageMap.find(image);
    if (imgIt == my_data->imageMap.end())
        return false;
    bool ignoreGlobal = false;
    // TODO: Make this robust for >1 aspect mask. Now it will just say ignore
    // potential errors in this case.
    if (sub_data->second.size() >=
        (imgIt->second->arrayLayers * imgIt->second->mipLevels + 1)) {
        ignoreGlobal = true;
    }
    for (auto imgsubpair : sub_data->second) {
        if (ignoreGlobal && !imgsubpair.hasSubresource)
            continue;
        auto img_data = my_data->imageLayoutMap.find(imgsubpair);
        if (img_data != my_data->imageLayoutMap.end()) {
            layouts.push_back(img_data->second.layout);
        }
    }
    return true;
}

// Verify that given imageView is valid
static VkBool32 validateImageView(const layer_data* my_data, const VkImageView* pImageView, const VkImageLayout imageLayout)
{
    VkBool32 skipCall = VK_FALSE;
    auto ivIt = my_data->imageViewMap.find(*pImageView);
    if (ivIt == my_data->imageViewMap.end()) {
        skipCall |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT, (uint64_t) *pImageView, __LINE__, DRAWSTATE_IMAGEVIEW_DESCRIPTOR_ERROR, "DS",
                "vkUpdateDescriptorSets: Attempt to update descriptor with invalid imageView %#" PRIxLEAST64, (uint64_t) *pImageView);
    } else {
        // Validate that imageLayout is compatible with aspectMask and image format
        VkImageAspectFlags aspectMask = ivIt->second->subresourceRange.aspectMask;
        VkImage image = ivIt->second->image;
        // TODO : Check here in case we have a bad image
        auto imgIt = my_data->imageMap.find(image);
        if (imgIt == my_data->imageMap.end()) {
            skipCall |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, (uint64_t) image, __LINE__, DRAWSTATE_IMAGEVIEW_DESCRIPTOR_ERROR, "DS",
                "vkUpdateDescriptorSets: Attempt to update descriptor with invalid image %#" PRIxLEAST64 " in imageView %#" PRIxLEAST64, (uint64_t) image, (uint64_t) *pImageView);
        } else {
            VkFormat format = (*imgIt).second->format;
            VkBool32 ds = vk_format_is_depth_or_stencil(format);
            switch (imageLayout) {
                case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                    // Only Color bit must be set
                    if ((aspectMask & VK_IMAGE_ASPECT_COLOR_BIT) != VK_IMAGE_ASPECT_COLOR_BIT) {
                        skipCall |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT, (uint64_t) *pImageView, __LINE__,
                            DRAWSTATE_INVALID_IMAGE_ASPECT, "DS", "vkUpdateDescriptorSets: Updating descriptor with layout VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL and imageView %#" PRIxLEAST64 ""
                                    " that does not have VK_IMAGE_ASPECT_COLOR_BIT set.", (uint64_t) *pImageView);
                    }
                    // format must NOT be DS
                    if (ds) {
                        skipCall |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT, (uint64_t) *pImageView, __LINE__,
                            DRAWSTATE_IMAGEVIEW_DESCRIPTOR_ERROR, "DS", "vkUpdateDescriptorSets: Updating descriptor with layout VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL and imageView %#" PRIxLEAST64 ""
                                    " but the image format is %s which is not a color format.", (uint64_t) *pImageView, string_VkFormat(format));
                    }
                    break;
                case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
                    // Depth or stencil bit must be set, but both must NOT be set
                    if (aspectMask & VK_IMAGE_ASPECT_DEPTH_BIT) {
                        if (aspectMask & VK_IMAGE_ASPECT_STENCIL_BIT) {
                            // both  must NOT be set
                            skipCall |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT, (uint64_t) *pImageView, __LINE__,
                            DRAWSTATE_INVALID_IMAGE_ASPECT, "DS", "vkUpdateDescriptorSets: Updating descriptor with imageView %#" PRIxLEAST64 ""
                                    " that has both STENCIL and DEPTH aspects set", (uint64_t) *pImageView);
                        }
                    } else if (!(aspectMask & VK_IMAGE_ASPECT_STENCIL_BIT)) {
                        // Neither were set
                        skipCall |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT, (uint64_t) *pImageView, __LINE__,
                            DRAWSTATE_INVALID_IMAGE_ASPECT, "DS", "vkUpdateDescriptorSets: Updating descriptor with layout %s and imageView %#" PRIxLEAST64 ""
                                    " that does not have STENCIL or DEPTH aspect set.", string_VkImageLayout(imageLayout), (uint64_t) *pImageView);
                    }
                    // format must be DS
                    if (!ds) {
                        skipCall |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT, (uint64_t) *pImageView, __LINE__,
                            DRAWSTATE_IMAGEVIEW_DESCRIPTOR_ERROR, "DS", "vkUpdateDescriptorSets: Updating descriptor with layout %s and imageView %#" PRIxLEAST64 ""
                                    " but the image format is %s which is not a depth/stencil format.", string_VkImageLayout(imageLayout), (uint64_t) *pImageView, string_VkFormat(format));
                    }
                    break;
                default:
                    // anything to check for other layouts?
                    break;
            }
        }
    }
    return skipCall;
}

// Verify that given bufferView is valid
static VkBool32 validateBufferView(const layer_data* my_data, const VkBufferView* pBufferView)
{
    VkBool32 skipCall = VK_FALSE;
    auto sampIt = my_data->bufferViewMap.find(*pBufferView);
    if (sampIt == my_data->bufferViewMap.end()) {
        skipCall |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_VIEW_EXT, (uint64_t) *pBufferView, __LINE__, DRAWSTATE_BUFFERVIEW_DESCRIPTOR_ERROR, "DS",
                "vkUpdateDescriptorSets: Attempt to update descriptor with invalid bufferView %#" PRIxLEAST64, (uint64_t) *pBufferView);
    } else {
        // TODO : Any further checks we want to do on the bufferView?
    }
    return skipCall;
}

// Verify that given bufferInfo is valid
static VkBool32 validateBufferInfo(const layer_data* my_data, const VkDescriptorBufferInfo* pBufferInfo)
{
    VkBool32 skipCall = VK_FALSE;
    auto sampIt = my_data->bufferMap.find(pBufferInfo->buffer);
    if (sampIt == my_data->bufferMap.end()) {
        skipCall |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, (uint64_t) pBufferInfo->buffer, __LINE__, DRAWSTATE_BUFFERINFO_DESCRIPTOR_ERROR, "DS",
                "vkUpdateDescriptorSets: Attempt to update descriptor where bufferInfo has invalid buffer %#" PRIxLEAST64, (uint64_t) pBufferInfo->buffer);
    } else {
        // TODO : Any further checks we want to do on the bufferView?
    }
    return skipCall;
}

static VkBool32 validateUpdateContents(const layer_data* my_data, const VkWriteDescriptorSet *pWDS, const VkDescriptorSetLayoutBinding* pLayoutBinding)
{
    VkBool32 skipCall = VK_FALSE;
    // First verify that for the given Descriptor type, the correct DescriptorInfo data is supplied
    const VkSampler* pSampler = NULL;
    VkBool32 immutable = VK_FALSE;
    uint32_t i = 0;
    // For given update type, verify that update contents are correct
    switch (pWDS->descriptorType) {
        case VK_DESCRIPTOR_TYPE_SAMPLER:
            for (i=0; i<pWDS->descriptorCount; ++i) {
                skipCall |= validateSampler(my_data, &(pWDS->pImageInfo[i].sampler), immutable);
            }
            break;
        case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
            for (i=0; i<pWDS->descriptorCount; ++i) {
                if (NULL == pLayoutBinding->pImmutableSamplers) {
                    pSampler = &(pWDS->pImageInfo[i].sampler);
                    if (immutable) {
                        skipCall |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT, (uint64_t) *pSampler, __LINE__, DRAWSTATE_INCONSISTENT_IMMUTABLE_SAMPLER_UPDATE, "DS",
                            "vkUpdateDescriptorSets: Update #%u is not an immutable sampler %#" PRIxLEAST64 ", but previous update(s) from this "
                            "VkWriteDescriptorSet struct used an immutable sampler. All updates from a single struct must either "
                            "use immutable or non-immutable samplers.", i, (uint64_t) *pSampler);
                    }
                } else {
                    if (i>0 && !immutable) {
                        skipCall |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT, (uint64_t) *pSampler, __LINE__, DRAWSTATE_INCONSISTENT_IMMUTABLE_SAMPLER_UPDATE, "DS",
                            "vkUpdateDescriptorSets: Update #%u is an immutable sampler, but previous update(s) from this "
                            "VkWriteDescriptorSet struct used a non-immutable sampler. All updates from a single struct must either "
                            "use immutable or non-immutable samplers.", i);
                    }
                    immutable = VK_TRUE;
                    pSampler = &(pLayoutBinding->pImmutableSamplers[i]);
                }
                skipCall |= validateSampler(my_data, pSampler, immutable);
            }
            // Intentionally fall through here to also validate image stuff
        case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
        case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
        case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
            for (i=0; i<pWDS->descriptorCount; ++i) {
                skipCall |= validateImageView(my_data, &(pWDS->pImageInfo[i].imageView), pWDS->pImageInfo[i].imageLayout);
            }
            break;
        case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
        case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
            for (i=0; i<pWDS->descriptorCount; ++i) {
                skipCall |= validateBufferView(my_data, &(pWDS->pTexelBufferView[i]));
            }
            break;
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
            for (i=0; i<pWDS->descriptorCount; ++i) {
                skipCall |= validateBufferInfo(my_data, &(pWDS->pBufferInfo[i]));
            }
            break;
        default:
            break;
    }
    return skipCall;
}
// Validate that given set is valid and that it's not being used by an in-flight CmdBuffer
// func_str is the name of the calling function
// Return VK_FALSE if no errors occur
// Return VK_TRUE if validation error occurs and callback returns VK_TRUE (to skip upcoming API call down the chain)
VkBool32 validateIdleDescriptorSet(const layer_data* my_data, VkDescriptorSet set, std::string func_str) {
    VkBool32 skip_call = VK_FALSE;
    auto set_node = my_data->setMap.find(set);
    if (set_node == my_data->setMap.end()) {
        skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT, (uint64_t)(set), __LINE__, DRAWSTATE_DOUBLE_DESTROY, "DS",
                             "Cannot call %s() on descriptor set %" PRIxLEAST64 " that has not been allocated.", func_str.c_str(), (uint64_t)(set));
    } else {
        if (set_node->second->in_use.load()) {
            skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT, (uint64_t)(set), __LINE__, DRAWSTATE_OBJECT_INUSE, "DS",
                                 "Cannot call %s() on descriptor set %" PRIxLEAST64 " that is in use by a command buffer.", func_str.c_str(), (uint64_t)(set));
        }
    }
    return skip_call;
}
static void invalidateBoundCmdBuffers(layer_data* dev_data, const SET_NODE* pSet)
{
    // Flag any CBs this set is bound to as INVALID
    for (auto cb : pSet->boundCmdBuffers) {
        auto cb_node = dev_data->commandBufferMap.find(cb);
        if (cb_node != dev_data->commandBufferMap.end()) {
            cb_node->second->state = CB_INVALID;
        }
    }
}
// update DS mappings based on write and copy update arrays
static VkBool32 dsUpdate(layer_data* my_data, VkDevice device, uint32_t descriptorWriteCount, const VkWriteDescriptorSet* pWDS, uint32_t descriptorCopyCount, const VkCopyDescriptorSet* pCDS)
{
    VkBool32 skipCall = VK_FALSE;

    LAYOUT_NODE* pLayout = NULL;
    VkDescriptorSetLayoutCreateInfo* pLayoutCI = NULL;
    // Validate Write updates
    uint32_t i = 0;
    for (i=0; i < descriptorWriteCount; i++) {
        VkDescriptorSet ds = pWDS[i].dstSet;
        SET_NODE* pSet = my_data->setMap[ds];
        // Set being updated cannot be in-flight
        if ((skipCall = validateIdleDescriptorSet(my_data, ds, "VkUpdateDescriptorSets")) == VK_TRUE)
            return skipCall;
        // If set is bound to any cmdBuffers, mark them invalid
        invalidateBoundCmdBuffers(my_data, pSet);
        GENERIC_HEADER* pUpdate = (GENERIC_HEADER*) &pWDS[i];
        pLayout = pSet->pLayout;
        // First verify valid update struct
        if ((skipCall = validUpdateStruct(my_data, device, pUpdate)) == VK_TRUE) {
            break;
        }
        uint32_t binding = 0, endIndex = 0;
        binding = pWDS[i].dstBinding;
        auto bindingToIndex = pLayout->bindingToIndexMap.find(binding);
        // Make sure that layout being updated has the binding being updated
        if (bindingToIndex == pLayout->bindingToIndexMap.end()) {
            skipCall |= log_msg(
                my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT, (uint64_t)(ds),
                __LINE__, DRAWSTATE_INVALID_UPDATE_INDEX, "DS",
                "Descriptor Set %" PRIu64 " does not have binding to match "
                                          "update binding %u for update type "
                                          "%s!",
                (uint64_t)(ds), binding,
                string_VkStructureType(pUpdate->sType));
        } else {
            // Next verify that update falls within size of given binding
            endIndex = getUpdateEndIndex(my_data, device, pLayout, binding, pWDS[i].dstArrayElement, pUpdate);
            if (getBindingEndIndex(pLayout, binding) < endIndex) {
                pLayoutCI = &pLayout->createInfo;
                string DSstr = vk_print_vkdescriptorsetlayoutcreateinfo(pLayoutCI, "{DS}    ");
                skipCall |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT, (uint64_t)(ds), __LINE__, DRAWSTATE_DESCRIPTOR_UPDATE_OUT_OF_BOUNDS, "DS",
                        "Descriptor update type of %s is out of bounds for matching binding %u in Layout w/ CI:\n%s!", string_VkStructureType(pUpdate->sType), binding, DSstr.c_str());
            } else { // TODO : should we skip update on a type mismatch or force it?
                uint32_t startIndex;
                startIndex =
                    getUpdateStartIndex(my_data, device, pLayout, binding,
                                        pWDS[i].dstArrayElement, pUpdate);
                // Layout bindings match w/ update, now verify that update type
                // & stageFlags are the same for entire update
                if ((skipCall = validateUpdateConsistency(
                         my_data, device, pLayout, pUpdate, startIndex,
                         endIndex)) == VK_FALSE) {
                    // The update is within bounds and consistent, but need to
                    // make sure contents make sense as well
                    if ((skipCall = validateUpdateContents(
                             my_data, &pWDS[i],
                             &pLayout->createInfo.pBindings[bindingToIndex->second])) ==
                        VK_FALSE) {
                        // Update is good. Save the update info
                        // Create new update struct for this set's shadow copy
                        GENERIC_HEADER* pNewNode = NULL;
                        skipCall |= shadowUpdateNode(my_data, device, pUpdate, &pNewNode);
                        if (NULL == pNewNode) {
                            skipCall |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT, (uint64_t)(ds), __LINE__, DRAWSTATE_OUT_OF_MEMORY, "DS",
                                    "Out of memory while attempting to allocate UPDATE struct in vkUpdateDescriptors()");
                        } else {
                            // Insert shadow node into LL of updates for this set
                            pNewNode->pNext = pSet->pUpdateStructs;
                            pSet->pUpdateStructs = pNewNode;
                            // Now update appropriate descriptor(s) to point to new Update node
                            for (uint32_t j = startIndex; j <= endIndex; j++) {
                                assert(j<pSet->descriptorCount);
                                pSet->ppDescriptors[j] = pNewNode;
                            }
                        }
                    }
                }
            }
        }
    }
    // Now validate copy updates
    for (i=0; i < descriptorCopyCount; ++i) {
        SET_NODE *pSrcSet = NULL, *pDstSet = NULL;
        LAYOUT_NODE *pSrcLayout = NULL, *pDstLayout = NULL;
        uint32_t srcStartIndex = 0, srcEndIndex = 0, dstStartIndex = 0, dstEndIndex = 0;
        // For each copy make sure that update falls within given layout and that types match
        pSrcSet = my_data->setMap[pCDS[i].srcSet];
        pDstSet = my_data->setMap[pCDS[i].dstSet];
        // Set being updated cannot be in-flight
        if ((skipCall = validateIdleDescriptorSet(my_data, pDstSet->set, "VkUpdateDescriptorSets")) == VK_TRUE)
            return skipCall;
        invalidateBoundCmdBuffers(my_data, pDstSet);
        pSrcLayout = pSrcSet->pLayout;
        pDstLayout = pDstSet->pLayout;
        // Validate that src binding is valid for src set layout
        if (pSrcLayout->bindingToIndexMap.find(pCDS[i].srcBinding) ==
            pSrcLayout->bindingToIndexMap.end()) {
            skipCall |=
                log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                        VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT,
                        (uint64_t)pSrcSet->set, __LINE__,
                        DRAWSTATE_INVALID_UPDATE_INDEX,
                        "DS", "Copy descriptor update %u has srcBinding %u "
                              "which is out of bounds for underlying SetLayout "
                              "%#" PRIxLEAST64 " which only has bindings 0-%u.",
                        i, pCDS[i].srcBinding, (uint64_t)pSrcLayout->layout,
                        pSrcLayout->createInfo.bindingCount - 1);
        } else if (pDstLayout->bindingToIndexMap.find(pCDS[i].dstBinding) ==
                   pDstLayout->bindingToIndexMap.end()) {
            skipCall |=
                log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                        VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT,
                        (uint64_t)pDstSet->set, __LINE__,
                        DRAWSTATE_INVALID_UPDATE_INDEX,
                        "DS", "Copy descriptor update %u has dstBinding %u "
                              "which is out of bounds for underlying SetLayout "
                              "%#" PRIxLEAST64 " which only has bindings 0-%u.",
                        i, pCDS[i].dstBinding, (uint64_t)pDstLayout->layout,
                        pDstLayout->createInfo.bindingCount - 1);
        } else {
            // Proceed with validation. Bindings are ok, but make sure update is within bounds of given layout
            srcEndIndex = getUpdateEndIndex(my_data, device, pSrcLayout, pCDS[i].srcBinding, pCDS[i].srcArrayElement, (const GENERIC_HEADER*)&(pCDS[i]));
            dstEndIndex = getUpdateEndIndex(my_data, device, pDstLayout, pCDS[i].dstBinding, pCDS[i].dstArrayElement, (const GENERIC_HEADER*)&(pCDS[i]));
            if (getBindingEndIndex(pSrcLayout, pCDS[i].srcBinding) < srcEndIndex) {
                pLayoutCI = &pSrcLayout->createInfo;
                string DSstr = vk_print_vkdescriptorsetlayoutcreateinfo(pLayoutCI, "{DS}    ");
                skipCall |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT, (uint64_t) pSrcSet->set, __LINE__, DRAWSTATE_DESCRIPTOR_UPDATE_OUT_OF_BOUNDS, "DS",
                    "Copy descriptor src update is out of bounds for matching binding %u in Layout w/ CI:\n%s!", pCDS[i].srcBinding, DSstr.c_str());
            } else if (getBindingEndIndex(pDstLayout, pCDS[i].dstBinding) < dstEndIndex) {
                pLayoutCI = &pDstLayout->createInfo;
                string DSstr = vk_print_vkdescriptorsetlayoutcreateinfo(pLayoutCI, "{DS}    ");
                skipCall |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT, (uint64_t) pDstSet->set, __LINE__, DRAWSTATE_DESCRIPTOR_UPDATE_OUT_OF_BOUNDS, "DS",
                    "Copy descriptor dest update is out of bounds for matching binding %u in Layout w/ CI:\n%s!", pCDS[i].dstBinding, DSstr.c_str());
            } else {
                srcStartIndex = getUpdateStartIndex(my_data, device, pSrcLayout, pCDS[i].srcBinding, pCDS[i].srcArrayElement, (const GENERIC_HEADER*)&(pCDS[i]));
                dstStartIndex = getUpdateStartIndex(my_data, device, pDstLayout, pCDS[i].dstBinding, pCDS[i].dstArrayElement, (const GENERIC_HEADER*)&(pCDS[i]));
                for (uint32_t j=0; j<pCDS[i].descriptorCount; ++j) {
                    // For copy just make sure that the types match and then perform the update
                    if (pSrcLayout->descriptorTypes[srcStartIndex+j] != pDstLayout->descriptorTypes[dstStartIndex+j]) {
                        skipCall |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT) 0, 0, __LINE__, DRAWSTATE_DESCRIPTOR_TYPE_MISMATCH, "DS",
                            "Copy descriptor update index %u, update count #%u, has src update descriptor type %s that does not match overlapping dest descriptor type of %s!",
                            i, j+1, string_VkDescriptorType(pSrcLayout->descriptorTypes[srcStartIndex+j]), string_VkDescriptorType(pDstLayout->descriptorTypes[dstStartIndex+j]));
                    } else {
                        // point dst descriptor at corresponding src descriptor
                        // TODO : This may be a hole. I believe copy should be its own copy,
                        //  otherwise a subsequent write update to src will incorrectly affect the copy
                        pDstSet->ppDescriptors[j+dstStartIndex] = pSrcSet->ppDescriptors[j+srcStartIndex];
                    }
                }
            }
        }
    }
    return skipCall;
}

// Verify that given pool has descriptors that are being requested for allocation
static VkBool32 validate_descriptor_availability_in_pool(layer_data* dev_data, DESCRIPTOR_POOL_NODE* pPoolNode, uint32_t count, const VkDescriptorSetLayout* pSetLayouts)
{
    VkBool32 skipCall = VK_FALSE;
    uint32_t i = 0, j = 0;
    for (i=0; i<count; ++i) {
        LAYOUT_NODE* pLayout = getLayoutNode(dev_data, pSetLayouts[i]);
        if (NULL == pLayout) {
            skipCall |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT, (uint64_t) pSetLayouts[i], __LINE__, DRAWSTATE_INVALID_LAYOUT, "DS",
                    "Unable to find set layout node for layout %#" PRIxLEAST64 " specified in vkAllocateDescriptorSets() call", (uint64_t) pSetLayouts[i]);
        } else {
            uint32_t typeIndex = 0, poolSizeCount = 0;
            for (j=0; j<pLayout->createInfo.bindingCount; ++j) {
                typeIndex = static_cast<uint32_t>(pLayout->createInfo.pBindings[j].descriptorType);
                poolSizeCount = pLayout->createInfo.pBindings[j].descriptorCount;
                if (poolSizeCount > pPoolNode->availableDescriptorTypeCount[typeIndex]) {
                    skipCall |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT, (uint64_t) pLayout->layout, __LINE__, DRAWSTATE_DESCRIPTOR_POOL_EMPTY, "DS",
                        "Unable to allocate %u descriptors of type %s from pool %#" PRIxLEAST64 ". This pool only has %u descriptors of this type remaining.",
                            poolSizeCount, string_VkDescriptorType(pLayout->createInfo.pBindings[j].descriptorType), (uint64_t) pPoolNode->pool, pPoolNode->availableDescriptorTypeCount[typeIndex]);
                } else { // Decrement available descriptors of this type
                    pPoolNode->availableDescriptorTypeCount[typeIndex] -= poolSizeCount;
                }
            }
        }
    }
    return skipCall;
}

// Free the shadowed update node for this Set
// NOTE : Calls to this function should be wrapped in mutex
static void freeShadowUpdateTree(SET_NODE* pSet)
{
    GENERIC_HEADER* pShadowUpdate = pSet->pUpdateStructs;
    pSet->pUpdateStructs = NULL;
    GENERIC_HEADER* pFreeUpdate = pShadowUpdate;
    // Clear the descriptor mappings as they will now be invalid
    memset(pSet->ppDescriptors, 0, pSet->descriptorCount*sizeof(GENERIC_HEADER*));
    while(pShadowUpdate) {
        pFreeUpdate = pShadowUpdate;
        pShadowUpdate = (GENERIC_HEADER*)pShadowUpdate->pNext;
        VkWriteDescriptorSet * pWDS = NULL;
        switch (pFreeUpdate->sType)
        {
            case VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET:
                pWDS = (VkWriteDescriptorSet*)pFreeUpdate;
                switch (pWDS->descriptorType) {
                case VK_DESCRIPTOR_TYPE_SAMPLER:
                case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
                case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
                    {
                        delete[] pWDS->pImageInfo;
                    }
                    break;
                case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
                case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
                    {
                        delete[] pWDS->pTexelBufferView;
                    }
                    break;
                case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
                case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
                case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
                    {
                        delete[] pWDS->pBufferInfo;
                    }
                    break;
                default:
                    break;
                }
                break;
            case VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET:
                break;
            default:
                assert(0);
                break;
        }
        delete pFreeUpdate;
    }
}

// Free all DS Pools including their Sets & related sub-structs
// NOTE : Calls to this function should be wrapped in mutex
static void deletePools(layer_data* my_data)
{
    if (my_data->descriptorPoolMap.size() <= 0)
        return;
    for (auto ii=my_data->descriptorPoolMap.begin(); ii!=my_data->descriptorPoolMap.end(); ++ii) {
        SET_NODE* pSet = (*ii).second->pSets;
        SET_NODE* pFreeSet = pSet;
        while (pSet) {
            pFreeSet = pSet;
            pSet = pSet->pNext;
            // Freeing layouts handled in deleteLayouts() function
            // Free Update shadow struct tree
            freeShadowUpdateTree(pFreeSet);
            if (pFreeSet->ppDescriptors) {
                delete[] pFreeSet->ppDescriptors;
            }
            delete pFreeSet;
        }
        delete (*ii).second;
    }
    my_data->descriptorPoolMap.clear();
}

// WARN : Once deleteLayouts() called, any layout ptrs in Pool/Set data structure will be invalid
// NOTE : Calls to this function should be wrapped in mutex
static void deleteLayouts(layer_data* my_data)
{
    if (my_data->descriptorSetLayoutMap.size() <= 0)
        return;
    for (auto ii=my_data->descriptorSetLayoutMap.begin(); ii!=my_data->descriptorSetLayoutMap.end(); ++ii) {
        LAYOUT_NODE* pLayout = (*ii).second;
        if (pLayout->createInfo.pBindings) {
            for (uint32_t i=0; i<pLayout->createInfo.bindingCount; i++) {
                if (pLayout->createInfo.pBindings[i].pImmutableSamplers)
                    delete[] pLayout->createInfo.pBindings[i].pImmutableSamplers;
            }
            delete[] pLayout->createInfo.pBindings;
        }
        delete pLayout;
    }
    my_data->descriptorSetLayoutMap.clear();
}

// Currently clearing a set is removing all previous updates to that set
//  TODO : Validate if this is correct clearing behavior
static void clearDescriptorSet(layer_data* my_data, VkDescriptorSet set)
{
    SET_NODE* pSet = getSetNode(my_data, set);
    if (!pSet) {
        // TODO : Return error
    } else {
        freeShadowUpdateTree(pSet);
    }
}

static void clearDescriptorPool(layer_data* my_data, const VkDevice device, const VkDescriptorPool pool, VkDescriptorPoolResetFlags flags)
{
    DESCRIPTOR_POOL_NODE* pPool = getPoolNode(my_data, pool);
    if (!pPool) {
        log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT, (uint64_t) pool, __LINE__, DRAWSTATE_INVALID_POOL, "DS",
                "Unable to find pool node for pool %#" PRIxLEAST64 " specified in vkResetDescriptorPool() call", (uint64_t) pool);
    } else {
        // TODO: validate flags
        // For every set off of this pool, clear it
        SET_NODE* pSet = pPool->pSets;
        while (pSet) {
            clearDescriptorSet(my_data, pSet->set);
            pSet = pSet->pNext;
        }
        // Reset available count to max count for this pool
        for (uint32_t i=0; i<pPool->availableDescriptorTypeCount.size(); ++i) {
            pPool->availableDescriptorTypeCount[i] = pPool->maxDescriptorTypeCount[i];
        }
    }
}

// For given CB object, fetch associated CB Node from map
static GLOBAL_CB_NODE* getCBNode(layer_data* my_data, const VkCommandBuffer cb)
{
    if (my_data->commandBufferMap.count(cb) == 0) {
        // TODO : How to pass cb as srcObj here?
        log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, 0, __LINE__, DRAWSTATE_INVALID_COMMAND_BUFFER, "DS",
                "Attempt to use CommandBuffer %#" PRIxLEAST64 " that doesn't exist!", (uint64_t)(cb));
        return NULL;
    }
    return my_data->commandBufferMap[cb];
}

// Free all CB Nodes
// NOTE : Calls to this function should be wrapped in mutex
static void deleteCommandBuffers(layer_data* my_data)
{
    if (my_data->commandBufferMap.size() <= 0) {
        return;
    }
    for (auto ii=my_data->commandBufferMap.begin(); ii!=my_data->commandBufferMap.end(); ++ii) {
        delete (*ii).second;
    }
    my_data->commandBufferMap.clear();
}

static VkBool32 report_error_no_cb_begin(const layer_data* dev_data, const VkCommandBuffer cb, const char* caller_name)
{
    return log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
        (uint64_t)cb, __LINE__, DRAWSTATE_NO_BEGIN_COMMAND_BUFFER, "DS",
        "You must call vkBeginCommandBuffer() before this call to %s", caller_name);
}

VkBool32 validateCmdsInCmdBuffer(const layer_data* dev_data, const GLOBAL_CB_NODE* pCB, const CMD_TYPE cmd_type) {
    if (!pCB->activeRenderPass) return VK_FALSE;
    VkBool32 skip_call = VK_FALSE;
    if (pCB->activeSubpassContents == VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS && cmd_type != CMD_EXECUTECOMMANDS) {
        skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
            DRAWSTATE_INVALID_COMMAND_BUFFER, "DS", "Commands cannot be called in a subpass using secondary command buffers.");
    } else if (pCB->activeSubpassContents == VK_SUBPASS_CONTENTS_INLINE && cmd_type == CMD_EXECUTECOMMANDS) {
        skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
            DRAWSTATE_INVALID_COMMAND_BUFFER, "DS", "vkCmdExecuteCommands() cannot be called in a subpass using inline commands.");
    }
    return skip_call;
}

static bool checkGraphicsBit(const layer_data* my_data, VkQueueFlags flags, const char* name) {
    if (!(flags & VK_QUEUE_GRAPHICS_BIT))
        return log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
            DRAWSTATE_INVALID_COMMAND_BUFFER, "DS", "Cannot call %s on a command buffer allocated from a pool without graphics capabilities.", name);
    return false;
}

static bool checkComputeBit(const layer_data* my_data, VkQueueFlags flags, const char* name) {
    if (!(flags & VK_QUEUE_COMPUTE_BIT))
        return log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
            DRAWSTATE_INVALID_COMMAND_BUFFER, "DS", "Cannot call %s on a command buffer allocated from a pool without compute capabilities.", name);
    return false;
}

static bool checkGraphicsOrComputeBit(const layer_data* my_data, VkQueueFlags flags, const char* name) {
    if (!((flags & VK_QUEUE_GRAPHICS_BIT) || (flags & VK_QUEUE_COMPUTE_BIT)))
        return log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
            DRAWSTATE_INVALID_COMMAND_BUFFER, "DS", "Cannot call %s on a command buffer allocated from a pool without graphics capabilities.", name);
    return false;
}

// Add specified CMD to the CmdBuffer in given pCB, flagging errors if CB is not
//  in the recording state or if there's an issue with the Cmd ordering
static VkBool32 addCmd(const layer_data* my_data, GLOBAL_CB_NODE* pCB, const CMD_TYPE cmd, const char* caller_name)
{
    VkBool32 skipCall = VK_FALSE;
    auto pool_data = my_data->commandPoolMap.find(pCB->createInfo.commandPool);
    if (pool_data != my_data->commandPoolMap.end()) {
        VkQueueFlags flags = my_data->physDevProperties.queue_family_properties[pool_data->second.queueFamilyIndex].queueFlags;
        switch (cmd)
        {
            case CMD_BINDPIPELINE:
            case CMD_BINDPIPELINEDELTA:
            case CMD_BINDDESCRIPTORSETS:
            case CMD_FILLBUFFER:
            case CMD_CLEARCOLORIMAGE:
            case CMD_SETEVENT:
            case CMD_RESETEVENT:
            case CMD_WAITEVENTS:
            case CMD_BEGINQUERY:
            case CMD_ENDQUERY:
            case CMD_RESETQUERYPOOL:
            case CMD_COPYQUERYPOOLRESULTS:
            case CMD_WRITETIMESTAMP:
                skipCall |= checkGraphicsOrComputeBit(my_data, flags, cmdTypeToString(cmd).c_str());
                break;
            case CMD_SETVIEWPORTSTATE:
            case CMD_SETSCISSORSTATE:
            case CMD_SETLINEWIDTHSTATE:
            case CMD_SETDEPTHBIASSTATE:
            case CMD_SETBLENDSTATE:
            case CMD_SETDEPTHBOUNDSSTATE:
            case CMD_SETSTENCILREADMASKSTATE:
            case CMD_SETSTENCILWRITEMASKSTATE:
            case CMD_SETSTENCILREFERENCESTATE:
            case CMD_BINDINDEXBUFFER:
            case CMD_BINDVERTEXBUFFER:
            case CMD_DRAW:
            case CMD_DRAWINDEXED:
            case CMD_DRAWINDIRECT:
            case CMD_DRAWINDEXEDINDIRECT:
            case CMD_BLITIMAGE:
            case CMD_CLEARATTACHMENTS:
            case CMD_CLEARDEPTHSTENCILIMAGE:
            case CMD_RESOLVEIMAGE:
            case CMD_BEGINRENDERPASS:
            case CMD_NEXTSUBPASS:
            case CMD_ENDRENDERPASS:
                skipCall |= checkGraphicsBit(my_data, flags, cmdTypeToString(cmd).c_str());
                break;
            case CMD_DISPATCH:
            case CMD_DISPATCHINDIRECT:
                skipCall |= checkComputeBit(my_data, flags, cmdTypeToString(cmd).c_str());
                break;
            case CMD_COPYBUFFER:
            case CMD_COPYIMAGE:
            case CMD_COPYBUFFERTOIMAGE:
            case CMD_COPYIMAGETOBUFFER:
            case CMD_CLONEIMAGEDATA:
            case CMD_UPDATEBUFFER:
            case CMD_PIPELINEBARRIER:
            case CMD_EXECUTECOMMANDS:
                break;
            default:
                break;
        }
    }
    if (pCB->state != CB_RECORDING) {
        skipCall |= report_error_no_cb_begin(my_data, pCB->commandBuffer, caller_name);
        skipCall |= validateCmdsInCmdBuffer(my_data, pCB, cmd);
        CMD_NODE cmdNode = {};
        // init cmd node and append to end of cmd LL
        cmdNode.cmdNumber = ++pCB->numCmds;
        cmdNode.type = cmd;
        pCB->cmds.push_back(cmdNode);
    }
    return skipCall;
}
// Reset the command buffer state
//  Maintain the createInfo and set state to CB_NEW, but clear all other state
static void resetCB(layer_data* my_data, const VkCommandBuffer cb)
{
    GLOBAL_CB_NODE* pCB = my_data->commandBufferMap[cb];
    if (pCB) {
        pCB->cmds.clear();
        // Reset CB state (note that createInfo is not cleared)
        pCB->commandBuffer = cb;
        memset(&pCB->beginInfo, 0, sizeof(VkCommandBufferBeginInfo));
        memset(&pCB->inheritanceInfo, 0, sizeof(VkCommandBufferInheritanceInfo));
        pCB->fence = 0;
        pCB->numCmds = 0;
        memset(pCB->drawCount, 0, NUM_DRAW_TYPES * sizeof(uint64_t));
        pCB->state = CB_NEW;
        pCB->submitCount = 0;
        pCB->status = 0;
        pCB->lastBoundPipeline = 0;
        pCB->lastVtxBinding = 0;
        pCB->boundVtxBuffers.clear();
        pCB->viewports.clear();
        pCB->scissors.clear();
        pCB->lineWidth = 0;
        pCB->depthBiasConstantFactor = 0;
        pCB->depthBiasClamp = 0;
        pCB->depthBiasSlopeFactor = 0;
        memset(pCB->blendConstants, 0, 4 * sizeof(float));
        pCB->minDepthBounds = 0;
        pCB->maxDepthBounds = 0;
        memset(&pCB->front, 0, sizeof(stencil_data));
        memset(&pCB->back, 0, sizeof(stencil_data));
        pCB->lastBoundDescriptorSet = 0;
        pCB->lastBoundPipelineLayout = 0;
        memset(&pCB->activeRenderPassBeginInfo, 0, sizeof(pCB->activeRenderPassBeginInfo));
        pCB->activeRenderPass = 0;
        pCB->activeSubpassContents = VK_SUBPASS_CONTENTS_INLINE;
        pCB->activeSubpass = 0;
        pCB->framebuffer = 0;
        // Before clearing uniqueBoundSets, remove this CB off of its boundCBs
        for (auto set : pCB->uniqueBoundSets) {
            auto set_node = my_data->setMap.find(set);
            if (set_node != my_data->setMap.end()) {
                set_node->second->boundCmdBuffers.erase(pCB->commandBuffer);
            }
        }
        pCB->uniqueBoundSets.clear();
        pCB->destroyedSets.clear();
        pCB->updatedSets.clear();
        pCB->boundDescriptorSets.clear();
        pCB->waitedEvents.clear();
        pCB->semaphores.clear();
        pCB->events.clear();
        pCB->waitedEventsBeforeQueryReset.clear();
        pCB->queryToStateMap.clear();
        pCB->activeQueries.clear();
        pCB->startedQueries.clear();
        pCB->imageLayoutMap.clear();
        pCB->eventToStageMap.clear();
        pCB->drawData.clear();
        pCB->currentDrawData.buffers.clear();
        pCB->primaryCommandBuffer = VK_NULL_HANDLE;
        pCB->secondaryCommandBuffers.clear();
        pCB->dynamicOffsets.clear();
    }
}

// Set PSO-related status bits for CB, including dynamic state set via PSO
static void set_cb_pso_status(GLOBAL_CB_NODE* pCB, const PIPELINE_NODE* pPipe)
{
    for (uint32_t i = 0; i < pPipe->cbStateCI.attachmentCount; i++) {
        if (0 != pPipe->pAttachments[i].colorWriteMask) {
            pCB->status |= CBSTATUS_COLOR_BLEND_WRITE_ENABLE;
        }
    }
    if (pPipe->dsStateCI.depthWriteEnable) {
        pCB->status |= CBSTATUS_DEPTH_WRITE_ENABLE;
    }
    if (pPipe->dsStateCI.stencilTestEnable) {
        pCB->status |= CBSTATUS_STENCIL_TEST_ENABLE;
    }
    // Account for any dynamic state not set via this PSO
    if (!pPipe->dynStateCI.dynamicStateCount) { // All state is static
        pCB->status = CBSTATUS_ALL;
    } else {
        // First consider all state on
        // Then unset any state that's noted as dynamic in PSO
        // Finally OR that into CB statemask
        CBStatusFlags psoDynStateMask = CBSTATUS_ALL;
        for (uint32_t i=0; i < pPipe->dynStateCI.dynamicStateCount; i++) {
            switch (pPipe->dynStateCI.pDynamicStates[i]) {
                case VK_DYNAMIC_STATE_VIEWPORT:
                    psoDynStateMask &= ~CBSTATUS_VIEWPORT_SET;
                    break;
                case VK_DYNAMIC_STATE_SCISSOR:
                    psoDynStateMask &= ~CBSTATUS_SCISSOR_SET;
                    break;
                case VK_DYNAMIC_STATE_LINE_WIDTH:
                    psoDynStateMask &= ~CBSTATUS_LINE_WIDTH_SET;
                    break;
                case VK_DYNAMIC_STATE_DEPTH_BIAS:
                    psoDynStateMask &= ~CBSTATUS_DEPTH_BIAS_SET;
                    break;
                case VK_DYNAMIC_STATE_BLEND_CONSTANTS:
                    psoDynStateMask &= ~CBSTATUS_BLEND_SET;
                    break;
                case VK_DYNAMIC_STATE_DEPTH_BOUNDS:
                    psoDynStateMask &= ~CBSTATUS_DEPTH_BOUNDS_SET;
                    break;
                case VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK:
                    psoDynStateMask &= ~CBSTATUS_STENCIL_READ_MASK_SET;
                    break;
                case VK_DYNAMIC_STATE_STENCIL_WRITE_MASK:
                    psoDynStateMask &= ~CBSTATUS_STENCIL_WRITE_MASK_SET;
                    break;
                case VK_DYNAMIC_STATE_STENCIL_REFERENCE:
                    psoDynStateMask &= ~CBSTATUS_STENCIL_REFERENCE_SET;
                    break;
                default:
                    // TODO : Flag error here
                    break;
            }
        }
        pCB->status |= psoDynStateMask;
    }
}

// Print the last bound Gfx Pipeline
static VkBool32 printPipeline(layer_data* my_data, const VkCommandBuffer cb)
{
    VkBool32 skipCall = VK_FALSE;
    GLOBAL_CB_NODE* pCB = getCBNode(my_data, cb);
    if (pCB) {
        PIPELINE_NODE *pPipeTrav = getPipeline(my_data, pCB->lastBoundPipeline);
        if (!pPipeTrav) {
            // nothing to print
        } else {
            skipCall |= log_msg(my_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, (VkDebugReportObjectTypeEXT) 0, 0, __LINE__, DRAWSTATE_NONE, "DS",
                    "%s", vk_print_vkgraphicspipelinecreateinfo(&pPipeTrav->graphicsPipelineCI, "{DS}").c_str());
        }
    }
    return skipCall;
}

// Print details of DS config to stdout
static VkBool32 printDSConfig(layer_data* my_data, const VkCommandBuffer cb)
{
    VkBool32 skipCall = VK_FALSE;
    GLOBAL_CB_NODE* pCB = getCBNode(my_data, cb);
    if (pCB && pCB->lastBoundDescriptorSet) {
        SET_NODE* pSet = getSetNode(my_data, pCB->lastBoundDescriptorSet);
        DESCRIPTOR_POOL_NODE* pPool = getPoolNode(my_data, pSet->pool);
        // Print out pool details
        skipCall |= log_msg(my_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, (VkDebugReportObjectTypeEXT) 0, 0, __LINE__, DRAWSTATE_NONE, "DS",
                "Details for pool %#" PRIxLEAST64 ".", (uint64_t) pPool->pool);
        string poolStr = vk_print_vkdescriptorpoolcreateinfo(&pPool->createInfo, " ");
        skipCall |= log_msg(my_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, (VkDebugReportObjectTypeEXT) 0, 0, __LINE__, DRAWSTATE_NONE, "DS",
                "%s", poolStr.c_str());
        // Print out set details
        char prefix[10];
        uint32_t index = 0;
        skipCall |= log_msg(my_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, (VkDebugReportObjectTypeEXT) 0, 0, __LINE__, DRAWSTATE_NONE, "DS",
                "Details for descriptor set %#" PRIxLEAST64 ".", (uint64_t) pSet->set);
        LAYOUT_NODE* pLayout = pSet->pLayout;
        // Print layout details
        skipCall |= log_msg(my_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, (VkDebugReportObjectTypeEXT) 0, 0, __LINE__, DRAWSTATE_NONE, "DS",
                "Layout #%u, (object %#" PRIxLEAST64 ") for DS %#" PRIxLEAST64 ".", index+1, (uint64_t)(pLayout->layout), (uint64_t)(pSet->set));
        sprintf(prefix, "  [L%u] ", index);
        string DSLstr = vk_print_vkdescriptorsetlayoutcreateinfo(&pLayout->createInfo, prefix).c_str();
        skipCall |= log_msg(my_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, (VkDebugReportObjectTypeEXT) 0, 0, __LINE__, DRAWSTATE_NONE, "DS",
                "%s", DSLstr.c_str());
        index++;
        GENERIC_HEADER* pUpdate = pSet->pUpdateStructs;
        if (pUpdate) {
            skipCall |= log_msg(my_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, (VkDebugReportObjectTypeEXT) 0, 0, __LINE__, DRAWSTATE_NONE, "DS",
                    "Update Chain [UC] for descriptor set %#" PRIxLEAST64 ":", (uint64_t) pSet->set);
            sprintf(prefix, "  [UC] ");
            skipCall |= log_msg(my_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, (VkDebugReportObjectTypeEXT) 0, 0, __LINE__, DRAWSTATE_NONE, "DS",
                    "%s", dynamic_display(pUpdate, prefix).c_str());
            // TODO : If there is a "view" associated with this update, print CI for that view
        } else {
            if (0 != pSet->descriptorCount) {
                skipCall |= log_msg(my_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, (VkDebugReportObjectTypeEXT) 0, 0, __LINE__, DRAWSTATE_NONE, "DS",
                        "No Update Chain for descriptor set %#" PRIxLEAST64 " which has %u descriptors (vkUpdateDescriptors has not been called)", (uint64_t) pSet->set, pSet->descriptorCount);
            } else {
                skipCall |= log_msg(my_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, (VkDebugReportObjectTypeEXT) 0, 0, __LINE__, DRAWSTATE_NONE, "DS",
                        "FYI: No descriptors in descriptor set %#" PRIxLEAST64 ".", (uint64_t) pSet->set);
            }
        }
    }
    return skipCall;
}

static void printCB(layer_data* my_data, const VkCommandBuffer cb)
{
    GLOBAL_CB_NODE* pCB = getCBNode(my_data, cb);
    if (pCB && pCB->cmds.size() > 0) {
        log_msg(my_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, (VkDebugReportObjectTypeEXT) 0, 0, __LINE__, DRAWSTATE_NONE, "DS",
                "Cmds in CB %p", (void*)cb);
        vector<CMD_NODE> cmds = pCB->cmds;
        for (auto ii=cmds.begin(); ii!=cmds.end(); ++ii) {
            // TODO : Need to pass cb as srcObj here
            log_msg(my_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, 0, __LINE__, DRAWSTATE_NONE, "DS",
                "  CMD#%" PRIu64 ": %s", (*ii).cmdNumber, cmdTypeToString((*ii).type).c_str());
        }
    } else {
        // Nothing to print
    }
}

static VkBool32 synchAndPrintDSConfig(layer_data* my_data, const VkCommandBuffer cb)
{
    VkBool32 skipCall = VK_FALSE;
    if (!(my_data->report_data->active_flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT)) {
        return skipCall;
    }
    skipCall |= printDSConfig(my_data, cb);
    skipCall |= printPipeline(my_data, cb);
    return skipCall;
}

// Flags validation error if the associated call is made inside a render pass. The apiName
// routine should ONLY be called outside a render pass.
static VkBool32 insideRenderPass(const layer_data* my_data, GLOBAL_CB_NODE *pCB, const char *apiName)
{
    VkBool32 inside = VK_FALSE;
    if (pCB->activeRenderPass) {
        inside = log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                         (uint64_t)pCB->commandBuffer, __LINE__, DRAWSTATE_INVALID_RENDERPASS_CMD, "DS",
                         "%s: It is invalid to issue this call inside an active render pass (%#" PRIxLEAST64 ")",
                         apiName, (uint64_t) pCB->activeRenderPass);
    }
    return inside;
}

// Flags validation error if the associated call is made outside a render pass. The apiName
// routine should ONLY be called inside a render pass.
static VkBool32 outsideRenderPass(const layer_data* my_data, GLOBAL_CB_NODE *pCB, const char *apiName)
{
    VkBool32 outside = VK_FALSE;
    if (((pCB->createInfo.level == VK_COMMAND_BUFFER_LEVEL_PRIMARY)   &&
         (!pCB->activeRenderPass))                                         ||
        ((pCB->createInfo.level == VK_COMMAND_BUFFER_LEVEL_SECONDARY) &&
         (!pCB->activeRenderPass)                                     &&
         !(pCB->beginInfo.flags & VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT))) {
        outside = log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                          (uint64_t)pCB->commandBuffer, __LINE__, DRAWSTATE_NO_ACTIVE_RENDERPASS, "DS",
                          "%s: This call must be issued inside an active render pass.", apiName);
    }
    return outside;
}

static void init_draw_state(layer_data *my_data, const VkAllocationCallbacks *pAllocator)
{
    uint32_t report_flags = 0;
    uint32_t debug_action = 0;
    FILE *log_output = NULL;
    const char *option_str;
    VkDebugReportCallbackEXT callback;
    // initialize DrawState options
    report_flags = getLayerOptionFlags("DrawStateReportFlags", 0);
    getLayerOptionEnum("DrawStateDebugAction", (uint32_t *) &debug_action);

    if (debug_action & VK_DBG_LAYER_ACTION_LOG_MSG)
    {
        option_str = getLayerOption("DrawStateLogFilename");
        log_output = getLayerLogOutput(option_str, "DrawState");
        VkDebugReportCallbackCreateInfoEXT dbgInfo;
        memset(&dbgInfo, 0, sizeof(dbgInfo));
        dbgInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
        dbgInfo.pfnCallback = log_callback;
        dbgInfo.pUserData = log_output;
        dbgInfo.flags = report_flags;
        layer_create_msg_callback(my_data->report_data, &dbgInfo, pAllocator, &callback);
        my_data->logging_callback.push_back(callback);
    }

    if (debug_action & VK_DBG_LAYER_ACTION_DEBUG_OUTPUT) {
        VkDebugReportCallbackCreateInfoEXT dbgInfo;
        memset(&dbgInfo, 0, sizeof(dbgInfo));
        dbgInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
        dbgInfo.pfnCallback = win32_debug_output_msg;
        dbgInfo.pUserData = log_output;
        dbgInfo.flags = report_flags;
        layer_create_msg_callback(my_data->report_data, &dbgInfo, pAllocator, &callback);
        my_data->logging_callback.push_back(callback);
    }

    if (!globalLockInitialized)
    {
        loader_platform_thread_create_mutex(&globalLock);
        globalLockInitialized = 1;
    }
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateInstance(const VkInstanceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkInstance* pInstance)
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
    if (result != VK_SUCCESS)
        return result;

    // TBD: Need any locking this early, in case this function is called at the
    // same time by more than one thread?
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(*pInstance), layer_data_map);
    my_data->instance_dispatch_table = new VkLayerInstanceDispatchTable;
    layer_init_instance_dispatch_table(*pInstance, my_data->instance_dispatch_table, fpGetInstanceProcAddr);

    my_data->report_data = debug_report_create_instance(
                               my_data->instance_dispatch_table,
                               *pInstance,
                               pCreateInfo->enabledExtensionCount,
                               pCreateInfo->ppEnabledExtensionNames);

    init_draw_state(my_data, pAllocator);

    return result;
}

/* hook DestroyInstance to remove tableInstanceMap entry */
VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyInstance(VkInstance instance, const VkAllocationCallbacks* pAllocator)
{
    // TODOSC : Shouldn't need any customization here
    dispatch_key key = get_dispatch_key(instance);
    // TBD: Need any locking this early, in case this function is called at the
    // same time by more than one thread?
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerInstanceDispatchTable *pTable = my_data->instance_dispatch_table;
    pTable->DestroyInstance(instance, pAllocator);

    loader_platform_thread_lock_mutex(&globalLock);

    // Clean up logging callback, if any
    while (my_data->logging_callback.size() > 0) {
        VkDebugReportCallbackEXT callback = my_data->logging_callback.back();
        layer_destroy_msg_callback(my_data->report_data, callback, pAllocator);
        my_data->logging_callback.pop_back();
    }

    layer_debug_report_destroy_instance(my_data->report_data);
    delete my_data->instance_dispatch_table;
    layer_data_map.erase(key);
    // TODO : Potential race here with separate threads creating/destroying instance
    if (layer_data_map.empty()) {
        // Release mutex when destroying last instance.
        loader_platform_thread_unlock_mutex(&globalLock);
        loader_platform_thread_delete_mutex(&globalLock);
        globalLockInitialized = 0;
    } else {
        loader_platform_thread_unlock_mutex(&globalLock);
    }
}

static void createDeviceRegisterExtensions(const VkDeviceCreateInfo* pCreateInfo, VkDevice device)
{
    uint32_t i;
    // TBD: Need any locking, in case this function is called at the same time
    // by more than one thread?
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    dev_data->device_extensions.debug_marker_enabled = false;
    dev_data->device_extensions.wsi_enabled = false;


    VkLayerDispatchTable    *pDisp = dev_data->device_dispatch_table;
    PFN_vkGetDeviceProcAddr  gpa   = pDisp->GetDeviceProcAddr;

    pDisp->CreateSwapchainKHR        = (PFN_vkCreateSwapchainKHR) gpa(device, "vkCreateSwapchainKHR");
    pDisp->DestroySwapchainKHR       = (PFN_vkDestroySwapchainKHR) gpa(device, "vkDestroySwapchainKHR");
    pDisp->GetSwapchainImagesKHR     = (PFN_vkGetSwapchainImagesKHR) gpa(device, "vkGetSwapchainImagesKHR");
    pDisp->AcquireNextImageKHR       = (PFN_vkAcquireNextImageKHR) gpa(device, "vkAcquireNextImageKHR");
    pDisp->QueuePresentKHR           = (PFN_vkQueuePresentKHR) gpa(device, "vkQueuePresentKHR");

    for (i = 0; i < pCreateInfo->enabledExtensionCount; i++) {
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {
            dev_data->device_extensions.wsi_enabled = true;
        }
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], DEBUG_MARKER_EXTENSION_NAME) == 0) {
            /* Found a matching extension name, mark it enabled and init dispatch table*/
            dev_data->device_extensions.debug_marker_enabled = true;
            initDebugMarkerTable(device);

        }
    }
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateDevice(VkPhysicalDevice gpu, const VkDeviceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDevice* pDevice)
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

    loader_platform_thread_lock_mutex(&globalLock);
    layer_data *my_instance_data = get_my_data_ptr(get_dispatch_key(gpu), layer_data_map);
    layer_data *my_device_data = get_my_data_ptr(get_dispatch_key(*pDevice), layer_data_map);

    // Setup device dispatch table
    my_device_data->device_dispatch_table = new VkLayerDispatchTable;
    layer_init_device_dispatch_table(*pDevice, my_device_data->device_dispatch_table, fpGetDeviceProcAddr);

    my_device_data->report_data = layer_debug_report_create_device(my_instance_data->report_data, *pDevice);
    createDeviceRegisterExtensions(pCreateInfo, *pDevice);
    // Get physical device limits for this device
    my_instance_data->instance_dispatch_table->GetPhysicalDeviceProperties(gpu, &(my_device_data->physDevProperties.properties));
    my_instance_data->instance_dispatch_table->GetPhysicalDeviceFeatures(gpu, &(my_device_data->physDevProperties.features));
    uint32_t count;
    my_instance_data->instance_dispatch_table->GetPhysicalDeviceQueueFamilyProperties(gpu, &count, nullptr);
    my_device_data->physDevProperties.queue_family_properties.resize(count);
    my_instance_data->instance_dispatch_table->GetPhysicalDeviceQueueFamilyProperties(gpu, &count, &my_device_data->physDevProperties.queue_family_properties[0]);
    // TODO: device limits should make sure these are compatible
    if (pCreateInfo->pEnabledFeatures) {
        my_device_data->physDevProperties.features =
            *pCreateInfo->pEnabledFeatures;
    } else {
        my_instance_data->instance_dispatch_table->GetPhysicalDeviceFeatures(
            gpu, &my_device_data->physDevProperties.features);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    return result;
}

// prototype
static void deleteRenderPasses(layer_data*);
VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyDevice(VkDevice device, const VkAllocationCallbacks* pAllocator)
{
    // TODOSC : Shouldn't need any customization here
    dispatch_key key = get_dispatch_key(device);
    layer_data* dev_data = get_my_data_ptr(key, layer_data_map);
    // Free all the memory
    loader_platform_thread_lock_mutex(&globalLock);
    deletePipelines(dev_data);
    deleteRenderPasses(dev_data);
    deleteCommandBuffers(dev_data);
    deletePools(dev_data);
    deleteLayouts(dev_data);
    dev_data->imageViewMap.clear();
    dev_data->imageMap.clear();
    dev_data->bufferViewMap.clear();
    dev_data->bufferMap.clear();
    loader_platform_thread_unlock_mutex(&globalLock);

    dev_data->device_dispatch_table->DestroyDevice(device, pAllocator);
    tableDebugMarkerMap.erase(key);
    delete dev_data->device_dispatch_table;
    layer_data_map.erase(key);
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

static const VkLayerProperties ds_global_layers[] = {
    {
        "VK_LAYER_LUNARG_draw_state",
        VK_API_VERSION,
        1,
        "LunarG Validation Layer",
    }
};

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceLayerProperties(
        uint32_t *pCount,
        VkLayerProperties*    pProperties)
{
    return util_GetLayerProperties(ARRAY_SIZE(ds_global_layers),
                                   ds_global_layers,
                                   pCount, pProperties);
}

static const VkExtensionProperties ds_device_extensions[] = {
    {
        DEBUG_MARKER_EXTENSION_NAME,
        1,
    }
};

static const VkLayerProperties ds_device_layers[] = {
    {
        "VK_LAYER_LUNARG_draw_state",
        VK_API_VERSION,
        1,
        "LunarG Validation Layer",
    }
};

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceExtensionProperties(
        VkPhysicalDevice                            physicalDevice,
        const char*                                 pLayerName,
        uint32_t*                                   pCount,
        VkExtensionProperties*                      pProperties)
{
    if (pLayerName == NULL) {
        dispatch_key key = get_dispatch_key(physicalDevice);
        layer_data *my_data = get_my_data_ptr(key, layer_data_map);
        return my_data->instance_dispatch_table->EnumerateDeviceExtensionProperties(
                                                    physicalDevice,
                                                    NULL,
                                                    pCount,
                                                    pProperties);
    } else {
        return util_GetExtensionProperties(ARRAY_SIZE(ds_device_extensions),
                                           ds_device_extensions,
                                           pCount, pProperties);
    }
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceLayerProperties(
        VkPhysicalDevice                            physicalDevice,
        uint32_t*                                   pCount,
        VkLayerProperties*                          pProperties)
{
    /* DrawState physical device layers are the same as global */
    return util_GetLayerProperties(ARRAY_SIZE(ds_device_layers), ds_device_layers,
                                   pCount, pProperties);
}

// This validates that the initial layout specified in the command buffer for
// the IMAGE is the same
// as the global IMAGE layout
VkBool32 ValidateCmdBufImageLayouts(VkCommandBuffer cmdBuffer) {
    VkBool32 skip_call = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    for (auto cb_image_data : pCB->imageLayoutMap) {
        VkImageLayout imageLayout;
        if (!FindLayout(dev_data, cb_image_data.first, imageLayout)) {
            skip_call |= log_msg(
                dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, 0, __LINE__,
                DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                "Cannot submit cmd buffer using deleted image %" PRIu64 ".",
                reinterpret_cast<const uint64_t &>(cb_image_data.first));
        } else {
            if (cb_image_data.second.initialLayout == VK_IMAGE_LAYOUT_UNDEFINED) {
                // TODO: Set memory invalid which is in mem_tracker currently
            }
            else if (imageLayout != cb_image_data.second.initialLayout) {
                skip_call |= log_msg(
                    dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                    VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, 0, __LINE__,
                    DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                    "Cannot submit cmd buffer using image with layout %s when "
                    "first use is %s.",
                    string_VkImageLayout(imageLayout),
                    string_VkImageLayout(cb_image_data.second.initialLayout));
            }
            SetLayout(dev_data, cb_image_data.first, cb_image_data.second.layout);
        }
    }
    return skip_call;
}
// Track which resources are in-flight by atomically incrementing their "in_use" count
VkBool32 validateAndIncrementResources(layer_data* my_data, GLOBAL_CB_NODE* pCB) {
    VkBool32 skip_call = VK_FALSE;
    for (auto drawDataElement : pCB->drawData) {
        for (auto buffer : drawDataElement.buffers) {
            auto buffer_data = my_data->bufferMap.find(buffer);
            if (buffer_data == my_data->bufferMap.end()) {
                skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, (uint64_t)(buffer), __LINE__, DRAWSTATE_INVALID_BUFFER, "DS",
                                     "Cannot submit cmd buffer using deleted buffer %" PRIu64 ".", (uint64_t)(buffer));
            } else {
                buffer_data->second.in_use.fetch_add(1);
            }
        }
    }
    for (auto set : pCB->uniqueBoundSets) {
        auto setNode = my_data->setMap.find(set);
        if (setNode == my_data->setMap.end()) {
            skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT, (uint64_t)(set), __LINE__, DRAWSTATE_INVALID_DESCRIPTOR_SET, "DS",
                "Cannot submit cmd buffer using deleted descriptor set %" PRIu64 ".", (uint64_t)(set));
        } else {
            setNode->second->in_use.fetch_add(1);
        }
    }
    for (auto semaphore : pCB->semaphores) {
        auto semaphoreNode = my_data->semaphoreMap.find(semaphore);
        if (semaphoreNode == my_data->semaphoreMap.end()) {
            skip_call |= log_msg(
                my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT,
                reinterpret_cast<uint64_t &>(semaphore), __LINE__,
                DRAWSTATE_INVALID_SEMAPHORE, "DS",
                "Cannot submit cmd buffer using deleted semaphore %" PRIu64 ".",
                reinterpret_cast<uint64_t &>(semaphore));
        } else {
            semaphoreNode->second.in_use.fetch_add(1);
        }
    }
    for (auto event : pCB->events) {
        auto eventNode = my_data->eventMap.find(event);
        if (eventNode == my_data->eventMap.end()) {
            skip_call |= log_msg(
                my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT,
                reinterpret_cast<uint64_t &>(event), __LINE__,
                DRAWSTATE_INVALID_EVENT, "DS",
                "Cannot submit cmd buffer using deleted event %" PRIu64 ".",
                reinterpret_cast<uint64_t &>(event));
        } else {
            eventNode->second.in_use.fetch_add(1);
        }
    }
    return skip_call;
}

void decrementResources(layer_data* my_data, VkCommandBuffer cmdBuffer) {
    GLOBAL_CB_NODE* pCB = getCBNode(my_data, cmdBuffer);
    for (auto drawDataElement : pCB->drawData) {
        for (auto buffer : drawDataElement.buffers) {
            auto buffer_data = my_data->bufferMap.find(buffer);
            if (buffer_data != my_data->bufferMap.end()) {
                buffer_data->second.in_use.fetch_sub(1);
            }
        }
    }
    for (auto set : pCB->uniqueBoundSets) {
        auto setNode = my_data->setMap.find(set);
        if (setNode != my_data->setMap.end()) {
            setNode->second->in_use.fetch_sub(1);
        }
    }
    for (auto semaphore : pCB->semaphores) {
        auto semaphoreNode = my_data->semaphoreMap.find(semaphore);
        if (semaphoreNode != my_data->semaphoreMap.end()) {
            semaphoreNode->second.in_use.fetch_sub(1);
        }
    }
    for (auto event : pCB->events) {
        auto eventNode = my_data->eventMap.find(event);
        if (eventNode != my_data->eventMap.end()) {
            eventNode->second.in_use.fetch_sub(1);
        }
    }
    for (auto queryStatePair : pCB->queryToStateMap) {
        my_data->queryToStateMap[queryStatePair.first] = queryStatePair.second;
    }
    for (auto eventStagePair : pCB->eventToStageMap) {
        my_data->eventMap[eventStagePair.first].stageMask =
            eventStagePair.second;
    }
}

void decrementResources(layer_data* my_data, uint32_t fenceCount, const VkFence* pFences) {
    for (uint32_t i = 0; i < fenceCount; ++i) {
        auto fence_data = my_data->fenceMap.find(pFences[i]);
        if (fence_data == my_data->fenceMap.end() || !fence_data->second.needsSignaled) return;
        fence_data->second.needsSignaled = false;
        fence_data->second.in_use.fetch_sub(1);
        if (fence_data->second.priorFence != VK_NULL_HANDLE) {
            decrementResources(my_data, 1, &fence_data->second.priorFence);
        }
        for (auto cmdBuffer : fence_data->second.cmdBuffers) {
            decrementResources(my_data, cmdBuffer);
        }
    }
}

void decrementResources(layer_data* my_data, VkQueue queue) {
    auto queue_data = my_data->queueMap.find(queue);
    if (queue_data != my_data->queueMap.end()) {
        for (auto cmdBuffer : queue_data->second.untrackedCmdBuffers) {
            decrementResources(my_data, cmdBuffer);
        }
        queue_data->second.untrackedCmdBuffers.clear();
        decrementResources(my_data, 1, &queue_data->second.priorFence);
    }
}

void trackCommandBuffers(layer_data* my_data, VkQueue queue, uint32_t cmdBufferCount, const VkCommandBuffer* pCmdBuffers, VkFence fence) {
    auto queue_data = my_data->queueMap.find(queue);
    if (fence != VK_NULL_HANDLE) {
        VkFence priorFence = VK_NULL_HANDLE;
        auto fence_data = my_data->fenceMap.find(fence);
        if (fence_data == my_data->fenceMap.end()) {
            return;
        }
        if (queue_data != my_data->queueMap.end()) {
            priorFence = queue_data->second.priorFence;
            queue_data->second.priorFence = fence;
            for (auto cmdBuffer : queue_data->second.untrackedCmdBuffers) {
                fence_data->second.cmdBuffers.push_back(cmdBuffer);
            }
            queue_data->second.untrackedCmdBuffers.clear();
        }
        fence_data->second.cmdBuffers.clear();
        fence_data->second.priorFence = priorFence;
        fence_data->second.needsSignaled = true;
        fence_data->second.queue = queue;
        fence_data->second.in_use.fetch_add(1);
        for (uint32_t i = 0; i < cmdBufferCount; ++i) {
            for (auto secondaryCmdBuffer :
                 my_data->commandBufferMap[pCmdBuffers[i]]
                     ->secondaryCommandBuffers) {
                fence_data->second.cmdBuffers.push_back(
                    secondaryCmdBuffer);
            }
            fence_data->second.cmdBuffers.push_back(pCmdBuffers[i]);
        }
    } else {
        if (queue_data != my_data->queueMap.end()) {
            for (uint32_t i = 0; i < cmdBufferCount; ++i) {
                for (auto secondaryCmdBuffer : my_data->commandBufferMap[pCmdBuffers[i]]->secondaryCommandBuffers) {
                   queue_data->second.untrackedCmdBuffers.push_back(secondaryCmdBuffer);
                }
                queue_data->second.untrackedCmdBuffers.push_back(pCmdBuffers[i]);
            }
        }
    }
    if (queue_data != my_data->queueMap.end()) {
        for (uint32_t i = 0; i < cmdBufferCount; ++i) {
            // Add cmdBuffers to both the global set and queue set
            for (auto secondaryCmdBuffer : my_data->commandBufferMap[pCmdBuffers[i]]->secondaryCommandBuffers) {
                my_data->globalInFlightCmdBuffers.insert(secondaryCmdBuffer);
                queue_data->second.inFlightCmdBuffers.insert(secondaryCmdBuffer);
            }
            my_data->globalInFlightCmdBuffers.insert(pCmdBuffers[i]);
            queue_data->second.inFlightCmdBuffers.insert(pCmdBuffers[i]);
        }
    }
}

bool validateCommandBufferSimultaneousUse(layer_data *dev_data,
                                           GLOBAL_CB_NODE *pCB) {
    bool skip_call = false;
    if (dev_data->globalInFlightCmdBuffers.count(pCB->commandBuffer) &&
        !(pCB->beginInfo.flags &
          VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT)) {
        skip_call |= log_msg(
            dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
            VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, 0, __LINE__,
            DRAWSTATE_INVALID_FENCE, "DS",
            "Command Buffer %#" PRIx64 " is already in use and is not marked "
            "for simultaneous use.",
            reinterpret_cast<uint64_t>(pCB->commandBuffer));
    }
    return skip_call;
}

static bool validateCommandBufferState(layer_data *dev_data,
                                           GLOBAL_CB_NODE *pCB) {
    bool skipCall = false;
    // Validate that cmd buffers have been updated
    if (CB_RECORDED != pCB->state) {
        if (CB_INVALID == pCB->state) {
            // Inform app of reason CB invalid
            if (!pCB->destroyedSets.empty()) {
                std::stringstream set_string;
                for (auto set : pCB->destroyedSets) {
                    set_string << " " << set;
                }
                skipCall |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, (uint64_t)(pCB->commandBuffer), __LINE__, DRAWSTATE_INVALID_COMMAND_BUFFER, "DS",
                    "You are submitting command buffer %#" PRIxLEAST64 " that is invalid because it had the following bound descriptor set(s) destroyed: %s", (uint64_t)(pCB->commandBuffer), set_string.str().c_str());
            }
            if (!pCB->updatedSets.empty()) {
                std::stringstream set_string;
                for (auto set : pCB->updatedSets) {
                    set_string << " " << set;
                }
                skipCall |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, (uint64_t)(pCB->commandBuffer), __LINE__, DRAWSTATE_INVALID_COMMAND_BUFFER, "DS",
                    "You are submitting command buffer %#" PRIxLEAST64 " that is invalid because it had the following bound descriptor set(s) updated: %s", (uint64_t)(pCB->commandBuffer), set_string.str().c_str());
            }
        } else { // Flag error for using CB w/o vkEndCommandBuffer() called
            skipCall |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, (uint64_t)(pCB->commandBuffer), __LINE__, DRAWSTATE_NO_END_COMMAND_BUFFER, "DS",
                "You must call vkEndCommandBuffer() on CB %#" PRIxLEAST64 " before this call to vkQueueSubmit()!", (uint64_t)(pCB->commandBuffer));
        }
    }
    return skipCall;
}

static VkBool32 validatePrimaryCommandBufferState(layer_data *dev_data,
                                                  GLOBAL_CB_NODE *pCB) {
    // Track in-use for resources off of primary and any secondary CBs
    VkBool32 skipCall = validateAndIncrementResources(dev_data, pCB);
    if (!pCB->secondaryCommandBuffers.empty()) {
        for (auto secondaryCmdBuffer : pCB->secondaryCommandBuffers) {
            skipCall |= validateAndIncrementResources(
                dev_data, dev_data->commandBufferMap[secondaryCmdBuffer]);
            GLOBAL_CB_NODE* pSubCB = getCBNode(dev_data, secondaryCmdBuffer);
            if (pSubCB->primaryCommandBuffer != pCB->commandBuffer) {
                log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                        VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, 0,
                        __LINE__,
                        DRAWSTATE_COMMAND_BUFFER_SINGLE_SUBMIT_VIOLATION, "DS",
                        "CB %#" PRIxLEAST64
                        " was submitted with secondary buffer %#" PRIxLEAST64
                        " but that buffer has subsequently been bound to "
                        "primary cmd buffer %#" PRIxLEAST64 ".",
                        reinterpret_cast<uint64_t>(pCB->commandBuffer),
                        reinterpret_cast<uint64_t>(secondaryCmdBuffer),
                        reinterpret_cast<uint64_t>(
                            pSubCB->primaryCommandBuffer));
            }
        }
    }
    // TODO : Verify if this also needs to be checked for secondary command
    //  buffers. If so, this block of code can move to
    //   validateCommandBufferState() function. vulkan GL106 filed to clarify
    if ((pCB->beginInfo.flags & VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT) &&
        (pCB->submitCount > 1)) {
        skipCall |=
            log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                    VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, 0, __LINE__,
                    DRAWSTATE_COMMAND_BUFFER_SINGLE_SUBMIT_VIOLATION, "DS",
                    "CB %#" PRIxLEAST64
                    " was begun w/ VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT "
                    "set, but has been submitted %#" PRIxLEAST64 " times.",
                    (uint64_t)(pCB->commandBuffer), pCB->submitCount);
    }
    skipCall |= validateCommandBufferState(dev_data, pCB);
    // If USAGE_SIMULTANEOUS_USE_BIT not set then CB cannot already be executing
    // on device
    skipCall |= validateCommandBufferSimultaneousUse(dev_data, pCB);
    return skipCall;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkQueueSubmit(VkQueue queue, uint32_t submitCount, const VkSubmitInfo* pSubmits, VkFence fence)
{
    VkBool32 skipCall = VK_FALSE;
    GLOBAL_CB_NODE* pCB = NULL;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(queue), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    for (uint32_t submit_idx = 0; submit_idx < submitCount; submit_idx++) {
        const VkSubmitInfo *submit = &pSubmits[submit_idx];
        vector<VkSemaphore> semaphoreList;
        for (uint32_t i=0; i < submit->waitSemaphoreCount; ++i) {
            semaphoreList.push_back(submit->pWaitSemaphores[i]);
            if (dev_data->semaphoreMap[submit->pWaitSemaphores[i]].signaled) {
                dev_data->semaphoreMap[submit->pWaitSemaphores[i]].signaled = 0;
            } else {
                skipCall |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, 0, __LINE__, DRAWSTATE_QUEUE_FORWARD_PROGRESS, "DS",
                        "Queue %#" PRIx64 " is waiting on semaphore %#" PRIx64 " that has no way to be signaled.",
                        (uint64_t)(queue), (uint64_t)(submit->pWaitSemaphores[i]));
            }
        }
        for (uint32_t i=0; i < submit->signalSemaphoreCount; ++i) {
            semaphoreList.push_back(submit->pSignalSemaphores[i]);
            dev_data->semaphoreMap[submit->pSignalSemaphores[i]].signaled = 1;
        }
        for (uint32_t i=0; i < submit->commandBufferCount; i++) {
            skipCall |= ValidateCmdBufImageLayouts(submit->pCommandBuffers[i]);
            pCB = getCBNode(dev_data, submit->pCommandBuffers[i]);
            pCB->semaphores = semaphoreList;
            pCB->submitCount++; // increment submit count
            skipCall |= validatePrimaryCommandBufferState(dev_data, pCB);
        }
        if ((fence != VK_NULL_HANDLE) && dev_data->fenceMap[fence].in_use.load()) {
            skipCall |= log_msg(
                dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT,
                (uint64_t)(fence), __LINE__,
                DRAWSTATE_INVALID_FENCE, "DS",
                "Fence %#" PRIx64 " is already in use by another submission.",
                (uint64_t)(fence));
        }
        trackCommandBuffers(dev_data, queue, submit->commandBufferCount,
                            submit->pCommandBuffers, fence);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall)
        return dev_data->device_dispatch_table->QueueSubmit(queue, submitCount, pSubmits, fence);
    return VK_ERROR_VALIDATION_FAILED_EXT;
}

// Note: This function assumes that the global lock is held by the calling
// thread.
VkBool32 cleanInFlightCmdBuffer(layer_data* my_data, VkCommandBuffer cmdBuffer) {
    VkBool32 skip_call = VK_FALSE;
    GLOBAL_CB_NODE* pCB = getCBNode(my_data, cmdBuffer);
    if (pCB) {
        for (auto queryEventsPair : pCB->waitedEventsBeforeQueryReset) {
            for (auto event : queryEventsPair.second) {
                if (my_data->eventMap[event].needsSignaled) {
                    skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT, 0, 0, DRAWSTATE_INVALID_QUERY, "DS",
                                     "Cannot get query results on queryPool %" PRIu64 " with index %d which was guarded by unsignaled event %" PRIu64 ".",
                                     (uint64_t)(queryEventsPair.first.pool), queryEventsPair.first.index, (uint64_t)(event));
                }
            }
        }
    }
    return skip_call;
}
// Remove given cmd_buffer from the global inFlight set.
//  Also, if given queue is valid, then remove the cmd_buffer from that queues
//  inFlightCmdBuffer set. Finally, check all other queues and if given cmd_buffer
//  is still in flight on another queue, add it back into the global set.
// Note: This function assumes that the global lock is held by the calling
// thread.
static inline void removeInFlightCmdBuffer(layer_data* dev_data, VkCommandBuffer cmd_buffer, VkQueue queue)
{
    // Pull it off of global list initially, but if we find it in any other queue list, add it back in
    dev_data->globalInFlightCmdBuffers.erase(cmd_buffer);
    if (dev_data->queueMap.find(queue) != dev_data->queueMap.end()) {
        dev_data->queueMap[queue].inFlightCmdBuffers.erase(cmd_buffer);
        for (auto q : dev_data->queues) {
            if ((q != queue) && (dev_data->queueMap[q].inFlightCmdBuffers.find(cmd_buffer) != dev_data->queueMap[q].inFlightCmdBuffers.end())) {
                dev_data->globalInFlightCmdBuffers.insert(cmd_buffer);
                break;
            }
        }
    }
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkWaitForFences(VkDevice device, uint32_t fenceCount, const VkFence* pFences, VkBool32 waitAll, uint64_t timeout)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->WaitForFences(device, fenceCount, pFences, waitAll, timeout);
    VkBool32 skip_call = VK_FALSE;
    loader_platform_thread_lock_mutex(&globalLock);
    if (result == VK_SUCCESS) {
        // When we know that all fences are complete we can clean/remove their CBs
        if (waitAll || fenceCount == 1) {
            for (uint32_t i = 0; i < fenceCount; ++i) {
                VkQueue fence_queue = dev_data->fenceMap[pFences[i]].queue;
                for (auto cmdBuffer : dev_data->fenceMap[pFences[i]].cmdBuffers) {
                    skip_call |= cleanInFlightCmdBuffer(dev_data, cmdBuffer);
                    removeInFlightCmdBuffer(dev_data, cmdBuffer, fence_queue);
                }
            }
            decrementResources(dev_data, fenceCount, pFences);
        }
        // NOTE : Alternate case not handled here is when some fences have completed. In
        //  this case for app to guarantee which fences completed it will have to call
        //  vkGetFenceStatus() at which point we'll clean/remove their CBs if complete.
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE != skip_call)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    return result;
}


VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkGetFenceStatus(VkDevice device, VkFence fence)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->GetFenceStatus(device, fence);
    VkBool32 skip_call = VK_FALSE;
    loader_platform_thread_lock_mutex(&globalLock);
    if (result == VK_SUCCESS) {
        auto fence_queue = dev_data->fenceMap[fence].queue;
        for (auto cmdBuffer : dev_data->fenceMap[fence].cmdBuffers) {
            skip_call |= cleanInFlightCmdBuffer(dev_data, cmdBuffer);
            removeInFlightCmdBuffer(dev_data, cmdBuffer, fence_queue);
        }
        decrementResources(dev_data, 1, &fence);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE != skip_call)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetDeviceQueue(VkDevice device, uint32_t queueFamilyIndex, uint32_t queueIndex, VkQueue* pQueue)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    dev_data->device_dispatch_table->GetDeviceQueue(device, queueFamilyIndex, queueIndex, pQueue);
    dev_data->queues.push_back(*pQueue);
    dev_data->queueMap[*pQueue].device = device;
    loader_platform_thread_unlock_mutex(&globalLock);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkQueueWaitIdle(VkQueue queue)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(queue), layer_data_map);
    decrementResources(dev_data, queue);
    VkBool32 skip_call = VK_FALSE;
    loader_platform_thread_lock_mutex(&globalLock);
    // Iterate over local set since we erase set members as we go in for loop
    auto local_cb_set = dev_data->queueMap[queue].inFlightCmdBuffers;
    for (auto cmdBuffer : local_cb_set) {
        skip_call |= cleanInFlightCmdBuffer(dev_data, cmdBuffer);
        removeInFlightCmdBuffer(dev_data, cmdBuffer, queue);
    }
    dev_data->queueMap[queue].inFlightCmdBuffers.clear();
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE != skip_call)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    return dev_data->device_dispatch_table->QueueWaitIdle(queue);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkDeviceWaitIdle(VkDevice device)
{
    VkBool32 skip_call = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    for (auto queue : dev_data->queues) {
        decrementResources(dev_data, queue);
        if (dev_data->queueMap.find(queue) != dev_data->queueMap.end()) {
            // Clear all of the queue inFlightCmdBuffers (global set cleared below)
            dev_data->queueMap[queue].inFlightCmdBuffers.clear();
        }
    }
    for (auto cmdBuffer : dev_data->globalInFlightCmdBuffers) {
        skip_call |= cleanInFlightCmdBuffer(dev_data, cmdBuffer);
    }
    dev_data->globalInFlightCmdBuffers.clear();
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE != skip_call)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    return dev_data->device_dispatch_table->DeviceWaitIdle(device);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyFence(VkDevice device, VkFence fence, const VkAllocationCallbacks* pAllocator)
{
    layer_data *dev_data =
        get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    bool skipCall = false;
    loader_platform_thread_lock_mutex(&globalLock);
    if (dev_data->fenceMap[fence].in_use.load()) {
        skipCall |=
            log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                    VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT, (uint64_t)(fence),
                    __LINE__, DRAWSTATE_INVALID_FENCE, "DS",
                    "Fence %#" PRIx64 " is in use by a command buffer.",
                    (uint64_t)(fence));
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    if (!skipCall)
        dev_data->device_dispatch_table->DestroyFence(device, fence,
                                                      pAllocator);
    // TODO : Clean up any internal data structures using this obj.
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroySemaphore(VkDevice device, VkSemaphore semaphore, const VkAllocationCallbacks* pAllocator)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    dev_data->device_dispatch_table->DestroySemaphore(device, semaphore, pAllocator);
    loader_platform_thread_lock_mutex(&globalLock);
    if (dev_data->semaphoreMap[semaphore].in_use.load()) {
        log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT,
                reinterpret_cast<uint64_t &>(semaphore), __LINE__,
                DRAWSTATE_INVALID_SEMAPHORE, "DS",
                "Cannot delete semaphore %" PRIx64 " which is in use.",
                reinterpret_cast<uint64_t &>(semaphore));
    }
    dev_data->semaphoreMap.erase(semaphore);
    loader_platform_thread_unlock_mutex(&globalLock);
    // TODO : Clean up any internal data structures using this obj.
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyEvent(VkDevice device, VkEvent event, const VkAllocationCallbacks* pAllocator)
{
    layer_data *dev_data =
        get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    bool skip_call = false;
    loader_platform_thread_lock_mutex(&globalLock);
    auto event_data = dev_data->eventMap.find(event);
    if (event_data != dev_data->eventMap.end() &&
        event_data->second.in_use.load()) {
        skip_call |=
            log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                    VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT,
                    reinterpret_cast<uint64_t &>(event), __LINE__,
                    DRAWSTATE_INVALID_EVENT, "DS",
                    "Cannot delete event %" PRIu64
                    " which is in use by a command buffer.",
                    reinterpret_cast<uint64_t &>(event));
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    if (!skip_call)
        dev_data->device_dispatch_table->DestroyEvent(device, event,
                                                      pAllocator);
    // TODO : Clean up any internal data structures using this obj.
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyQueryPool(VkDevice device, VkQueryPool queryPool, const VkAllocationCallbacks* pAllocator)
{
    get_my_data_ptr(get_dispatch_key(device), layer_data_map)->device_dispatch_table->DestroyQueryPool(device, queryPool, pAllocator);
    // TODO : Clean up any internal data structures using this obj.
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetQueryPoolResults(VkDevice device, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount,
                                                     size_t dataSize, void* pData, VkDeviceSize stride, VkQueryResultFlags flags) {
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    unordered_map<QueryObject, vector<VkCommandBuffer>> queriesInFlight;
    GLOBAL_CB_NODE* pCB = nullptr;
    loader_platform_thread_lock_mutex(&globalLock);
    for (auto cmdBuffer : dev_data->globalInFlightCmdBuffers) {
        pCB = getCBNode(dev_data, cmdBuffer);
        for (auto queryStatePair : pCB->queryToStateMap) {
            queriesInFlight[queryStatePair.first].push_back(cmdBuffer);
        }
    }
    VkBool32 skip_call = VK_FALSE;
    for (uint32_t i = 0; i < queryCount; ++i) {
        QueryObject query = {queryPool, firstQuery + i};
        auto queryElement = queriesInFlight.find(query);
        auto queryToStateElement = dev_data->queryToStateMap.find(query);
        if (queryToStateElement != dev_data->queryToStateMap.end()) {
        }
        // Available and in flight
        if(queryElement != queriesInFlight.end() && queryToStateElement != dev_data->queryToStateMap.end() && queryToStateElement->second) {
            for (auto cmdBuffer : queryElement->second) {
                pCB = getCBNode(dev_data, cmdBuffer);
                auto queryEventElement = pCB->waitedEventsBeforeQueryReset.find(query);
                if (queryEventElement == pCB->waitedEventsBeforeQueryReset.end()) {
                    skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT, 0, __LINE__,
                                         DRAWSTATE_INVALID_QUERY, "DS", "Cannot get query results on queryPool %" PRIu64 " with index %d which is in flight.",
                                         (uint64_t)(queryPool), firstQuery + i);
                } else {
                    for (auto event : queryEventElement->second) {
                        dev_data->eventMap[event].needsSignaled = true;
                    }
                }
            }
        // Unavailable and in flight
        } else if (queryElement != queriesInFlight.end() && queryToStateElement != dev_data->queryToStateMap.end() && !queryToStateElement->second) {
            // TODO : Can there be the same query in use by multiple command buffers in flight?
            bool make_available = false;
            for (auto cmdBuffer : queryElement->second) {
                pCB = getCBNode(dev_data, cmdBuffer);
                make_available |= pCB->queryToStateMap[query];
            }
            if (!(((flags & VK_QUERY_RESULT_PARTIAL_BIT) || (flags & VK_QUERY_RESULT_WAIT_BIT)) && make_available)) {
                skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT, 0, __LINE__, DRAWSTATE_INVALID_QUERY, "DS",
                                     "Cannot get query results on queryPool %" PRIu64 " with index %d which is unavailable.",
                                     (uint64_t)(queryPool), firstQuery + i);
            }
        // Unavailable
        } else if (queryToStateElement != dev_data->queryToStateMap.end() && !queryToStateElement->second) {
            skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT, 0, __LINE__, DRAWSTATE_INVALID_QUERY, "DS",
                                 "Cannot get query results on queryPool %" PRIu64 " with index %d which is unavailable.",
                                 (uint64_t)(queryPool), firstQuery + i);
        // Unitialized
        } else if (queryToStateElement == dev_data->queryToStateMap.end()) {
            skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT, 0, __LINE__, DRAWSTATE_INVALID_QUERY, "DS",
                                 "Cannot get query results on queryPool %" PRIu64 " with index %d which is uninitialized.",
                                 (uint64_t)(queryPool), firstQuery + i);
        }
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    if (skip_call)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    return dev_data->device_dispatch_table->GetQueryPoolResults(device, queryPool, firstQuery, queryCount, dataSize, pData, stride, flags);
}

VkBool32 validateIdleBuffer(const layer_data* my_data, VkBuffer buffer) {
    VkBool32 skip_call = VK_FALSE;
    auto buffer_data = my_data->bufferMap.find(buffer);
    if (buffer_data == my_data->bufferMap.end()) {
        skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, (uint64_t)(buffer), __LINE__, DRAWSTATE_DOUBLE_DESTROY, "DS",
                             "Cannot free buffer %" PRIxLEAST64 " that has not been allocated.", (uint64_t)(buffer));
    } else {
        if (buffer_data->second.in_use.load()) {
            skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, (uint64_t)(buffer), __LINE__, DRAWSTATE_OBJECT_INUSE, "DS",
                                 "Cannot free buffer %" PRIxLEAST64 " that is in use by a command buffer.", (uint64_t)(buffer));
        }
    }
    return skip_call;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyBuffer(VkDevice device, VkBuffer buffer, const VkAllocationCallbacks* pAllocator)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    if (!validateIdleBuffer(dev_data, buffer)) {
        loader_platform_thread_unlock_mutex(&globalLock);
        dev_data->device_dispatch_table->DestroyBuffer(device, buffer, pAllocator);
        loader_platform_thread_lock_mutex(&globalLock);
    }
    dev_data->bufferMap.erase(buffer);
    loader_platform_thread_unlock_mutex(&globalLock);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyBufferView(VkDevice device, VkBufferView bufferView, const VkAllocationCallbacks* pAllocator)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    dev_data->device_dispatch_table->DestroyBufferView(device, bufferView, pAllocator);
    loader_platform_thread_lock_mutex(&globalLock);
    dev_data->bufferViewMap.erase(bufferView);
    loader_platform_thread_unlock_mutex(&globalLock);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyImage(VkDevice device, VkImage image, const VkAllocationCallbacks* pAllocator)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    dev_data->device_dispatch_table->DestroyImage(device, image, pAllocator);
    loader_platform_thread_lock_mutex(&globalLock);
    dev_data->imageMap.erase(image);
    loader_platform_thread_unlock_mutex(&globalLock);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyImageView(VkDevice device, VkImageView imageView, const VkAllocationCallbacks* pAllocator)
{
    get_my_data_ptr(get_dispatch_key(device), layer_data_map)->device_dispatch_table->DestroyImageView(device, imageView, pAllocator);
    // TODO : Clean up any internal data structures using this obj.
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyShaderModule(VkDevice device, VkShaderModule shaderModule, const VkAllocationCallbacks* pAllocator)
{
    get_my_data_ptr(get_dispatch_key(device), layer_data_map)->device_dispatch_table->DestroyShaderModule(device, shaderModule, pAllocator);
    // TODO : Clean up any internal data structures using this obj.
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyPipeline(VkDevice device, VkPipeline pipeline, const VkAllocationCallbacks* pAllocator)
{
    get_my_data_ptr(get_dispatch_key(device), layer_data_map)->device_dispatch_table->DestroyPipeline(device, pipeline, pAllocator);
    // TODO : Clean up any internal data structures using this obj.
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyPipelineLayout(VkDevice device, VkPipelineLayout pipelineLayout, const VkAllocationCallbacks* pAllocator)
{
    get_my_data_ptr(get_dispatch_key(device), layer_data_map)->device_dispatch_table->DestroyPipelineLayout(device, pipelineLayout, pAllocator);
    // TODO : Clean up any internal data structures using this obj.
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroySampler(VkDevice device, VkSampler sampler, const VkAllocationCallbacks* pAllocator)
{
    get_my_data_ptr(get_dispatch_key(device), layer_data_map)->device_dispatch_table->DestroySampler(device, sampler, pAllocator);
    // TODO : Clean up any internal data structures using this obj.
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout descriptorSetLayout, const VkAllocationCallbacks* pAllocator)
{
    get_my_data_ptr(get_dispatch_key(device), layer_data_map)->device_dispatch_table->DestroyDescriptorSetLayout(device, descriptorSetLayout, pAllocator);
    // TODO : Clean up any internal data structures using this obj.
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool, const VkAllocationCallbacks* pAllocator)
{
    get_my_data_ptr(get_dispatch_key(device), layer_data_map)->device_dispatch_table->DestroyDescriptorPool(device, descriptorPool, pAllocator);
    // TODO : Clean up any internal data structures using this obj.
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkFreeCommandBuffers(VkDevice device, VkCommandPool commandPool, uint32_t count, const VkCommandBuffer *pCommandBuffers)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);

    bool skip_call = false;
    loader_platform_thread_lock_mutex(&globalLock);
    for (uint32_t i = 0; i < count; i++) {
        if (dev_data->globalInFlightCmdBuffers.count(pCommandBuffers[i])) {
            skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                reinterpret_cast<uint64_t>(pCommandBuffers[i]), __LINE__, DRAWSTATE_INVALID_COMMAND_BUFFER_RESET, "DS",
                "Attempt to free command buffer (%#" PRIxLEAST64 ") which is in use.", reinterpret_cast<uint64_t>(pCommandBuffers[i]));
        }
        // Delete CB information structure, and remove from commandBufferMap
        auto cb = dev_data->commandBufferMap.find(pCommandBuffers[i]);
        if (cb != dev_data->commandBufferMap.end()) {
            // reset prior to delete for data clean-up
            resetCB(dev_data, (*cb).second->commandBuffer);
            delete (*cb).second;
            dev_data->commandBufferMap.erase(cb);
        }

        // Remove commandBuffer reference from commandPoolMap
        dev_data->commandPoolMap[commandPool].commandBuffers.remove(pCommandBuffers[i]);
    }
    loader_platform_thread_unlock_mutex(&globalLock);

    if (!skip_call)
        dev_data->device_dispatch_table->FreeCommandBuffers(device, commandPool, count, pCommandBuffers);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateCommandPool(VkDevice device, const VkCommandPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkCommandPool* pCommandPool)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);

    VkResult result = dev_data->device_dispatch_table->CreateCommandPool(device, pCreateInfo, pAllocator, pCommandPool);

    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        dev_data->commandPoolMap[*pCommandPool].createFlags = pCreateInfo->flags;
        dev_data->commandPoolMap[*pCommandPool].queueFamilyIndex = pCreateInfo->queueFamilyIndex;
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateQueryPool(
    VkDevice device, const VkQueryPoolCreateInfo *pCreateInfo,
    const VkAllocationCallbacks *pAllocator, VkQueryPool *pQueryPool) {

    layer_data *dev_data =
        get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->CreateQueryPool(
        device, pCreateInfo, pAllocator, pQueryPool);
    if (result == VK_SUCCESS) {
        loader_platform_thread_lock_mutex(&globalLock);
        dev_data->queryPoolMap[*pQueryPool].createInfo = *pCreateInfo;
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VkBool32 validateCommandBuffersNotInUse(const layer_data* dev_data, VkCommandPool commandPool) {
    VkBool32 skipCall = VK_FALSE;
    auto pool_data = dev_data->commandPoolMap.find(commandPool);
    if (pool_data != dev_data->commandPoolMap.end()) {
        for (auto cmdBuffer : pool_data->second.commandBuffers) {
            if (dev_data->globalInFlightCmdBuffers.count(cmdBuffer)) {
                skipCall |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_POOL_EXT, (uint64_t)(commandPool),
                                     __LINE__, DRAWSTATE_OBJECT_INUSE, "DS", "Cannot reset command pool %" PRIx64 " when allocated command buffer %" PRIx64 " is in use.",
                                     (uint64_t)(commandPool), (uint64_t)(cmdBuffer));
            }
        }
    }
    return skipCall;
}

// Destroy commandPool along with all of the commandBuffers allocated from that pool
VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyCommandPool(VkDevice device, VkCommandPool commandPool, const VkAllocationCallbacks* pAllocator)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);

    // Must remove cmdpool from cmdpoolmap, after removing all cmdbuffers in its list from the commandPoolMap
    if (dev_data->commandPoolMap.find(commandPool) != dev_data->commandPoolMap.end()) {
        for (auto poolCb = dev_data->commandPoolMap[commandPool].commandBuffers.begin(); poolCb != dev_data->commandPoolMap[commandPool].commandBuffers.end();) {
            auto del_cb = dev_data->commandBufferMap.find(*poolCb);
            delete (*del_cb).second;                                        // delete CB info structure
            dev_data->commandBufferMap.erase(del_cb);                       // Remove this command buffer from cbMap
            poolCb = dev_data->commandPoolMap[commandPool].commandBuffers.erase(poolCb);   // Remove CB reference from commandPoolMap's list
        }
    }
    dev_data->commandPoolMap.erase(commandPool);

    loader_platform_thread_unlock_mutex(&globalLock);

    if (VK_TRUE == validateCommandBuffersNotInUse(dev_data, commandPool))
        return;

    dev_data->device_dispatch_table->DestroyCommandPool(device, commandPool, pAllocator);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkResetCommandPool(
    VkDevice                device,
    VkCommandPool           commandPool,
    VkCommandPoolResetFlags flags)
{
    layer_data *dev_data               = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult    result                = VK_ERROR_VALIDATION_FAILED_EXT;

    if (VK_TRUE == validateCommandBuffersNotInUse(dev_data, commandPool))
        return VK_ERROR_VALIDATION_FAILED_EXT;

    result = dev_data->device_dispatch_table->ResetCommandPool(device, commandPool, flags);
    // Reset all of the CBs allocated from this pool
    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        auto it = dev_data->commandPoolMap[commandPool].commandBuffers.begin();
        while (it != dev_data->commandPoolMap[commandPool].commandBuffers.end()) {
            resetCB(dev_data, (*it));
            ++it;
        }
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL
vkResetFences(VkDevice device, uint32_t fenceCount, const VkFence *pFences) {
    layer_data *dev_data =
        get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    bool skipCall = false;
    loader_platform_thread_lock_mutex(&globalLock);
    for (uint32_t i = 0; i < fenceCount; ++i) {
        if (dev_data->fenceMap[pFences[i]].in_use.load()) {
            skipCall |=
                log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                        VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT,
                        reinterpret_cast<const uint64_t &>(pFences[i]),
                        __LINE__, DRAWSTATE_INVALID_FENCE, "DS",
                        "Fence %#" PRIx64 " is in use by a command buffer.",
                        reinterpret_cast<const uint64_t &>(pFences[i]));
        }
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    if (!skipCall)
        result = dev_data->device_dispatch_table->ResetFences(
            device, fenceCount, pFences);
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyFramebuffer(VkDevice device, VkFramebuffer framebuffer, const VkAllocationCallbacks* pAllocator)
{
    get_my_data_ptr(get_dispatch_key(device), layer_data_map)->device_dispatch_table->DestroyFramebuffer(device, framebuffer, pAllocator);
    // TODO : Clean up any internal data structures using this obj.
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyRenderPass(VkDevice device, VkRenderPass renderPass, const VkAllocationCallbacks* pAllocator)
{
    get_my_data_ptr(get_dispatch_key(device), layer_data_map)->device_dispatch_table->DestroyRenderPass(device, renderPass, pAllocator);
    // TODO : Clean up any internal data structures using this obj.
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateBuffer(VkDevice device, const VkBufferCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkBuffer* pBuffer)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->CreateBuffer(device, pCreateInfo, pAllocator, pBuffer);
    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        // TODO : This doesn't create deep copy of pQueueFamilyIndices so need to fix that if/when we want that data to be valid
        dev_data->bufferMap[*pBuffer].create_info = unique_ptr<VkBufferCreateInfo>(new VkBufferCreateInfo(*pCreateInfo));
        dev_data->bufferMap[*pBuffer].in_use.store(0);
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateBufferView(VkDevice device, const VkBufferViewCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkBufferView* pView)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->CreateBufferView(device, pCreateInfo, pAllocator, pView);
    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        dev_data->bufferViewMap[*pView] = unique_ptr<VkBufferViewCreateInfo>(new VkBufferViewCreateInfo(*pCreateInfo));
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateImage(VkDevice device, const VkImageCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkImage* pImage)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->CreateImage(device, pCreateInfo, pAllocator, pImage);
    if (VK_SUCCESS == result) {
        IMAGE_NODE image_node;
        image_node.layout = pCreateInfo->initialLayout;
        image_node.format = pCreateInfo->format;
        loader_platform_thread_lock_mutex(&globalLock);
        dev_data->imageMap[*pImage] = unique_ptr<VkImageCreateInfo>(new VkImageCreateInfo(*pCreateInfo));
        ImageSubresourcePair subpair = {*pImage, false, VkImageSubresource()};
        dev_data->imageSubresourceMap[*pImage].push_back(subpair); 
        dev_data->imageLayoutMap[subpair] = image_node;
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateImageView(VkDevice device, const VkImageViewCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkImageView* pView)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->CreateImageView(device, pCreateInfo, pAllocator, pView);
    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        dev_data->imageViewMap[*pView] = unique_ptr<VkImageViewCreateInfo>(new VkImageViewCreateInfo(*pCreateInfo));
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL
    vkCreateFence(VkDevice device, const VkFenceCreateInfo* pCreateInfo,
                  const VkAllocationCallbacks* pAllocator, VkFence* pFence) {
    layer_data *dev_data =
        get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->CreateFence(
        device, pCreateInfo, pAllocator, pFence);
    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        dev_data->fenceMap[*pFence].in_use.store(0);
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

// TODO handle pipeline caches
VKAPI_ATTR VkResult VKAPI_CALL
    vkCreatePipelineCache(VkDevice device,
                          const VkPipelineCacheCreateInfo *pCreateInfo,
                          const VkAllocationCallbacks *pAllocator,
                          VkPipelineCache *pPipelineCache) {
    layer_data *dev_data =
        get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->CreatePipelineCache(
        device, pCreateInfo, pAllocator, pPipelineCache);
    return result;
}

VKAPI_ATTR void VKAPI_CALL vkDestroyPipelineCache(
    VkDevice                                    device,
    VkPipelineCache                             pipelineCache,
    const VkAllocationCallbacks* pAllocator)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    dev_data->device_dispatch_table->DestroyPipelineCache(device, pipelineCache, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetPipelineCacheData(
    VkDevice                                    device,
    VkPipelineCache                             pipelineCache,
    size_t*                                     pDataSize,
    void*                                       pData)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->GetPipelineCacheData(device, pipelineCache, pDataSize, pData);
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL vkMergePipelineCaches(
    VkDevice                                    device,
    VkPipelineCache                             dstCache,
    uint32_t                                    srcCacheCount,
    const VkPipelineCache*                      pSrcCaches)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->MergePipelineCaches(device, dstCache, srcCacheCount, pSrcCaches);
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateGraphicsPipelines(
    VkDevice                            device,
    VkPipelineCache                     pipelineCache,
    uint32_t                            count,
    const VkGraphicsPipelineCreateInfo *pCreateInfos,
    const VkAllocationCallbacks        *pAllocator,
    VkPipeline                         *pPipelines)
{
    VkResult result = VK_SUCCESS;
    //TODO What to do with pipelineCache?
    // The order of operations here is a little convoluted but gets the job done
    //  1. Pipeline create state is first shadowed into PIPELINE_NODE struct
    //  2. Create state is then validated (which uses flags setup during shadowing)
    //  3. If everything looks good, we'll then create the pipeline and add NODE to pipelineMap
    VkBool32 skipCall = VK_FALSE;
    // TODO : Improve this data struct w/ unique_ptrs so cleanup below is automatic
    vector<PIPELINE_NODE*> pPipeNode(count);
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);

    uint32_t i=0;
    loader_platform_thread_lock_mutex(&globalLock);

    for (i=0; i<count; i++) {
        pPipeNode[i] = initGraphicsPipeline(dev_data, &pCreateInfos[i], NULL);
        skipCall |= verifyPipelineCreateState(dev_data, device, pPipeNode[i]);
    }

    if (VK_FALSE == skipCall) {
        loader_platform_thread_unlock_mutex(&globalLock);
        result = dev_data->device_dispatch_table->CreateGraphicsPipelines(device,
            pipelineCache, count, pCreateInfos, pAllocator, pPipelines);
        loader_platform_thread_lock_mutex(&globalLock);
        for (i=0; i<count; i++) {
            pPipeNode[i]->pipeline = pPipelines[i];
            dev_data->pipelineMap[pPipeNode[i]->pipeline] = pPipeNode[i];
        }
        loader_platform_thread_unlock_mutex(&globalLock);
    } else {
        for (i=0; i<count; i++) {
            if (pPipeNode[i]) {
                // If we allocated a pipeNode, need to clean it up here
                delete[] pPipeNode[i]->pVertexBindingDescriptions;
                delete[] pPipeNode[i]->pVertexAttributeDescriptions;
                delete[] pPipeNode[i]->pAttachments;
                delete pPipeNode[i];
            }
        }
        loader_platform_thread_unlock_mutex(&globalLock);
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateComputePipelines(
    VkDevice                            device,
    VkPipelineCache                     pipelineCache,
    uint32_t                            count,
    const VkComputePipelineCreateInfo  *pCreateInfos,
    const VkAllocationCallbacks        *pAllocator,
    VkPipeline                         *pPipelines)
{
    VkResult result   = VK_SUCCESS;
    VkBool32 skipCall = VK_FALSE;

    // TODO : Improve this data struct w/ unique_ptrs so cleanup below is automatic
    vector<PIPELINE_NODE*> pPipeNode(count);
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);

    uint32_t i=0;
    loader_platform_thread_lock_mutex(&globalLock);
    for (i=0; i<count; i++) {
        // TODO: Verify compute stage bits

        // Create and initialize internal tracking data structure
        pPipeNode[i] = new PIPELINE_NODE;
        memcpy(&pPipeNode[i]->computePipelineCI, (const void*)&pCreateInfos[i], sizeof(VkComputePipelineCreateInfo));

        // TODO: Add Compute Pipeline Verification
        // skipCall |= verifyPipelineCreateState(dev_data, device, pPipeNode[i]);
    }

    if (VK_FALSE == skipCall) {
        loader_platform_thread_unlock_mutex(&globalLock);
        result = dev_data->device_dispatch_table->CreateComputePipelines(device, pipelineCache, count, pCreateInfos, pAllocator, pPipelines);
        loader_platform_thread_lock_mutex(&globalLock);
        for (i=0; i<count; i++) {
            pPipeNode[i]->pipeline = pPipelines[i];
            dev_data->pipelineMap[pPipeNode[i]->pipeline] = pPipeNode[i];
        }
        loader_platform_thread_unlock_mutex(&globalLock);
    } else {
        for (i=0; i<count; i++) {
            if (pPipeNode[i]) {
                // Clean up any locally allocated data structures
                delete pPipeNode[i];
            }
        }
        loader_platform_thread_unlock_mutex(&globalLock);
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateSampler(VkDevice device, const VkSamplerCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSampler* pSampler)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->CreateSampler(device, pCreateInfo, pAllocator, pSampler);
    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        dev_data->sampleMap[*pSampler] = unique_ptr<SAMPLER_NODE>(new SAMPLER_NODE(pSampler, pCreateInfo));
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateDescriptorSetLayout(VkDevice device, const VkDescriptorSetLayoutCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorSetLayout* pSetLayout)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->CreateDescriptorSetLayout(device, pCreateInfo, pAllocator, pSetLayout);
    if (VK_SUCCESS == result) {
        // TODOSC : Capture layout bindings set
        LAYOUT_NODE* pNewNode = new LAYOUT_NODE;
        if (NULL == pNewNode) {
            if (log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT, (uint64_t) *pSetLayout, __LINE__, DRAWSTATE_OUT_OF_MEMORY, "DS",
                    "Out of memory while attempting to allocate LAYOUT_NODE in vkCreateDescriptorSetLayout()"))
                return VK_ERROR_VALIDATION_FAILED_EXT;
        }
        memcpy((void*)&pNewNode->createInfo, pCreateInfo, sizeof(VkDescriptorSetLayoutCreateInfo));
        pNewNode->createInfo.pBindings = new VkDescriptorSetLayoutBinding[pCreateInfo->bindingCount];
        memcpy((void*)pNewNode->createInfo.pBindings, pCreateInfo->pBindings, sizeof(VkDescriptorSetLayoutBinding)*pCreateInfo->bindingCount);
        // g++ does not like reserve with size 0
        if (pCreateInfo->bindingCount)
            pNewNode->bindingToIndexMap.reserve(pCreateInfo->bindingCount);
        uint32_t totalCount = 0;
        for (uint32_t i = 0; i < pCreateInfo->bindingCount; i++) {
            if (!pNewNode->bindingToIndexMap.emplace(pCreateInfo->pBindings[i].binding, i).second) {
                if (log_msg(
                        dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                        VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT,
                        (uint64_t)*pSetLayout, __LINE__,
                        DRAWSTATE_INVALID_LAYOUT, "DS",
                        "duplicated binding number in "
                        "VkDescriptorSetLayoutBinding"))
                    return VK_ERROR_VALIDATION_FAILED_EXT;
            } else {
                pNewNode->bindingToIndexMap[pCreateInfo->pBindings[i].binding] = i;
            }
            totalCount += pCreateInfo->pBindings[i].descriptorCount;
            if (pCreateInfo->pBindings[i].pImmutableSamplers) {
                VkSampler** ppIS = (VkSampler**)&pNewNode->createInfo.pBindings[i].pImmutableSamplers;
                *ppIS = new VkSampler[pCreateInfo->pBindings[i].descriptorCount];
                memcpy(*ppIS, pCreateInfo->pBindings[i].pImmutableSamplers, pCreateInfo->pBindings[i].descriptorCount*sizeof(VkSampler));
            }
        }
        pNewNode->layout = *pSetLayout;
        pNewNode->startIndex = 0;
        if (totalCount > 0) {
            pNewNode->descriptorTypes.resize(totalCount);
            pNewNode->stageFlags.resize(totalCount);
            uint32_t offset = 0;
            uint32_t j = 0;
            VkDescriptorType dType;
            for (uint32_t i=0; i<pCreateInfo->bindingCount; i++) {
                dType = pCreateInfo->pBindings[i].descriptorType;
                for (j = 0; j < pCreateInfo->pBindings[i].descriptorCount; j++) {
                    pNewNode->descriptorTypes[offset + j] = dType;
                    pNewNode->stageFlags[offset + j] = pCreateInfo->pBindings[i].stageFlags;
                    if ((dType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC) ||
                        (dType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC)) {
                        pNewNode->dynamicDescriptorCount++;
                    }
                }
                offset += j;
            }
            pNewNode->endIndex = pNewNode->startIndex + totalCount - 1;
        } else { // no descriptors
            pNewNode->endIndex = 0;
        }
        // Put new node at Head of global Layer list
        loader_platform_thread_lock_mutex(&globalLock);
        dev_data->descriptorSetLayoutMap[*pSetLayout] = pNewNode;
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreatePipelineLayout(VkDevice device, const VkPipelineLayoutCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkPipelineLayout* pPipelineLayout)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->CreatePipelineLayout(device, pCreateInfo, pAllocator, pPipelineLayout);
    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        // TODOSC : Merge capture of the setLayouts per pipeline
        PIPELINE_LAYOUT_NODE& plNode = dev_data->pipelineLayoutMap[*pPipelineLayout];
        plNode.descriptorSetLayouts.resize(pCreateInfo->setLayoutCount);
        uint32_t i = 0;
        for (i=0; i<pCreateInfo->setLayoutCount; ++i) {
            plNode.descriptorSetLayouts[i] = pCreateInfo->pSetLayouts[i];
        }
        plNode.pushConstantRanges.resize(pCreateInfo->pushConstantRangeCount);
        for (i=0; i<pCreateInfo->pushConstantRangeCount; ++i) {
            plNode.pushConstantRanges[i] = pCreateInfo->pPushConstantRanges[i];
        }
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateDescriptorPool(VkDevice device, const VkDescriptorPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorPool* pDescriptorPool)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->CreateDescriptorPool(device, pCreateInfo, pAllocator, pDescriptorPool);
    if (VK_SUCCESS == result) {
        // Insert this pool into Global Pool LL at head
        if (log_msg(dev_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT, (uint64_t) *pDescriptorPool, __LINE__, DRAWSTATE_OUT_OF_MEMORY, "DS",
                "Created Descriptor Pool %#" PRIxLEAST64, (uint64_t) *pDescriptorPool))
            return VK_ERROR_VALIDATION_FAILED_EXT;
        DESCRIPTOR_POOL_NODE* pNewNode = new DESCRIPTOR_POOL_NODE(*pDescriptorPool, pCreateInfo);
        if (NULL == pNewNode) {
            if (log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT, (uint64_t) *pDescriptorPool, __LINE__, DRAWSTATE_OUT_OF_MEMORY, "DS",
                    "Out of memory while attempting to allocate DESCRIPTOR_POOL_NODE in vkCreateDescriptorPool()"))
                return VK_ERROR_VALIDATION_FAILED_EXT;
        } else {
            loader_platform_thread_lock_mutex(&globalLock);
            dev_data->descriptorPoolMap[*pDescriptorPool] = pNewNode;
            loader_platform_thread_unlock_mutex(&globalLock);
        }
    } else {
        // Need to do anything if pool create fails?
    }
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkResetDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool, VkDescriptorPoolResetFlags flags)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->ResetDescriptorPool(device, descriptorPool, flags);
    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        clearDescriptorPool(dev_data, device, descriptorPool, flags);
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkAllocateDescriptorSets(VkDevice device, const VkDescriptorSetAllocateInfo* pAllocateInfo, VkDescriptorSet* pDescriptorSets)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);

    loader_platform_thread_lock_mutex(&globalLock);
    // Verify that requested descriptorSets are available in pool
    DESCRIPTOR_POOL_NODE *pPoolNode = getPoolNode(dev_data, pAllocateInfo->descriptorPool);
    if (!pPoolNode) {
        skipCall |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT, (uint64_t) pAllocateInfo->descriptorPool, __LINE__, DRAWSTATE_INVALID_POOL, "DS",
                "Unable to find pool node for pool %#" PRIxLEAST64 " specified in vkAllocateDescriptorSets() call", (uint64_t) pAllocateInfo->descriptorPool);
    } else { // Make sure pool has all the available descriptors before calling down chain
        skipCall |= validate_descriptor_availability_in_pool(dev_data, pPoolNode, pAllocateInfo->descriptorSetCount, pAllocateInfo->pSetLayouts);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = dev_data->device_dispatch_table->AllocateDescriptorSets(device, pAllocateInfo, pDescriptorSets);
    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        DESCRIPTOR_POOL_NODE *pPoolNode = getPoolNode(dev_data, pAllocateInfo->descriptorPool);
        if (pPoolNode) {
            if (pAllocateInfo->descriptorSetCount == 0) {
                log_msg(dev_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT, pAllocateInfo->descriptorSetCount, __LINE__, DRAWSTATE_NONE, "DS",
                        "AllocateDescriptorSets called with 0 count");
            }
            for (uint32_t i = 0; i < pAllocateInfo->descriptorSetCount; i++) {
                log_msg(dev_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT, (uint64_t) pDescriptorSets[i], __LINE__, DRAWSTATE_NONE, "DS",
                        "Created Descriptor Set %#" PRIxLEAST64, (uint64_t) pDescriptorSets[i]);
                // Create new set node and add to head of pool nodes
                SET_NODE* pNewNode = new SET_NODE;
                if (NULL == pNewNode) {
                    if (log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT, (uint64_t) pDescriptorSets[i], __LINE__, DRAWSTATE_OUT_OF_MEMORY, "DS",
                            "Out of memory while attempting to allocate SET_NODE in vkAllocateDescriptorSets()"))
                        return VK_ERROR_VALIDATION_FAILED_EXT;
                } else {
                    // TODO : Pool should store a total count of each type of Descriptor available
                    //  When descriptors are allocated, decrement the count and validate here
                    //  that the count doesn't go below 0. One reset/free need to bump count back up.
                    // Insert set at head of Set LL for this pool
                    pNewNode->pNext = pPoolNode->pSets;
                    pNewNode->in_use.store(0);
                    pPoolNode->pSets = pNewNode;
                    LAYOUT_NODE* pLayout = getLayoutNode(dev_data, pAllocateInfo->pSetLayouts[i]);
                    if (NULL == pLayout) {
                        if (log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT, (uint64_t) pAllocateInfo->pSetLayouts[i], __LINE__, DRAWSTATE_INVALID_LAYOUT, "DS",
                                "Unable to find set layout node for layout %#" PRIxLEAST64 " specified in vkAllocateDescriptorSets() call", (uint64_t) pAllocateInfo->pSetLayouts[i]))
                            return VK_ERROR_VALIDATION_FAILED_EXT;
                    }
                    pNewNode->pLayout = pLayout;
                    pNewNode->pool = pAllocateInfo->descriptorPool;
                    pNewNode->set = pDescriptorSets[i];
                    pNewNode->descriptorCount = (pLayout->createInfo.bindingCount != 0) ? pLayout->endIndex + 1 : 0;
                    if (pNewNode->descriptorCount) {
                        size_t descriptorArraySize = sizeof(GENERIC_HEADER*)*pNewNode->descriptorCount;
                        pNewNode->ppDescriptors = new GENERIC_HEADER*[descriptorArraySize];
                        memset(pNewNode->ppDescriptors, 0, descriptorArraySize);
                    }
                    dev_data->setMap[pDescriptorSets[i]] = pNewNode;
                }
            }
        }
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkFreeDescriptorSets(VkDevice device, VkDescriptorPool descriptorPool, uint32_t count, const VkDescriptorSet* pDescriptorSets)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    // Make sure that no sets being destroyed are in-flight
    loader_platform_thread_lock_mutex(&globalLock);
    for (uint32_t i=0; i<count; ++i)
        skipCall |= validateIdleDescriptorSet(dev_data, pDescriptorSets[i], "vkFreeDesriptorSets");
    DESCRIPTOR_POOL_NODE *pPoolNode = getPoolNode(dev_data, descriptorPool);
    if (pPoolNode && !(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT & pPoolNode->createInfo.flags)) {
        // Can't Free from a NON_FREE pool
        skipCall |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, (uint64_t)device, __LINE__, DRAWSTATE_CANT_FREE_FROM_NON_FREE_POOL, "DS",
                    "It is invalid to call vkFreeDescriptorSets() with a pool created without setting VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT.");
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE != skipCall)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = dev_data->device_dispatch_table->FreeDescriptorSets(device, descriptorPool, count, pDescriptorSets);
    if (VK_SUCCESS == result) {
        // For each freed descriptor add it back into the pool as available
        loader_platform_thread_lock_mutex(&globalLock);
        for (uint32_t i=0; i<count; ++i) {
            SET_NODE* pSet = dev_data->setMap[pDescriptorSets[i]]; // getSetNode() without locking
            invalidateBoundCmdBuffers(dev_data, pSet);
            LAYOUT_NODE* pLayout = pSet->pLayout;
            uint32_t typeIndex = 0, poolSizeCount = 0;
            for (uint32_t j=0; j<pLayout->createInfo.bindingCount; ++j) {
                typeIndex = static_cast<uint32_t>(pLayout->createInfo.pBindings[j].descriptorType);
                poolSizeCount = pLayout->createInfo.pBindings[j].descriptorCount;
                pPoolNode->availableDescriptorTypeCount[typeIndex] += poolSizeCount;
            }
        }
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    // TODO : Any other clean-up or book-keeping to do here?
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkUpdateDescriptorSets(VkDevice device, uint32_t descriptorWriteCount, const VkWriteDescriptorSet* pDescriptorWrites, uint32_t descriptorCopyCount, const VkCopyDescriptorSet* pDescriptorCopies)
{
    // dsUpdate will return VK_TRUE only if a bailout error occurs, so we want to call down tree when update returns VK_FALSE
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    VkBool32 rtn = dsUpdate(dev_data,
                            device,
                            descriptorWriteCount,
                            pDescriptorWrites,
                            descriptorCopyCount,
                            pDescriptorCopies);
    loader_platform_thread_unlock_mutex(&globalLock);
    if (!rtn) {
        dev_data->device_dispatch_table->UpdateDescriptorSets(device, descriptorWriteCount, pDescriptorWrites, descriptorCopyCount, pDescriptorCopies);
    }
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkAllocateCommandBuffers(VkDevice device, const VkCommandBufferAllocateInfo* pCreateInfo, VkCommandBuffer* pCommandBuffer)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->AllocateCommandBuffers(device, pCreateInfo, pCommandBuffer);
    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        for (uint32_t i = 0; i < pCreateInfo->commandBufferCount; i++) {
            // Validate command pool
            if (dev_data->commandPoolMap.find(pCreateInfo->commandPool) != dev_data->commandPoolMap.end()) {
                // Add command buffer to its commandPool map
                dev_data->commandPoolMap[pCreateInfo->commandPool].commandBuffers.push_back(pCommandBuffer[i]);
                GLOBAL_CB_NODE* pCB = new GLOBAL_CB_NODE;
                // Add command buffer to map
                dev_data->commandBufferMap[pCommandBuffer[i]] = pCB;
                resetCB(dev_data, pCommandBuffer[i]);
                pCB->createInfo    = *pCreateInfo;
                pCB->device        = device;
            }
        }
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkBeginCommandBuffer(VkCommandBuffer commandBuffer, const VkCommandBufferBeginInfo* pBeginInfo)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    // Validate command buffer level
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->createInfo.level != VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
            // Secondary Command Buffer
            const VkCommandBufferInheritanceInfo *pInfo = pBeginInfo->pInheritanceInfo;
            if (!pInfo) {
                skipCall |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, reinterpret_cast<uint64_t>(commandBuffer), __LINE__,
                                    DRAWSTATE_BEGIN_CB_INVALID_STATE, "DS", "vkBeginCommandBuffer(): Secondary Command Buffer (%p) must have inheritance info.",
                                    reinterpret_cast<void*>(commandBuffer));
            } else {
                if (pBeginInfo->flags & VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT) {
                    if (!pInfo->renderPass) { // renderpass should NOT be null for an Secondary CB
                        skipCall |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, reinterpret_cast<uint64_t>(commandBuffer),
                                            __LINE__, DRAWSTATE_BEGIN_CB_INVALID_STATE, "DS",
                                            "vkBeginCommandBuffer(): Secondary Command Buffers (%p) must specify a valid renderpass parameter.", reinterpret_cast<void*>(commandBuffer));
                    }
                    if (!pInfo->framebuffer) { // framebuffer may be null for an Secondary CB, but this affects perf
                        skipCall |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, reinterpret_cast<uint64_t>(commandBuffer),
                                            __LINE__, DRAWSTATE_BEGIN_CB_INVALID_STATE, "DS", 
                                            "vkBeginCommandBuffer(): Secondary Command Buffers (%p) may perform better if a valid framebuffer parameter is specified.",
                                            reinterpret_cast<void*>(commandBuffer));
                    } else {
                        string errorString = "";
                        VkRenderPass fbRP = dev_data->frameBufferMap[pInfo->framebuffer]->renderPass;
                        if (!verify_renderpass_compatibility(dev_data, fbRP, pInfo->renderPass, errorString)) {
                            // renderPass that framebuffer was created with must be compatible with local renderPass
                            skipCall |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, reinterpret_cast<uint64_t>(commandBuffer),
                                                __LINE__, DRAWSTATE_RENDERPASS_INCOMPATIBLE, "DS",
                                                "vkBeginCommandBuffer(): Secondary Command Buffer (%p) renderPass (%#" PRIxLEAST64 ") is incompatible w/ framebuffer (%#" PRIxLEAST64
                                                ") w/ render pass (%#" PRIxLEAST64 ") due to: %s", reinterpret_cast<void*>(commandBuffer), (uint64_t)(pInfo->renderPass),
                                                (uint64_t)(pInfo->framebuffer), (uint64_t)(fbRP), errorString.c_str());
                        }
                    }
                }
                if ((pInfo->occlusionQueryEnable == VK_FALSE || dev_data->physDevProperties.features.occlusionQueryPrecise == VK_FALSE) && (pInfo->queryFlags & VK_QUERY_CONTROL_PRECISE_BIT)) {
                    skipCall |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, reinterpret_cast<uint64_t>(commandBuffer),
                                        __LINE__, DRAWSTATE_BEGIN_CB_INVALID_STATE, "DS",
                                        "vkBeginCommandBuffer(): Secondary Command Buffer (%p) must not have VK_QUERY_CONTROL_PRECISE_BIT if occulusionQuery is disabled or the device does not "
                                        "support precise occlusion queries.", reinterpret_cast<void*>(commandBuffer));
                }
            }
            if (pInfo && pInfo->renderPass != VK_NULL_HANDLE) {
                auto rp_data = dev_data->renderPassMap.find(pInfo->renderPass);
                if (rp_data != dev_data->renderPassMap.end() && rp_data->second && rp_data->second->pCreateInfo) {
                    if (pInfo->subpass >= rp_data->second->pCreateInfo->subpassCount) {
                        skipCall |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, (uint64_t)commandBuffer,
                            __LINE__, DRAWSTATE_BEGIN_CB_INVALID_STATE, "DS",
                            "vkBeginCommandBuffer(): Secondary Command Buffers (%p) must has a subpass index (%d) that is less than the number of subpasses (%d).",
                            (void*)commandBuffer, pInfo->subpass, rp_data->second->pCreateInfo->subpassCount);
                    }
                }
            }
        }
        if (CB_RECORDING == pCB->state) {
            skipCall |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, (uint64_t)commandBuffer, __LINE__, DRAWSTATE_BEGIN_CB_INVALID_STATE, "DS",
                "vkBeginCommandBuffer(): Cannot call Begin on CB (%#" PRIxLEAST64 ") in the RECORDING state. Must first call vkEndCommandBuffer().", (uint64_t)commandBuffer);
        } else if (CB_RECORDED == pCB->state) {
            VkCommandPool cmdPool = pCB->createInfo.commandPool;
            if (!(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT & dev_data->commandPoolMap[cmdPool].createFlags)) {
                skipCall |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, (uint64_t)commandBuffer,
                    __LINE__, DRAWSTATE_INVALID_COMMAND_BUFFER_RESET, "DS",
                    "Call to vkBeginCommandBuffer() on command buffer (%#" PRIxLEAST64 ") attempts to implicitly reset cmdBuffer created from command pool (%#" PRIxLEAST64 ") that does NOT have the VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT bit set.",
                    (uint64_t) commandBuffer, (uint64_t) cmdPool);
            }
            resetCB(dev_data, commandBuffer);
        }
        // Set updated state here in case implicit reset occurs above
        pCB->state = CB_RECORDING;
        pCB->beginInfo = *pBeginInfo;
        if (pCB->beginInfo.pInheritanceInfo) {
            pCB->inheritanceInfo = *(pCB->beginInfo.pInheritanceInfo);
            pCB->beginInfo.pInheritanceInfo = &pCB->inheritanceInfo;
        }
    } else {
        skipCall |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, (uint64_t)commandBuffer, __LINE__, DRAWSTATE_INVALID_COMMAND_BUFFER, "DS",
                "In vkBeginCommandBuffer() and unable to find CommandBuffer Node for CB %p!", (void*)commandBuffer);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE != skipCall) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    VkResult result = dev_data->device_dispatch_table->BeginCommandBuffer(commandBuffer, pBeginInfo);
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEndCommandBuffer(VkCommandBuffer commandBuffer)
{
    VkBool32 skipCall = VK_FALSE;
    VkResult result = VK_SUCCESS;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state != CB_RECORDING) {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkEndCommandBuffer()");
        }
        for (auto query : pCB->activeQueries) {
            skipCall |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, DRAWSTATE_INVALID_QUERY, "DS",
                "Ending command buffer with in progress query: queryPool %" PRIu64 ", index %d", (uint64_t)(query.pool), query.index);
        }
    }
    if (VK_FALSE == skipCall) {
        loader_platform_thread_unlock_mutex(&globalLock);
        result = dev_data->device_dispatch_table->EndCommandBuffer(commandBuffer);
        loader_platform_thread_lock_mutex(&globalLock);
        if (VK_SUCCESS == result) {
            pCB->state = CB_RECORDED;
            // Reset CB status flags
            pCB->status = 0;
            printCB(dev_data, commandBuffer);
        }
    } else {
        result = VK_ERROR_VALIDATION_FAILED_EXT;
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkResetCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferResetFlags flags)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    VkCommandPool cmdPool = pCB->createInfo.commandPool;
    if (!(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT & dev_data->commandPoolMap[cmdPool].createFlags)) {
        skipCall |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, (uint64_t) commandBuffer,
                        __LINE__, DRAWSTATE_INVALID_COMMAND_BUFFER_RESET, "DS",
                        "Attempt to reset command buffer (%#" PRIxLEAST64 ") created from command pool (%#" PRIxLEAST64 ") that does NOT have the VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT bit set.",
                        (uint64_t) commandBuffer, (uint64_t) cmdPool);
    }
    if (dev_data->globalInFlightCmdBuffers.count(commandBuffer)) {
        skipCall |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, (uint64_t) commandBuffer,
                        __LINE__, DRAWSTATE_INVALID_COMMAND_BUFFER_RESET, "DS",
                        "Attempt to reset command buffer (%#" PRIxLEAST64 ") which is in use.", reinterpret_cast<uint64_t>(commandBuffer));
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    if (skipCall != VK_FALSE)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = dev_data->device_dispatch_table->ResetCommandBuffer(commandBuffer, flags);
    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        resetCB(dev_data, commandBuffer);
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdBindPipeline(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        skipCall |= addCmd(dev_data, pCB, CMD_BINDPIPELINE, "vkCmdBindPipeline()");
        if ((VK_PIPELINE_BIND_POINT_COMPUTE == pipelineBindPoint) && (pCB->activeRenderPass)) {
            skipCall |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT, (uint64_t) pipeline,
                                __LINE__, DRAWSTATE_INVALID_RENDERPASS_CMD, "DS",
                                "Incorrectly binding compute pipeline (%#" PRIxLEAST64 ") during active RenderPass (%#" PRIxLEAST64 ")",
                                (uint64_t) pipeline, (uint64_t) pCB->activeRenderPass);
        } else if (VK_PIPELINE_BIND_POINT_GRAPHICS == pipelineBindPoint) {
            skipCall |=  outsideRenderPass(dev_data, pCB, "vkCmdBindPipeline");
        }

        PIPELINE_NODE* pPN = getPipeline(dev_data, pipeline);
        if (pPN) {
            pCB->lastBoundPipeline = pipeline;
            set_cb_pso_status(pCB, pPN);
            skipCall |= validatePipelineState(dev_data, pCB, pipelineBindPoint, pipeline);
        } else {
            skipCall |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT,
                                (uint64_t) pipeline, __LINE__, DRAWSTATE_INVALID_PIPELINE, "DS",
                                "Attempt to bind Pipeline %#" PRIxLEAST64 " that doesn't exist!", (uint64_t)(pipeline));
        }
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdBindPipeline(commandBuffer, pipelineBindPoint, pipeline);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetViewport(
    VkCommandBuffer                         commandBuffer,
    uint32_t                                firstViewport,
    uint32_t                                viewportCount,
    const VkViewport*                       pViewports)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        skipCall |= addCmd(dev_data, pCB, CMD_SETVIEWPORTSTATE, "vkCmdSetViewport()");
        pCB->status |= CBSTATUS_VIEWPORT_SET;
        pCB->viewports.resize(viewportCount);
        memcpy(pCB->viewports.data(), pViewports, viewportCount * sizeof(VkViewport));
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdSetViewport(commandBuffer, firstViewport, viewportCount, pViewports);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetScissor(
    VkCommandBuffer                         commandBuffer,
    uint32_t                                firstScissor,
    uint32_t                                scissorCount,
    const VkRect2D*                         pScissors)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        skipCall |= addCmd(dev_data, pCB, CMD_SETSCISSORSTATE, "vkCmdSetScissor()");
        pCB->status |= CBSTATUS_SCISSOR_SET;
        pCB->scissors.resize(scissorCount);
        memcpy(pCB->scissors.data(), pScissors, scissorCount * sizeof(VkRect2D));
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdSetScissor(commandBuffer, firstScissor, scissorCount, pScissors);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetLineWidth(VkCommandBuffer commandBuffer, float lineWidth)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        skipCall |= addCmd(dev_data, pCB, CMD_SETLINEWIDTHSTATE, "vkCmdSetLineWidth()");
        pCB->status |= CBSTATUS_LINE_WIDTH_SET;
        pCB->lineWidth = lineWidth;
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdSetLineWidth(commandBuffer, lineWidth);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetDepthBias(
    VkCommandBuffer                         commandBuffer,
    float                               depthBiasConstantFactor,
    float                               depthBiasClamp,
    float                               depthBiasSlopeFactor)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        skipCall |= addCmd(dev_data, pCB, CMD_SETDEPTHBIASSTATE, "vkCmdSetDepthBias()");
        pCB->status |= CBSTATUS_DEPTH_BIAS_SET;
        pCB->depthBiasConstantFactor = depthBiasConstantFactor;
        pCB->depthBiasClamp = depthBiasClamp;
        pCB->depthBiasSlopeFactor = depthBiasSlopeFactor;
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdSetDepthBias(commandBuffer, depthBiasConstantFactor, depthBiasClamp, depthBiasSlopeFactor);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetBlendConstants(VkCommandBuffer commandBuffer, const float blendConstants[4])
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        skipCall |= addCmd(dev_data, pCB, CMD_SETBLENDSTATE, "vkCmdSetBlendConstants()");
        pCB->status |= CBSTATUS_BLEND_SET;
        memcpy(pCB->blendConstants, blendConstants, 4 * sizeof(float));
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdSetBlendConstants(commandBuffer, blendConstants);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetDepthBounds(
    VkCommandBuffer                         commandBuffer,
    float                               minDepthBounds,
    float                               maxDepthBounds)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        skipCall |= addCmd(dev_data, pCB, CMD_SETDEPTHBOUNDSSTATE, "vkCmdSetDepthBounds()");
        pCB->status |= CBSTATUS_DEPTH_BOUNDS_SET;
        pCB->minDepthBounds = minDepthBounds;
        pCB->maxDepthBounds = maxDepthBounds;
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdSetDepthBounds(commandBuffer, minDepthBounds, maxDepthBounds);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetStencilCompareMask(
    VkCommandBuffer                         commandBuffer,
    VkStencilFaceFlags                  faceMask,
    uint32_t                            compareMask)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        skipCall |= addCmd(dev_data, pCB, CMD_SETSTENCILREADMASKSTATE, "vkCmdSetStencilCompareMask()");
        if (faceMask & VK_STENCIL_FACE_FRONT_BIT) {
            pCB->front.compareMask = compareMask;
        }
        if (faceMask & VK_STENCIL_FACE_BACK_BIT) {
            pCB->back.compareMask = compareMask;
        }
        /* TODO: Do we need to track front and back separately? */
        /* TODO: We aren't capturing the faceMask, do we need to? */
        pCB->status |= CBSTATUS_STENCIL_READ_MASK_SET;
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdSetStencilCompareMask(commandBuffer, faceMask, compareMask);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetStencilWriteMask(
    VkCommandBuffer                         commandBuffer,
    VkStencilFaceFlags                  faceMask,
    uint32_t                            writeMask)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        skipCall |= addCmd(dev_data, pCB, CMD_SETSTENCILWRITEMASKSTATE, "vkCmdSetStencilWriteMask()");
        if (faceMask & VK_STENCIL_FACE_FRONT_BIT) {
            pCB->front.writeMask = writeMask;
        }
        if (faceMask & VK_STENCIL_FACE_BACK_BIT) {
            pCB->back.writeMask = writeMask;
        }
        pCB->status |= CBSTATUS_STENCIL_WRITE_MASK_SET;
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdSetStencilWriteMask(commandBuffer, faceMask, writeMask);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetStencilReference(
    VkCommandBuffer                         commandBuffer,
    VkStencilFaceFlags                  faceMask,
    uint32_t                            reference)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        skipCall |= addCmd(dev_data, pCB, CMD_SETSTENCILREFERENCESTATE, "vkCmdSetStencilReference()");
        if (faceMask & VK_STENCIL_FACE_FRONT_BIT) {
            pCB->front.reference = reference;
        }
        if (faceMask & VK_STENCIL_FACE_BACK_BIT) {
            pCB->back.reference = reference;
        }
        pCB->status |= CBSTATUS_STENCIL_REFERENCE_SET;
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdSetStencilReference(commandBuffer, faceMask, reference);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdBindDescriptorSets(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t firstSet, uint32_t setCount, const VkDescriptorSet* pDescriptorSets, uint32_t dynamicOffsetCount, const uint32_t* pDynamicOffsets)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_RECORDING) {
            if ((VK_PIPELINE_BIND_POINT_COMPUTE == pipelineBindPoint) && (pCB->activeRenderPass)) {
                skipCall |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT) 0, 0, __LINE__, DRAWSTATE_INVALID_RENDERPASS_CMD, "DS",
                        "Incorrectly binding compute DescriptorSets during active RenderPass (%#" PRIxLEAST64 ")", (uint64_t) pCB->activeRenderPass);
            } else if (VK_PIPELINE_BIND_POINT_GRAPHICS == pipelineBindPoint) {
                skipCall |= outsideRenderPass(dev_data, pCB, "vkCmdBindDescriptorSets");
            }
            if (VK_FALSE == skipCall) {
                // Track total count of dynamic descriptor types to make sure we have an offset for each one
                uint32_t totalDynamicDescriptors = 0;
                string errorString = "";
                uint32_t lastSetIndex = firstSet+setCount-1;
                if (lastSetIndex >= pCB->boundDescriptorSets.size())
                    pCB->boundDescriptorSets.resize(lastSetIndex+1);
                VkDescriptorSet oldFinalBoundSet = pCB->boundDescriptorSets[lastSetIndex];
                for (uint32_t i=0; i<setCount; i++) {
                    SET_NODE* pSet = getSetNode(dev_data, pDescriptorSets[i]);
                    if (pSet) {
                        pCB->uniqueBoundSets.insert(pDescriptorSets[i]);
                        pSet->boundCmdBuffers.insert(commandBuffer);
                        pCB->lastBoundDescriptorSet = pDescriptorSets[i];
                        pCB->lastBoundPipelineLayout = layout;
                        pCB->boundDescriptorSets[i+firstSet] = pDescriptorSets[i];
                        skipCall |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT, (uint64_t) pDescriptorSets[i], __LINE__, DRAWSTATE_NONE, "DS",
                                "DS %#" PRIxLEAST64 " bound on pipeline %s", (uint64_t) pDescriptorSets[i], string_VkPipelineBindPoint(pipelineBindPoint));
                        if (!pSet->pUpdateStructs && (pSet->descriptorCount != 0)) {
                            skipCall |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT, (uint64_t) pDescriptorSets[i], __LINE__, DRAWSTATE_DESCRIPTOR_SET_NOT_UPDATED, "DS",
                                    "DS %#" PRIxLEAST64 " bound but it was never updated. You may want to either update it or not bind it.", (uint64_t) pDescriptorSets[i]);
                        }
                        // Verify that set being bound is compatible with overlapping setLayout of pipelineLayout
                        if (!verify_set_layout_compatibility(dev_data, pSet, layout, i+firstSet, errorString)) {
                            skipCall |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT, (uint64_t) pDescriptorSets[i], __LINE__, DRAWSTATE_PIPELINE_LAYOUTS_INCOMPATIBLE, "DS",
                                    "descriptorSet #%u being bound is not compatible with overlapping layout in pipelineLayout due to: %s", i, errorString.c_str());
                        }
                        if (pSet->pLayout->dynamicDescriptorCount) {
                            // First make sure we won't overstep bounds of pDynamicOffsets array
                            if ((totalDynamicDescriptors + pSet->pLayout->dynamicDescriptorCount) > dynamicOffsetCount) {
                                skipCall |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT, (uint64_t) pDescriptorSets[i], __LINE__, DRAWSTATE_INVALID_DYNAMIC_OFFSET_COUNT, "DS",
                                    "descriptorSet #%u (%#" PRIxLEAST64 ") requires %u dynamicOffsets, but only %u dynamicOffsets are left in pDynamicOffsets array. There must be one dynamic offset for each dynamic descriptor being bound.",
                                        i, (uint64_t) pDescriptorSets[i], pSet->pLayout->dynamicDescriptorCount, (dynamicOffsetCount - totalDynamicDescriptors));
                            } else { // Validate and store dynamic offsets with the set
                                // Validate Dynamic Offset Minimums
                                uint32_t cur_dyn_offset = totalDynamicDescriptors;
                                for (uint32_t d = 0; d < pSet->descriptorCount; d++) {
                                    if (pSet->pLayout->descriptorTypes[d] == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC) {
                                        if (vk_safe_modulo(pDynamicOffsets[cur_dyn_offset], dev_data->physDevProperties.properties.limits.minUniformBufferOffsetAlignment) != 0) {
                                            skipCall |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, 0,
                                            __LINE__, DRAWSTATE_INVALID_UNIFORM_BUFFER_OFFSET, "DS",
                                            "vkCmdBindDescriptorSets(): pDynamicOffsets[%d] is %d but must be a multiple of device limit minUniformBufferOffsetAlignment %#" PRIxLEAST64,
                                            cur_dyn_offset, pDynamicOffsets[cur_dyn_offset], dev_data->physDevProperties.properties.limits.minUniformBufferOffsetAlignment);
                                        }
                                        cur_dyn_offset++;
                                    } else if (pSet->pLayout->descriptorTypes[d] == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC) {
                                        if (vk_safe_modulo(pDynamicOffsets[cur_dyn_offset], dev_data->physDevProperties.properties.limits.minStorageBufferOffsetAlignment) != 0) {
                                            skipCall |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, 0,
                                            __LINE__, DRAWSTATE_INVALID_STORAGE_BUFFER_OFFSET, "DS",
                                            "vkCmdBindDescriptorSets(): pDynamicOffsets[%d] is %d but must be a multiple of device limit minStorageBufferOffsetAlignment %#" PRIxLEAST64,
                                            cur_dyn_offset, pDynamicOffsets[cur_dyn_offset], dev_data->physDevProperties.properties.limits.minStorageBufferOffsetAlignment);
                                        }
                                        cur_dyn_offset++;
                                    }
                                }
                                // Keep running total of dynamic descriptor count to verify at the end
                                totalDynamicDescriptors += pSet->pLayout->dynamicDescriptorCount;
                            }
                        }
                    } else {
                        skipCall |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT, (uint64_t) pDescriptorSets[i], __LINE__, DRAWSTATE_INVALID_SET, "DS",
                                "Attempt to bind DS %#" PRIxLEAST64 " that doesn't exist!", (uint64_t) pDescriptorSets[i]);
                    }
                }
                skipCall |= addCmd(dev_data, pCB, CMD_BINDDESCRIPTORSETS, "vkCmdBindDescrsiptorSets()");
                // For any previously bound sets, need to set them to "invalid" if they were disturbed by this update
                if (firstSet > 0) { // Check set #s below the first bound set
                    for (uint32_t i=0; i<firstSet; ++i) {
                        if (pCB->boundDescriptorSets[i] && !verify_set_layout_compatibility(dev_data, dev_data->setMap[pCB->boundDescriptorSets[i]], layout, i, errorString)) {
                            skipCall |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT, (uint64_t) pCB->boundDescriptorSets[i], __LINE__, DRAWSTATE_NONE, "DS",
                                "DescriptorSetDS %#" PRIxLEAST64 " previously bound as set #%u was disturbed by newly bound pipelineLayout (%#" PRIxLEAST64 ")", (uint64_t) pCB->boundDescriptorSets[i], i, (uint64_t) layout);
                            pCB->boundDescriptorSets[i] = VK_NULL_HANDLE;
                        }
                    }
                }
                // Check if newly last bound set invalidates any remaining bound sets
                if ((pCB->boundDescriptorSets.size()-1) > (lastSetIndex)) {
                    if (oldFinalBoundSet && !verify_set_layout_compatibility(dev_data, dev_data->setMap[oldFinalBoundSet], layout, lastSetIndex, errorString)) {
                        skipCall |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT, (uint64_t) oldFinalBoundSet, __LINE__, DRAWSTATE_NONE, "DS",
                            "DescriptorSetDS %#" PRIxLEAST64 " previously bound as set #%u is incompatible with set %#" PRIxLEAST64 " newly bound as set #%u so set #%u and any subsequent sets were disturbed by newly bound pipelineLayout (%#" PRIxLEAST64 ")", (uint64_t) oldFinalBoundSet, lastSetIndex, (uint64_t) pCB->boundDescriptorSets[lastSetIndex], lastSetIndex, lastSetIndex+1, (uint64_t) layout);
                        pCB->boundDescriptorSets.resize(lastSetIndex+1);
                    }
                }
                //  dynamicOffsetCount must equal the total number of dynamic descriptors in the sets being bound
                if (totalDynamicDescriptors != dynamicOffsetCount) {
                    skipCall |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, (uint64_t) commandBuffer, __LINE__, DRAWSTATE_INVALID_DYNAMIC_OFFSET_COUNT, "DS",
                            "Attempting to bind %u descriptorSets with %u dynamic descriptors, but dynamicOffsetCount is %u. It should exactly match the number of dynamic descriptors.", setCount, totalDynamicDescriptors, dynamicOffsetCount);
                }
                if (dynamicOffsetCount) {
                    // Save dynamicOffsets bound to this CB
                    pCB->dynamicOffsets.assign(pDynamicOffsets, pDynamicOffsets + dynamicOffsetCount);
                }
            }
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdBindDescriptorSets()");
        }
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdBindDescriptorSets(commandBuffer, pipelineBindPoint, layout, firstSet, setCount, pDescriptorSets, dynamicOffsetCount, pDynamicOffsets);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdBindIndexBuffer(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        skipCall |= addCmd(dev_data, pCB, CMD_BINDINDEXBUFFER, "vkCmdBindIndexBuffer()");
        VkDeviceSize offset_align = 0;
        switch (indexType) {
            case VK_INDEX_TYPE_UINT16:
                offset_align = 2;
                break;
            case VK_INDEX_TYPE_UINT32:
                offset_align = 4;
                break;
            default:
                // ParamChecker should catch bad enum, we'll also throw alignment error below if offset_align stays 0
                break;
        }
        if (!offset_align || (offset % offset_align)) {
            skipCall |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT) 0, 0, __LINE__, DRAWSTATE_VTX_INDEX_ALIGNMENT_ERROR, "DS",
                "vkCmdBindIndexBuffer() offset (%#" PRIxLEAST64 ") does not fall on alignment (%s) boundary.", offset, string_VkIndexType(indexType));
        }
        pCB->status |= CBSTATUS_INDEX_BUFFER_BOUND;
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdBindIndexBuffer(commandBuffer, buffer, offset, indexType);
}

void updateResourceTracking(GLOBAL_CB_NODE* pCB, uint32_t firstBinding, uint32_t bindingCount, const VkBuffer* pBuffers) {
    uint32_t end = firstBinding + bindingCount;
    if (pCB->currentDrawData.buffers.size() < end) {
        pCB->currentDrawData.buffers.resize(end);
    }
    for (uint32_t i = 0; i < bindingCount; ++i) {
        pCB->currentDrawData.buffers[i + firstBinding] = pBuffers[i];
    }
}

void updateResourceTrackingOnDraw(GLOBAL_CB_NODE* pCB) {
    pCB->drawData.push_back(pCB->currentDrawData);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdBindVertexBuffers(
    VkCommandBuffer                             commandBuffer,
    uint32_t                                    firstBinding,
    uint32_t                                    bindingCount,
    const VkBuffer                             *pBuffers,
    const VkDeviceSize                         *pOffsets)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        addCmd(dev_data, pCB, CMD_BINDVERTEXBUFFER, "vkCmdBindVertexBuffer()");
        updateResourceTracking(pCB, firstBinding, bindingCount, pBuffers);
    } else {
        skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdBindVertexBuffer()");
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdBindVertexBuffers(commandBuffer, firstBinding, bindingCount, pBuffers, pOffsets);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdDraw(VkCommandBuffer commandBuffer, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        skipCall |= addCmd(dev_data, pCB, CMD_DRAW, "vkCmdDraw()");
        pCB->drawCount[DRAW]++;
        skipCall |= validate_draw_state(dev_data, pCB, VK_FALSE);
        // TODO : Need to pass commandBuffer as srcObj here
        skipCall |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, 0, __LINE__, DRAWSTATE_NONE, "DS",
                "vkCmdDraw() call #%" PRIu64 ", reporting DS state:", g_drawCount[DRAW]++);
        skipCall |= synchAndPrintDSConfig(dev_data, commandBuffer);
        if (VK_FALSE == skipCall) {
            updateResourceTrackingOnDraw(pCB);
        }
        skipCall |= outsideRenderPass(dev_data, pCB, "vkCmdDraw");
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdDraw(commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdDrawIndexed(VkCommandBuffer commandBuffer, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    VkBool32 skipCall = VK_FALSE;
    if (pCB) {
        skipCall |= addCmd(dev_data, pCB, CMD_DRAWINDEXED, "vkCmdDrawIndexed()");
        pCB->drawCount[DRAW_INDEXED]++;
        skipCall |= validate_draw_state(dev_data, pCB, VK_TRUE);
        // TODO : Need to pass commandBuffer as srcObj here
        skipCall |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, 0, __LINE__, DRAWSTATE_NONE, "DS",
                "vkCmdDrawIndexed() call #%" PRIu64 ", reporting DS state:", g_drawCount[DRAW_INDEXED]++);
        skipCall |= synchAndPrintDSConfig(dev_data, commandBuffer);
        if (VK_FALSE == skipCall) {
            updateResourceTrackingOnDraw(pCB);
        }
        skipCall |= outsideRenderPass(dev_data, pCB, "vkCmdDrawIndexed");
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdDrawIndexed(commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdDrawIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t count, uint32_t stride)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    VkBool32 skipCall = VK_FALSE;
    if (pCB) {
        skipCall |= addCmd(dev_data, pCB, CMD_DRAWINDIRECT, "vkCmdDrawIndirect()");
        pCB->drawCount[DRAW_INDIRECT]++;
        skipCall |= validate_draw_state(dev_data, pCB, VK_FALSE);
        // TODO : Need to pass commandBuffer as srcObj here
        skipCall |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, 0, __LINE__, DRAWSTATE_NONE, "DS",
                "vkCmdDrawIndirect() call #%" PRIu64 ", reporting DS state:", g_drawCount[DRAW_INDIRECT]++);
        skipCall |= synchAndPrintDSConfig(dev_data, commandBuffer);
        if (VK_FALSE == skipCall) {
            updateResourceTrackingOnDraw(pCB);
        }
        skipCall |= outsideRenderPass(dev_data, pCB, "vkCmdDrawIndirect");
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdDrawIndirect(commandBuffer, buffer, offset, count, stride);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdDrawIndexedIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t count, uint32_t stride)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        skipCall |= addCmd(dev_data, pCB, CMD_DRAWINDEXEDINDIRECT, "vkCmdDrawIndexedIndirect()");
        pCB->drawCount[DRAW_INDEXED_INDIRECT]++;
        loader_platform_thread_unlock_mutex(&globalLock);
        skipCall |= validate_draw_state(dev_data, pCB, VK_TRUE);
        loader_platform_thread_lock_mutex(&globalLock);
        // TODO : Need to pass commandBuffer as srcObj here
        skipCall |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, 0, __LINE__, DRAWSTATE_NONE, "DS",
                "vkCmdDrawIndexedIndirect() call #%" PRIu64 ", reporting DS state:", g_drawCount[DRAW_INDEXED_INDIRECT]++);
        skipCall |= synchAndPrintDSConfig(dev_data, commandBuffer);
        if (VK_FALSE == skipCall) {
            updateResourceTrackingOnDraw(pCB);
        }
        skipCall |= outsideRenderPass(dev_data, pCB, "vkCmdDrawIndexedIndirect");
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdDrawIndexedIndirect(commandBuffer, buffer, offset, count, stride);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdDispatch(VkCommandBuffer commandBuffer, uint32_t x, uint32_t y, uint32_t z)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        skipCall |= addCmd(dev_data, pCB, CMD_DISPATCH, "vkCmdDispatch()");
        skipCall |= insideRenderPass(dev_data, pCB, "vkCmdDispatch");
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdDispatch(commandBuffer, x, y, z);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdDispatchIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        skipCall |= addCmd(dev_data, pCB, CMD_DISPATCHINDIRECT, "vkCmdDispatchIndirect()");
        skipCall |= insideRenderPass(dev_data, pCB, "vkCmdDispatchIndirect");
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdDispatchIndirect(commandBuffer, buffer, offset);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdCopyBuffer(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferCopy* pRegions)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        skipCall |= addCmd(dev_data, pCB, CMD_COPYBUFFER, "vkCmdCopyBuffer()");
        skipCall |= insideRenderPass(dev_data, pCB, "vkCmdCopyBuffer");
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, regionCount, pRegions);
}

VkBool32 VerifySourceImageLayout(VkCommandBuffer cmdBuffer, VkImage srcImage, VkImageSubresourceLayers subLayers, VkImageLayout srcImageLayout) {
    VkBool32 skip_call = VK_FALSE;

    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    for (uint32_t i = 0; i < subLayers.layerCount; ++i) {
        uint32_t layer = i + subLayers.baseArrayLayer;
        VkImageSubresource sub = {subLayers.aspectMask, subLayers.mipLevel, layer};
        IMAGE_CMD_BUF_NODE node;
        if (!FindLayout(pCB, srcImage, sub, node)) {
            SetLayout(pCB, srcImage, sub, {srcImageLayout, srcImageLayout});
            continue;
        }
        if (node.layout != srcImageLayout) {
            // TODO: Improve log message in the next pass
            skip_call |=
                log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                        VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, 0,
                        __LINE__, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                        "Cannot copy from an image whose source layout is %s "
                        "and doesn't match the current layout %s.",
                        string_VkImageLayout(srcImageLayout),
                        string_VkImageLayout(node.layout));
        }
    }
    if (srcImageLayout != VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
        if (srcImageLayout == VK_IMAGE_LAYOUT_GENERAL) {
            // LAYOUT_GENERAL is allowed, but may not be performance optimal, flag as perf warning.
            skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                                 "Layout for input image should be TRANSFER_SRC_OPTIMAL instead of GENERAL.");
        } else {
            skip_call |=
                log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                        (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                        DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                        "Layout for input image is %s but can only be "
                        "TRANSFER_SRC_OPTIMAL or GENERAL.",
                        string_VkImageLayout(srcImageLayout));
        }
    }
    return skip_call;
}

VkBool32 VerifyDestImageLayout(VkCommandBuffer cmdBuffer, VkImage destImage, VkImageSubresourceLayers subLayers, VkImageLayout destImageLayout) {
    VkBool32 skip_call = VK_FALSE;

    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    for (uint32_t i = 0; i < subLayers.layerCount; ++i) {
        uint32_t layer = i + subLayers.baseArrayLayer;
        VkImageSubresource sub = {subLayers.aspectMask, subLayers.mipLevel, layer};
        IMAGE_CMD_BUF_NODE node;
        if (!FindLayout(pCB, destImage, sub, node)) {
            SetLayout(pCB, destImage, sub, {destImageLayout, destImageLayout});
            continue;
        }
        if (node.layout != destImageLayout) {
            skip_call |=
                log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                        VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, 0,
                        __LINE__, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                        "Cannot copy from an image whose dest layout is %s and "
                        "doesn't match the current layout %s.",
                        string_VkImageLayout(destImageLayout),
                        string_VkImageLayout(node.layout));
        }
    }
    if (destImageLayout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        if (destImageLayout == VK_IMAGE_LAYOUT_GENERAL) {
            // LAYOUT_GENERAL is allowed, but may not be performance optimal, flag as perf warning.
            skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                                 "Layout for output image should be TRANSFER_DST_OPTIMAL instead of GENERAL.");
        } else {
            skip_call |=
                log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                        (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                        DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                        "Layout for output image is %s but can only be "
                        "TRANSFER_DST_OPTIMAL or GENERAL.",
                        string_VkImageLayout(destImageLayout));
        }
    }
    return skip_call;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdCopyImage(VkCommandBuffer commandBuffer,
                                             VkImage srcImage,
                                             VkImageLayout srcImageLayout,
                                             VkImage dstImage,
                                             VkImageLayout dstImageLayout,
                                             uint32_t regionCount, const VkImageCopy* pRegions)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        skipCall |= addCmd(dev_data, pCB, CMD_COPYIMAGE, "vkCmdCopyImage()");
        skipCall |= insideRenderPass(dev_data, pCB, "vkCmdCopyImage");
        for (uint32_t i = 0; i < regionCount; ++i) {
            skipCall |= VerifySourceImageLayout(commandBuffer, srcImage, pRegions[i].srcSubresource, srcImageLayout);
            skipCall |= VerifyDestImageLayout(commandBuffer, dstImage, pRegions[i].dstSubresource, dstImageLayout);
        }
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdCopyImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdBlitImage(VkCommandBuffer commandBuffer,
                                             VkImage srcImage, VkImageLayout srcImageLayout,
                                             VkImage dstImage, VkImageLayout dstImageLayout,
                                             uint32_t regionCount, const VkImageBlit* pRegions,
                                             VkFilter filter)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        skipCall |= addCmd(dev_data, pCB, CMD_BLITIMAGE, "vkCmdBlitImage()");
        skipCall |= insideRenderPass(dev_data, pCB, "vkCmdBlitImage");
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdBlitImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions, filter);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdCopyBufferToImage(VkCommandBuffer commandBuffer,
                                                     VkBuffer srcBuffer,
                                                     VkImage dstImage, VkImageLayout dstImageLayout,
                                                     uint32_t regionCount, const VkBufferImageCopy* pRegions)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        skipCall |= addCmd(dev_data, pCB, CMD_COPYBUFFERTOIMAGE, "vkCmdCopyBufferToImage()");
        skipCall |= insideRenderPass(dev_data, pCB, "vkCmdCopyBufferToImage");
        for (uint32_t i = 0; i < regionCount; ++i) {
            skipCall |= VerifySourceImageLayout(commandBuffer, dstImage,
                                                pRegions[i].imageSubresource,
                                                dstImageLayout);
        }
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdCopyBufferToImage(commandBuffer, srcBuffer, dstImage, dstImageLayout, regionCount, pRegions);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdCopyImageToBuffer(VkCommandBuffer commandBuffer,
                                                     VkImage srcImage, VkImageLayout srcImageLayout,
                                                     VkBuffer dstBuffer,
                                                     uint32_t regionCount, const VkBufferImageCopy* pRegions)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        skipCall |= addCmd(dev_data, pCB, CMD_COPYIMAGETOBUFFER, "vkCmdCopyImageToBuffer()");
        skipCall |= insideRenderPass(dev_data, pCB, "vkCmdCopyImageToBuffer");
        for (uint32_t i = 0; i < regionCount; ++i) {
            skipCall |= VerifySourceImageLayout(commandBuffer, srcImage,
                                                pRegions[i].imageSubresource,
                                                srcImageLayout);
        }
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdCopyImageToBuffer(commandBuffer, srcImage, srcImageLayout, dstBuffer, regionCount, pRegions);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdUpdateBuffer(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize dataSize, const uint32_t* pData)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        skipCall |= addCmd(dev_data, pCB, CMD_UPDATEBUFFER, "vkCmdUpdateBuffer()");
        skipCall |= insideRenderPass(dev_data, pCB, "vkCmdCopyUpdateBuffer");
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdUpdateBuffer(commandBuffer, dstBuffer, dstOffset, dataSize, pData);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdFillBuffer(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize size, uint32_t data)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        skipCall |= addCmd(dev_data, pCB, CMD_FILLBUFFER, "vkCmdFillBuffer()");
        skipCall |= insideRenderPass(dev_data, pCB, "vkCmdCopyFillBuffer");
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdFillBuffer(commandBuffer, dstBuffer, dstOffset, size, data);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdClearAttachments(
    VkCommandBuffer                                 commandBuffer,
    uint32_t                                    attachmentCount,
    const VkClearAttachment*                    pAttachments,
    uint32_t                                    rectCount,
    const VkClearRect*                          pRects)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        skipCall |= addCmd(dev_data, pCB, CMD_CLEARATTACHMENTS, "vkCmdClearAttachments()");
        // Warn if this is issued prior to Draw Cmd and clearing the entire attachment
        if (!hasDrawCmd(pCB)                                                                         &&
            (pCB->activeRenderPassBeginInfo.renderArea.extent.width  == pRects[0].rect.extent.width) &&
            (pCB->activeRenderPassBeginInfo.renderArea.extent.height == pRects[0].rect.extent.height)) {
            // TODO : commandBuffer should be srcObj
            skipCall |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, 0, __LINE__, DRAWSTATE_CLEAR_CMD_BEFORE_DRAW, "DS",
                    "vkCmdClearAttachments() issued on CB object 0x%" PRIxLEAST64 " prior to any Draw Cmds."
                    " It is recommended you use RenderPass LOAD_OP_CLEAR on Attachments prior to any Draw.", (uint64_t)(commandBuffer));
        }
        skipCall |= outsideRenderPass(dev_data, pCB, "vkCmdClearAttachments");
    }

    // Validate that attachment is in reference list of active subpass
    if (pCB->activeRenderPass) {
        const VkRenderPassCreateInfo *pRPCI = dev_data->renderPassMap[pCB->activeRenderPass]->pCreateInfo;
        const VkSubpassDescription   *pSD   = &pRPCI->pSubpasses[pCB->activeSubpass];

        for (uint32_t attachment_idx = 0; attachment_idx < attachmentCount; attachment_idx++) {
            const VkClearAttachment *attachment = &pAttachments[attachment_idx];
            if (attachment->aspectMask & VK_IMAGE_ASPECT_COLOR_BIT) {
                VkBool32 found = VK_FALSE;
                for (uint32_t i = 0; i < pSD->colorAttachmentCount; i++) {
                    if (attachment->colorAttachment == pSD->pColorAttachments[i].attachment) {
                        found = VK_TRUE;
                        break;
                    }
                }
                if (VK_FALSE == found) {
                    skipCall |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                            (uint64_t)commandBuffer, __LINE__, DRAWSTATE_MISSING_ATTACHMENT_REFERENCE, "DS",
                            "vkCmdClearAttachments() attachment index %d not found in attachment reference array of active subpass %d",
                            attachment->colorAttachment, pCB->activeSubpass);
                }
            } else if (attachment->aspectMask & (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT)) {
                if (!pSD->pDepthStencilAttachment                                      ||  // Says no DS will be used in active subpass
                    (pSD->pDepthStencilAttachment->attachment == VK_ATTACHMENT_UNUSED)) {  // Says no DS will be used in active subpass

                    skipCall |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                        (uint64_t)commandBuffer, __LINE__, DRAWSTATE_MISSING_ATTACHMENT_REFERENCE, "DS",
                        "vkCmdClearAttachments() attachment index %d does not match depthStencilAttachment.attachment (%d) found in active subpass %d",
                        attachment->colorAttachment,
                        (pSD->pDepthStencilAttachment) ? pSD->pDepthStencilAttachment->attachment : VK_ATTACHMENT_UNUSED,
                        pCB->activeSubpass);
                }
            }
        }
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdClearAttachments(commandBuffer, attachmentCount, pAttachments, rectCount, pRects);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdClearColorImage(
        VkCommandBuffer commandBuffer,
        VkImage image, VkImageLayout imageLayout,
        const VkClearColorValue *pColor,
        uint32_t rangeCount, const VkImageSubresourceRange* pRanges)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        skipCall |= addCmd(dev_data, pCB, CMD_CLEARCOLORIMAGE, "vkCmdClearColorImage()");
        skipCall |= insideRenderPass(dev_data, pCB, "vkCmdClearColorImage");
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdClearColorImage(commandBuffer, image, imageLayout, pColor, rangeCount, pRanges);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdClearDepthStencilImage(
        VkCommandBuffer commandBuffer,
        VkImage image, VkImageLayout imageLayout,
        const VkClearDepthStencilValue *pDepthStencil,
        uint32_t rangeCount,
        const VkImageSubresourceRange* pRanges)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        skipCall |= addCmd(dev_data, pCB, CMD_CLEARDEPTHSTENCILIMAGE, "vkCmdClearDepthStencilImage()");
        skipCall |= insideRenderPass(dev_data, pCB, "vkCmdClearDepthStencilImage");
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdClearDepthStencilImage(commandBuffer, image, imageLayout, pDepthStencil, rangeCount, pRanges);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdResolveImage(VkCommandBuffer commandBuffer,
                                                VkImage srcImage, VkImageLayout srcImageLayout,
                                                VkImage dstImage, VkImageLayout dstImageLayout,
                                                uint32_t regionCount, const VkImageResolve* pRegions)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        skipCall |= addCmd(dev_data, pCB, CMD_RESOLVEIMAGE, "vkCmdResolveImage()");
        skipCall |= insideRenderPass(dev_data, pCB, "vkCmdResolveImage");
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdResolveImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetEvent(VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags stageMask)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        skipCall |= addCmd(dev_data, pCB, CMD_SETEVENT, "vkCmdSetEvent()");
        skipCall |= insideRenderPass(dev_data, pCB, "vkCmdSetEvent");
        pCB->events.push_back(event);
        pCB->eventToStageMap[event] = stageMask;
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdSetEvent(commandBuffer, event, stageMask);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdResetEvent(VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags stageMask)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        skipCall |= addCmd(dev_data, pCB, CMD_RESETEVENT, "vkCmdResetEvent()");
        skipCall |= insideRenderPass(dev_data, pCB, "vkCmdResetEvent");
        pCB->events.push_back(event);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdResetEvent(commandBuffer, event, stageMask);
}

VkBool32 TransitionImageLayouts(VkCommandBuffer cmdBuffer, uint32_t memBarrierCount, const VkImageMemoryBarrier* pImgMemBarriers) {
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    VkBool32 skip = VK_FALSE;

    for (uint32_t i = 0; i < memBarrierCount; ++i) {
        auto mem_barrier = &pImgMemBarriers[i];
        if (!mem_barrier)
            continue;
        // TODO: Do not iterate over every possibility - consolidate where
        // possible
        for (uint32_t j = 0; j < mem_barrier->subresourceRange.levelCount;
             j++) {
            uint32_t level = mem_barrier->subresourceRange.baseMipLevel + j;
            for (uint32_t k = 0; k < mem_barrier->subresourceRange.layerCount;
                 k++) {
                uint32_t layer =
                    mem_barrier->subresourceRange.baseArrayLayer + k;
                VkImageSubresource sub = {
                    mem_barrier->subresourceRange.aspectMask, level, layer};
                IMAGE_CMD_BUF_NODE node;
                if (!FindLayout(pCB, mem_barrier->image, sub, node)) {
                    SetLayout(pCB, mem_barrier->image, sub,
                              {mem_barrier->oldLayout, mem_barrier->newLayout});
                    continue;
                }
                if (mem_barrier->oldLayout == VK_IMAGE_LAYOUT_UNDEFINED) {
                    // TODO: Set memory invalid which is in mem_tracker currently
                }
                else if (node.layout != mem_barrier->oldLayout) {
                    skip |= log_msg(
                        dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                        (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                        DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                        "You cannot transition the layout from %s "
                        "when current layout is %s.",
                        string_VkImageLayout(mem_barrier->oldLayout),
                        string_VkImageLayout(node.layout));
                }
                SetLayout(pCB, mem_barrier->image, sub, mem_barrier->newLayout);
            }
        }
    }
    return skip;
}

// Print readable FlagBits in FlagMask
std::string string_VkAccessFlags(VkAccessFlags accessMask)
{
    std::string result;
    std::string separator;

    if (accessMask == 0) {
        result = "[None]";
    } else {
        result = "[";
        for (auto i = 0; i < 32; i++) {
            if (accessMask & (1 << i)) {
                result = result + separator + string_VkAccessFlagBits((VkAccessFlagBits)(1 << i));
                separator = " | ";
            }
        }
        result = result + "]";
    }
    return result;
}

// AccessFlags MUST have 'required_bit' set, and may have one or more of 'optional_bits' set.
// If required_bit is zero, accessMask must have at least one of 'optional_bits' set
// TODO: Add tracking to ensure that at least one barrier has been set for these layout transitions
VkBool32 ValidateMaskBits(const layer_data* my_data, VkCommandBuffer cmdBuffer, const VkAccessFlags& accessMask, const VkImageLayout& layout,
                          VkAccessFlags required_bit, VkAccessFlags optional_bits, const char* type) {
    VkBool32 skip_call = VK_FALSE;

    if ((accessMask & required_bit) || (!required_bit && (accessMask & optional_bits))) {
        if (accessMask & !(required_bit | optional_bits)) {
            // TODO: Verify against Valid Use
            skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, DRAWSTATE_INVALID_BARRIER, "DS",
                                 "Additional bits in %s accessMask %d %s are specified when layout is %s.",
                                 type, accessMask, string_VkAccessFlags(accessMask).c_str(), string_VkImageLayout(layout));
        }
    } else {
        if (!required_bit) {
            skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, DRAWSTATE_INVALID_BARRIER, "DS",
                                 "%s AccessMask %d %s must contain at least one of access bits %d %s when layout is %s, unless the app has previously added a barrier for this transition.",
                                  type, accessMask, string_VkAccessFlags(accessMask).c_str(), optional_bits,
                                  string_VkAccessFlags(optional_bits).c_str(), string_VkImageLayout(layout));
        } else {
            std::string opt_bits;
            if (optional_bits != 0) {
                std::stringstream ss;
                ss << optional_bits;
                opt_bits = "and may have optional bits " + ss.str() + ' ' + string_VkAccessFlags(optional_bits);
            }
            skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, DRAWSTATE_INVALID_BARRIER, "DS",
                                 "%s AccessMask %d %s must have required access bit %d %s %s when layout is %s, unless the app has previously added a barrier for this transition.",
                                  type, accessMask, string_VkAccessFlags(accessMask).c_str(),
                                  required_bit, string_VkAccessFlags(required_bit).c_str(),
                                  opt_bits.c_str(), string_VkImageLayout(layout));
        }
    }
    return skip_call;
}

VkBool32 ValidateMaskBitsFromLayouts(const layer_data* my_data, VkCommandBuffer cmdBuffer, const VkAccessFlags& accessMask, const VkImageLayout& layout, const char* type) {
    VkBool32 skip_call = VK_FALSE;
    switch (layout) {
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: {
            skip_call |= ValidateMaskBits(my_data, cmdBuffer, accessMask, layout, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT, type);
            break;
        }
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL: {
            skip_call |= ValidateMaskBits(my_data, cmdBuffer, accessMask, layout, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT, type);
            break;
        }
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: {
            skip_call |= ValidateMaskBits(my_data, cmdBuffer, accessMask, layout, VK_ACCESS_TRANSFER_WRITE_BIT, 0, type);
            break;
        }
        case VK_IMAGE_LAYOUT_PREINITIALIZED: {
            skip_call |= ValidateMaskBits(my_data, cmdBuffer, accessMask, layout, VK_ACCESS_HOST_WRITE_BIT, 0, type);
            break;
        }
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL: {
            skip_call |= ValidateMaskBits(my_data, cmdBuffer, accessMask, layout, 0, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_SHADER_READ_BIT, type);
            break;
        }
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL: {
            skip_call |= ValidateMaskBits(my_data, cmdBuffer, accessMask, layout, 0, VK_ACCESS_INPUT_ATTACHMENT_READ_BIT | VK_ACCESS_SHADER_READ_BIT, type);
            break;
        }
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL: {
            skip_call |= ValidateMaskBits(my_data, cmdBuffer, accessMask, layout, VK_ACCESS_TRANSFER_READ_BIT, 0, type);
            break;
        }
        case VK_IMAGE_LAYOUT_UNDEFINED: {
            if (accessMask != 0) {
                // TODO: Verify against Valid Use section spec
                skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, DRAWSTATE_INVALID_BARRIER, "DS",
                                     "Additional bits in %s accessMask %d %s are specified when layout is %s.", type, accessMask, string_VkAccessFlags(accessMask).c_str(),
                                     string_VkImageLayout(layout));
            }
            break;
        }
        case VK_IMAGE_LAYOUT_GENERAL:
        default: {
            break;
        }
    }
    return skip_call;
}

VkBool32 ValidateBarriers(VkCommandBuffer cmdBuffer, uint32_t memBarrierCount,
                          const VkMemoryBarrier *pMemBarriers,
                          uint32_t bufferBarrierCount,
                          const VkBufferMemoryBarrier *pBufferMemBarriers,
                          uint32_t imageMemBarrierCount,
                          const VkImageMemoryBarrier *pImageMemBarriers) {
    VkBool32 skip_call = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    if (pCB->activeRenderPass && memBarrierCount) {
        if (!dev_data->renderPassMap[pCB->activeRenderPass]->hasSelfDependency[pCB->activeSubpass]) {
            skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, DRAWSTATE_INVALID_BARRIER, "DS",
                                 "Barriers cannot be set during subpass %d with no self dependency specified.", pCB->activeSubpass);
        }
    }
    for (uint32_t i = 0; i < imageMemBarrierCount; ++i) {
        auto mem_barrier = &pImageMemBarriers[i];
        if (pCB->activeRenderPass) {
            skip_call |=
                log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                        (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                        DRAWSTATE_INVALID_BARRIER, "DS",
                        "Image Barriers cannot be used during a render pass.");
        }
        if (mem_barrier) {
            skip_call |= ValidateMaskBitsFromLayouts(dev_data, cmdBuffer, mem_barrier->srcAccessMask, mem_barrier->oldLayout, "Source");
            skip_call |= ValidateMaskBitsFromLayouts(dev_data, cmdBuffer, mem_barrier->dstAccessMask, mem_barrier->newLayout, "Dest");
            if (mem_barrier->newLayout == VK_IMAGE_LAYOUT_UNDEFINED ||
                mem_barrier->newLayout == VK_IMAGE_LAYOUT_PREINITIALIZED) {
                log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                        (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                        DRAWSTATE_INVALID_BARRIER, "DS",
                        "Image Layout cannot be transitioned to UNDEFINED or "
                        "PREINITIALIZED.");
            }
            auto image_data = dev_data->imageMap.find(mem_barrier->image);
            VkFormat format;
            uint32_t arrayLayers, mipLevels;
            bool imageFound = false;
            if (image_data != dev_data->imageMap.end()) {
                format = image_data->second->format;
                arrayLayers = image_data->second->arrayLayers;
                mipLevels = image_data->second->mipLevels;
                imageFound = true;
            } else if (dev_data->device_extensions.wsi_enabled) {
                auto imageswap_data =
                    dev_data->device_extensions.imageToSwapchainMap.find(
                        mem_barrier->image);
                if (imageswap_data !=
                    dev_data->device_extensions.imageToSwapchainMap.end()) {
                    auto swapchain_data =
                        dev_data->device_extensions.swapchainMap.find(
                            imageswap_data->second);
                    if (swapchain_data !=
                        dev_data->device_extensions.swapchainMap.end()) {
                        format = swapchain_data->second->createInfo.imageFormat;
                        arrayLayers =
                            swapchain_data->second->createInfo.imageArrayLayers;
                        mipLevels = 1;
                        imageFound = true;
                    }
                }
            }
            if (imageFound) {
                if (vk_format_is_depth_and_stencil(format) &&
                    (!(mem_barrier->subresourceRange.aspectMask &
                       VK_IMAGE_ASPECT_DEPTH_BIT) ||
                     !(mem_barrier->subresourceRange.aspectMask &
                       VK_IMAGE_ASPECT_STENCIL_BIT))) {
                    log_msg(dev_data->report_data,
                            VK_DEBUG_REPORT_ERROR_BIT_EXT,
                            (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                            DRAWSTATE_INVALID_BARRIER, "DS",
                            "Image is a depth and stencil format and thus must "
                            "have both VK_IMAGE_ASPECT_DEPTH_BIT and "
                            "VK_IMAGE_ASPECT_STENCIL_BIT set.");
                }
                if ((mem_barrier->subresourceRange.baseArrayLayer +
                     mem_barrier->subresourceRange.layerCount) > arrayLayers) {
                    log_msg(
                        dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                        (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                        DRAWSTATE_INVALID_BARRIER, "DS",
                        "Subresource must have the sum of the "
                        "baseArrayLayer (%d) and layerCount (%d) be less "
                        "than or equal to the total number of layers (%d).",
                        mem_barrier->subresourceRange.baseArrayLayer,
                        mem_barrier->subresourceRange.layerCount, arrayLayers);
                }
                if ((mem_barrier->subresourceRange.baseMipLevel +
                     mem_barrier->subresourceRange.levelCount) > mipLevels) {
                    log_msg(
                        dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                        (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                        DRAWSTATE_INVALID_BARRIER, "DS",
                        "Subresource must have the sum of the baseMipLevel "
                        "(%d) and levelCount (%d) be less than or equal to "
                        "the total number of levels (%d).",
                        mem_barrier->subresourceRange.baseMipLevel,
                        mem_barrier->subresourceRange.levelCount, mipLevels);
                }
            }
        }
    }
    for (uint32_t i = 0; i < bufferBarrierCount; ++i) {
        auto mem_barrier = &pBufferMemBarriers[i];
        if (pCB->activeRenderPass) {
            skip_call |=
                log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                        (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                        DRAWSTATE_INVALID_BARRIER, "DS",
                        "Buffer Barriers cannot be used during a render pass.");
        }
        if (!mem_barrier)
            continue;
        auto buffer_data = dev_data->bufferMap.find(mem_barrier->buffer);
        uint64_t buffer_size = buffer_data->second.create_info
                                   ? reinterpret_cast<uint64_t &>(
                                         buffer_data->second.create_info->size)
                                   : 0;
        if (buffer_data != dev_data->bufferMap.end() &&
            mem_barrier->offset + mem_barrier->size > buffer_size) {
            skip_call |=
                log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                        (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                        DRAWSTATE_INVALID_BARRIER, "DS",
                        "Buffer Barrier 0x%" PRIx64 " has offset %" PRIu64
                        " and size %" PRIu64
                        " whose sum is greater than total size %" PRIu64 ".",
                        reinterpret_cast<const uint64_t &>(mem_barrier->buffer),
                        reinterpret_cast<const uint64_t &>(mem_barrier->offset),
                        reinterpret_cast<const uint64_t &>(mem_barrier->size),
                        buffer_size);
        }
    }
    return skip_call;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdWaitEvents(
                    VkCommandBuffer commandBuffer, uint32_t eventCount, const VkEvent* pEvents,
                    VkPipelineStageFlags sourceStageMask, VkPipelineStageFlags dstStageMask,
                    uint32_t memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers,
                    uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier* pBufferMemoryBarriers,
                    uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier* pImageMemoryBarriers)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        VkPipelineStageFlags stageMask = 0;
        for (uint32_t i = 0; i < eventCount; ++i) {
            pCB->waitedEvents.push_back(pEvents[i]);
            pCB->events.push_back(pEvents[i]);
            auto event_data = pCB->eventToStageMap.find(pEvents[i]);
            if (event_data != pCB->eventToStageMap.end()) {
                stageMask |= event_data->second;
            } else {
                auto global_event_data = dev_data->eventMap.find(pEvents[i]);
                if (global_event_data == dev_data->eventMap.end()) {
                    skipCall |= log_msg(
                        dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                        VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT,
                        reinterpret_cast<const uint64_t &>(pEvents[i]),
                        __LINE__, DRAWSTATE_INVALID_FENCE, "DS",
                        "Fence 0x%" PRIx64
                        " cannot be waited on if it has never been set.",
                        reinterpret_cast<const uint64_t &>(pEvents[i]));
                } else {
                    stageMask |= global_event_data->second.stageMask;
                }
            }
        }
        if (sourceStageMask != stageMask) {
            skipCall |= log_msg(
                dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                DRAWSTATE_INVALID_FENCE, "DS",
                "srcStageMask in vkCmdWaitEvents must be the bitwise OR of the "
                "stageMask parameters used in calls to vkCmdSetEvent and "
                "VK_PIPELINE_STAGE_HOST_BIT if used with vkSetEvent.");
        }
        if (pCB->state == CB_RECORDING) {
            skipCall |= addCmd(dev_data, pCB, CMD_WAITEVENTS, "vkCmdWaitEvents()");
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdWaitEvents()");
        }
        skipCall |= TransitionImageLayouts(commandBuffer, imageMemoryBarrierCount, pImageMemoryBarriers);
        skipCall |=
            ValidateBarriers(commandBuffer, memoryBarrierCount, pMemoryBarriers,
                             bufferMemoryBarrierCount, pBufferMemoryBarriers,
                             imageMemoryBarrierCount, pImageMemoryBarriers);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdWaitEvents(commandBuffer, eventCount, pEvents, sourceStageMask, dstStageMask,
                                                       memoryBarrierCount, pMemoryBarriers,
                                                       bufferMemoryBarrierCount, pBufferMemoryBarriers,
                                                       imageMemoryBarrierCount, pImageMemoryBarriers);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdPipelineBarrier(
                    VkCommandBuffer commandBuffer, VkPipelineStageFlags srcStageMask,
                    VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags,
                    uint32_t memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers,
                    uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier* pBufferMemoryBarriers,
                    uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier* pImageMemoryBarriers)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        skipCall |= addCmd(dev_data, pCB, CMD_PIPELINEBARRIER, "vkCmdPipelineBarrier()");
        skipCall |= TransitionImageLayouts(commandBuffer, imageMemoryBarrierCount, pImageMemoryBarriers);
        skipCall |=
            ValidateBarriers(commandBuffer, memoryBarrierCount, pMemoryBarriers,
                             bufferMemoryBarrierCount, pBufferMemoryBarriers,
                             imageMemoryBarrierCount, pImageMemoryBarriers);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask, dependencyFlags,
                                                            memoryBarrierCount, pMemoryBarriers,
                                                            bufferMemoryBarrierCount, pBufferMemoryBarriers,
                                                            imageMemoryBarrierCount, pImageMemoryBarriers);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdBeginQuery(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t slot, VkFlags flags)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        QueryObject query = {queryPool, slot};
        pCB->activeQueries.insert(query);
        if (!pCB->startedQueries.count(query)) {
          pCB->startedQueries.insert(query);
        }
        skipCall |= addCmd(dev_data, pCB, CMD_BEGINQUERY, "vkCmdBeginQuery()");
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdBeginQuery(commandBuffer, queryPool, slot, flags);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdEndQuery(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t slot)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        QueryObject query = {queryPool, slot};
        if (!pCB->activeQueries.count(query)) {
            skipCall |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, DRAWSTATE_INVALID_QUERY, "DS",
                "Ending a query before it was started: queryPool %" PRIu64 ", index %d", (uint64_t)(queryPool), slot);
        } else {
            pCB->activeQueries.erase(query);
        }
        pCB->queryToStateMap[query] = 1;
        if (pCB->state == CB_RECORDING) {
            skipCall |= addCmd(dev_data, pCB, CMD_ENDQUERY, "VkCmdEndQuery()");
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdEndQuery()");
        }
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdEndQuery(commandBuffer, queryPool, slot);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdResetQueryPool(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        for (uint32_t i = 0; i < queryCount; i++) {
            QueryObject query = {queryPool, firstQuery + i};
            pCB->waitedEventsBeforeQueryReset[query] = pCB->waitedEvents;
            pCB->queryToStateMap[query] = 0;
        }
        if (pCB->state == CB_RECORDING) {
            skipCall |= addCmd(dev_data, pCB, CMD_RESETQUERYPOOL, "VkCmdResetQueryPool()");
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdResetQueryPool()");
        }
        skipCall |= insideRenderPass(dev_data, pCB, "vkCmdQueryPool");
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdResetQueryPool(commandBuffer, queryPool, firstQuery, queryCount);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdCopyQueryPoolResults(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t firstQuery,
                                                     uint32_t queryCount, VkBuffer dstBuffer, VkDeviceSize dstOffset,
                                                     VkDeviceSize stride, VkQueryResultFlags flags)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        for (uint32_t i = 0; i < queryCount; i++) {
            QueryObject query = {queryPool, firstQuery + i};
            if(!pCB->queryToStateMap[query]) {
                skipCall |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, DRAWSTATE_INVALID_QUERY, "DS",
                                    "Requesting a copy from query to buffer with invalid query: queryPool %" PRIu64 ", index %d", (uint64_t)(queryPool), firstQuery + i);
            }
        }
        if (pCB->state == CB_RECORDING) {
            skipCall |= addCmd(dev_data, pCB, CMD_COPYQUERYPOOLRESULTS, "vkCmdCopyQueryPoolResults()");
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdCopyQueryPoolResults()");
        }
        skipCall |= insideRenderPass(dev_data, pCB, "vkCmdCopyQueryPoolResults");
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdCopyQueryPoolResults(commandBuffer, queryPool,
                           firstQuery, queryCount, dstBuffer, dstOffset, stride, flags);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdWriteTimestamp(VkCommandBuffer commandBuffer, VkPipelineStageFlagBits pipelineStage, VkQueryPool queryPool, uint32_t slot)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        QueryObject query = {queryPool, slot};
        pCB->queryToStateMap[query] = 1;
        if (pCB->state == CB_RECORDING) {
            skipCall |= addCmd(dev_data, pCB, CMD_WRITETIMESTAMP, "vkCmdWriteTimestamp()");
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdWriteTimestamp()");
        }
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdWriteTimestamp(commandBuffer, pipelineStage, queryPool, slot);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateFramebuffer(VkDevice device, const VkFramebufferCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkFramebuffer* pFramebuffer)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->CreateFramebuffer(device, pCreateInfo, pAllocator, pFramebuffer);
    if (VK_SUCCESS == result) {
        // Shadow create info and store in map
        VkFramebufferCreateInfo* localFBCI = new VkFramebufferCreateInfo(*pCreateInfo);
        if (pCreateInfo->pAttachments) {
            localFBCI->pAttachments = new VkImageView[localFBCI->attachmentCount];
            memcpy((void*)localFBCI->pAttachments, pCreateInfo->pAttachments, localFBCI->attachmentCount*sizeof(VkImageView));
        }
        loader_platform_thread_lock_mutex(&globalLock);
        dev_data->frameBufferMap[*pFramebuffer] = localFBCI;
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

// Store the DAG.
struct DAGNode {
    uint32_t pass;
    std::vector<uint32_t> prev;
    std::vector<uint32_t> next;
};

VkBool32 FindDependency(const int index, const int dependent, const std::vector<DAGNode>& subpass_to_node, std::unordered_set<uint32_t>& processed_nodes) {
    // If we have already checked this node we have not found a dependency path so return false.
    if (processed_nodes.count(index))
        return VK_FALSE;
    processed_nodes.insert(index);
    const DAGNode& node = subpass_to_node[index];
    // Look for a dependency path. If one exists return true else recurse on the previous nodes.
    if (std::find(node.prev.begin(), node.prev.end(), dependent) == node.prev.end()) {
        for (auto elem : node.prev) {
            if (FindDependency(elem, dependent, subpass_to_node, processed_nodes))
                return VK_TRUE;
        }
    } else {
        return VK_TRUE;
    }
    return VK_FALSE;
}

VkBool32 CheckDependencyExists(const layer_data* my_data, VkDevice device, const int subpass, const std::vector<uint32_t>& dependent_subpasses, const std::vector<DAGNode>& subpass_to_node, VkBool32& skip_call) {
    VkBool32 result = VK_TRUE;
    // Loop through all subpasses that share the same attachment and make sure a dependency exists
    for (uint32_t k = 0; k < dependent_subpasses.size(); ++k) {
        if (subpass == dependent_subpasses[k])
            continue;
        const DAGNode& node = subpass_to_node[subpass];
        // Check for a specified dependency between the two nodes. If one exists we are done.
        auto prev_elem = std::find(node.prev.begin(), node.prev.end(), dependent_subpasses[k]);
        auto next_elem = std::find(node.next.begin(), node.next.end(), dependent_subpasses[k]);
        if (prev_elem == node.prev.end() && next_elem == node.next.end()) {
            // If no dependency exits an implicit dependency still might. If so, warn and if not throw an error.
            std::unordered_set<uint32_t> processed_nodes;
            if (FindDependency(subpass, dependent_subpasses[k], subpass_to_node, processed_nodes) ||
                FindDependency(dependent_subpasses[k], subpass, subpass_to_node, processed_nodes)) {
                // TODO: Verify against Valid Use section of spec
                skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, DRAWSTATE_INVALID_RENDERPASS, "DS",
                                     "A dependency between subpasses %d and %d must exist but only an implicit one is specified.",
                                     subpass, dependent_subpasses[k]);
            } else {
                skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, DRAWSTATE_INVALID_RENDERPASS, "DS",
                                     "A dependency between subpasses %d and %d must exist but one is not specified.",
                                     subpass, dependent_subpasses[k]);
                result = VK_FALSE;
            }
        }
    }
    return result;
}

VkBool32 CheckPreserved(const layer_data* my_data, VkDevice device, const VkRenderPassCreateInfo* pCreateInfo, const int index, const uint32_t attachment, const std::vector<DAGNode>& subpass_to_node, int depth, VkBool32& skip_call) {
    const DAGNode& node = subpass_to_node[index];
    // If this node writes to the attachment return true as next nodes need to preserve the attachment.
    const VkSubpassDescription& subpass = pCreateInfo->pSubpasses[index];
    for (uint32_t j = 0; j < subpass.colorAttachmentCount; ++j) {
        if (attachment == subpass.pColorAttachments[j].attachment)
            return VK_TRUE;
    }
    if (subpass.pDepthStencilAttachment &&
        subpass.pDepthStencilAttachment->attachment != VK_ATTACHMENT_UNUSED) {
        if (attachment == subpass.pDepthStencilAttachment->attachment)
            return VK_TRUE;
    }
    VkBool32 result = VK_FALSE;
    // Loop through previous nodes and see if any of them write to the attachment.
    for (auto elem : node.prev) {
        result |= CheckPreserved(my_data, device, pCreateInfo, elem, attachment, subpass_to_node, depth + 1, skip_call);
    }
    // If the attachment was written to by a previous node than this node needs to preserve it.
    if (result && depth > 0) {
        const VkSubpassDescription& subpass = pCreateInfo->pSubpasses[index];
        VkBool32 has_preserved = VK_FALSE;
        for (uint32_t j = 0; j < subpass.preserveAttachmentCount; ++j) {
            if (subpass.pPreserveAttachments[j] == attachment) {
                has_preserved = VK_TRUE;
                break;
            }
        }
        if (has_preserved == VK_FALSE) {
            skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, DRAWSTATE_INVALID_RENDERPASS, "DS",
                                 "Attachment %d is used by a later subpass and must be preserved in subpass %d.", attachment, index);
        }
    }
    return result;
}

VkBool32 ValidateDependencies(const layer_data* my_data, VkDevice device, const VkRenderPassCreateInfo* pCreateInfo, const std::vector<DAGNode>& subpass_to_node) {
    VkBool32 skip_call = VK_FALSE;
    std::vector<std::vector<uint32_t>> output_attachment_to_subpass(pCreateInfo->attachmentCount);
    std::vector<std::vector<uint32_t>> input_attachment_to_subpass(pCreateInfo->attachmentCount);
    // Find for each attachment the subpasses that use them.
    for (uint32_t i = 0; i < pCreateInfo->subpassCount; ++i) {
        const VkSubpassDescription& subpass = pCreateInfo->pSubpasses[i];
        for (uint32_t j = 0; j < subpass.inputAttachmentCount; ++j) {
            input_attachment_to_subpass[subpass.pInputAttachments[j].attachment].push_back(i);
        }
        for (uint32_t j = 0; j < subpass.colorAttachmentCount; ++j) {
            output_attachment_to_subpass[subpass.pColorAttachments[j].attachment].push_back(i);
        }
        if (subpass.pDepthStencilAttachment && subpass.pDepthStencilAttachment->attachment != VK_ATTACHMENT_UNUSED) {
            output_attachment_to_subpass[subpass.pDepthStencilAttachment->attachment].push_back(i);
        }
    }
    // If there is a dependency needed make sure one exists
    for (uint32_t i = 0; i < pCreateInfo->subpassCount; ++i) {
        const VkSubpassDescription& subpass = pCreateInfo->pSubpasses[i];
        // If the attachment is an input then all subpasses that output must have a dependency relationship
        for (uint32_t j = 0; j < subpass.inputAttachmentCount; ++j) {
            const uint32_t& attachment = subpass.pInputAttachments[j].attachment;
            CheckDependencyExists(my_data, device, i, output_attachment_to_subpass[attachment], subpass_to_node, skip_call);
        }
        // If the attachment is an output then all subpasses that use the attachment must have a dependency relationship
        for (uint32_t j = 0; j < subpass.colorAttachmentCount; ++j) {
            const uint32_t& attachment = subpass.pColorAttachments[j].attachment;
            CheckDependencyExists(my_data, device, i, output_attachment_to_subpass[attachment], subpass_to_node, skip_call);
            CheckDependencyExists(my_data, device, i, input_attachment_to_subpass[attachment], subpass_to_node, skip_call);
        }
        if (subpass.pDepthStencilAttachment && subpass.pDepthStencilAttachment->attachment != VK_ATTACHMENT_UNUSED) {
            const uint32_t& attachment = subpass.pDepthStencilAttachment->attachment;
            CheckDependencyExists(my_data, device, i, output_attachment_to_subpass[attachment], subpass_to_node, skip_call);
            CheckDependencyExists(my_data, device, i, input_attachment_to_subpass[attachment], subpass_to_node, skip_call);
        }
    }
    // Loop through implicit dependencies, if this pass reads make sure the attachment is preserved for all passes after it was written.
    for (uint32_t i = 0; i < pCreateInfo->subpassCount; ++i) {
        const VkSubpassDescription& subpass = pCreateInfo->pSubpasses[i];
        for (uint32_t j = 0; j < subpass.inputAttachmentCount; ++j) {
            CheckPreserved(my_data, device, pCreateInfo, i, subpass.pInputAttachments[j].attachment, subpass_to_node, 0, skip_call);
        }
    }
    return skip_call;
}

VkBool32 ValidateLayouts(const layer_data* my_data, VkDevice device, const VkRenderPassCreateInfo* pCreateInfo) {
    VkBool32 skip = VK_FALSE;

    for (uint32_t i = 0; i < pCreateInfo->subpassCount; ++i) {
        const VkSubpassDescription& subpass = pCreateInfo->pSubpasses[i];
        for (uint32_t j = 0; j < subpass.inputAttachmentCount; ++j) {
            if (subpass.pInputAttachments[j].layout != VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL &&
                subpass.pInputAttachments[j].layout != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
                if (subpass.pInputAttachments[j].layout == VK_IMAGE_LAYOUT_GENERAL) {
                    // TODO: Verify Valid Use in spec. I believe this is allowed (valid) but may not be optimal performance
                    skip |= log_msg(my_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                                    "Layout for input attachment is GENERAL but should be READ_ONLY_OPTIMAL.");
                } else {
                    skip |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                                    "Layout for input attachment is %d but can only be READ_ONLY_OPTIMAL or GENERAL.", subpass.pInputAttachments[j].attachment);
                }
            }
        }
        for (uint32_t j = 0; j < subpass.colorAttachmentCount; ++j) {
            if (subpass.pColorAttachments[j].layout != VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
                if (subpass.pColorAttachments[j].layout == VK_IMAGE_LAYOUT_GENERAL) {
                    // TODO: Verify Valid Use in spec. I believe this is allowed (valid) but may not be optimal performance
                    skip |= log_msg(my_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                                    "Layout for color attachment is GENERAL but should be COLOR_ATTACHMENT_OPTIMAL.");
                } else {
                    skip |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                                    "Layout for color attachment is %d but can only be COLOR_ATTACHMENT_OPTIMAL or GENERAL.", subpass.pColorAttachments[j].attachment);
                }
            }
        }
        if ((subpass.pDepthStencilAttachment != NULL) &&
            (subpass.pDepthStencilAttachment->attachment != VK_ATTACHMENT_UNUSED)) {
            if (subpass.pDepthStencilAttachment->layout != VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
                if (subpass.pDepthStencilAttachment->layout == VK_IMAGE_LAYOUT_GENERAL) {
                    // TODO: Verify Valid Use in spec. I believe this is allowed (valid) but may not be optimal performance
                    skip |= log_msg(my_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                                    "Layout for depth attachment is GENERAL but should be DEPTH_STENCIL_ATTACHMENT_OPTIMAL.");
                } else {
                    skip |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                                    "Layout for depth attachment is %d but can only be DEPTH_STENCIL_ATTACHMENT_OPTIMAL or GENERAL.", subpass.pDepthStencilAttachment->attachment);
                }
            }
        }
    }
    return skip;
}

VkBool32 CreatePassDAG(const layer_data* my_data, VkDevice device, const VkRenderPassCreateInfo* pCreateInfo, std::vector<DAGNode>& subpass_to_node, std::vector<bool>& has_self_dependency) {
    VkBool32 skip_call = VK_FALSE;
    for (uint32_t i = 0; i < pCreateInfo->subpassCount; ++i) {
        DAGNode& subpass_node = subpass_to_node[i];
        subpass_node.pass = i;
    }
    for (uint32_t i = 0; i < pCreateInfo->dependencyCount; ++i) {
        const VkSubpassDependency& dependency = pCreateInfo->pDependencies[i];
        if (dependency.srcSubpass > dependency.dstSubpass && dependency.srcSubpass != VK_SUBPASS_EXTERNAL && dependency.dstSubpass != VK_SUBPASS_EXTERNAL) {
            skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, DRAWSTATE_INVALID_RENDERPASS, "DS",
                                 "Depedency graph must be specified such that an earlier pass cannot depend on a later pass.");
        } else if (dependency.srcSubpass == VK_SUBPASS_EXTERNAL && dependency.dstSubpass == VK_SUBPASS_EXTERNAL) {
            skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, DRAWSTATE_INVALID_RENDERPASS, "DS",
                                 "The src and dest subpasses cannot both be external.");
        } else if (dependency.srcSubpass == dependency.dstSubpass) {
            has_self_dependency[dependency.srcSubpass] = true;
        }
        if (dependency.dstSubpass != VK_SUBPASS_EXTERNAL) {
            subpass_to_node[dependency.dstSubpass].prev.push_back(dependency.srcSubpass);
        }
        if (dependency.srcSubpass != VK_SUBPASS_EXTERNAL) {
            subpass_to_node[dependency.srcSubpass].next.push_back(dependency.dstSubpass);
        }
    }
    return skip_call;
}
// TODOSC : Add intercept of vkCreateShaderModule

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateShaderModule(
        VkDevice device,
        const VkShaderModuleCreateInfo *pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkShaderModule *pShaderModule)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkBool32 skip_call = VK_FALSE;
    if (!shader_is_spirv(pCreateInfo)) {
        skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                /* dev */ 0, __LINE__, SHADER_CHECKER_NON_SPIRV_SHADER, "SC",
                "Shader is not SPIR-V");
    }

    if (VK_FALSE != skip_call)
        return VK_ERROR_VALIDATION_FAILED_EXT;

    VkResult res = my_data->device_dispatch_table->CreateShaderModule(device, pCreateInfo, pAllocator, pShaderModule);

    if (res == VK_SUCCESS) {
        loader_platform_thread_lock_mutex(&globalLock);
        my_data->shaderModuleMap[*pShaderModule] = new shader_module(pCreateInfo);
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return res;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateRenderPass(VkDevice device, const VkRenderPassCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkRenderPass* pRenderPass)
{
    VkBool32 skip_call = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    // Create DAG
    std::vector<bool> has_self_dependency(pCreateInfo->subpassCount);
    std::vector<DAGNode> subpass_to_node(pCreateInfo->subpassCount);
    skip_call |= CreatePassDAG(dev_data, device, pCreateInfo, subpass_to_node, has_self_dependency);
    // Validate using DAG
    skip_call |= ValidateDependencies(dev_data, device, pCreateInfo, subpass_to_node);
    skip_call |= ValidateLayouts(dev_data, device, pCreateInfo);
    if (VK_FALSE != skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    VkResult result = dev_data->device_dispatch_table->CreateRenderPass(device, pCreateInfo, pAllocator, pRenderPass);
    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        // TODOSC : Merge in tracking of renderpass from ShaderChecker
        // Shadow create info and store in map
        VkRenderPassCreateInfo* localRPCI = new VkRenderPassCreateInfo(*pCreateInfo);
        if (pCreateInfo->pAttachments) {
            localRPCI->pAttachments = new VkAttachmentDescription[localRPCI->attachmentCount];
            memcpy((void*)localRPCI->pAttachments, pCreateInfo->pAttachments, localRPCI->attachmentCount*sizeof(VkAttachmentDescription));
        }
        if (pCreateInfo->pSubpasses) {
            localRPCI->pSubpasses = new VkSubpassDescription[localRPCI->subpassCount];
            memcpy((void*)localRPCI->pSubpasses, pCreateInfo->pSubpasses, localRPCI->subpassCount*sizeof(VkSubpassDescription));

            for (uint32_t i = 0; i < localRPCI->subpassCount; i++) {
                VkSubpassDescription *subpass = (VkSubpassDescription *) &localRPCI->pSubpasses[i];
                const uint32_t attachmentCount = subpass->inputAttachmentCount +
                    subpass->colorAttachmentCount * (1 + (subpass->pResolveAttachments?1:0)) +
                    ((subpass->pDepthStencilAttachment) ? 1 : 0) + subpass->preserveAttachmentCount;
                VkAttachmentReference *attachments = new VkAttachmentReference[attachmentCount];

                memcpy(attachments, subpass->pInputAttachments,
                        sizeof(attachments[0]) * subpass->inputAttachmentCount);
                subpass->pInputAttachments = attachments;
                attachments += subpass->inputAttachmentCount;

                memcpy(attachments, subpass->pColorAttachments,
                        sizeof(attachments[0]) * subpass->colorAttachmentCount);
                subpass->pColorAttachments = attachments;
                attachments += subpass->colorAttachmentCount;

                if (subpass->pResolveAttachments) {
                    memcpy(attachments, subpass->pResolveAttachments,
                            sizeof(attachments[0]) * subpass->colorAttachmentCount);
                    subpass->pResolveAttachments = attachments;
                    attachments += subpass->colorAttachmentCount;
                }

                if (subpass->pDepthStencilAttachment) {
                    memcpy(attachments, subpass->pDepthStencilAttachment,
                            sizeof(attachments[0]) * 1);
                    subpass->pDepthStencilAttachment = attachments;
                    attachments += 1;
                }

                memcpy(attachments, subpass->pPreserveAttachments,
                        sizeof(attachments[0]) * subpass->preserveAttachmentCount);
                subpass->pPreserveAttachments = &attachments->attachment;
            }
        }
        if (pCreateInfo->pDependencies) {
            localRPCI->pDependencies = new VkSubpassDependency[localRPCI->dependencyCount];
            memcpy((void*)localRPCI->pDependencies, pCreateInfo->pDependencies, localRPCI->dependencyCount*sizeof(VkSubpassDependency));
        }
        dev_data->renderPassMap[*pRenderPass] = new RENDER_PASS_NODE(localRPCI);
        dev_data->renderPassMap[*pRenderPass]->hasSelfDependency = has_self_dependency;
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}
// Free the renderpass shadow
static void deleteRenderPasses(layer_data* my_data)
{
    if (my_data->renderPassMap.size() <= 0)
        return;
    for (auto ii=my_data->renderPassMap.begin(); ii!=my_data->renderPassMap.end(); ++ii) {
        const VkRenderPassCreateInfo* pRenderPassInfo = (*ii).second->pCreateInfo;
        if (pRenderPassInfo->pAttachments) {
            delete[] pRenderPassInfo->pAttachments;
        }
        if (pRenderPassInfo->pSubpasses) {
            for (uint32_t i=0; i<pRenderPassInfo->subpassCount; ++i) {
                // Attachements are all allocated in a block, so just need to
                //  find the first non-null one to delete
                if (pRenderPassInfo->pSubpasses[i].pInputAttachments) {
                    delete[] pRenderPassInfo->pSubpasses[i].pInputAttachments;
                } else if (pRenderPassInfo->pSubpasses[i].pColorAttachments) {
                    delete[] pRenderPassInfo->pSubpasses[i].pColorAttachments;
                } else if (pRenderPassInfo->pSubpasses[i].pResolveAttachments) {
                    delete[] pRenderPassInfo->pSubpasses[i].pResolveAttachments;
                } else if (pRenderPassInfo->pSubpasses[i].pPreserveAttachments) {
                    delete[] pRenderPassInfo->pSubpasses[i].pPreserveAttachments;
                }
            }
            delete[] pRenderPassInfo->pSubpasses;
        }
        if (pRenderPassInfo->pDependencies) {
            delete[] pRenderPassInfo->pDependencies;
        }
        delete pRenderPassInfo;
        delete (*ii).second;
    }
    my_data->renderPassMap.clear();
}

VkBool32 VerifyFramebufferAndRenderPassLayouts(VkCommandBuffer cmdBuffer, const VkRenderPassBeginInfo* pRenderPassBegin) {
    VkBool32 skip_call = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    const VkRenderPassCreateInfo* pRenderPassInfo = dev_data->renderPassMap[pRenderPassBegin->renderPass]->pCreateInfo;
    const VkFramebufferCreateInfo* pFramebufferInfo = dev_data->frameBufferMap[pRenderPassBegin->framebuffer];
    if (pRenderPassInfo->attachmentCount != pFramebufferInfo->attachmentCount) {
        skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, DRAWSTATE_INVALID_RENDERPASS, "DS",
                             "You cannot start a render pass using a framebuffer with a different number of attachments.");
    }
    for (uint32_t i = 0; i < pRenderPassInfo->attachmentCount; ++i) {
        const VkImageView& image_view = pFramebufferInfo->pAttachments[i];
        auto image_data = dev_data->imageViewMap.find(image_view);
        assert(image_data != dev_data->imageViewMap.end());
        const VkImage& image = image_data->second->image;
        const VkImageSubresourceRange& subRange = image_data->second->subresourceRange;
        IMAGE_CMD_BUF_NODE newNode = {pRenderPassInfo->pAttachments[i].initialLayout, pRenderPassInfo->pAttachments[i].initialLayout};
        // TODO: Do not iterate over every possibility - consolidate where possible
        for (uint32_t j = 0; j < subRange.levelCount; j++) {
            uint32_t level = subRange.baseMipLevel + j;
            for (uint32_t k = 0; k < subRange.layerCount; k++) {
                uint32_t layer = subRange.baseArrayLayer + k;
                VkImageSubresource sub = {subRange.aspectMask, level, layer};
                IMAGE_CMD_BUF_NODE node;
                if (!FindLayout(pCB, image, sub, node)) {
                    SetLayout(pCB, image, sub, newNode);
                    continue;
                }
                if (newNode.layout != node.layout) {
                    skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, DRAWSTATE_INVALID_RENDERPASS, "DS",
                                 "You cannot start a render pass using attachment %i where the intial layout differs from the starting layout.", i);
                }
            }
        }
    }
    return skip_call;
}

void TransitionSubpassLayouts(VkCommandBuffer cmdBuffer, const VkRenderPassBeginInfo* pRenderPassBegin, const int subpass_index) {
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    auto render_pass_data = dev_data->renderPassMap.find(pRenderPassBegin->renderPass);
    if (render_pass_data == dev_data->renderPassMap.end()) {
        return;
    }
    const VkRenderPassCreateInfo* pRenderPassInfo = render_pass_data->second->pCreateInfo;
    auto framebuffer_data = dev_data->frameBufferMap.find(pRenderPassBegin->framebuffer);
    if (framebuffer_data == dev_data->frameBufferMap.end()) {
        return;
    }
    const VkFramebufferCreateInfo* pFramebufferInfo = framebuffer_data->second;
    const VkSubpassDescription& subpass = pRenderPassInfo->pSubpasses[subpass_index];
    for (uint32_t j = 0; j < subpass.inputAttachmentCount; ++j) {
        const VkImageView& image_view = pFramebufferInfo->pAttachments[subpass.pInputAttachments[j].attachment];
        SetLayout(dev_data, pCB, image_view,
                  subpass.pInputAttachments[j].layout);
    }
    for (uint32_t j = 0; j < subpass.colorAttachmentCount; ++j) {
        const VkImageView& image_view = pFramebufferInfo->pAttachments[subpass.pColorAttachments[j].attachment];
        SetLayout(dev_data, pCB, image_view,
                  subpass.pColorAttachments[j].layout);
    }
    if ((subpass.pDepthStencilAttachment != NULL) &&
        (subpass.pDepthStencilAttachment->attachment != VK_ATTACHMENT_UNUSED)) {
        const VkImageView& image_view = pFramebufferInfo->pAttachments[subpass.pDepthStencilAttachment->attachment];
        SetLayout(dev_data, pCB, image_view,
                  subpass.pDepthStencilAttachment->layout);
    }
}

VkBool32 validatePrimaryCommandBuffer(const layer_data* my_data, const GLOBAL_CB_NODE* pCB, const std::string& cmd_name) {
    VkBool32 skip_call = VK_FALSE;
    if (pCB->createInfo.level != VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
        skip_call |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, DRAWSTATE_INVALID_COMMAND_BUFFER, "DS",
                             "Cannot execute command %s on a secondary command buffer.", cmd_name.c_str());
    }
    return skip_call;
}

void TransitionFinalSubpassLayouts(VkCommandBuffer cmdBuffer, const VkRenderPassBeginInfo* pRenderPassBegin) {
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    auto render_pass_data = dev_data->renderPassMap.find(pRenderPassBegin->renderPass);
    if (render_pass_data == dev_data->renderPassMap.end()) {
        return;
    }
    const VkRenderPassCreateInfo* pRenderPassInfo = render_pass_data->second->pCreateInfo;
    auto framebuffer_data = dev_data->frameBufferMap.find(pRenderPassBegin->framebuffer);
    if (framebuffer_data == dev_data->frameBufferMap.end()) {
        return;
    }
    const VkFramebufferCreateInfo* pFramebufferInfo = framebuffer_data->second;
    for (uint32_t i = 0; i < pRenderPassInfo->attachmentCount; ++i) {
        const VkImageView& image_view = pFramebufferInfo->pAttachments[i];
        SetLayout(dev_data, pCB, image_view,
                  pRenderPassInfo->pAttachments[i].finalLayout);
    }
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdBeginRenderPass(VkCommandBuffer commandBuffer, const VkRenderPassBeginInfo *pRenderPassBegin, VkSubpassContents contents)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pRenderPassBegin && pRenderPassBegin->renderPass) {
            skipCall |= VerifyFramebufferAndRenderPassLayouts(commandBuffer, pRenderPassBegin);
            skipCall |= insideRenderPass(dev_data, pCB, "vkCmdBeginRenderPass");
            skipCall |= validatePrimaryCommandBuffer(dev_data, pCB, "vkCmdBeginRenderPass");
            skipCall |= addCmd(dev_data, pCB, CMD_BEGINRENDERPASS, "vkCmdBeginRenderPass()");
            pCB->activeRenderPass = pRenderPassBegin->renderPass;
            // This is a shallow copy as that is all that is needed for now
            pCB->activeRenderPassBeginInfo = *pRenderPassBegin;
            pCB->activeSubpass = 0;
            pCB->activeSubpassContents = contents;
            pCB->framebuffer = pRenderPassBegin->framebuffer;
        } else {
            skipCall |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT) 0, 0, __LINE__, DRAWSTATE_INVALID_RENDERPASS, "DS",
                    "You cannot use a NULL RenderPass object in vkCmdBeginRenderPass()");
        }
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall) {
        dev_data->device_dispatch_table->CmdBeginRenderPass(commandBuffer, pRenderPassBegin, contents);
        loader_platform_thread_lock_mutex(&globalLock);
        // This is a shallow copy as that is all that is needed for now
        dev_data->renderPassBeginInfo = *pRenderPassBegin;
        dev_data->currentSubpass = 0;
        loader_platform_thread_unlock_mutex(&globalLock);
    }
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdNextSubpass(VkCommandBuffer commandBuffer, VkSubpassContents contents)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    TransitionSubpassLayouts(commandBuffer, &dev_data->renderPassBeginInfo, ++dev_data->currentSubpass);
    if (pCB) {
        skipCall |= validatePrimaryCommandBuffer(dev_data, pCB, "vkCmdNextSubpass");
        skipCall |= addCmd(dev_data, pCB, CMD_NEXTSUBPASS, "vkCmdNextSubpass()");
        pCB->activeSubpass++;
        pCB->activeSubpassContents = contents;
        TransitionSubpassLayouts(commandBuffer, &pCB->activeRenderPassBeginInfo, pCB->activeSubpass);
        if (pCB->lastBoundPipeline) {
            skipCall |= validatePipelineState(dev_data, pCB, VK_PIPELINE_BIND_POINT_GRAPHICS, pCB->lastBoundPipeline);
        }
        skipCall |= outsideRenderPass(dev_data, pCB, "vkCmdNextSubpass");
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdNextSubpass(commandBuffer, contents);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdEndRenderPass(VkCommandBuffer commandBuffer)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    TransitionFinalSubpassLayouts(commandBuffer, &dev_data->renderPassBeginInfo);
    if (pCB) {
        skipCall |= outsideRenderPass(dev_data, pCB, "vkCmdEndRenderpass");
        skipCall |= validatePrimaryCommandBuffer(dev_data, pCB, "vkCmdEndRenderPass");
        skipCall |= addCmd(dev_data, pCB, CMD_ENDRENDERPASS, "vkCmdEndRenderPass()");
        TransitionFinalSubpassLayouts(commandBuffer, &pCB->activeRenderPassBeginInfo);
        pCB->activeRenderPass = 0;
        pCB->activeSubpass = 0;
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdEndRenderPass(commandBuffer);
}

bool logInvalidAttachmentMessage(layer_data* dev_data, VkCommandBuffer secondaryBuffer, VkRenderPass secondaryPass, VkRenderPass primaryPass, uint32_t primaryAttach, uint32_t secondaryAttach, const char* msg) {
    return log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT) 0, 0, __LINE__, DRAWSTATE_INVALID_SECONDARY_COMMAND_BUFFER, "DS",
        "vkCmdExecuteCommands() called w/ invalid Cmd Buffer %p which has a render pass %" PRIx64 " that is not compatible with the current render pass %" PRIx64 "."
        "Attachment %" PRIu32 " is not compatable with %" PRIu32 ". %s",
        (void*)secondaryBuffer, (uint64_t)(secondaryPass), (uint64_t)(primaryPass), primaryAttach, secondaryAttach, msg);
}

bool validateAttachmentCompatibility(layer_data* dev_data, VkCommandBuffer primaryBuffer, VkRenderPass primaryPass, uint32_t primaryAttach, VkCommandBuffer secondaryBuffer, VkRenderPass secondaryPass, uint32_t secondaryAttach, bool is_multi) {
    bool skip_call = false;
    auto primary_data = dev_data->renderPassMap.find(primaryPass);
    auto secondary_data = dev_data->renderPassMap.find(secondaryPass);
    if (primary_data->second->pCreateInfo->attachmentCount <= primaryAttach) {
        primaryAttach = VK_ATTACHMENT_UNUSED;
    }
    if (secondary_data->second->pCreateInfo->attachmentCount <= secondaryAttach) {
        secondaryAttach = VK_ATTACHMENT_UNUSED;
    }
    if (primaryAttach == VK_ATTACHMENT_UNUSED && secondaryAttach == VK_ATTACHMENT_UNUSED) {
        return skip_call;
    }
    if (primaryAttach == VK_ATTACHMENT_UNUSED) {
        skip_call |= logInvalidAttachmentMessage(dev_data, secondaryBuffer, secondaryPass, primaryPass, primaryAttach, secondaryAttach, "The first is unused while the second is not.");
        return skip_call;
    }
    if (secondaryAttach == VK_ATTACHMENT_UNUSED) {
        skip_call |= logInvalidAttachmentMessage(dev_data, secondaryBuffer, secondaryPass, primaryPass, primaryAttach, secondaryAttach, "The second is unused while the first is not.");
        return skip_call;
    }
    if (primary_data->second->pCreateInfo->pAttachments[primaryAttach].format != secondary_data->second->pCreateInfo->pAttachments[secondaryAttach].format) {
        skip_call |= logInvalidAttachmentMessage(dev_data, secondaryBuffer, secondaryPass, primaryPass, primaryAttach, secondaryAttach, "They have different formats.");
    }
    if (primary_data->second->pCreateInfo->pAttachments[primaryAttach].samples != secondary_data->second->pCreateInfo->pAttachments[secondaryAttach].samples) {
        skip_call |= logInvalidAttachmentMessage(dev_data, secondaryBuffer, secondaryPass, primaryPass, primaryAttach, secondaryAttach, "They have different samples.");
    }
    if (is_multi && primary_data->second->pCreateInfo->pAttachments[primaryAttach].flags != secondary_data->second->pCreateInfo->pAttachments[secondaryAttach].flags) {
        skip_call |= logInvalidAttachmentMessage(dev_data, secondaryBuffer, secondaryPass, primaryPass, primaryAttach, secondaryAttach, "They have different flags.");
    }
    return skip_call;
}

bool validateSubpassCompatibility(layer_data* dev_data, VkCommandBuffer primaryBuffer, VkRenderPass primaryPass, VkCommandBuffer secondaryBuffer, VkRenderPass secondaryPass, const int subpass, bool is_multi) {
    bool skip_call = false;
    auto primary_data = dev_data->renderPassMap.find(primaryPass);
    auto secondary_data = dev_data->renderPassMap.find(secondaryPass);
    const VkSubpassDescription& primary_desc = primary_data->second->pCreateInfo->pSubpasses[subpass];
    const VkSubpassDescription& secondary_desc = secondary_data->second->pCreateInfo->pSubpasses[subpass];
    uint32_t maxInputAttachmentCount = std::max(primary_desc.inputAttachmentCount, secondary_desc.inputAttachmentCount);
    for (uint32_t i = 0; i < maxInputAttachmentCount; ++i) {
        uint32_t primary_input_attach = VK_ATTACHMENT_UNUSED, secondary_input_attach = VK_ATTACHMENT_UNUSED;
        if (i < primary_desc.inputAttachmentCount) {
            primary_input_attach = primary_desc.pInputAttachments[i].attachment;
        }
        if (i < secondary_desc.inputAttachmentCount) {
            secondary_input_attach = secondary_desc.pInputAttachments[i].attachment;
        }
        skip_call |= validateAttachmentCompatibility(dev_data, primaryBuffer, primaryPass, primary_input_attach, secondaryBuffer, secondaryPass, secondary_input_attach, is_multi);
    }
    maxInputAttachmentCount = std::max(primary_desc.colorAttachmentCount, secondary_desc.colorAttachmentCount);
    for (uint32_t i = 0; i < maxInputAttachmentCount; ++i) {
        uint32_t primary_color_attach = VK_ATTACHMENT_UNUSED, secondary_color_attach = VK_ATTACHMENT_UNUSED;
        if (i < primary_desc.colorAttachmentCount) {
            primary_color_attach = primary_desc.pColorAttachments[i].attachment;
        }
        if (i < secondary_desc.colorAttachmentCount) {
            secondary_color_attach = secondary_desc.pColorAttachments[i].attachment;
        }
        skip_call |= validateAttachmentCompatibility(dev_data, primaryBuffer, primaryPass, primary_color_attach, secondaryBuffer, secondaryPass, secondary_color_attach, is_multi);
        uint32_t primary_resolve_attach = VK_ATTACHMENT_UNUSED, secondary_resolve_attach = VK_ATTACHMENT_UNUSED;
        if (i < primary_desc.colorAttachmentCount && primary_desc.pResolveAttachments) {
            primary_resolve_attach = primary_desc.pResolveAttachments[i].attachment;
        }
        if (i < secondary_desc.colorAttachmentCount && secondary_desc.pResolveAttachments) {
            secondary_resolve_attach = secondary_desc.pResolveAttachments[i].attachment;
        }
        skip_call |= validateAttachmentCompatibility(dev_data, primaryBuffer, primaryPass, primary_resolve_attach, secondaryBuffer, secondaryPass, secondary_resolve_attach, is_multi);
    }
    uint32_t primary_depthstencil_attach = VK_ATTACHMENT_UNUSED, secondary_depthstencil_attach = VK_ATTACHMENT_UNUSED;
    if (primary_desc.pDepthStencilAttachment) {
        primary_depthstencil_attach = primary_desc.pDepthStencilAttachment[0].attachment;
    }
    if (secondary_desc.pDepthStencilAttachment) {
        secondary_depthstencil_attach = secondary_desc.pDepthStencilAttachment[0].attachment;
    }
    skip_call |= validateAttachmentCompatibility(dev_data, primaryBuffer, primaryPass, primary_depthstencil_attach, secondaryBuffer, secondaryPass, secondary_depthstencil_attach, is_multi);
    return skip_call;
}

bool validateRenderPassCompatibility(layer_data* dev_data, VkCommandBuffer primaryBuffer, VkRenderPass primaryPass, VkCommandBuffer secondaryBuffer, VkRenderPass secondaryPass) {
    bool skip_call = false;
    // Early exit if renderPass objects are identical (and therefore compatible)
    if (primaryPass == secondaryPass)
        return skip_call;
    auto primary_data = dev_data->renderPassMap.find(primaryPass);
    auto secondary_data = dev_data->renderPassMap.find(secondaryPass);
    if (primary_data == dev_data->renderPassMap.end() || primary_data->second == nullptr) {
        skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT) 0, 0, __LINE__, DRAWSTATE_INVALID_SECONDARY_COMMAND_BUFFER, "DS",
            "vkCmdExecuteCommands() called w/ invalid current Cmd Buffer %p which has invalid render pass %" PRIx64 ".",
            (void*)primaryBuffer, (uint64_t)(primaryPass));
        return skip_call;
    }
    if (secondary_data == dev_data->renderPassMap.end() || secondary_data->second == nullptr) {
        skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT) 0, 0, __LINE__, DRAWSTATE_INVALID_SECONDARY_COMMAND_BUFFER, "DS",
            "vkCmdExecuteCommands() called w/ invalid secondary Cmd Buffer %p which has invalid render pass %" PRIx64 ".",
            (void*)secondaryBuffer, (uint64_t)(secondaryPass));
        return skip_call;
    }
    if (primary_data->second->pCreateInfo->subpassCount != secondary_data->second->pCreateInfo->subpassCount) {
        skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT) 0, 0, __LINE__, DRAWSTATE_INVALID_SECONDARY_COMMAND_BUFFER, "DS",
            "vkCmdExecuteCommands() called w/ invalid Cmd Buffer %p which has a render pass %" PRIx64 " that is not compatible with the current render pass %" PRIx64 "."
            "They have a different number of subpasses.",
            (void*)secondaryBuffer, (uint64_t)(secondaryPass), (uint64_t)(primaryPass));
        return skip_call;
    }
    bool is_multi = primary_data->second->pCreateInfo->subpassCount > 1;
    for (uint32_t i = 0; i < primary_data->second->pCreateInfo->subpassCount; ++i) {
        skip_call |= validateSubpassCompatibility(dev_data, primaryBuffer, primaryPass, secondaryBuffer, secondaryPass, i, is_multi);
    }
    return skip_call;
}

bool validateFramebuffer(layer_data* dev_data, VkCommandBuffer primaryBuffer, const GLOBAL_CB_NODE* pCB, VkCommandBuffer secondaryBuffer, const GLOBAL_CB_NODE* pSubCB) {
    bool skip_call = false;
    if (!pSubCB->beginInfo.pInheritanceInfo) {
        return skip_call;
    }
    VkFramebuffer primary_fb = pCB->framebuffer;
    VkFramebuffer secondary_fb = pSubCB->beginInfo.pInheritanceInfo->framebuffer;
    if (secondary_fb != VK_NULL_HANDLE) {
        if (primary_fb != secondary_fb) {
            skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT) 0, 0, __LINE__, DRAWSTATE_INVALID_SECONDARY_COMMAND_BUFFER, "DS",
                "vkCmdExecuteCommands() called w/ invalid Cmd Buffer %p which has a framebuffer %" PRIx64 " that is not compatible with the current framebuffer %" PRIx64 ".",
                (void*)secondaryBuffer, (uint64_t)(secondary_fb), (uint64_t)(primary_fb));
        }
        auto fb_data = dev_data->frameBufferMap.find(secondary_fb);
        if (fb_data == dev_data->frameBufferMap.end() || !fb_data->second) {
            skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT) 0, 0, __LINE__, DRAWSTATE_INVALID_SECONDARY_COMMAND_BUFFER, "DS",
                "vkCmdExecuteCommands() called w/ invalid Cmd Buffer %p which has invalid framebuffer %" PRIx64 ".",
                (void*)secondaryBuffer, (uint64_t)(secondary_fb));
            return skip_call;
        }
        skip_call |= validateRenderPassCompatibility(dev_data, secondaryBuffer, fb_data->second->renderPass, secondaryBuffer, pSubCB->beginInfo.pInheritanceInfo->renderPass);
    }
    return skip_call;
}

bool validateSecondaryCommandBufferState(layer_data *dev_data,
                                         GLOBAL_CB_NODE *pCB,
                                         GLOBAL_CB_NODE *pSubCB) {
    bool skipCall = false;
    unordered_set<int> activeTypes;
    for (auto queryObject : pCB->activeQueries) {
        auto queryPoolData = dev_data->queryPoolMap.find(queryObject.pool);
        if (queryPoolData != dev_data->queryPoolMap.end()) {
          if (queryPoolData->second.createInfo.queryType ==
                  VK_QUERY_TYPE_PIPELINE_STATISTICS &&
              pSubCB->beginInfo.pInheritanceInfo) {
            VkQueryPipelineStatisticFlags cmdBufStatistics =
                pSubCB->beginInfo.pInheritanceInfo->pipelineStatistics;
            if ((cmdBufStatistics &
                 queryPoolData->second.createInfo.pipelineStatistics) !=
                cmdBufStatistics) {
              skipCall |= log_msg(
                  dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                  (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                  DRAWSTATE_INVALID_SECONDARY_COMMAND_BUFFER, "DS",
                  "vkCmdExecuteCommands() called w/ invalid Cmd Buffer %p "
                  "which has invalid active query pool %" PRIx64
                  ". Pipeline statistics is being queried so the command "
                  "buffer must have all bits set on the queryPool.",
                  reinterpret_cast<void *>(pCB->commandBuffer),
                  reinterpret_cast<const uint64_t&>(queryPoolData->first));
            }
            }
            activeTypes.insert(queryPoolData->second.createInfo.queryType);
        }
    }
    for (auto queryObject : pSubCB->startedQueries) {
      auto queryPoolData = dev_data->queryPoolMap.find(queryObject.pool);
      if (queryPoolData != dev_data->queryPoolMap.end() &&
          activeTypes.count(queryPoolData->second.createInfo.queryType)) {
        skipCall |=
            log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                    (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                    DRAWSTATE_INVALID_SECONDARY_COMMAND_BUFFER, "DS",
                    "vkCmdExecuteCommands() called w/ invalid Cmd Buffer %p "
                    "which has invalid active query pool %" PRIx64
                    "of type %d but a query of that type has been started on "
                    "secondary Cmd Buffer %p.",
                    reinterpret_cast<void *>(pCB->commandBuffer),
                    reinterpret_cast<const uint64_t&>(queryPoolData->first),
                    queryPoolData->second.createInfo.queryType,
                    reinterpret_cast<void *>(pSubCB->commandBuffer));
        }
    }
    return skipCall;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdExecuteCommands(VkCommandBuffer commandBuffer, uint32_t commandBuffersCount, const VkCommandBuffer* pCommandBuffers)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        GLOBAL_CB_NODE* pSubCB = NULL;
        for (uint32_t i=0; i<commandBuffersCount; i++) {
            pSubCB = getCBNode(dev_data, pCommandBuffers[i]);
            if (!pSubCB) {
                skipCall |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT) 0, 0, __LINE__, DRAWSTATE_INVALID_SECONDARY_COMMAND_BUFFER, "DS",
                    "vkCmdExecuteCommands() called w/ invalid Cmd Buffer %p in element %u of pCommandBuffers array.", (void*)pCommandBuffers[i], i);
            } else if (VK_COMMAND_BUFFER_LEVEL_PRIMARY == pSubCB->createInfo.level) {
                skipCall |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT) 0, 0, __LINE__, DRAWSTATE_INVALID_SECONDARY_COMMAND_BUFFER, "DS",
                    "vkCmdExecuteCommands() called w/ Primary Cmd Buffer %p in element %u of pCommandBuffers array. All cmd buffers in pCommandBuffers array must be secondary.", (void*)pCommandBuffers[i], i);
            } else if (pCB->activeRenderPass) { // Secondary CB w/i RenderPass must have *CONTINUE_BIT set
                if (!(pSubCB->beginInfo.flags & VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT)) {
                    skipCall |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, (uint64_t)pCommandBuffers[i], __LINE__, DRAWSTATE_BEGIN_CB_INVALID_STATE, "DS",
                        "vkCmdExecuteCommands(): Secondary Command Buffer (%p) executed within render pass (%#" PRIxLEAST64 ") must have had vkBeginCommandBuffer() called w/ VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT set.", (void*)pCommandBuffers[i], (uint64_t)pCB->activeRenderPass);
                } else {
                    // Make sure render pass is compatible with parent command buffer pass if has continue
                    skipCall |= validateRenderPassCompatibility(dev_data, commandBuffer, pCB->activeRenderPass, pCommandBuffers[i], pSubCB->beginInfo.pInheritanceInfo->renderPass);
                    skipCall |= validateFramebuffer(dev_data, commandBuffer, pCB, pCommandBuffers[i], pSubCB);
                }
                string errorString = "";
                if (!verify_renderpass_compatibility(dev_data, pCB->activeRenderPass, pSubCB->beginInfo.pInheritanceInfo->renderPass, errorString)) {
                    skipCall |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, (uint64_t)pCommandBuffers[i], __LINE__, DRAWSTATE_RENDERPASS_INCOMPATIBLE, "DS",
                        "vkCmdExecuteCommands(): Secondary Command Buffer (%p) w/ render pass (%#" PRIxLEAST64 ") is incompatible w/ primary command buffer (%p) w/ render pass (%#" PRIxLEAST64 ") due to: %s",
                            (void*)pCommandBuffers[i], (uint64_t)pSubCB->beginInfo.pInheritanceInfo->renderPass, (void*)commandBuffer, (uint64_t)pCB->activeRenderPass, errorString.c_str());
                }
                //  If framebuffer for secondary CB is not NULL, then it must match FB from vkCmdBeginRenderPass()
                //   that this CB will be executed in AND framebuffer must have been created w/ RP compatible w/ renderpass
                if (pSubCB->beginInfo.pInheritanceInfo->framebuffer) {
                    if (pSubCB->beginInfo.pInheritanceInfo->framebuffer != pCB->activeRenderPassBeginInfo.framebuffer) {
                        skipCall |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, (uint64_t)pCommandBuffers[i], __LINE__, DRAWSTATE_FRAMEBUFFER_INCOMPATIBLE, "DS",
                            "vkCmdExecuteCommands(): Secondary Command Buffer (%p) references framebuffer (%#" PRIxLEAST64 ") that does not match framebuffer (%#" PRIxLEAST64 ") in active renderpass (%#" PRIxLEAST64 ").",
                            (void*)pCommandBuffers[i], (uint64_t)pSubCB->beginInfo.pInheritanceInfo->framebuffer, (uint64_t)pCB->activeRenderPassBeginInfo.framebuffer, (uint64_t)pCB->activeRenderPass);
                    }
                }
            }
            // TODO(mlentine): Move more logic into this method
            skipCall |=
                validateSecondaryCommandBufferState(dev_data, pCB, pSubCB);
            skipCall |= validateCommandBufferState(dev_data, pSubCB);
            // Secondary cmdBuffers are considered pending execution starting w/
            // being recorded
            if (!(pSubCB->beginInfo.flags &
                  VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT)) {
                if (dev_data->globalInFlightCmdBuffers.find(
                        pSubCB->commandBuffer) !=
                    dev_data->globalInFlightCmdBuffers.end()) {
                    skipCall |= log_msg(
                        dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                        VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                        (uint64_t)(pCB->commandBuffer), __LINE__,
                        DRAWSTATE_INVALID_CB_SIMULTANEOUS_USE, "DS",
                        "Attempt to simultaneously execute CB %#" PRIxLEAST64
                        " w/o VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT "
                        "set!",
                        (uint64_t)(pCB->commandBuffer));
                }
                if (pCB->beginInfo.flags & VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT) {
                    // Warn that non-simultaneous secondary cmd buffer renders primary non-simultaneous
                    skipCall |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, (uint64_t)(pCommandBuffers[i]), __LINE__, DRAWSTATE_INVALID_CB_SIMULTANEOUS_USE, "DS",
                            "vkCmdExecuteCommands(): Secondary Command Buffer (%#" PRIxLEAST64 ") does not have VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT set and will cause primary command buffer (%#" PRIxLEAST64 ") to be treated as if it does not have VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT set, even though it does.",
                            (uint64_t)(pCommandBuffers[i]), (uint64_t)(pCB->commandBuffer));
                    pCB->beginInfo.flags &= ~VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
                }
            }
            if (!pCB->activeQueries.empty() &&
                !dev_data->physDevProperties.features.inheritedQueries) {
                skipCall |= log_msg(
                    dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                    VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                    reinterpret_cast<uint64_t>(pCommandBuffers[i]), __LINE__,
                    DRAWSTATE_INVALID_COMMAND_BUFFER, "DS",
                    "vkCmdExecuteCommands(): Secondary Command Buffer "
                    "(%#" PRIxLEAST64 ") cannot be submitted with a query in "
                                      "flight and inherited queries not "
                                      "supported on this device.",
                    reinterpret_cast<uint64_t>(pCommandBuffers[i]));
            }
            pSubCB->primaryCommandBuffer = pCB->commandBuffer;
            pCB->secondaryCommandBuffers.insert(pSubCB->commandBuffer);
            dev_data->globalInFlightCmdBuffers.insert(pSubCB->commandBuffer);
        }
        skipCall |= validatePrimaryCommandBuffer(dev_data, pCB, "vkCmdExecuteComands");
        skipCall |= addCmd(dev_data, pCB, CMD_EXECUTECOMMANDS, "vkCmdExecuteComands()");
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdExecuteCommands(commandBuffer, commandBuffersCount, pCommandBuffers);
}

VkBool32 ValidateMapImageLayouts(VkDevice device, VkDeviceMemory mem) {
    VkBool32 skip_call = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    auto mem_data = dev_data->memImageMap.find(mem);
    if (mem_data != dev_data->memImageMap.end()) {
        std::vector<VkImageLayout> layouts;
        if (FindLayouts(dev_data, mem_data->second, layouts)) {
            for (auto layout : layouts) {
                if (layout != VK_IMAGE_LAYOUT_PREINITIALIZED &&
                    layout != VK_IMAGE_LAYOUT_GENERAL) {
                    skip_call |= log_msg(
                        dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                        (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                        DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                        "Cannot map an image with layout %s. Only "
                        "GENERAL or PREINITIALIZED are supported.",
                        string_VkImageLayout(layout));
                }
            }
        }
    }
    return skip_call;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkMapMemory(
    VkDevice         device,
    VkDeviceMemory   mem,
    VkDeviceSize     offset,
    VkDeviceSize     size,
    VkFlags          flags,
    void           **ppData)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);

    VkBool32 skip_call = VK_FALSE;
    loader_platform_thread_lock_mutex(&globalLock);
    skip_call = ValidateMapImageLayouts(device, mem);
    loader_platform_thread_unlock_mutex(&globalLock);

    if (VK_FALSE == skip_call) {
        return dev_data->device_dispatch_table->MapMemory(device, mem, offset, size, flags, ppData);
    }
    return VK_ERROR_VALIDATION_FAILED_EXT;
}

VKAPI_ATTR VkResult VKAPI_CALL vkBindImageMemory(
    VkDevice                                    device,
    VkImage                                     image,
    VkDeviceMemory                              mem,
    VkDeviceSize                                memOffset)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->BindImageMemory(device, image, mem, memOffset);
    loader_platform_thread_lock_mutex(&globalLock);
    dev_data->memImageMap[mem] = image;
    loader_platform_thread_unlock_mutex(&globalLock);
    return result;
}


VKAPI_ATTR VkResult VKAPI_CALL vkSetEvent(VkDevice device, VkEvent event) {
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    dev_data->eventMap[event].needsSignaled = false;
    dev_data->eventMap[event].stageMask = VK_PIPELINE_STAGE_HOST_BIT;
    loader_platform_thread_unlock_mutex(&globalLock);
    VkResult result = dev_data->device_dispatch_table->SetEvent(device, event);
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL vkQueueBindSparse(
    VkQueue                                     queue,
    uint32_t                                    bindInfoCount,
    const VkBindSparseInfo*                     pBindInfo,
    VkFence                                     fence)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(queue), layer_data_map);
    VkBool32 skip_call = VK_FALSE;

    loader_platform_thread_lock_mutex(&globalLock);
    for (uint32_t bindIdx=0; bindIdx < bindInfoCount; ++bindIdx) {
        const VkBindSparseInfo& bindInfo = pBindInfo[bindIdx];
        for (uint32_t i=0; i < bindInfo.waitSemaphoreCount; ++i) {
            if (dev_data->semaphoreMap[bindInfo.pWaitSemaphores[i]].signaled) {
                dev_data->semaphoreMap[bindInfo.pWaitSemaphores[i]].signaled =
                    0;
            } else {
                skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, 0, __LINE__, DRAWSTATE_QUEUE_FORWARD_PROGRESS, "DS",
                        "Queue %#" PRIx64 " is waiting on semaphore %#" PRIx64 " that has no way to be signaled.",
                        (uint64_t)(queue), (uint64_t)(bindInfo.pWaitSemaphores[i]));
            }
        }
        for (uint32_t i=0; i < bindInfo.signalSemaphoreCount; ++i) {
            dev_data->semaphoreMap[bindInfo.pSignalSemaphores[i]].signaled = 1;
        }
    }
    loader_platform_thread_unlock_mutex(&globalLock);

    if (VK_FALSE == skip_call)
        return dev_data->device_dispatch_table->QueueBindSparse(queue, bindInfoCount, pBindInfo, fence);
    else
        return VK_ERROR_VALIDATION_FAILED_EXT;
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateSemaphore(
    VkDevice                                    device,
    const VkSemaphoreCreateInfo*                pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkSemaphore*                                pSemaphore)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->CreateSemaphore(device, pCreateInfo, pAllocator, pSemaphore);
    if (result == VK_SUCCESS) {
        loader_platform_thread_lock_mutex(&globalLock);
        dev_data->semaphoreMap[*pSemaphore].signaled = 0;
        dev_data->semaphoreMap[*pSemaphore].in_use.store(0);
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateSwapchainKHR(
    VkDevice                        device,
    const VkSwapchainCreateInfoKHR *pCreateInfo,
    const VkAllocationCallbacks    *pAllocator,
    VkSwapchainKHR                 *pSwapchain)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->CreateSwapchainKHR(device, pCreateInfo, pAllocator, pSwapchain);

    if (VK_SUCCESS == result) {
        SWAPCHAIN_NODE *swapchain_data = new SWAPCHAIN_NODE(pCreateInfo);
        loader_platform_thread_lock_mutex(&globalLock);
        dev_data->device_extensions.swapchainMap[*pSwapchain] = swapchain_data;
        loader_platform_thread_unlock_mutex(&globalLock);
    }

    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroySwapchainKHR(
    VkDevice                        device,
    VkSwapchainKHR                  swapchain,
    const VkAllocationCallbacks     *pAllocator)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);

    loader_platform_thread_lock_mutex(&globalLock);
    auto swapchain_data = dev_data->device_extensions.swapchainMap.find(swapchain);
    if (swapchain_data != dev_data->device_extensions.swapchainMap.end()) {
        if (swapchain_data->second->images.size() > 0) {
            for (auto swapchain_image : swapchain_data->second->images) {
                auto image_sub =
                    dev_data->imageSubresourceMap.find(swapchain_image);
                if (image_sub != dev_data->imageSubresourceMap.end()) {
                    for (auto imgsubpair : image_sub->second) {
                        auto image_item =
                            dev_data->imageLayoutMap.find(imgsubpair);
                        if (image_item != dev_data->imageLayoutMap.end()) {
                            dev_data->imageLayoutMap.erase(image_item);
                        }
                    }
                    dev_data->imageSubresourceMap.erase(image_sub);
                }
            }
        }
        delete swapchain_data->second;
        dev_data->device_extensions.swapchainMap.erase(swapchain);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    dev_data->device_dispatch_table->DestroySwapchainKHR(device, swapchain, pAllocator);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkGetSwapchainImagesKHR(
    VkDevice                device,
    VkSwapchainKHR          swapchain,
    uint32_t*               pCount,
    VkImage*                pSwapchainImages)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->GetSwapchainImagesKHR(device, swapchain, pCount, pSwapchainImages);

    if (result == VK_SUCCESS && pSwapchainImages != NULL) {
        // This should never happen and is checked by param checker.
        if (!pCount) return result;
        loader_platform_thread_lock_mutex(&globalLock);
        for (uint32_t i = 0; i < *pCount; ++i) {
            IMAGE_NODE image_node;
            image_node.layout = VK_IMAGE_LAYOUT_UNDEFINED;
            auto swapchain_node = dev_data->device_extensions.swapchainMap[swapchain];
            image_node.format = swapchain_node->createInfo.imageFormat;
            swapchain_node->images.push_back(pSwapchainImages[i]);
            ImageSubresourcePair subpair = {pSwapchainImages[i], false,
                                            VkImageSubresource()};
            dev_data->imageSubresourceMap[pSwapchainImages[i]].push_back(
                subpair);
            dev_data->imageLayoutMap[subpair] = image_node;
            dev_data->device_extensions
                .imageToSwapchainMap[pSwapchainImages[i]] = swapchain;
        }
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkQueuePresentKHR(VkQueue queue, const VkPresentInfoKHR* pPresentInfo)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(queue), layer_data_map);
    VkBool32 skip_call = VK_FALSE;

    if (pPresentInfo) {
        loader_platform_thread_lock_mutex(&globalLock);
        for (uint32_t i=0; i < pPresentInfo->waitSemaphoreCount; ++i) {
            if (dev_data->semaphoreMap[pPresentInfo->pWaitSemaphores[i]]
                    .signaled) {
                dev_data->semaphoreMap[pPresentInfo->pWaitSemaphores[i]]
                    .signaled = 0;
            } else {
                skip_call |= log_msg(
                    dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                    VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, 0, __LINE__,
                    DRAWSTATE_QUEUE_FORWARD_PROGRESS, "DS",
                    "Queue %#" PRIx64 " is waiting on semaphore %#" PRIx64
                    " that has no way to be signaled.",
                    (uint64_t)(queue),
                    (uint64_t)(pPresentInfo->pWaitSemaphores[i]));
            }
        }
        for (uint32_t i = 0; i < pPresentInfo->swapchainCount; ++i) {
            auto swapchain_data = dev_data->device_extensions.swapchainMap.find(pPresentInfo->pSwapchains[i]);
            if (swapchain_data != dev_data->device_extensions.swapchainMap.end() && pPresentInfo->pImageIndices[i] < swapchain_data->second->images.size()) {
                VkImage image = swapchain_data->second->images[pPresentInfo->pImageIndices[i]];
                vector<VkImageLayout> layouts;
                if (FindLayouts(dev_data, image, layouts)) {
                    for (auto layout : layouts) {
                        if (layout != VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
                            skip_call |= log_msg(
                                dev_data->report_data,
                                VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT,
                                reinterpret_cast<uint64_t &>(queue), __LINE__,
                                DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                                "Images passed to present must be in layout "
                                "PRESENT_SOURCE_KHR but is in %s",
                                string_VkImageLayout(layout));
                        }
                    }
                }
            }
        }
        loader_platform_thread_unlock_mutex(&globalLock);
    }

    if (VK_FALSE == skip_call)
        return dev_data->device_dispatch_table->QueuePresentKHR(queue, pPresentInfo);
    return VK_ERROR_VALIDATION_FAILED_EXT;
}

VKAPI_ATTR VkResult VKAPI_CALL vkAcquireNextImageKHR(
    VkDevice                                    device,
    VkSwapchainKHR                              swapchain,
    uint64_t                                    timeout,
    VkSemaphore                                 semaphore,
    VkFence                                     fence,
    uint32_t*                                   pImageIndex)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->AcquireNextImageKHR(device, swapchain, timeout, semaphore, fence, pImageIndex);
    loader_platform_thread_lock_mutex(&globalLock);
    // FIXME/TODO: Need to add some thing code the "fence" parameter
    dev_data->semaphoreMap[semaphore].signaled = 1;
    loader_platform_thread_unlock_mutex(&globalLock);
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugReportCallbackEXT(
        VkInstance                                      instance,
        const VkDebugReportCallbackCreateInfoEXT*       pCreateInfo,
        const VkAllocationCallbacks*                    pAllocator,
        VkDebugReportCallbackEXT*                       pMsgCallback)
{
    layer_data* my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    VkLayerInstanceDispatchTable *pTable = my_data->instance_dispatch_table;
    VkResult res = pTable->CreateDebugReportCallbackEXT(instance, pCreateInfo, pAllocator, pMsgCallback);
    if (VK_SUCCESS == res) {
        loader_platform_thread_lock_mutex(&globalLock);
        res = layer_create_msg_callback(my_data->report_data, pCreateInfo, pAllocator, pMsgCallback);
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return res;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyDebugReportCallbackEXT(
        VkInstance                                  instance,
        VkDebugReportCallbackEXT                 msgCallback,
        const VkAllocationCallbacks*                pAllocator)
{
    layer_data* my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    VkLayerInstanceDispatchTable *pTable = my_data->instance_dispatch_table;
    pTable->DestroyDebugReportCallbackEXT(instance, msgCallback, pAllocator);
    loader_platform_thread_lock_mutex(&globalLock);
    layer_destroy_msg_callback(my_data->report_data, msgCallback, pAllocator);
    loader_platform_thread_unlock_mutex(&globalLock);
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
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    my_data->instance_dispatch_table->DebugReportMessageEXT(instance, flags, objType, object, location, msgCode, pLayerPrefix, pMsg);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdDbgMarkerBegin(VkCommandBuffer commandBuffer, const char* pMarker)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (!dev_data->device_extensions.debug_marker_enabled) {
        skipCall |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, (uint64_t)commandBuffer, __LINE__, DRAWSTATE_INVALID_EXTENSION, "DS",
                "Attempt to use CmdDbgMarkerBegin but extension disabled!");
        return;
    } else if (pCB) {
        skipCall |= addCmd(dev_data, pCB, CMD_DBGMARKERBEGIN, "vkCmdDbgMarkerBegin()");
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall)
        debug_marker_dispatch_table(commandBuffer)->CmdDbgMarkerBegin(commandBuffer, pMarker);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdDbgMarkerEnd(VkCommandBuffer commandBuffer)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (!dev_data->device_extensions.debug_marker_enabled) {
        skipCall |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, (uint64_t)commandBuffer, __LINE__, DRAWSTATE_INVALID_EXTENSION, "DS",
                "Attempt to use CmdDbgMarkerEnd but extension disabled!");
        return;
    } else if (pCB) {
        skipCall |= addCmd(dev_data, pCB, CMD_DBGMARKEREND, "vkCmdDbgMarkerEnd()");
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall)
        debug_marker_dispatch_table(commandBuffer)->CmdDbgMarkerEnd(commandBuffer);
}

VK_LAYER_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr(VkDevice dev, const char* funcName)
{
    if (!strcmp(funcName, "vkGetDeviceProcAddr"))
        return (PFN_vkVoidFunction) vkGetDeviceProcAddr;
    if (!strcmp(funcName, "vkDestroyDevice"))
        return (PFN_vkVoidFunction) vkDestroyDevice;
    if (!strcmp(funcName, "vkQueueSubmit"))
        return (PFN_vkVoidFunction) vkQueueSubmit;
    if (!strcmp(funcName, "vkWaitForFences"))
        return (PFN_vkVoidFunction) vkWaitForFences;
    if (!strcmp(funcName, "vkGetFenceStatus"))
        return (PFN_vkVoidFunction) vkGetFenceStatus;
    if (!strcmp(funcName, "vkQueueWaitIdle"))
        return (PFN_vkVoidFunction) vkQueueWaitIdle;
    if (!strcmp(funcName, "vkDeviceWaitIdle"))
        return (PFN_vkVoidFunction) vkDeviceWaitIdle;
    if (!strcmp(funcName, "vkGetDeviceQueue"))
        return (PFN_vkVoidFunction) vkGetDeviceQueue;
    if (!strcmp(funcName, "vkDestroyInstance"))
        return (PFN_vkVoidFunction) vkDestroyInstance;
    if (!strcmp(funcName, "vkDestroyDevice"))
        return (PFN_vkVoidFunction) vkDestroyDevice;
    if (!strcmp(funcName, "vkDestroyFence"))
        return (PFN_vkVoidFunction) vkDestroyFence;
    if (!strcmp(funcName, "vkResetFences"))
        return (PFN_vkVoidFunction)vkResetFences;
    if (!strcmp(funcName, "vkDestroySemaphore"))
        return (PFN_vkVoidFunction) vkDestroySemaphore;
    if (!strcmp(funcName, "vkDestroyEvent"))
        return (PFN_vkVoidFunction) vkDestroyEvent;
    if (!strcmp(funcName, "vkDestroyQueryPool"))
        return (PFN_vkVoidFunction) vkDestroyQueryPool;
    if (!strcmp(funcName, "vkDestroyBuffer"))
        return (PFN_vkVoidFunction) vkDestroyBuffer;
    if (!strcmp(funcName, "vkDestroyBufferView"))
        return (PFN_vkVoidFunction) vkDestroyBufferView;
    if (!strcmp(funcName, "vkDestroyImage"))
        return (PFN_vkVoidFunction) vkDestroyImage;
    if (!strcmp(funcName, "vkDestroyImageView"))
        return (PFN_vkVoidFunction) vkDestroyImageView;
    if (!strcmp(funcName, "vkDestroyShaderModule"))
        return (PFN_vkVoidFunction) vkDestroyShaderModule;
    if (!strcmp(funcName, "vkDestroyPipeline"))
        return (PFN_vkVoidFunction) vkDestroyPipeline;
    if (!strcmp(funcName, "vkDestroyPipelineLayout"))
        return (PFN_vkVoidFunction) vkDestroyPipelineLayout;
    if (!strcmp(funcName, "vkDestroySampler"))
        return (PFN_vkVoidFunction) vkDestroySampler;
    if (!strcmp(funcName, "vkDestroyDescriptorSetLayout"))
        return (PFN_vkVoidFunction) vkDestroyDescriptorSetLayout;
    if (!strcmp(funcName, "vkDestroyDescriptorPool"))
        return (PFN_vkVoidFunction) vkDestroyDescriptorPool;
    if (!strcmp(funcName, "vkDestroyFramebuffer"))
        return (PFN_vkVoidFunction) vkDestroyFramebuffer;
    if (!strcmp(funcName, "vkDestroyRenderPass"))
        return (PFN_vkVoidFunction) vkDestroyRenderPass;
    if (!strcmp(funcName, "vkCreateBuffer"))
        return (PFN_vkVoidFunction) vkCreateBuffer;
    if (!strcmp(funcName, "vkCreateBufferView"))
        return (PFN_vkVoidFunction) vkCreateBufferView;
    if (!strcmp(funcName, "vkCreateImage"))
        return (PFN_vkVoidFunction) vkCreateImage;
    if (!strcmp(funcName, "vkCreateImageView"))
        return (PFN_vkVoidFunction) vkCreateImageView;
    if (!strcmp(funcName, "vkCreateFence"))
        return (PFN_vkVoidFunction) vkCreateFence;
    if (!strcmp(funcName, "CreatePipelineCache"))
        return (PFN_vkVoidFunction) vkCreatePipelineCache;
    if (!strcmp(funcName, "DestroyPipelineCache"))
        return (PFN_vkVoidFunction) vkDestroyPipelineCache;
    if (!strcmp(funcName, "GetPipelineCacheData"))
        return (PFN_vkVoidFunction) vkGetPipelineCacheData;
    if (!strcmp(funcName, "MergePipelineCaches"))
        return (PFN_vkVoidFunction) vkMergePipelineCaches;
    if (!strcmp(funcName, "vkCreateGraphicsPipelines"))
        return (PFN_vkVoidFunction) vkCreateGraphicsPipelines;
    if (!strcmp(funcName, "vkCreateComputePipelines"))
        return (PFN_vkVoidFunction) vkCreateComputePipelines;
    if (!strcmp(funcName, "vkCreateSampler"))
        return (PFN_vkVoidFunction) vkCreateSampler;
    if (!strcmp(funcName, "vkCreateDescriptorSetLayout"))
        return (PFN_vkVoidFunction) vkCreateDescriptorSetLayout;
    if (!strcmp(funcName, "vkCreatePipelineLayout"))
        return (PFN_vkVoidFunction) vkCreatePipelineLayout;
    if (!strcmp(funcName, "vkCreateDescriptorPool"))
        return (PFN_vkVoidFunction) vkCreateDescriptorPool;
    if (!strcmp(funcName, "vkResetDescriptorPool"))
        return (PFN_vkVoidFunction) vkResetDescriptorPool;
    if (!strcmp(funcName, "vkAllocateDescriptorSets"))
        return (PFN_vkVoidFunction) vkAllocateDescriptorSets;
    if (!strcmp(funcName, "vkFreeDescriptorSets"))
        return (PFN_vkVoidFunction) vkFreeDescriptorSets;
    if (!strcmp(funcName, "vkUpdateDescriptorSets"))
        return (PFN_vkVoidFunction) vkUpdateDescriptorSets;
    if (!strcmp(funcName, "vkCreateCommandPool"))
        return (PFN_vkVoidFunction) vkCreateCommandPool;
    if (!strcmp(funcName, "vkDestroyCommandPool"))
        return (PFN_vkVoidFunction) vkDestroyCommandPool;
    if (!strcmp(funcName, "vkResetCommandPool"))
        return (PFN_vkVoidFunction) vkResetCommandPool;
    if (!strcmp(funcName, "vkCreateQueryPool"))
        return (PFN_vkVoidFunction)vkCreateQueryPool;
    if (!strcmp(funcName, "vkAllocateCommandBuffers"))
        return (PFN_vkVoidFunction) vkAllocateCommandBuffers;
    if (!strcmp(funcName, "vkFreeCommandBuffers"))
        return (PFN_vkVoidFunction) vkFreeCommandBuffers;
    if (!strcmp(funcName, "vkBeginCommandBuffer"))
        return (PFN_vkVoidFunction) vkBeginCommandBuffer;
    if (!strcmp(funcName, "vkEndCommandBuffer"))
        return (PFN_vkVoidFunction) vkEndCommandBuffer;
    if (!strcmp(funcName, "vkResetCommandBuffer"))
        return (PFN_vkVoidFunction) vkResetCommandBuffer;
    if (!strcmp(funcName, "vkCmdBindPipeline"))
        return (PFN_vkVoidFunction) vkCmdBindPipeline;
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
    if (!strcmp(funcName, "vkCmdClearDepthStencilImage"))
        return (PFN_vkVoidFunction) vkCmdClearDepthStencilImage;
    if (!strcmp(funcName, "vkCmdClearAttachments"))
        return (PFN_vkVoidFunction) vkCmdClearAttachments;
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
    if (!strcmp(funcName, "vkCreateFramebuffer"))
        return (PFN_vkVoidFunction) vkCreateFramebuffer;
    if (!strcmp(funcName, "vkCreateShaderModule"))
        return (PFN_vkVoidFunction) vkCreateShaderModule;
    if (!strcmp(funcName, "vkCreateRenderPass"))
        return (PFN_vkVoidFunction) vkCreateRenderPass;
    if (!strcmp(funcName, "vkCmdBeginRenderPass"))
        return (PFN_vkVoidFunction) vkCmdBeginRenderPass;
    if (!strcmp(funcName, "vkCmdNextSubpass"))
        return (PFN_vkVoidFunction) vkCmdNextSubpass;
    if (!strcmp(funcName, "vkCmdEndRenderPass"))
        return (PFN_vkVoidFunction) vkCmdEndRenderPass;
    if (!strcmp(funcName, "vkCmdExecuteCommands"))
        return (PFN_vkVoidFunction) vkCmdExecuteCommands;
    if (!strcmp(funcName, "vkSetEvent"))
        return (PFN_vkVoidFunction) vkSetEvent;
    if (!strcmp(funcName, "vkMapMemory"))
        return (PFN_vkVoidFunction) vkMapMemory;
    if (!strcmp(funcName, "vkGetQueryPoolResults"))
        return (PFN_vkVoidFunction) vkGetQueryPoolResults;
    if (!strcmp(funcName, "vkBindImageMemory"))
        return (PFN_vkVoidFunction) vkBindImageMemory;
    if (!strcmp(funcName, "vkQueueBindSparse"))
        return (PFN_vkVoidFunction) vkQueueBindSparse;
    if (!strcmp(funcName, "vkCreateSemaphore"))
        return (PFN_vkVoidFunction) vkCreateSemaphore;

    if (dev == NULL)
        return NULL;

    layer_data *dev_data;
    dev_data = get_my_data_ptr(get_dispatch_key(dev), layer_data_map);

    if (dev_data->device_extensions.wsi_enabled)
    {
        if (!strcmp(funcName, "vkCreateSwapchainKHR"))
            return (PFN_vkVoidFunction) vkCreateSwapchainKHR;
        if (!strcmp(funcName, "vkDestroySwapchainKHR"))
            return (PFN_vkVoidFunction) vkDestroySwapchainKHR;
        if (!strcmp(funcName, "vkGetSwapchainImagesKHR"))
            return (PFN_vkVoidFunction) vkGetSwapchainImagesKHR;
        if (!strcmp(funcName, "vkAcquireNextImageKHR"))
            return (PFN_vkVoidFunction) vkAcquireNextImageKHR;
        if (!strcmp(funcName, "vkQueuePresentKHR"))
            return (PFN_vkVoidFunction) vkQueuePresentKHR;
    }

    VkLayerDispatchTable* pTable = dev_data->device_dispatch_table;
    if (dev_data->device_extensions.debug_marker_enabled)
    {
        if (!strcmp(funcName, "vkCmdDbgMarkerBegin"))
            return (PFN_vkVoidFunction) vkCmdDbgMarkerBegin;
        if (!strcmp(funcName, "vkCmdDbgMarkerEnd"))
            return (PFN_vkVoidFunction) vkCmdDbgMarkerEnd;
    }
    {
        if (pTable->GetDeviceProcAddr == NULL)
            return NULL;
        return pTable->GetDeviceProcAddr(dev, funcName);
    }
}

VK_LAYER_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance instance, const char* funcName)
{
    if (!strcmp(funcName, "vkGetInstanceProcAddr"))
        return (PFN_vkVoidFunction) vkGetInstanceProcAddr;
    if (!strcmp(funcName, "vkGetDeviceProcAddr"))
        return (PFN_vkVoidFunction) vkGetDeviceProcAddr;
    if (!strcmp(funcName, "vkCreateInstance"))
        return (PFN_vkVoidFunction) vkCreateInstance;
    if (!strcmp(funcName, "vkCreateDevice"))
        return (PFN_vkVoidFunction) vkCreateDevice;
    if (!strcmp(funcName, "vkDestroyInstance"))
        return (PFN_vkVoidFunction) vkDestroyInstance;
    if (!strcmp(funcName, "vkEnumerateInstanceLayerProperties"))
        return (PFN_vkVoidFunction) vkEnumerateInstanceLayerProperties;
    if (!strcmp(funcName, "vkEnumerateInstanceExtensionProperties"))
        return (PFN_vkVoidFunction) vkEnumerateInstanceExtensionProperties;
    if (!strcmp(funcName, "vkEnumerateDeviceLayerProperties"))
        return (PFN_vkVoidFunction) vkEnumerateDeviceLayerProperties;
    if (!strcmp(funcName, "vkEnumerateDeviceExtensionProperties"))
        return (PFN_vkVoidFunction) vkEnumerateDeviceExtensionProperties;

    if (instance == NULL)
        return NULL;

    PFN_vkVoidFunction fptr;

    layer_data* my_data;
    my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    fptr = debug_report_get_instance_proc_addr(my_data->report_data, funcName);
    if (fptr)
        return fptr;

    VkLayerInstanceDispatchTable* pTable = my_data->instance_dispatch_table;
    if (pTable->GetInstanceProcAddr == NULL)
        return NULL;
    return pTable->GetInstanceProcAddr(instance, funcName);
}
