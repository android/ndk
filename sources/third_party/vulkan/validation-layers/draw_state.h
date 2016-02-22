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
 * Author: Courtney Goeltzenleuchter <courtneygo@google.com>
 * Author: Tobin Ehlis <tobine@google.com>
 * Author: Chris Forbes <chrisf@ijw.co.nz>
 * Author: Mark Lobodzinski <mark@lunarg.com>
 */

#include "vulkan/vk_layer.h"
#include <atomic>
#include <vector>
#include <memory>

using std::vector;

// Draw State ERROR codes
typedef enum _DRAW_STATE_ERROR {
    DRAWSTATE_NONE,           // Used for INFO & other non-error messages
    DRAWSTATE_INTERNAL_ERROR, // Error with DrawState internal data structures
    DRAWSTATE_NO_PIPELINE_BOUND,       // Unable to identify a bound pipeline
    DRAWSTATE_INVALID_POOL,            // Invalid DS pool
    DRAWSTATE_INVALID_SET,             // Invalid DS
    DRAWSTATE_INVALID_LAYOUT,          // Invalid DS layout
    DRAWSTATE_INVALID_IMAGE_LAYOUT,    // Invalid Image layout
    DRAWSTATE_INVALID_PIPELINE,        // Invalid Pipeline handle referenced
    DRAWSTATE_INVALID_PIPELINE_LAYOUT, // Invalid PipelineLayout
    DRAWSTATE_INVALID_PIPELINE_CREATE_STATE, // Attempt to create a pipeline
                                             // with invalid state
    DRAWSTATE_INVALID_COMMAND_BUFFER,        // Invalid CommandBuffer referenced
    DRAWSTATE_INVALID_BARRIER,               // Invalid Barrier
    DRAWSTATE_INVALID_BUFFER,                // Invalid Buffer
    DRAWSTATE_INVALID_QUERY,                 // Invalid Query
    DRAWSTATE_INVALID_FENCE,                 // Invalid Fence
    DRAWSTATE_INVALID_SEMAPHORE,             // Invalid Semaphore
    DRAWSTATE_INVALID_EVENT,                 // Invalid Event
    DRAWSTATE_VTX_INDEX_OUT_OF_BOUNDS,   // binding in vkCmdBindVertexData() too
                                         // large for PSO's
                                         // pVertexBindingDescriptions array
    DRAWSTATE_VTX_INDEX_ALIGNMENT_ERROR, // binding offset in
                                         // vkCmdBindIndexBuffer() out of
                                         // alignment based on indexType
    // DRAWSTATE_MISSING_DOT_PROGRAM,              // No "dot" program in order
    // to generate png image
    DRAWSTATE_OUT_OF_MEMORY,            // malloc failed
    DRAWSTATE_INVALID_DESCRIPTOR_SET,   // Descriptor Set handle is unknown
    DRAWSTATE_DESCRIPTOR_TYPE_MISMATCH, // Type in layout vs. update are not the
                                        // same
    DRAWSTATE_DESCRIPTOR_STAGEFLAGS_MISMATCH,  // StageFlags in layout are not
                                               // the same throughout a single
                                               // VkWriteDescriptorSet update
    DRAWSTATE_DESCRIPTOR_UPDATE_OUT_OF_BOUNDS, // Descriptors set for update out
                                               // of bounds for corresponding
                                               // layout section
    DRAWSTATE_DESCRIPTOR_POOL_EMPTY, // Attempt to allocate descriptor from a
                                     // pool with no more descriptors of that
                                     // type available
    DRAWSTATE_CANT_FREE_FROM_NON_FREE_POOL, // Invalid to call
                                            // vkFreeDescriptorSets on Sets
                                            // allocated from a NON_FREE Pool
    DRAWSTATE_INVALID_UPDATE_INDEX,  // Index of requested update is invalid for
                                     // specified descriptors set
    DRAWSTATE_INVALID_UPDATE_STRUCT, // Struct in DS Update tree is of invalid
                                     // type
    DRAWSTATE_NUM_SAMPLES_MISMATCH,  // Number of samples in bound PSO does not
                                     // match number in FB of current RenderPass
    DRAWSTATE_NO_END_COMMAND_BUFFER, // Must call vkEndCommandBuffer() before
                                     // QueueSubmit on that commandBuffer
    DRAWSTATE_NO_BEGIN_COMMAND_BUFFER, // Binding cmds or calling End on CB that
                                       // never had vkBeginCommandBuffer()
                                       // called on it
    DRAWSTATE_COMMAND_BUFFER_SINGLE_SUBMIT_VIOLATION, // Cmd Buffer created with
    // VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    // flag is submitted
    // multiple times
    DRAWSTATE_INVALID_SECONDARY_COMMAND_BUFFER, // vkCmdExecuteCommands() called
                                                // with a primary commandBuffer
                                                // in pCommandBuffers array
    DRAWSTATE_VIEWPORT_NOT_BOUND, // Draw submitted with no viewport state bound
    DRAWSTATE_SCISSOR_NOT_BOUND,  // Draw submitted with no scissor state bound
    DRAWSTATE_LINE_WIDTH_NOT_BOUND, // Draw submitted with no line width state
                                    // bound
    DRAWSTATE_DEPTH_BIAS_NOT_BOUND, // Draw submitted with no depth bias state
                                    // bound
    DRAWSTATE_BLEND_NOT_BOUND, // Draw submitted with no blend state bound when
                               // color write enabled
    DRAWSTATE_DEPTH_BOUNDS_NOT_BOUND, // Draw submitted with no depth bounds
                                      // state bound when depth enabled
    DRAWSTATE_STENCIL_NOT_BOUND, // Draw submitted with no stencil state bound
                                 // when stencil enabled
    DRAWSTATE_INDEX_BUFFER_NOT_BOUND, // Draw submitted with no depth-stencil
                                      // state bound when depth write enabled
    DRAWSTATE_PIPELINE_LAYOUTS_INCOMPATIBLE, // Draw submitted PSO Pipeline
                                             // layout that's not compatible
                                             // with layout from
                                             // BindDescriptorSets
    DRAWSTATE_RENDERPASS_INCOMPATIBLE,  // Incompatible renderpasses between
                                        // secondary cmdBuffer and primary
                                        // cmdBuffer or framebuffer
    DRAWSTATE_FRAMEBUFFER_INCOMPATIBLE, // Incompatible framebuffer between
                                        // secondary cmdBuffer and active
                                        // renderPass
    DRAWSTATE_INVALID_RENDERPASS,       // Use of a NULL or otherwise invalid
                                        // RenderPass object
    DRAWSTATE_INVALID_RENDERPASS_CMD,   // Invalid cmd submitted while a
                                        // RenderPass is active
    DRAWSTATE_NO_ACTIVE_RENDERPASS, // Rendering cmd submitted without an active
                                    // RenderPass
    DRAWSTATE_DESCRIPTOR_SET_NOT_UPDATED, // DescriptorSet bound but it was
                                          // never updated. This is a warning
                                          // code.
    DRAWSTATE_DESCRIPTOR_SET_NOT_BOUND,   // DescriptorSet used by pipeline at
                                          // draw time is not bound, or has been
                                          // disturbed (which would have flagged
                                          // previous warning)
    DRAWSTATE_INVALID_DYNAMIC_OFFSET_COUNT, // DescriptorSets bound with
                                            // different number of dynamic
                                            // descriptors that were included in
                                            // dynamicOffsetCount
    DRAWSTATE_CLEAR_CMD_BEFORE_DRAW, // Clear cmd issued before any Draw in
                                     // CommandBuffer, should use RenderPass Ops
                                     // instead
    DRAWSTATE_BEGIN_CB_INVALID_STATE, // CB state at Begin call is bad. Can be
                                      // Primary/Secondary CB created with
                                      // mismatched FB/RP information or CB in
                                      // RECORDING state
    DRAWSTATE_INVALID_CB_SIMULTANEOUS_USE, // CmdBuffer is being used in
                                           // violation of
    // VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT
    // rules (i.e. simultaneous use w/o
    // that bit set)
    DRAWSTATE_INVALID_COMMAND_BUFFER_RESET, // Attempting to call Reset (or
                                            // Begin on recorded cmdBuffer) that
                                            // was allocated from Pool w/o
    // VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
    // bit set
    DRAWSTATE_VIEWPORT_SCISSOR_MISMATCH, // Count for viewports and scissors
                                         // mismatch and/or state doesn't match
                                         // count
    DRAWSTATE_INVALID_IMAGE_ASPECT, // Image aspect is invalid for the current
                                    // operation
    DRAWSTATE_MISSING_ATTACHMENT_REFERENCE, // Attachment reference must be
                                            // present in active subpass
    DRAWSTATE_INVALID_EXTENSION,
    DRAWSTATE_SAMPLER_DESCRIPTOR_ERROR, // A Descriptor of *_SAMPLER type is
                                        // being updated with an invalid or bad
                                        // Sampler
    DRAWSTATE_INCONSISTENT_IMMUTABLE_SAMPLER_UPDATE, // Descriptors of
                                                     // *COMBINED_IMAGE_SAMPLER
                                                     // type are being updated
                                                     // where some, but not all,
                                                     // of the updates use
                                                     // immutable samplers
    DRAWSTATE_IMAGEVIEW_DESCRIPTOR_ERROR,  // A Descriptor of *_IMAGE or
                                           // *_ATTACHMENT type is being updated
                                           // with an invalid or bad ImageView
    DRAWSTATE_BUFFERVIEW_DESCRIPTOR_ERROR, // A Descriptor of *_TEXEL_BUFFER
                                           // type is being updated with an
                                           // invalid or bad BufferView
    DRAWSTATE_BUFFERINFO_DESCRIPTOR_ERROR, // A Descriptor of
                                           // *_[UNIFORM|STORAGE]_BUFFER_[DYNAMIC]
                                           // type is being updated with an
                                           // invalid or bad BufferView
    DRAWSTATE_DYNAMIC_OFFSET_OVERFLOW, // At draw time the dynamic offset
                                       // combined with buffer offset and range
                                       // oversteps size of buffer
    DRAWSTATE_DOUBLE_DESTROY,          // Destroying an object twice
    DRAWSTATE_OBJECT_INUSE, // Destroying or modifying an object in use by a
                            // command buffer
    DRAWSTATE_QUEUE_FORWARD_PROGRESS, // Queue cannot guarantee forward progress
    DRAWSTATE_INVALID_UNIFORM_BUFFER_OFFSET, // Dynamic Uniform Buffer Offsets
                                             // violate device limit
    DRAWSTATE_INVALID_STORAGE_BUFFER_OFFSET, // Dynamic Storage Buffer Offsets
                                             // violate device limit
    DRAWSTATE_INDEPENDENT_BLEND, // If independent blending is not enabled, all
                                 // elements of pAttachmentsMustBeIdentical
    DRAWSTATE_DISABLED_LOGIC_OP, // If logic operations is not enabled, logicOpEnable
                                 // must be VK_FALSE
    DRAWSTATE_INVALID_LOGIC_OP,  // If logicOpEnable is VK_TRUE, logicOp must
                                 // must be a valid VkLogicOp value
} DRAW_STATE_ERROR;

typedef enum _SHADER_CHECKER_ERROR {
    SHADER_CHECKER_NONE,
    SHADER_CHECKER_FS_MIXED_BROADCAST,      /* FS writes broadcast output AND custom outputs */
    SHADER_CHECKER_INTERFACE_TYPE_MISMATCH, /* Type mismatch between shader stages or shader and pipeline */
    SHADER_CHECKER_OUTPUT_NOT_CONSUMED,     /* Entry appears in output interface, but missing in input */
    SHADER_CHECKER_INPUT_NOT_PRODUCED,      /* Entry appears in input interface, but missing in output */
    SHADER_CHECKER_NON_SPIRV_SHADER,        /* Shader image is not SPIR-V */
    SHADER_CHECKER_INCONSISTENT_SPIRV,      /* General inconsistency within a SPIR-V module */
    SHADER_CHECKER_UNKNOWN_STAGE,           /* Stage is not supported by analysis */
    SHADER_CHECKER_INCONSISTENT_VI,         /* VI state contains conflicting binding or attrib descriptions */
    SHADER_CHECKER_MISSING_DESCRIPTOR,      /* Shader attempts to use a descriptor binding not declared in the layout */
    SHADER_CHECKER_BAD_SPECIALIZATION,      /* Specialization map entry points outside specialization data block */
    SHADER_CHECKER_MISSING_ENTRYPOINT,      /* Shader module does not contain the requested entrypoint */
} SHADER_CHECKER_ERROR;

typedef enum _DRAW_TYPE
{
    DRAW                  = 0,
    DRAW_INDEXED          = 1,
    DRAW_INDIRECT         = 2,
    DRAW_INDEXED_INDIRECT = 3,
    DRAW_BEGIN_RANGE      = DRAW,
    DRAW_END_RANGE        = DRAW_INDEXED_INDIRECT,
    NUM_DRAW_TYPES        = (DRAW_END_RANGE - DRAW_BEGIN_RANGE + 1),
} DRAW_TYPE;

typedef struct _SHADER_DS_MAPPING {
    uint32_t slotCount;
    VkDescriptorSetLayoutCreateInfo* pShaderMappingSlot;
} SHADER_DS_MAPPING;

typedef struct _GENERIC_HEADER {
    VkStructureType sType;
    const void*    pNext;
} GENERIC_HEADER;

typedef struct _PIPELINE_NODE {
    VkPipeline                              pipeline;
    VkGraphicsPipelineCreateInfo            graphicsPipelineCI;
    VkPipelineVertexInputStateCreateInfo    vertexInputCI;
    VkPipelineInputAssemblyStateCreateInfo  iaStateCI;
    VkPipelineTessellationStateCreateInfo   tessStateCI;
    VkPipelineViewportStateCreateInfo       vpStateCI;
    VkPipelineRasterizationStateCreateInfo  rsStateCI;
    VkPipelineMultisampleStateCreateInfo    msStateCI;
    VkPipelineColorBlendStateCreateInfo     cbStateCI;
    VkPipelineDepthStencilStateCreateInfo   dsStateCI;
    VkPipelineDynamicStateCreateInfo        dynStateCI;
    VkPipelineShaderStageCreateInfo         vsCI;
    VkPipelineShaderStageCreateInfo         tcsCI;
    VkPipelineShaderStageCreateInfo         tesCI;
    VkPipelineShaderStageCreateInfo         gsCI;
    VkPipelineShaderStageCreateInfo         fsCI;
    // Compute shader is include in VkComputePipelineCreateInfo
    VkComputePipelineCreateInfo          computePipelineCI;
    // Flag of which shader stages are active for this pipeline
    uint32_t                             active_shaders;
    // Capture which sets are actually used by the shaders of this pipeline
    std::set<unsigned>                   active_sets;
    // Vtx input info (if any)
    uint32_t                             vtxBindingCount;   // number of bindings
    VkVertexInputBindingDescription*     pVertexBindingDescriptions;
    uint32_t                             vtxAttributeCount; // number of attributes
    VkVertexInputAttributeDescription*   pVertexAttributeDescriptions;
    uint32_t                             attachmentCount;   // number of CB attachments
    VkPipelineColorBlendAttachmentState* pAttachments;
    // Default constructor
    _PIPELINE_NODE():pipeline{},
                     graphicsPipelineCI{},
                     vertexInputCI{},
                     iaStateCI{},
                     tessStateCI{},
                     vpStateCI{},
                     rsStateCI{},
                     msStateCI{},
                     cbStateCI{},
                     dsStateCI{},
                     dynStateCI{},
                     vsCI{},
                     tcsCI{},
                     tesCI{},
                     gsCI{},
                     fsCI{},
                     computePipelineCI{},
                     active_shaders(0),
                     vtxBindingCount(0),
                     pVertexBindingDescriptions(0),
                     vtxAttributeCount(0),
                     pVertexAttributeDescriptions(0),
                     attachmentCount(0),
                     pAttachments(0)
                     {};
} PIPELINE_NODE;

class BASE_NODE {
  public:
    std::atomic_int in_use;
};

typedef struct _SAMPLER_NODE {
    VkSampler           sampler;
    VkSamplerCreateInfo createInfo;

    _SAMPLER_NODE(const VkSampler* ps, const VkSamplerCreateInfo* pci) : sampler(*ps), createInfo(*pci) {};
} SAMPLER_NODE;

typedef struct _IMAGE_NODE {
    VkImageLayout layout;
    VkFormat      format;
} IMAGE_NODE;

typedef struct _IMAGE_CMD_BUF_NODE {
    VkImageLayout initialLayout;
    VkImageLayout layout;
} IMAGE_CMD_BUF_NODE;

class BUFFER_NODE : public BASE_NODE {
  public:
    using BASE_NODE::in_use;
    unique_ptr<VkBufferCreateInfo> create_info;
};

struct RENDER_PASS_NODE {
    VkRenderPassCreateInfo const* pCreateInfo;
    std::vector<bool> hasSelfDependency;
    vector<std::vector<VkFormat>> subpassColorFormats;

    RENDER_PASS_NODE(VkRenderPassCreateInfo const *pCreateInfo) : pCreateInfo(pCreateInfo)
    {
        uint32_t i;

        subpassColorFormats.reserve(pCreateInfo->subpassCount);
        for (i = 0; i < pCreateInfo->subpassCount; i++) {
            const VkSubpassDescription *subpass = &pCreateInfo->pSubpasses[i];
            vector<VkFormat> color_formats;
            uint32_t j;

            color_formats.reserve(subpass->colorAttachmentCount);
            for (j = 0; j < subpass->colorAttachmentCount; j++) {
                const uint32_t att = subpass->pColorAttachments[j].attachment;
                const VkFormat format = pCreateInfo->pAttachments[att].format;

                color_formats.push_back(format);
            }

            subpassColorFormats.push_back(color_formats);
        }
    }
};

class PHYS_DEV_PROPERTIES_NODE {
  public:
    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;
    vector<VkQueueFamilyProperties> queue_family_properties;
};

class FENCE_NODE : public BASE_NODE {
  public:
    using BASE_NODE::in_use;
    VkQueue queue;
    vector<VkCommandBuffer> cmdBuffers;
    bool needsSignaled;
    VkFence priorFence;
};

class SEMAPHORE_NODE : public BASE_NODE {
  public:
    using BASE_NODE::in_use;
    uint32_t signaled;
};

class EVENT_NODE : public BASE_NODE {
  public:
    using BASE_NODE::in_use;
    bool needsSignaled;
    VkPipelineStageFlags stageMask;
};

class QUEUE_NODE {
  public:
    VkDevice device;
    VkFence priorFence;
    vector<VkCommandBuffer> untrackedCmdBuffers;
    unordered_set<VkCommandBuffer> inFlightCmdBuffers;
};

class QUERY_POOL_NODE : public BASE_NODE {
  public:
    VkQueryPoolCreateInfo createInfo;
};

// Descriptor Data structures
// Layout Node has the core layout data
typedef struct _LAYOUT_NODE {
    VkDescriptorSetLayout           layout;
    VkDescriptorSetLayoutCreateInfo createInfo;
    uint32_t startIndex;             // 1st index of this layout
    uint32_t endIndex;               // last index of this layout
    uint32_t dynamicDescriptorCount; // Total count of dynamic descriptors used
                                     // by this layout
    vector<VkDescriptorType> descriptorTypes; // Type per descriptor in this
                                              // layout to verify correct
                                              // updates
    vector<VkShaderStageFlags> stageFlags; // stageFlags per descriptor in this
                                           // layout to verify correct updates
    unordered_map<uint32_t, uint32_t> bindingToIndexMap; // map set binding # to
                                                         // pBindings index
    // Default constructor
    _LAYOUT_NODE():layout{},
                   createInfo{},
                   startIndex(0),
                   endIndex(0),
                   dynamicDescriptorCount(0)
                   {};
} LAYOUT_NODE;

// Store layouts and pushconstants for PipelineLayout
struct PIPELINE_LAYOUT_NODE {
    vector<VkDescriptorSetLayout>  descriptorSetLayouts;
    vector<VkPushConstantRange>    pushConstantRanges;
};

class SET_NODE : public BASE_NODE {
  public:
    using BASE_NODE::in_use;
    VkDescriptorSet      set;
    VkDescriptorPool     pool;
    // Head of LL of all Update structs for this set
    GENERIC_HEADER*      pUpdateStructs;
    // Total num of descriptors in this set (count of its layout plus all prior layouts)
    uint32_t             descriptorCount;
    GENERIC_HEADER**     ppDescriptors; // Array where each index points to update node for its slot
    LAYOUT_NODE*         pLayout; // Layout for this set
    SET_NODE*            pNext;
    unordered_set<VkCommandBuffer> boundCmdBuffers; // Cmd buffers that this set has been bound to
    SET_NODE() : pUpdateStructs(NULL), ppDescriptors(NULL), pLayout(NULL), pNext(NULL) {};
};

typedef struct _DESCRIPTOR_POOL_NODE {
    VkDescriptorPool           pool;
    uint32_t                   maxSets;
    VkDescriptorPoolCreateInfo createInfo;
    SET_NODE*                  pSets; // Head of LL of sets for this Pool
    vector<uint32_t>           maxDescriptorTypeCount; // max # of descriptors of each type in this pool
    vector<uint32_t>           availableDescriptorTypeCount; // available # of descriptors of each type in this pool

    _DESCRIPTOR_POOL_NODE(const VkDescriptorPool pool,
                          const VkDescriptorPoolCreateInfo *pCreateInfo)
        : pool(pool), maxSets(pCreateInfo->maxSets), createInfo(*pCreateInfo),
          pSets(NULL), maxDescriptorTypeCount(VK_DESCRIPTOR_TYPE_RANGE_SIZE),
          availableDescriptorTypeCount(VK_DESCRIPTOR_TYPE_RANGE_SIZE) {
        if (createInfo.poolSizeCount) { // Shadow type struct from ptr into local struct
            size_t poolSizeCountSize = createInfo.poolSizeCount * sizeof(VkDescriptorPoolSize);
            createInfo.pPoolSizes = new VkDescriptorPoolSize[poolSizeCountSize];
            memcpy((void*)createInfo.pPoolSizes, pCreateInfo->pPoolSizes, poolSizeCountSize);
            // Now set max counts for each descriptor type based on count of that type times maxSets
            uint32_t i=0;
            for (i=0; i<createInfo.poolSizeCount; ++i) {
                uint32_t typeIndex = static_cast<uint32_t>(createInfo.pPoolSizes[i].type);
                uint32_t poolSizeCount = createInfo.pPoolSizes[i].descriptorCount;
                maxDescriptorTypeCount[typeIndex] += poolSizeCount;
            }
            for (i=0; i<maxDescriptorTypeCount.size(); ++i) {
                maxDescriptorTypeCount[i] *= createInfo.maxSets;
                // Initially the available counts are equal to the max counts
                availableDescriptorTypeCount[i] = maxDescriptorTypeCount[i];
            }
        } else {
            createInfo.pPoolSizes = NULL; // Make sure this is NULL so we don't try to clean it up
        }
    }
    ~_DESCRIPTOR_POOL_NODE() {
        if (createInfo.pPoolSizes) {
            delete[] createInfo.pPoolSizes;
        }
        // TODO : pSets are currently freed in deletePools function which uses freeShadowUpdateTree function
        //  need to migrate that struct to smart ptrs for auto-cleanup
    }
} DESCRIPTOR_POOL_NODE;

// Cmd Buffer Tracking
typedef enum _CMD_TYPE
{
    CMD_BINDPIPELINE,
    CMD_BINDPIPELINEDELTA,
    CMD_SETVIEWPORTSTATE,
    CMD_SETSCISSORSTATE,
    CMD_SETLINEWIDTHSTATE,
    CMD_SETDEPTHBIASSTATE,
    CMD_SETBLENDSTATE,
    CMD_SETDEPTHBOUNDSSTATE,
    CMD_SETSTENCILREADMASKSTATE,
    CMD_SETSTENCILWRITEMASKSTATE,
    CMD_SETSTENCILREFERENCESTATE,
    CMD_BINDDESCRIPTORSETS,
    CMD_BINDINDEXBUFFER,
    CMD_BINDVERTEXBUFFER,
    CMD_DRAW,
    CMD_DRAWINDEXED,
    CMD_DRAWINDIRECT,
    CMD_DRAWINDEXEDINDIRECT,
    CMD_DISPATCH,
    CMD_DISPATCHINDIRECT,
    CMD_COPYBUFFER,
    CMD_COPYIMAGE,
    CMD_BLITIMAGE,
    CMD_COPYBUFFERTOIMAGE,
    CMD_COPYIMAGETOBUFFER,
    CMD_CLONEIMAGEDATA,
    CMD_UPDATEBUFFER,
    CMD_FILLBUFFER,
    CMD_CLEARCOLORIMAGE,
    CMD_CLEARATTACHMENTS,
    CMD_CLEARDEPTHSTENCILIMAGE,
    CMD_RESOLVEIMAGE,
    CMD_SETEVENT,
    CMD_RESETEVENT,
    CMD_WAITEVENTS,
    CMD_PIPELINEBARRIER,
    CMD_BEGINQUERY,
    CMD_ENDQUERY,
    CMD_RESETQUERYPOOL,
    CMD_COPYQUERYPOOLRESULTS,
    CMD_WRITETIMESTAMP,
    CMD_INITATOMICCOUNTERS,
    CMD_LOADATOMICCOUNTERS,
    CMD_SAVEATOMICCOUNTERS,
    CMD_BEGINRENDERPASS,
    CMD_NEXTSUBPASS,
    CMD_ENDRENDERPASS,
    CMD_EXECUTECOMMANDS,
    CMD_DBGMARKERBEGIN,
    CMD_DBGMARKEREND,
} CMD_TYPE;
// Data structure for holding sequence of cmds in cmd buffer
typedef struct _CMD_NODE {
    CMD_TYPE          type;
    uint64_t          cmdNumber;
} CMD_NODE;

typedef enum _CB_STATE
{
    CB_NEW,       // Newly created CB w/o any cmds
    CB_RECORDING, // BeginCB has been called on this CB
    CB_RECORDED,  // EndCB has been called on this CB
    CB_INVALID    // CB had a bound descriptor set destroyed or updated
} CB_STATE;
// CB Status -- used to track status of various bindings on cmd buffer objects
typedef VkFlags CBStatusFlags;
typedef enum _CBStatusFlagBits
{
    CBSTATUS_NONE                              = 0x00000000, // No status is set
    CBSTATUS_VIEWPORT_SET                      = 0x00000001, // Viewport has been set
    CBSTATUS_LINE_WIDTH_SET                    = 0x00000002, // Line width has been set
    CBSTATUS_DEPTH_BIAS_SET                    = 0x00000004, // Depth bias has been set
    CBSTATUS_COLOR_BLEND_WRITE_ENABLE          = 0x00000008, // PSO w/ CB Enable set has been set
    CBSTATUS_BLEND_SET                         = 0x00000010, // Blend state object has been set
    CBSTATUS_DEPTH_WRITE_ENABLE                = 0x00000020, // PSO w/ Depth Enable set has been set
    CBSTATUS_STENCIL_TEST_ENABLE               = 0x00000040, // PSO w/ Stencil Enable set has been set
    CBSTATUS_DEPTH_BOUNDS_SET                  = 0x00000080, // Depth bounds state object has been set
    CBSTATUS_STENCIL_READ_MASK_SET             = 0x00000100, // Stencil read mask has been set
    CBSTATUS_STENCIL_WRITE_MASK_SET            = 0x00000200, // Stencil write mask has been set
    CBSTATUS_STENCIL_REFERENCE_SET             = 0x00000400, // Stencil reference has been set
    CBSTATUS_INDEX_BUFFER_BOUND                = 0x00000800, // Index buffer has been set
    CBSTATUS_SCISSOR_SET                       = 0x00001000, // Scissor has been set
    CBSTATUS_ALL                               = 0x00001FFF, // All dynamic state set
} CBStatusFlagBits;

typedef struct stencil_data {
    uint32_t                     compareMask;
    uint32_t                     writeMask;
    uint32_t                     reference;
} CBStencilData;

typedef struct _DRAW_DATA {
    vector<VkBuffer> buffers;
} DRAW_DATA;

struct ImageSubresourcePair {
    VkImage image;
    bool hasSubresource;
    VkImageSubresource subresource;
};

bool operator==(const ImageSubresourcePair &img1, const ImageSubresourcePair &img2) {
    if (img1.image != img2.image || img1.hasSubresource != img2.hasSubresource) return false;
    return !img1.hasSubresource || (img1.subresource.aspectMask == img2.subresource.aspectMask &&
            img1.subresource.mipLevel == img2.subresource.mipLevel &&
            img1.subresource.arrayLayer == img2.subresource.arrayLayer);
}

namespace std {
template <> struct hash<ImageSubresourcePair> {
    size_t operator()(ImageSubresourcePair img) const throw() {
        size_t hashVal =
            hash<uint64_t>()(reinterpret_cast<uint64_t &>(img.image));
        hashVal ^= hash<bool>()(img.hasSubresource);
        if (img.hasSubresource) {
            hashVal ^= hash<uint32_t>()(reinterpret_cast<uint32_t &>(img.subresource.aspectMask));
            hashVal ^= hash<uint32_t>()(img.subresource.mipLevel);
            hashVal ^= hash<uint32_t>()(img.subresource.arrayLayer);
        }
        return hashVal;
    }
};
}

struct QueryObject {
    VkQueryPool pool;
    uint32_t index;
};

bool operator==(const QueryObject& query1, const QueryObject& query2) {
    return (query1.pool == query2.pool && query1.index == query2.index);
}

namespace std {
template <>
struct hash<QueryObject> {
    size_t operator()(QueryObject query) const throw() {
        return hash<uint64_t>()((uint64_t)(query.pool)) ^ hash<uint32_t>()(query.index);
    }
};
}

// Cmd Buffer Wrapper Struct
typedef struct _GLOBAL_CB_NODE {
    VkCommandBuffer              commandBuffer;
    VkCommandBufferAllocateInfo  createInfo;
    VkCommandBufferBeginInfo     beginInfo;
    VkCommandBufferInheritanceInfo inheritanceInfo;
    VkFence                      fence;    // fence tracking this cmd buffer
    VkDevice                     device;   // device this DB belongs to
    uint64_t                     numCmds;  // number of cmds in this CB
    uint64_t                     drawCount[NUM_DRAW_TYPES]; // Count of each type of draw in this CB
    CB_STATE                     state;  // Track cmd buffer update state
    uint64_t                     submitCount; // Number of times CB has been submitted
    CBStatusFlags                status; // Track status of various bindings on cmd buffer
    vector<CMD_NODE>             cmds; // vector of commands bound to this command buffer
    // Currently storing "lastBound" objects on per-CB basis
    //  long-term may want to create caches of "lastBound" states and could have
    //  each individual CMD_NODE referencing its own "lastBound" state
    VkPipeline                   lastBoundPipeline;
    uint32_t                     lastVtxBinding;
    vector<VkBuffer>             boundVtxBuffers;
    vector<VkViewport>           viewports;
    vector<VkRect2D>             scissors;
    float                        lineWidth;
    float                        depthBiasConstantFactor;
    float                        depthBiasClamp;
    float                        depthBiasSlopeFactor;
    float                        blendConstants[4];
    float                        minDepthBounds;
    float                        maxDepthBounds;
    CBStencilData                front;
    CBStencilData                back;
    VkDescriptorSet              lastBoundDescriptorSet;
    VkPipelineLayout             lastBoundPipelineLayout;
    VkRenderPassBeginInfo        activeRenderPassBeginInfo;
    VkRenderPass                 activeRenderPass;
    VkSubpassContents            activeSubpassContents;
    uint32_t                     activeSubpass;
    VkFramebuffer                framebuffer;
    // Capture unique std::set of descriptorSets that are bound to this CB.
    std::set<VkDescriptorSet>    uniqueBoundSets;
    // Keep running track of which sets are bound to which set# at any given time
    // Track descriptor sets that are destroyed or updated while bound to CB
    std::set<VkDescriptorSet>    destroyedSets;
    std::set<VkDescriptorSet>    updatedSets;
    vector<VkDescriptorSet>      boundDescriptorSets; // Index is set# that given set is bound to
    vector<VkEvent>              waitedEvents;
    vector<VkSemaphore> semaphores;
    vector<VkEvent> events;
    unordered_map<QueryObject, vector<VkEvent> > waitedEventsBeforeQueryReset;
    unordered_map<QueryObject, bool> queryToStateMap; // 0 is unavailable, 1 is available
    unordered_set<QueryObject>   activeQueries;
    unordered_set<QueryObject> startedQueries;
    unordered_map<ImageSubresourcePair, IMAGE_CMD_BUF_NODE> imageLayoutMap;
    unordered_map<VkImage, vector<ImageSubresourcePair>> imageSubresourceMap;
    unordered_map<VkEvent, VkPipelineStageFlags> eventToStageMap;
    vector<DRAW_DATA>            drawData;
    DRAW_DATA                    currentDrawData;
    VkCommandBuffer primaryCommandBuffer;
    // If cmd buffer is primary, track secondary command buffers pending
    // execution
    std::unordered_set<VkCommandBuffer> secondaryCommandBuffers;
    vector<uint32_t>             dynamicOffsets; // one dynamic offset per dynamic descriptor bound to this CB
} GLOBAL_CB_NODE;

typedef struct _SWAPCHAIN_NODE {
    VkSwapchainCreateInfoKHR    createInfo;
    uint32_t*                   pQueueFamilyIndices;
    std::vector<VkImage>        images;
    _SWAPCHAIN_NODE(const VkSwapchainCreateInfoKHR *pCreateInfo) :
        createInfo(*pCreateInfo),
        pQueueFamilyIndices(NULL)
    {
        if (pCreateInfo->queueFamilyIndexCount) {
            pQueueFamilyIndices = new uint32_t[pCreateInfo->queueFamilyIndexCount];
            memcpy(pQueueFamilyIndices, pCreateInfo->pQueueFamilyIndices, pCreateInfo->queueFamilyIndexCount*sizeof(uint32_t));
            createInfo.pQueueFamilyIndices = pQueueFamilyIndices;
        }
    }
    ~_SWAPCHAIN_NODE()
    {
        if (pQueueFamilyIndices)
            delete pQueueFamilyIndices;
    }
} SWAPCHAIN_NODE;
