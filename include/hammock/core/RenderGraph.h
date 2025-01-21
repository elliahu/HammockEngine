#pragma once

#include <hammock/core/Device.h>
#include <hammock/core/CoreUtils.h>
#include <functional>
#include <cassert>
#include <complex>
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
            Image2D,
            Image3D,
            ImageCubeMap,
            Image2DArray,
            Image3DArray,
            ImageCubeMapArray,
            SwapChainColorAttachment,
            SwapChainDepthStencilAttachment,
            ColorAttachment,
            DepthStencilAttachment,
        } type;

        bool isRenderingAttachment() const {
            return type == Type::ColorAttachment || type == Type::DepthStencilAttachment || type ==
                   Type::SwapChainColorAttachment || type == Type::SwapChainDepthStencilAttachment;
        }

        bool isColorAttachment() const {
            return type == Type::ColorAttachment || type == Type::SwapChainColorAttachment;
        }

        bool isDepthAttachment() const {
            return type == Type::DepthStencilAttachment || type == Type::SwapChainDepthStencilAttachment;
        }

        bool isSwapChainAttachment() const {
            return type == Type::SwapChainColorAttachment || type == Type::SwapChainDepthStencilAttachment;
        }

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
        VkImageLayout requiredLayout; // Layout before rendering
        VkImageLayout finalLayout; // layout after rendering, only for output resources, ignored for input resources
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
        std::vector<std::reference_wrapper<ResourceNode> > inputs;
        std::vector<std::reference_wrapper<ResourceNode> > outputs;
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

        enum class TransitionStage {
            RequiredLayout,
            FinalLayout
        };

        /**
         *  Checks if resource needs barrier transition give its access
         * @return true if resource needs barrier transition, else false
         */
        bool isNeeded(TransitionStage trStage) const {
            if (node.isRenderingAttachment()) {
                ASSERT(std::holds_alternative<ImageResourceRef>(node.refs[frameIndex].resource),
                       "Node is image but does not hold image ref")
                ;
                const ImageResourceRef &ref = std::get<ImageResourceRef>(node.refs[frameIndex].resource);
                if (trStage == TransitionStage::RequiredLayout) {
                    return ref.currentLayout != access.requiredLayout;
                } else if (trStage == TransitionStage::FinalLayout) {
                    return ref.currentLayout != access.finalLayout;
                }
                return false;
            }
            // TODO support for buffer transition
            return false;
        }

        /**
         * Applies pre-render pipeline barrier transition
         */
        void apply(TransitionStage trStage) const;
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
        }

        /**
         * Adds a resource node to the render graph
         * @param resource Resource node to be added
         */
        void addResource(const ResourceNode &resource) {
            resources[resource.name] = resource;
        }

        ResourceNode &getResource(const std::string &name) {
            ASSERT(resources.contains(name), "Resource with name '" + name + "' does not exist");
            return resources.at(name);
        }

        /**
         * Adds render pass to the render graph and creates VkRenderPass for it if it is not a present pass
         * @param pass Render pass to be added
         */
        void addPass(const RenderPassNode &pass) {
            for (auto &p: passes) {
                ASSERT(p.name != pass.name, "Pass with this name aleady exists");
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
                uint32_t imageIndex = renderContext.getImageIndex();

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

                        // TODO automatically find out next render pass using this resource and set final layout of the attachment to the requiredLayout to save one barrier

                        // Apply barrier if it is needed
                        if (Barrier barrier(resourceNode, access, cmd,
                                            resourceNode.isSwapChainAttachment() ? imageIndex : frameIndex); barrier.
                            isNeeded(Barrier::TransitionStage::RequiredLayout)) {
                            barrier.apply(Barrier::TransitionStage::RequiredLayout);
                        }

                        context.inputs.push_back(resourceNode);
                    }

                    std::vector<VkRenderingAttachmentInfo> colorAttachments;
                    VkRenderingAttachmentInfo depthStencilAttachment;

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
                        if (Barrier barrier(resourceNode, access, cmd,
                                            resourceNode.isSwapChainAttachment() ? imageIndex : frameIndex); barrier.
                            isNeeded(
                                Barrier::TransitionStage::RequiredLayout)) {
                            barrier.apply(Barrier::TransitionStage::RequiredLayout);
                        }

                        context.outputs.push_back(resourceNode);

                        if (resourceNode.isRenderingAttachment()) {
                            ImageResourceRef &imageRef = std::get<ImageResourceRef>(
                                resourceNode.isSwapChainAttachment()
                                    ? resourceNode.refs[imageIndex].resource
                                    : resourceNode.refs[frameIndex].resource
                            );

                            VkRenderingAttachmentInfo attachment{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
                            attachment.imageView = imageRef.view;
                            attachment.imageLayout = imageRef.currentLayout;
                            attachment.loadOp = access.loadOp;
                            attachment.storeOp = access.storeOp;
                            attachment.clearValue = imageRef.clearValue;

                            if (resourceNode.isColorAttachment()) {
                                colorAttachments.push_back(attachment);
                            } else if (resourceNode.isDepthAttachment()) {
                                depthStencilAttachment = attachment;
                            }
                        }
                    }

                    VkRenderingInfo renderingInfo{VK_STRUCTURE_TYPE_RENDERING_INFO_KHR};
                    renderingInfo.renderArea = {
                        0, 0, renderContext.getSwapChain()->getSwapChainExtent().width,
                        renderContext.getSwapChain()->getSwapChainExtent().height
                    };
                    renderingInfo.layerCount = 1;
                    renderingInfo.colorAttachmentCount = colorAttachments.size();
                    renderingInfo.pColorAttachments = colorAttachments.data();
                    renderingInfo.pDepthAttachment = &depthStencilAttachment;
                    renderingInfo.pStencilAttachment = nullptr;

                    // Start a dynamic rendering
                    vkCmdBeginRendering(cmd, &renderingInfo);

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


                    // End dynamic rendering
                    vkCmdEndRendering(cmd);

                    // post rendering barrier
                    for (auto &access: pass.outputs) {
                        ResourceNode &resourceNode = resources[access.resourceName];
                        if (Barrier barrier(resourceNode, access, cmd, resourceNode.isSwapChainAttachment() ? imageIndex : frameIndex); barrier.isNeeded(
                            Barrier::TransitionStage::FinalLayout)) {
                            barrier.apply(Barrier::TransitionStage::FinalLayout);
                        }
                    }
                }

                // End work on the current frame and submit the command buffer
                renderContext.endFrame();
            }
        }
    };
};
