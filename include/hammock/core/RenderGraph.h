#pragma once

#include <functional>
#include <cassert>
#include <complex>
#include <variant>
#include <type_traits>
#include <optional>

#include <hammock/core/Device.h>
#include <hammock/core/CoreUtils.h>
#include "hammock/core/ResourceManager.h"
#include "hammock/core/FrameManager.h"
#include "hammock/core/Types.h"


namespace hammock {
    typedef std::function<experimental::ResourceHandle(experimental::ResourceManager &, uint32_t frameIndex)>
    ResourceResolver;

    /**
     * RenderGraph node representing a resource
     */
    struct ResourceNode {
        enum class Type {
            UniformBuffer,
            VertexBuffer,
            IndexBuffer,
            StorageBuffer,
            PushConstantData,
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

        // Name is used for lookup
        std::string name;

        // Resolver is used to resolve the actual resource
        ResourceResolver resolver;

        // Chase to store handles to avoid constant recreation
        std::vector<experimental::ResourceHandle> cachedHandles;

        // Needs recreation
        bool isDirty = true;

        experimental::ResourceHandle resolve(experimental::ResourceManager &rm, uint32_t frameIndex) {
            if (isDirty || cachedHandles.empty()) {
                cachedHandles.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
                isDirty = false;
            }

            if (cachedHandles[frameIndex].isValid()) {
                cachedHandles[frameIndex] = resolver(rm, frameIndex);
            }

            return cachedHandles[frameIndex];
        }


        bool isBuffer() const {
            return type == Type::UniformBuffer || type == Type::VertexBuffer || type == Type::IndexBuffer || type ==
                   Type::StorageBuffer;
        }

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
    };

    /**
     * Describes a render pass context. This data is passed into rendering callback, and it is only infor available for rendering
     */
    struct RenderPassContext {
        VkCommandBuffer commandBuffer;
        uint32_t frameIndex;
        std::unordered_map<std::string, std::reference_wrapper<ResourceNode> > inputs;
        std::unordered_map<std::string, std::reference_wrapper<ResourceNode> > outputs;
    };

    struct ResourceAccess {
        std::string resourceName;
        VkImageLayout requiredLayout = VK_IMAGE_LAYOUT_UNDEFINED; // Layout before rendering
        VkImageLayout finalLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        // layout after rendering, only for output resources, ignored for input resources
        VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        VkPipelineStageFlags stageFlags = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    };

    enum class RenderPassType {
        // Represents a pass that draws into some image
        Graphics,
        // Represents a compute pass
        Compute,
        // Represents a transfer pass - data is moved from one location to another (eg. host -> device or device -> host)
        Transfer
    };

    enum class ViewPortSize {
        SwapChainRelative,
        Fixed,
    };

    /**
     * RenderGraph node representing a render pass
     */
    struct RenderPassNode {
        RenderPassType type; // Type of the pass
        ViewPortSize viewportSize; // Size of viewport

        std::string name; // Name of the pass for debug purposes and lookup
        HmckVec4 viewport;
        std::vector<ResourceAccess> inputs; // Read accesses
        std::vector<ResourceAccess> outputs; // Write accesses


        // callback for rendering
        std::function<void(RenderPassContext)> executeFunc;

        // builder methods

        RenderPassNode &read(ResourceAccess access) {
            inputs.emplace_back(access);
            return *this;
        }

        RenderPassNode &write(ResourceAccess access) {
            outputs.emplace_back(access);
            return *this;
        }

        RenderPassNode &readModifyWrite(ResourceAccess access) {
            // TODO
            return *this;
        }

        RenderPassNode &execute(std::function<void(RenderPassContext)> exec) {
            executeFunc = std::move(exec);
            return *this;
        }
    };


    /**
     * Wraps necessary operation needed for pipeline barrier transitions for a single resource
     */
    class PipelineBarrier {
    public:
        enum class TransitionStage {
            RequiredLayout,
            FinalLayout
        };

        explicit PipelineBarrier(experimental::ResourceManager &rm, FrameManager &renderContext, ResourceNode &node,
                                 ResourceAccess &access,
                                 TransitionStage transitionStage) : rm(rm), renderContext(renderContext), node(node),
                                                                    access(access), transitionStage(transitionStage) {
        }

        /**
         *  Checks if resource needs barrier transition given its access
         * @return true if resource needs barrier transition, else false
         */
        bool isNeeded() const {
            if (node.isRenderingAttachment()) {
                if (node.isSwapChainAttachment()) {
                    return true;
                }
                experimental::ResourceHandle handle = node.resolve(rm, renderContext.getFrameIndex());
                experimental::Image *attachment = rm.getResource<experimental::Image>(handle);

                if (transitionStage == TransitionStage::RequiredLayout) {
                    return attachment->getLayout() != access.requiredLayout;
                }
                if (transitionStage == TransitionStage::FinalLayout) {
                    return attachment->getLayout() != access.finalLayout;
                }
                return false;
            }
            if (node.isBuffer()) {
                // TODO support for buffer transition
            }
            return false;
        }

        /**
         * Applies pre-render pipeline barrier transition
         */
        void apply() const;

    private:
        experimental::ResourceManager &rm;
        FrameManager &renderContext;
        ResourceNode &node;
        ResourceAccess &access;
        TransitionStage transitionStage;
    };

    /**
     * RenderGraph is a class representing a directed acyclic graph (DAG) that describes the process of creation of a frame.
     * It consists of resource nodes and render pass nodes. Render pass node can be dependent on some resources and
     * can itself create resources that other passes may depend on. RenderGraph analyzes these dependencies, makes adjustments when possible,
     * makes necessary transitions between resource states. These transitions are done just in time as well as resource allocation.
     *  TODO support for Read Modify Write - render pass writes and reads from the same resource
     *  TODO support for conditional resource
     *  TODO support for swapchain dependent rendperPass
     */
    class RenderGraph {
        // Vulkan device
        Device &device;
        // Rendering context
        FrameManager &renderContext;
        experimental::ResourceManager &rm;
        // Maximal numer of frames that GPU works on concurrently
        uint32_t maxFramesInFlight;
        // Holds all the resources
        std::unordered_map<std::string, ResourceNode> resources;
        // Holds all the render passes
        std::vector<RenderPassNode> passes;
        // Array of indexes into list of passes. Order is optimized by the optimizer.
        std::vector<uint32_t> optimizedOrderOfPasses;

    public:
        RenderGraph(Device &device, experimental::ResourceManager &rm, FrameManager &context): device(device), rm(rm),
            renderContext(context),
            maxFramesInFlight(SwapChain::MAX_FRAMES_IN_FLIGHT) {
        }

        ~RenderGraph() {
        }


        /**
         * Creates a resource in the graph
         * @tparam Type Type of the resource Node
         * @tparam ResourceType Type of the actual resource
         * @tparam DescriptionType Type of the description of the resource based on the type of the resource 
         * @param name Name of the resource for lookup and reference
         * @param desc Description of the resource
         */
        template<ResourceNode::Type Type, typename ResourceType, typename DescriptionType>
        void addResource(const std::string &name, const DescriptionType &desc) {
            ResourceNode node;
            node.type = Type;
            node.name = name;
            node.resolver = [name, desc](experimental::ResourceManager &rm, uint32_t frameIndex) {
                return rm.createResource<frameIndex, ResourceType>(name, desc);
            };
            resources[name] = std::move(node);
        }

        /**
         * Create a static (external, non-buffered) resource
         * @tparam Type Type of the resource node
         * @param name Name of the resource
         * @param handle Handle of the actuall resource
         */
        template<ResourceNode::Type Type>
        void addStaticResource(const std::string &name, experimental::ResourceHandle handle) {
            ResourceNode node;
            node.type = Type;
            node.name = name;
            node.resolver = [handle](experimental::ResourceManager &rm, uint32_t frameIndex) {
                return handle;
            };
            resources[name] = std::move(node);
        }


        /**
         * Create a resource that dependes on swapchain size
         * @tparam Type Type of the dependant resource node
         * @tparam ResourceType Type of the actual resource
         * @tparam DescriptionType Type of the resource description
         * @param name Name of the resource
         * @param modifier Callback to create a description of the dependant resource based on the swapchain size
         */
        template<ResourceNode::Type Type, typename ResourceType, typename DescriptionType>
        void addSwapChainDependentResource(const std::string &name,
                                           std::function<DescriptionType(VkExtent2D)> modifier) {
            ResourceNode node;
            node.type = Type;
            node.name = name;
            node.resolver = [this,name, modifier](experimental::ResourceManager &rm, uint32_t frameIndex) {
                VkExtent2D swapChainExtent = renderContext.getSwapChain()->getSwapChainExtent();
                ASSERT(modifier, "Modifier is null!");
                DescriptionType depDesc = modifier(swapChainExtent);
                return rm.createResource<frameIndex, ResourceType>(name, depDesc);
            };
            resources[name] = std::move(node);
        }

        /**
         * Creates a resource that dependes on another resource
         * @tparam Type Type of the resource node
         * @tparam ResourceType Type of the actual resource
         * @tparam DescriptionType Type of the resource description
         * @param name Name of the dependent resource
         * @param dependency Name of dependency resource
         * @param modifier Call back that creates dependent description base on the dependency description
         */
        template<ResourceNode::Type Type, typename ResourceType, typename DescriptionType>
        void addDependentResource(const std::string &name, const std::string &dependency,
                                  std::function<DescriptionType(experimental::ResourceHandle)> modifier) {
            ResourceNode node;
            node.type = Type;
            node.name = name;
            node.resolver = [this, name, dependency,modifier](experimental::ResourceManager &rm, uint32_t frameIndex) {
                auto depHandle = resources.at(dependency).resolve(rm, frameIndex);
                ASSERT(modifier, "Modifier is null!");
                auto newDesc = modifier(depHandle);
                return rm.createResource<ResourceType>(name, newDesc);
            };
            resources[name] = std::move(node);
        }

        /**
         * Creates a resource that represents swapchain image. This way you can reference to swapchain image using the resource name.
         * @tparam Type Type of the SwapChain attachment (SwapChainColorAttachment or SwapChainDepthStencilAttachment)
         * @param name Name of the resource
         */
        template<ResourceNode::Type Type, typename = std::enable_if_t<
            Type == ResourceNode::Type::SwapChainColorAttachment || Type ==
            ResourceNode::Type::SwapChainDepthStencilAttachment> >
        void addSwapChainImageResource(const std::string &name) {
            ResourceNode node;
            node.name = name;
            node.type = Type;
            node.resolver = nullptr;
            resources[name] = std::move(node);
        }



        template<RenderPassType Type, ViewPortSize ViewPortSize, float ViewPortWidth = 1.0f, float ViewPortHeight = 1.0f, float ViewPortDepthMin = 0.f, float ViewPortDepthMax = 1.f>
        RenderPassNode &addPass(const std::string &name) {
            RenderPassNode node;
            node.name = name;
            node.type = Type;
            node.viewportSize = ViewPortSize;
            node.viewport = HmckVec4{ViewPortWidth, ViewPortHeight, ViewPortDepthMin, ViewPortDepthMax};
            passes.push_back(node);
            return passes.back();
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
         * Applies pipeline barrier transition where it is needed based on the stage
         * @param pass Render pass
         * @param cmd Valid command buffer
         * @param stage Stage of the barrier
         */
        void applyPipelineBarriers(RenderPassNode &pass, PipelineBarrier::TransitionStage stage) {
            for (auto &access: pass.outputs) {
                ASSERT(resources.find(access.resourceName) != resources.end(),
                       "Could not find the output");

                ResourceNode &resourceNode = resources[access.resourceName];
                PipelineBarrier barrier(rm, renderContext, resourceNode, access, stage);
                if (barrier.isNeeded()) {
                    barrier.apply();
                }
            }
        }

        /**
         * Returns list of color attachments of a render pass
         * @param pass Render pass
         * @return Returns list of render passes color attachments
         */
        std::vector<VkRenderingAttachmentInfo> collectColorAttachmentInfos(RenderPassNode &pass) {
            std::vector<VkRenderingAttachmentInfo> colorAttachments;
            for (auto &access: pass.outputs) {
                ResourceNode &node = resources[access.resourceName];
                if (node.isSwapChainAttachment() && node.isColorAttachment()) {
                    VkRenderingAttachmentInfo attachmentInfo{};
                    attachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
                    attachmentInfo.imageView = renderContext.getSwapChain()->
                            getImageView(renderContext.getImageIndex());
                    attachmentInfo.imageLayout = access.requiredLayout;
                    attachmentInfo.clearValue = {};
                    attachmentInfo.loadOp = access.loadOp;
                    attachmentInfo.storeOp = access.storeOp;
                    colorAttachments.push_back(attachmentInfo);
                } else if (node.isColorAttachment()) {
                    ASSERT(node.resolver, "Resolver is nullptr!");
                    experimental::ResourceHandle handle = node.resolve(rm, renderContext.getFrameIndex());
                    experimental::Image *image = rm.getResource<experimental::Image>(handle);
                    VkRenderingAttachmentInfo attachmentInfo = image->getRenderingAttachmentInfo();
                    attachmentInfo.loadOp = access.loadOp;
                    attachmentInfo.storeOp = access.storeOp;
                    colorAttachments.push_back(attachmentInfo);
                }
            }

            return colorAttachments;
        }

        /**
         * Returns depth attachment of a render pass
         * @param pass Render pass
         * @return Returns VkRenderingAttachmentInfo of depth attachment from a pass
         */
        std::optional<VkRenderingAttachmentInfo> collectDepthStencilAttachmentInfo(RenderPassNode &pass) {
            for (auto &access: pass.outputs) {
                ResourceNode &node = resources[access.resourceName];
                if (node.isSwapChainAttachment() && node.isDepthAttachment()) {
                    VkRenderingAttachmentInfo attachmentInfo{};
                    attachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
                    attachmentInfo.imageView = renderContext.getSwapChain()->
                            getDepthImageView(renderContext.getImageIndex());
                    attachmentInfo.imageLayout = access.requiredLayout;
                    attachmentInfo.clearValue = {};
                    attachmentInfo.loadOp = access.loadOp;
                    attachmentInfo.storeOp = access.storeOp;
                    return attachmentInfo;
                }

                if (node.isDepthAttachment()) {
                    ASSERT(node.resolver, "Resolver is nullptr!");
                    experimental::ResourceHandle handle = node.resolve(rm, renderContext.getFrameIndex());
                    experimental::Image *image = rm.getResource<experimental::Image>(handle);
                    VkRenderingAttachmentInfo attachmentInfo = image->getRenderingAttachmentInfo();
                    attachmentInfo.loadOp = access.loadOp;
                    attachmentInfo.storeOp = access.storeOp;
                    return attachmentInfo;
                }
            }

            return std::nullopt;
        }

        /**
         * Creates context for the givem renderpass
         * @param pass Render pass
         * @return Returns RenderPassContext for the given renderpass
         */
        RenderPassContext createRenderPassContext(RenderPassNode &pass) {
            RenderPassContext context{};

            // Fill the context with input resources
            for (auto &access: pass.inputs) {
                ASSERT(resources.find(access.resourceName) != resources.end(),
                       "Could not find the input");
                ResourceNode &resourceNode = resources[access.resourceName];
                // TODO automatically find out next render pass using this resource and set final layout of the attachment to the requiredLayout to save one barrier
                context.inputs.emplace(resourceNode.name, resourceNode);
            }

            // fill context with input resources
            for (auto &access: pass.outputs) {
                ASSERT(resources.find(access.resourceName) != resources.end(),
                       "Could not find the output");
                ResourceNode &resourceNode = resources[access.resourceName];
                context.outputs.emplace(resourceNode.name, resourceNode);
            }

            // Add the command buffer and frame index to the context
            context.commandBuffer = renderContext.getCurrentCommandBuffer();
            context.frameIndex = renderContext.getFrameIndex();

            return context;
        }

        /**
         * Begins the rendering using the dynamic rendering extension
         * @param pass Current Render pass
         */
        void beginRendering(RenderPassNode &pass) {
            // Collect attachments
            std::vector<VkRenderingAttachmentInfo> colorAttachments = collectColorAttachmentInfos(pass);
            auto depthStencilOptional = collectDepthStencilAttachmentInfo(pass);

            VkRenderingInfo renderingInfo{VK_STRUCTURE_TYPE_RENDERING_INFO_KHR};
            renderingInfo.renderArea = {
                0, 0, renderContext.getSwapChain()->getSwapChainExtent().width,
                renderContext.getSwapChain()->getSwapChainExtent().height
            };
            renderingInfo.layerCount = 1;
            renderingInfo.colorAttachmentCount = colorAttachments.size();
            renderingInfo.pColorAttachments = colorAttachments.data();
            renderingInfo.pDepthAttachment = depthStencilOptional.has_value() ? &depthStencilOptional.value() : VK_NULL_HANDLE;
            renderingInfo.pStencilAttachment = VK_NULL_HANDLE;

            // Start a dynamic rendering
            vkCmdBeginRendering(renderContext.getCurrentCommandBuffer(), &renderingInfo);

            // Set viewport and scissors
            VkViewport viewport = Init::viewport(pass.viewport.X, pass.viewport.Y, pass.viewport.Z, pass.viewport.W);
            if (pass.viewportSize == ViewPortSize::SwapChainRelative) {
                viewport = Init::viewport(pass.viewport.X * renderContext.getSwapChain()->getSwapChainExtent().width, pass.viewport.Y * renderContext.getSwapChain()->getSwapChainExtent().height, pass.viewport.Z, pass.viewport.W);
            }
            VkRect2D scissor{{0, 0}, VkExtent2D{static_cast<uint32_t>(viewport.width), static_cast<uint32_t>(viewport.height)}};
            vkCmdSetViewport(renderContext.getCurrentCommandBuffer(), 0, 1, &viewport);
            vkCmdSetScissor(renderContext.getCurrentCommandBuffer(), 0, 1, &scissor);
        }

        /**
         * Ends the rendering for current renderpass
         */
        void endRendering() {
            vkCmdEndRendering(renderContext.getCurrentCommandBuffer());
        }


        /**
         * Executes the graph and makes necessary barrier transitions.
         */
        void execute() {
            ASSERT(optimizedOrderOfPasses.size() == passes.size(),
                   "RenderPass queue sizes don't match. This should not happen.");

            // Begin frame by creating master command buffer
            if (VkCommandBuffer cmd = renderContext.beginFrame()) {
                // Loop over passes in the optimized order
                for (const auto &passIndex: optimizedOrderOfPasses) {
                    ASSERT(
                        std::find(optimizedOrderOfPasses.begin(),optimizedOrderOfPasses.end(), passIndex) !=
                        optimizedOrderOfPasses.end(),
                        "Could not find the pass based of index. This should not happen")
                    ;
                    // Render pass node and its context
                    RenderPassNode &pass = passes[passIndex];

                    // Apply required barriers
                    applyPipelineBarriers(pass, PipelineBarrier::TransitionStage::RequiredLayout);

                    // Begin rendering
                    beginRendering(pass);

                    // Create render passs context
                    RenderPassContext passCtx = createRenderPassContext(pass);

                    // Dispatch the render pass callback
                    pass.executeFunc(passCtx);

                    // End rendering
                    endRendering();

                    // post rendering barrier
                    applyPipelineBarriers(pass, PipelineBarrier::TransitionStage::FinalLayout);
                }

                // End work on the current frame and submit the command buffer
                renderContext.endFrame();
            }
        }
    };
};
