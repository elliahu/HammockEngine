#pragma once

#include <hammock/core/Resource.h>
#include <hammock/core/Device.h>
#include <hammock/resources/Buffer.h>
#include <hammock/utils/Logger.h>
#include <hammock/core/SwapChain.h>
#include <cassert>
#include <variant>


namespace hammock {
    namespace rendergraph {
        // Relative size target
        enum class RelativeSize {
            SwapChainRelative,
            FrameBufferRelative,
        };

        // Description of a Buffer
        struct BufferDesc {
            VkDeviceSize instanceSize;
            uint32_t instanceCount;
            VkBufferUsageFlags usageFlags;
            VkMemoryPropertyFlags memoryPropertyFlags;
        };

        // Resource ref for buffer
        struct BufferResourceRef {
            VkBuffer buffer;
            VmaAllocation allocation;
            BufferDesc desc;
        };

        // Description of image
        struct ImageDesc {
            HmckVec2 size;
            RelativeSize relativeSize;
            uint32_t channels = 0, depth = 1, layers = 1, mips = 1;
            VkImageLayout currentLayout;
            VkImageUsageFlags usage;
            VkFormat format;
            VkImageType imageType;
            VkImageViewType imageViewType;
            VkImageUsageFlags imageUsageFlags;
            VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            VkFilter filter = VK_FILTER_LINEAR;
            VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            VkBorderColor borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
            VkSamplerMipmapMode mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            VkSampleCountFlagBits imageSampleCount = VK_SAMPLE_COUNT_1_BIT;
        };

        // Resource ref for image
        struct ImageResourceRef {
            VkImage image;
            VkImageView view;
            VkSampler sampler;
            VmaAllocation allocation;
            ImageDesc desc;
        };

        // Resource reference
        struct ResourceRef {
            // Variant to hold either buffer or image resource
            std::variant<BufferResourceRef, ImageResourceRef> resource;
        };


        struct ResourceNode {
            enum class Type {
                Buffer,
                Image,
                SwapChain
            } type;

            // Name is used for lookup
            std::string name;

            // Whether resource lives only within a frame
            bool isTransient = true;
            // Whether resource has one instance per frame in flight or just one overall
            bool isBuffered = true;
            // True if resource is externally owned (e.g. swapchain images)
            bool isExternal = false;

            // One handle per frame in flight
            std::vector<ResourceRef> handles;
        };

        struct ResourceAccess {
            std::string resourceName;
            VkImageLayout requiredLayout;
            VkAccessFlags accessFlags;
            VkPipelineStageFlags stageFlags;
        };


        struct RenderPassNode {
            std::string name;
            std::vector<ResourceAccess> inputs;
            std::vector<ResourceAccess> outputs;

            // callback
            std::function<void(VkCommandBuffer, uint32_t)> executeFunc;
        };

        struct ImageBarrier {
            std::string resourceName;
            VkImageLayout oldLayout;
            VkImageLayout newLayout;
            VkAccessFlags srcAccessFlags;
            VkAccessFlags dstAccessFlags;
            VkPipelineStageFlags srcStageFlags;
            VkPipelineStageFlags dstStageFlags;
        };

        class RenderGraph {
            Device &device;
            uint32_t maxFramesInFlight;
            std::unordered_map<std::string, ResourceNode> resources;
            std::vector<RenderPassNode> passes;
            std::vector<ImageBarrier> barriers;

        public:
            RenderGraph(Device &device, uint32_t maxFramesInFlight): device(device),
                                                                     maxFramesInFlight(maxFramesInFlight) {
            }

            // TODO ad destructor to release resources that are managed by the rendergraph

            // Add a resource to the graph
            void addResource(const ResourceNode& resource) {
                resources[resource.name] = resource;
            }

            // Add a render pass to the graph
            void addPass(const RenderPassNode& pass) {
                passes.push_back(pass);
            }

            // Compile the graph - analyze dependencies and create necessary barriers
            void compile() {
                // Analyze resource dependencies between passes
                for (size_t i = 0; i < passes.size(); i++) {
                    for (const auto& output : passes[i].outputs) {
                        // Find next pass that uses this resource as input
                        for (size_t j = i + 1; j < passes.size(); j++) {
                            for (const auto& input : passes[j].inputs) {
                                if (output.resourceName == input.resourceName) {
                                    // Create necessary barrier
                                    ImageBarrier barrier{
                                        output.resourceName,
                                        output.requiredLayout,
                                        input.requiredLayout,
                                        output.accessFlags,
                                        input.accessFlags,
                                        output.stageFlags,
                                        input.stageFlags
                                    };
                                    barriers.push_back(barrier);
                                }
                            }
                        }
                    }
                }
            }

            // Execute the graph for current frame
            void execute(VkCommandBuffer cmd, uint32_t frameIndex) {
                // Insert barriers between passes
                for (const auto& barrier : barriers) {
                    VkImageMemoryBarrier imageBarrier{};
                    // Fill in barrier details...
                    vkCmdPipelineBarrier(cmd,
                        barrier.srcStageFlags,
                        barrier.dstStageFlags,
                        0,
                        0, nullptr,
                        0, nullptr,
                        1, &imageBarrier);
                }

                // Execute passes
                for (const auto& pass : passes) {
                    pass.executeFunc(cmd, frameIndex);
                }
            }

        };
    }
};
