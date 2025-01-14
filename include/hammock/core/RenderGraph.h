#pragma once

#include <hammock/core/Resource.h>
#include <hammock/core/Device.h>
#include <hammock/core/SwapChain.h>
#include <hammock/core/CoreUtils.h>
#include <cassert>
#include <variant>

#include "hammock/core/RenderContext.h"


namespace hammock {
    namespace rendergraph {
        /**
        * Describes general buffer
        */
        struct BufferDesc {
            VkDeviceSize instanceSize;
            uint32_t instanceCount;
            VkBufferUsageFlags usageFlags;
            VkMemoryPropertyFlags memoryPropertyFlags;
            VkDeviceSize minOffsetAlignment;
        };

        /**
        * Reference to Vulkan buffer.
        */
        struct BufferResourceRef {
            VkBuffer buffer;
            VmaAllocation allocation;
        };

        /**
         * Describes the base for the relative size
         */
        enum class RelativeSize {
            SwapChainRelative,
            FrameBufferRelative,
        };

        /**
        * Describes general image.
        */
        struct ImageDesc {
            HmckVec2 size{};
            RelativeSize relativeSize = RelativeSize::SwapChainRelative;
            uint32_t channels = 4, depth = 1, layers = 1, mips = 1;
            VkFormat format;
            VkImageUsageFlags usage;
            VkImageType imageType = VK_IMAGE_TYPE_2D;
            VkImageViewType imageViewType = VK_IMAGE_VIEW_TYPE_2D;
            bool createSampler = true;
            VkFilter filter = VK_FILTER_LINEAR;
            VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            VkBorderColor borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
            VkSamplerMipmapMode mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            VkSampleCountFlagBits imageSampleCount = VK_SAMPLE_COUNT_1_BIT;
        };

        /**
         * Reference to Vulkan image.
         */
        struct ImageResourceRef {
            VkImage image;
            VkImageView view;
            VkSampler sampler;
            VmaAllocation allocation;
            VkImageLayout currentLayout;
            VkAttachmentDescription attachmentDesc;
        };


        /**
         * Resource reference. It can hold a reference to image, or buffer.
         */
        struct ResourceRef {
            // Variant to hold either buffer or image resource
            std::variant<BufferResourceRef, ImageResourceRef> resource;
        };

        /**
         * RenderGraph node representing a resource
         */
        struct ResourceNode {
            enum class Type {
                Buffer,
                Image,
                SwapChain
                // Special type of image. If resource is SwapChain, it is the final output resource that gets presented.
            } type;

            std::variant<BufferDesc, ImageDesc> desc;

            // Name is used for lookup
            std::string name;

            // Whether resource lives only within a frame
            bool isTransient = true;
            // Whether resource has one instance per frame in flight or just one overall
            bool isBuffered = true;
            // True if resource is externally owned (e.g. swapchain images)
            bool isExternal = false;

            // One handle per frame in flight
            std::vector<ResourceRef> refs;
        };

        /**
         * Describes access intentions of a resource. Resource is references by its name from a resource map.
         */
        struct ResourceAccess {
            std::string resourceName;
            VkImageLayout requiredLayout;
            VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            VkPipelineStageFlags stageFlags = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        };

        struct RenderPassContext {
            VkRenderPass renderPass;
            VkCommandBuffer commandBuffer;
            uint32_t frameIndex;
        };

        /**
         * RenderGraph node representing a render pass
         */
        struct RenderPassNode {
            enum class Type {
                // Represents a pass that draws into some image
                Graphics,
                // Represents a compute pass
                Compute,
                // Represents a transfer pass - data is moved from one location to another (eg. host -> device or device -> host)
                Transfer
            } type; // Type of the pass

            std::string name; // Name of the pass for debug purposes and lookup
            std::vector<ResourceAccess> inputs; // Read accesses
            std::vector<ResourceAccess> outputs; // Write accesses

            // callback for rendering
            std::function<void(VkCommandBuffer, uint32_t)> executeFunc;
        };


        /**
         * Wraps necessary operation needed for pipeline barrier transitions for a single resource
         */
        class Barrier {
            ResourceNode &node;
            ResourceAccess &access;
            VkCommandBuffer commandBuffer;
            uint32_t frameIndex = 0;

        public:
            explicit Barrier(ResourceNode &node, ResourceAccess &access, VkCommandBuffer commandBuffer,
                             const uint32_t frameIndex) : node(node),
                                                          access(access), commandBuffer(commandBuffer),
                                                          frameIndex(frameIndex) {
            }

            /**
             *  Checks if resource needs barrier transition give its access
             * @return true if resource needs barrier transition, else false
             */
            bool isNeeded() const {
                if (node.type == ResourceNode::Type::Image) {
                    ASSERT(std::holds_alternative<ImageResourceRef>(node.refs[frameIndex].resource),
                           "BarrierManager::needsBarrier sanity check: Node is image node yet does not hold image ref.")
                    ;
                    ImageResourceRef ref = std::get<ImageResourceRef>(node.refs[frameIndex].resource);
                    return ref.currentLayout != access.requiredLayout;
                }
                // TODO support for buffer transition
                return false;
            }

            /**
             * Applies pipeline barrier transition
             */
            void apply() {
                if (node.type == ResourceNode::Type::Image) {
                    // Verify that we're dealing with an image resource
                    ASSERT(std::holds_alternative<ImageResourceRef>(node.refs[frameIndex].resource),
                           "Barrier::apply sanity check: Node is image node yet does not hold image ref.");

                    // Get the image reference for the current frame
                    ImageResourceRef &ref = std::get<ImageResourceRef>(node.refs[frameIndex].resource);

                    ASSERT(std::holds_alternative<ImageDesc>(node.desc),
                           "Barrier::apply sanity check: Node is image node yet does not hold image desc.");
                    ImageDesc &desc = std::get<ImageDesc>(node.desc);

                    // Skip if no transition is needed, this should already be checked but anyway...
                    if (ref.currentLayout == access.requiredLayout) {
                        return;
                    }

                    // Create image memory barrier
                    VkImageMemoryBarrier imageBarrier{};
                    imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                    imageBarrier.oldLayout = ref.currentLayout;
                    imageBarrier.newLayout = access.requiredLayout;
                    imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    imageBarrier.image = ref.image;

                    // Define image subresource range
                    imageBarrier.subresourceRange.aspectMask =
                            isDepthStencil(desc.format)
                                ? VK_IMAGE_ASPECT_DEPTH_BIT
                                : VK_IMAGE_ASPECT_COLOR_BIT;
                    imageBarrier.subresourceRange.baseMipLevel = 0;
                    imageBarrier.subresourceRange.levelCount = desc.mips;
                    imageBarrier.subresourceRange.baseArrayLayer = 0;
                    imageBarrier.subresourceRange.layerCount = desc.layers;

                    // Set access masks based on layout transitions
                    switch (ref.currentLayout) {
                        case VK_IMAGE_LAYOUT_UNDEFINED:
                            imageBarrier.srcAccessMask = 0;
                            break;
                        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                            imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                            break;
                        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                            imageBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
                            break;
                        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                            imageBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                            break;
                        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                            imageBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                            break;
                        default:
                            imageBarrier.srcAccessMask = 0;
                    }

                    switch (access.requiredLayout) {
                        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                            imageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                            break;
                        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                            imageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                            break;
                        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                            imageBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                            break;
                        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                            imageBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                            break;
                        default:
                            imageBarrier.dstAccessMask = 0;
                    }

                    // Issue the pipeline barrier command
                    vkCmdPipelineBarrier(
                        commandBuffer, // commandBuffer
                        access.stageFlags, // srcStageMask
                        access.stageFlags, // dstStageMask
                        0, // dependencyFlags
                        0, nullptr, // Memory barriers
                        0, nullptr, // Buffer memory barriers
                        1, &imageBarrier // Image memory barriers
                    );

                    // Update the current layout
                    ref.currentLayout = access.requiredLayout;
                }
                // TODO support for buffer transition
            }
        };

        /**
         * RenderGraph is a class representing a directed acyclic graph (DAG) that describes the process of creation of a frame.
         * It consists of resource nodes and render pass nodes. Render pass node can be dependent on some resources and
         * can itself create resources that other passes may depend on. RenderGraph analyzes these dependencies, makes adjustments when possible,
         * makes necessary transitions between resource states. These transitions are done just in time as well as resource allocation.
         *  TODO support for Read Modify Write - render pass writes and reads from the same resource
         */
        class RenderGraph {
            // Vulkan device
            Device &device;
            // Rendering context
            RenderContext &renderContext;
            // Maximal numer of frames that GPU works on concurrently
            uint32_t maxFramesInFlight;
            // Holds all the resources
            std::unordered_map<std::string, ResourceNode> resources;
            // Holds all the render passes
            std::vector<RenderPassNode> passes;
            // Array of indexes into list of passes. Order is optimized by the optimizer.
            std::vector<uint32_t> optimizedOrderOfPasses;
            // Pass from which the rendering starts (top of the graph)
            uint32_t rootPass = 0;
            // SwapChain extent
            VkExtent2D extent;

            /**
             * Creates a resource for specific node
             * @param resourceNode Node for which to create a resource
             */
            void createResource(ResourceNode &resourceNode, ResourceAccess &access);

            /**
             *  Creates a buffer with a ref
             * @param resourceRef ResourceRef for resource to be created
             * @param bufferDesc Desc struct that describes the buffer to be created
             */
            void createBuffer(ResourceRef &resourceRef, BufferDesc &bufferDesc) const;

            /**
             *  Creates image with a ref
             * @param resourceRef ResourceRef for resource to be created
             * @param imageDesc Desc struct that describes the image to be created
             */
            void createImage(ResourceRef &resourceRef, ImageDesc &desc, ResourceAccess &access) const;

            /**
             * Loops through resources and destroys those that are transient. This should be called at the end of a frame
             */
            void destroyTransientResources();

            /**
             * Destroys resource connected with this node
             * @param resourceNode Resource node which resource will be destroyed
             */
            void destroyResource(ResourceNode &resourceNode) const;

        public:
            RenderGraph(Device &device, RenderContext &context): device(device), renderContext(context),
                                                                 extent(context.getSwapChain()->getSwapChainExtent()),
                                                                 maxFramesInFlight(SwapChain::MAX_FRAMES_IN_FLIGHT) {
            }

            ~RenderGraph() {
                for (auto &resource: resources) {
                    ResourceNode &resourceNode = resource.second;

                    // We skip nodes of resources that are externally managed, these were created somewhere else and should be destroyed there as well
                    if (resourceNode.isExternal) { continue; }

                    destroyResource(resourceNode);
                }
            }

            /**
             * Adds a resource node to the render graph
             * @param resource Resource node to be added
             */
            void addResource(const ResourceNode &resource) {
                resources[resource.name] = resource;
            }

            /**
             * Adds render pass to the render graph
             * @param pass Render pass to be added
             */
            void addPass(const RenderPassNode &pass) {
                passes.push_back(pass);
            }

            /**
             * Compiles the render graph - analyzes dependencies and makes optimization
             */
            void compile() {
                // TODO we are now making no optimizations and assume that rendering goes in order of render pass submission

                // To be replaced when above task is implemented
                for (int i = 0; i < passes.size(); i++) {
                    optimizedOrderOfPasses.push_back(i);
                }
            }


            /**
             * Executes the graph and makes necessary barrier transitions.
             * @param cmd CommandBuffer to which the render calls will be recorded
             * @param frameIndex Index of the current frame.This is used to identify which resource ref should be used of the resource node is buffered.
             */
            void execute() {
                ASSERT(optimizedOrderOfPasses.size() == passes.size(),
                       "RenderGraph::execute sanity check: RenderPass queue size don't match. This should not happen.");

                // Begin frame by creating master command buffer
                if (VkCommandBuffer cmd = renderContext.beginFrame()) {

                    // Get the frame index. This is used to select buffered refs
                    uint32_t frameIndex = renderContext.getFrameIndex();

                    // Loop over passes in the optimized order
                    for (const auto &passIndex: optimizedOrderOfPasses) {
                        ASSERT(
                            std::find(optimizedOrderOfPasses.begin(),optimizedOrderOfPasses.end(), passIndex) !=
                            optimizedOrderOfPasses.end(),
                            "RenderGraph::execute sanity check: Could not find the pass based of index. This should not happen")
                        ;
                        RenderPassNode &pass = passes[passIndex];

                        // Check for necessary transitions of input resources
                        for (auto &access: pass.inputs) {
                            ASSERT(resources.find(access.resourceName) != resources.end(),
                                   "RenderGraph::execute sanity check: Could not find the input");

                            ResourceNode &resourceNode = resources[access.resourceName];

                            // We do not allow on-the-fly creation of input resources

                            // Apply barrier if it is needed
                            if (Barrier barrier(resourceNode, access, cmd, frameIndex); barrier.isNeeded()) {
                                barrier.apply();
                            }
                        }

                        // Check for necessary transitions of output resources
                        // If the resource was just created we need to transform it into correct layout
                        for (auto &access: pass.outputs) {
                            ASSERT(resources.find(access.resourceName) != resources.end(),
                                   "RenderGraph::execute sanity check: Could not find the output");
                            ResourceNode &resourceNode = resources[access.resourceName];

                            // Check if the node contains refs to resources
                            if (resourceNode.refs.empty()) {
                                // Resource node does not contain resource refs
                                // That means this is first time using this resource and needs to be created
                                createResource(resourceNode, access);
                            }

                            // Apply barrier if it is needed
                            if (Barrier barrier(resourceNode, access, cmd, frameIndex); barrier.isNeeded()) {
                                barrier.apply();
                            }
                        }
                    }
                }
            }
        };
    }
};
