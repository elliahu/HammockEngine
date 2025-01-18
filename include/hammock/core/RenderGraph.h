#pragma once

#include <hammock/core/Device.h>
#include <hammock/core/CoreUtils.h>
#include <functional>
#include <cassert>
#include <variant>

#include "hammock/core/RenderContext.h"
#include "hammock/core/Types.h"


namespace hammock {
    /**
     * RenderGraph node representing a resource
     */
    struct ResourceNode {
        enum class Type {
            Buffer,
            Image,
            SwapChainImage
            // Special type of image. If resource is SwapChain, it is the final output resource that gets presented.
        } type;



        // Name is used for lookup
        std::string name;

        // Resource description
        std::variant<BufferDesc, ImageDesc> desc;

        // One handle per frame in flight
        std::vector<ResourceRef> refs;

        // Whether resource lives only within a frame
        bool isTransient = true;
        // Whether resource has one instance per frame in flight or just one overall
        bool isBuffered = true;
        // True if resource is externally owned (e.g. swapchain images)
        bool isExternal = false;
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

    /**
     * Describes a render pass context. This data is passed into rendering callback, and it is only infor available for rendering
     */
    struct RenderPassContext {
        VkCommandBuffer commandBuffer;
        uint32_t frameIndex;
        std::vector<std::reference_wrapper<ResourceNode>> inputs;
        std::vector<std::reference_wrapper<ResourceNode>> outputs;
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
        VkExtent2D extent;
        std::vector<ResourceAccess> inputs; // Read accesses
        std::vector<ResourceAccess> outputs; // Write accesses

        // callback for rendering
        std::function<void(RenderPassContext)> executeFunc;

        std::vector<VkFramebuffer> framebuffers;
        VkRenderPass renderPass = VK_NULL_HANDLE;
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
                       "Node is image but does not hold image ref")
                ;
                const ImageResourceRef& ref = std::get<ImageResourceRef>(node.refs[frameIndex].resource);
                return ref.currentLayout != access.requiredLayout;
            }
            // TODO support for buffer transition
            return false;
        }

        /**
         * Applies pipeline barrier transition
         */
        void apply() const;
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
        // Pass that is called last
        std::string presentPass;

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

        /**
         * Creates the actual render pass and framebuffers (one framebuffer for each frame in flight)
         * @param renderPassNode Render pass node for which to create render pass and framebuffers (one framebuffer of each frame in flight)
         */
        void createRenderPassAndFramebuffers(RenderPassNode &renderPassNode) const;

        /**
         * Destroys the VkRenderPass attached to the node and its VkFramebuffer (each one) as well
         * @param renderPassNode Renderp ass node which will be destroyed
         */
        void destroyRenderPassAndFrambuffers(RenderPassNode &renderPassNode) {
            ASSERT(!renderPassNode.framebuffers.empty(), "Render pass has no framebuffer");
            ASSERT(renderPassNode.renderPass, "RenderPass is null");

            for (int i = 0; i < renderPassNode.framebuffers.size(); i++) {
                vkDestroyFramebuffer(device.device(), renderPassNode.framebuffers[i], nullptr);
            }
            vkDestroyRenderPass(device.device(), renderPassNode.renderPass, nullptr);
        }

    public:
        RenderGraph(Device &device, RenderContext &context): device(device), renderContext(context),
                                                             maxFramesInFlight(SwapChain::MAX_FRAMES_IN_FLIGHT) {
        }

        ~RenderGraph() {
            for (auto &resource: resources) {
                ResourceNode &resourceNode = resource.second;

                // We skip nodes of resources that are externally managed, these were created somewhere else and should be destroyed there as well
                if (resourceNode.isExternal) { continue; }

                destroyResource(resourceNode);
            }

            for (int i = 0; i < passes.size(); i++) {
                if (passes[i].renderPass != VK_NULL_HANDLE) {
                    destroyRenderPassAndFrambuffers(passes[i]);
                }
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
            if (pass.renderPass != VK_NULL_HANDLE) {
                ASSERT(pass.framebuffers.size() >= SwapChain::MAX_FRAMES_IN_FLIGHT, "Render pass must have at least MAX_FRAMES_IN_FLIGHT framebuffers");
            }
            passes.push_back(pass);
        }

        /**
         * Adds a render pass as a present pass
         * @param pass Render pass to be added
         */
        void addPresentPass(const RenderPassNode &pass) {
            addPass(pass);
            ASSERT(presentPass.empty(), "Present pass already set");
            presentPass = pass.name;
        }

        /**
         * Compiles the render graph - analyzes dependencies and makes optimization
         */
        void compile() {
            ASSERT(!presentPass.empty(), "Missing present pass");

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
                   "RenderPass queue size don't match. This should not happen.");

            // Begin frame by creating master command buffer
            if (VkCommandBuffer cmd = renderContext.beginFrame()) {
                // Get the frame index. This is used to select buffered refs
                uint32_t frameIndex = renderContext.getFrameIndex();

                // Loop over passes in the optimized order
                for (const auto &passIndex: optimizedOrderOfPasses) {
                    ASSERT(
                        std::find(optimizedOrderOfPasses.begin(),optimizedOrderOfPasses.end(), passIndex) !=
                        optimizedOrderOfPasses.end(),
                        "Could not find the pass based of index. This should not happen")
                    ;
                    // Render pass node and its context
                    RenderPassNode &pass = passes[passIndex];
                    RenderPassContext context{};

                    // Check for necessary transitions of input resources
                    for (auto &access: pass.inputs) {
                        ASSERT(resources.find(access.resourceName) != resources.end(),
                               "Could not find the input");

                        ResourceNode &resourceNode = resources[access.resourceName];

                        // We do not allow on-the-fly creation of input resources

                        // Apply barrier if it is needed
                        if (Barrier barrier(resourceNode, access, cmd, frameIndex); barrier.isNeeded()) {
                            barrier.apply();
                        }

                        context.inputs.push_back(resourceNode);
                    }

                    // clear values for framebuffer
                    std::vector<VkClearValue> clearValues{};

                    // Check for necessary transitions of output resources
                    // If the resource was just created we need to transform it into correct layout
                    for (auto &access: pass.outputs) {
                        ASSERT(resources.find(access.resourceName) != resources.end(),
                               "Could not find the output");
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

                        context.outputs.push_back(resourceNode);

                        if (resourceNode.type == ResourceNode::Type::Image) {
                            ImageDesc &dec = std::get<ImageDesc>(resourceNode.desc);
                            if (isDepthStencil(dec.format)) {
                                clearValues.push_back({.depthStencil = {1.0f, 0}});
                            } else {
                                clearValues.push_back({.color = {1.0f, 1.0f, 0.0f, 1.0f}});
                            }
                        }
                    }
                    // Check if this is a present pass
                    if (pass.name == presentPass) {
                        // Use the swap chain render pass
                        renderContext.beginSwapChainRenderPass(cmd);
                    } else {
                        // create the render pass if it does not exist yet
                        if (pass.renderPass == VK_NULL_HANDLE) {
                            // TODO find out next render pass using this resource and set final layout of the attachment to the requiredLayout to save one barrier
                            createRenderPassAndFramebuffers(pass);
                        }

                        // begin render pass
                        VkRenderPassBeginInfo renderPassInfo{};
                        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                        renderPassInfo.renderPass = pass.renderPass;
                        renderPassInfo.framebuffer = pass.framebuffers[frameIndex];
                        renderPassInfo.renderArea.offset = {0, 0};
                        renderPassInfo.renderArea.extent = pass.extent;
                        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
                        renderPassInfo.pClearValues = clearValues.data();
                        vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
                    }

                    // Set viewport and scissors
                    VkViewport viewport = hammock::Init::viewport(
                        static_cast<float>(pass.extent.width), static_cast<float>(pass.extent.height),
                        0.0f, 1.0f);
                    VkRect2D scissor{{0, 0}, pass.extent};
                    vkCmdSetViewport(cmd, 0, 1, &viewport);
                    vkCmdSetScissor(cmd, 0, 1, &scissor);

                    // Add the command buffer and frame index to the context
                    context.commandBuffer = cmd;
                    context.frameIndex = frameIndex;

                    // Dispatch the render pass callback
                    pass.executeFunc(context);

                    // End the render pass
                    vkCmdEndRenderPass(cmd);
                }

                // End work on the current frame and submit the command buffer
                renderContext.endFrame();
            }
        }
    };
};
