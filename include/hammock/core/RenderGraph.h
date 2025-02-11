#pragma once

#include <functional>
#include <cassert>
#include <complex>
#include <utility>
#include <variant>
#include <type_traits>
#include <optional>

#include <hammock/core/Device.h>
#include <hammock/core/CoreUtils.h>
#include <hammock/resources/Descriptors.h>

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

        // cache to store handles to avoid constant recreation
        std::vector<experimental::ResourceHandle> cachedHandles;

        // Needs recreation
        bool isDirty = true;

        /**
         * Returns handle corresponding to resource of specific frame
         * @param rm ResourceManager where resource is registered
         * @param frameIndex Frame index of the resource
         * @return Returns resolved handle
         */
        experimental::ResourceHandle resolve(experimental::ResourceManager &rm, uint32_t frameIndex) {
            if (isDirty || cachedHandles.empty()) {
                cachedHandles.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
                isDirty = false;
            }

            if (!cachedHandles[frameIndex].isValid()) {
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
        // TODO change this to resource handles
        std::unordered_map<std::string, ResourceNode *> inputs;
        std::unordered_map<std::string, ResourceNode *> outputs;
        std::vector<VkDescriptorSet> descriptorSets;
    };

    /**
     * Describes how the resource will be accessed
     */
    struct ResourceAccess {
        std::string resourceName; // Name of the accessed resource
        VkImageLayout requiredLayout = VK_IMAGE_LAYOUT_UNDEFINED; // Layout before rendering
        VkImageLayout finalLayout = VK_IMAGE_LAYOUT_UNDEFINED; // Layout after rendering, optional
        VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // Load op
        VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_STORE; // Store op
        VkPipelineStageFlags stageFlags = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    };

    enum RenderPassFlags : int32_t {
        RENDER_PASS_FLAGS_NONE = 0,
        RENDER_PASS_FLAGS_SWAPCHAIN_WRITE = 1 << 0,
        RENDER_PASS_FLAGS_CONTRIBUTING = 1 << 1,
    };

    /**
     * RenderGraph node representing a render pass
     */
    struct RenderPassNode {
        typedef std::function<void(RenderPassContext)> ExecuteFunction;

        struct Descriptor {
            std::shared_ptr<DescriptorSetLayout> layout;
            std::vector<VkDescriptorSet> setCache;
        };

        struct DescriptorBinding {
            uint32_t bindingIndex;
            std::vector<std::string> bindingNames;
            VkShaderStageFlags stageFlags;
            VkDescriptorBindingFlags bindingFlags = 0;
        };

        CommandQueueFamily type; // Type of the pass
        int32_t flags = 0;
        RelativeViewPortSize viewportSize; // Size of viewport

        std::string name; // Name of the pass for debug purposes and lookup
        HmckVec4 viewport;
        std::vector<ResourceAccess> inputs; // Read accesses
        std::vector<ResourceAccess> outputs; // Write accesses
        std::unordered_map<uint32_t, std::vector<DescriptorBinding> > descriptorLayoutInfos{};


        std::unordered_map<uint32_t, Descriptor> descriptors;
        // TODO samplers should be standalone resources
        std::vector<VkSampler> samplers;

        std::vector<RenderPassNode *> dependencies;


        // callback for rendering
        ExecuteFunction executeFunc = nullptr;

        VkDescriptorSet resolveDescriptorSet(uint32_t setIndex, uint32_t frameIndex) {
            ASSERT(descriptors.contains(setIndex), "Invalid set index or called before build");
            Descriptor &descriptor = descriptors.at(setIndex);
            return descriptor.setCache[frameIndex];
        }


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

        RenderPassNode &execute(ExecuteFunction exec) {
            executeFunc = exec;
            return *this;
        }

        RenderPassNode &descriptor(const uint32_t set, std::vector<DescriptorBinding> bindings) {
            ASSERT(!descriptorLayoutInfos.contains(set), "Descriptor already set");
            descriptorLayoutInfos[set] = std::move(bindings);
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

        explicit PipelineBarrier(experimental::ResourceManager &rm, FrameManager &renderContext,
                                 VkCommandBuffer commandBuffer, ResourceNode &node,
                                 ResourceAccess &access,
                                 TransitionStage transitionStage) : rm(rm), renderContext(renderContext),
                                                                    commandBuffer(commandBuffer), node(node),
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
                    return attachment->getLayout() != access.finalLayout && access.finalLayout !=
                           VK_IMAGE_LAYOUT_UNDEFINED;
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
        VkCommandBuffer commandBuffer;
    };

    /**
     * RenderGraph is a class representing a directed acyclic graph (DAG) that describes the process of creation of a frame.
     * It consists of resource nodes and render pass nodes. Render pass node can be dependent on some resources and
     * can itself create resources that other passes may depend on. RenderGraph analyzes these dependencies, makes adjustments when possible,
     * makes necessary transitions between resource states. These transitions are done just in time as well as resource allocation.
     *  TODO support for Read Modify Write - render pass writes and reads from the same resource
     *  TODO support for conditional resource
     *  TODO support for swapchain dependent rendperPass
     *  TODO support for pipeline caching
     */
    class RenderGraph {
        struct SubmissionGroup {
            CommandQueueFamily queueFamily;
            std::vector<RenderPassNode *> renderPassNodes;
            std::vector<VkSemaphore> waitSemaphores; // one semaphore per frame in flight
            std::vector<VkSemaphore> signalSemaphores; // one semaphore per frame in flight
            VkPipelineStageFlags waitStage;
            std::vector<VkCommandBuffer> commandBuffers; // one command buffer per frame in flight
            size_t globalStartIndex = 0; // The global sorted order index of the first pass in this group.
        };

        // This structure pairs a SubmissionGroup pointer with its global start index.
        struct GroupWithGlobalIndex {
            SubmissionGroup *group;
            size_t globalStartIndex;
        };

        // Vulkan device
        Device &device;
        // Rendering context
        FrameManager &fm;
        experimental::ResourceManager &rm;
        DescriptorPool &pool;
        // Holds all the resources
        std::unordered_map<std::string, ResourceNode> resources;
        // Holds all the render passes
        std::vector<RenderPassNode> passes;

        std::vector<RenderPassNode *> sortedPasses; // contains topologically sorted passes
        std::unordered_map<CommandQueueFamily, std::vector<SubmissionGroup> > groupsByQueue;
        // submission groups by queue family
        std::vector<GroupWithGlobalIndex> allGroups; // all groups in execution order

    public:
        RenderGraph(Device &device, experimental::ResourceManager &rm,
                    FrameManager &fm, DescriptorPool &pool): device(device), rm(rm), fm(fm), pool(pool) {
            Logger::log(LOG_LEVEL_DEBUG, "Creating rendegraph\n");
        }

        ~RenderGraph() {
            Logger::log(LOG_LEVEL_DEBUG, "Releasing rendegraph\n");
            // Render pass hold samplers for all attachments and these need to be released
            for (auto &pass: passes) {
                for (auto &sampler: pass.samplers) {
                    vkDestroySampler(device.device(), sampler, nullptr);
                }
            }
            passes.clear();
            resources.clear();

            // Free command buffer and destroy sync objects
            for (auto &group: allGroups) {
                if (group.group->queueFamily == CommandQueueFamily::Graphics) {
                    vkFreeCommandBuffers(device.device(), device.getGraphicsCommandPool(), group.group->commandBuffers.size(), group.group->commandBuffers.data());
                }

                if (group.group->queueFamily == CommandQueueFamily::Compute) {
                    vkFreeCommandBuffers(device.device(), device.getComputeCommandPool(), group.group->commandBuffers.size(), group.group->commandBuffers.data());
                }

                if (group.group->queueFamily == CommandQueueFamily::Transfer) {
                    vkFreeCommandBuffers(device.device(), device.getTransferCommandPool(), group.group->commandBuffers.size(), group.group->commandBuffers.data());
                }

                for (auto& signal: group.group->signalSemaphores) {
                    vkDestroySemaphore(device.device(), signal, nullptr);
                }

                for (auto& wait: group.group->waitSemaphores) {
                    vkDestroySemaphore(device.device(), wait, nullptr);
                }
            }

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
                return rm.createResource<ResourceType>(name, desc);
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
                VkExtent2D swapChainExtent = fm.getSwapChain()->getSwapChainExtent();
                ASSERT(modifier, "Modifier is null!");
                DescriptionType depDesc = modifier(swapChainExtent);
                return rm.createResource<ResourceType>(name, depDesc);
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


        /**
         *
         * @tparam QueueFamily Type of the Pass (Graphics, Transfer, Compute)
         * @tparam ViewPortSize ViewPortSize class (SwapChainRelative, Fixed)
         * @tparam ViewPortWidth ViewPort width multiplier (if Fixed, then absolute width)
         * @tparam ViewPortHeight ViewPort height multiplier (if Fixed, then absolute height)
         * @tparam ViewPortDepthMin Min depth value
         * @tparam ViewPortDepthMax Max depth value
         * @param name Name of the render pass
         * @return Returns reference to the RenderPassNode node that can be used to set additional parameters. See RenderPassNode definition.
         */
        template<CommandQueueFamily QueueFamily, RelativeViewPortSize ViewPortSize =
                    RelativeViewPortSize::SwapChainRelative, float
            ViewPortWidth = 1.0f, float ViewPortHeight = 1.0f
            , float ViewPortDepthMin = 0.f, float ViewPortDepthMax = 1.f>
        RenderPassNode &addPass(const std::string &name) {
            RenderPassNode node;
            node.name = name;
            node.type = QueueFamily;
            node.viewportSize = ViewPortSize;
            node.viewport = HmckVec4{ViewPortWidth, ViewPortHeight, ViewPortDepthMin, ViewPortDepthMax};
            passes.push_back(node);
            return passes.back();
        }

        /**
         * Returns list of descriptor set layouts used in the render pass
         * @param passName Name of the render pass
         * @return Vector of descriptor set layouts used in the render pass
         */
        std::vector<VkDescriptorSetLayout> getDescriptorSetLayouts(const std::string &passName) {
            std::vector<VkDescriptorSetLayout> layouts = {};
            for (auto &pass: passes) {
                if (pass.name == passName) {
                    for (const auto &descriptor: pass.descriptors) {
                        layouts.push_back(descriptor.second.layout->getDescriptorSetLayout());
                    }
                }
            }
            return layouts;
        }

        void prepareResources() {
            for (auto &pass: passes) {
                for (auto &input: pass.inputs) {
                    ResourceNode &resource = resources[input.resourceName];
                    for (int frameIndex = 0; frameIndex < SwapChain::MAX_FRAMES_IN_FLIGHT; frameIndex++) {
                        if (resource.isRenderingAttachment()) {
                            experimental::Image *image = rm.getResource<experimental::Image>(
                                resource.resolve(rm, frameIndex));
                            image->queueImageLayoutTransition(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                        }
                        if (resource.type == ResourceNode::Type::UniformBuffer) {
                            experimental::Buffer *buffer = rm.getResource<experimental::Buffer>(
                                resource.resolve(rm, frameIndex));
                            buffer->map();
                        }
                    }
                }
            }
        }

        void purgeNonContributingPasses() {
            size_t originalSize = passes.size();
            // Identify swapchain writers and initialize queue
            std::queue<size_t> worklist;
            for (int i = 0; i < passes.size(); i++) {
                for (auto &write: passes[i].outputs) {
                    ResourceNode &resource = resources[write.resourceName];
                    if (resource.isSwapChainAttachment()) {
                        passes[i].flags |= RENDER_PASS_FLAGS_SWAPCHAIN_WRITE;
                        passes[i].flags |= RENDER_PASS_FLAGS_CONTRIBUTING;
                        worklist.push(i);
                    }
                }
            }

            // Traverse upwards marking contributing passes
            while (!worklist.empty()) {
                size_t passIndex = worklist.front();
                worklist.pop();

                const auto &pass = passes[passIndex];

                // Iterate over the inputs of this pass
                for (const auto &input: pass.inputs) {
                    auto it = resources.find(input.resourceName);
                    if (it != resources.end()) {
                        // Find the passes that write to this resource
                        for (int i = 0; i < passes.size(); i++) {
                            for (auto &write: passes[i].outputs) {
                                if (write.resourceName == input.resourceName) {
                                    ResourceNode &resource = resources[write.resourceName];
                                    if (!(passes[i].flags & RENDER_PASS_FLAGS_CONTRIBUTING)) {
                                        passes[i].flags |= RENDER_PASS_FLAGS_CONTRIBUTING;
                                        worklist.push(i);
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // Remove non-contributing passes
            std::erase_if(passes, [](const RenderPassNode &pass) {
                return !(pass.flags & RENDER_PASS_FLAGS_CONTRIBUTING);
            });

            Logger::log(LOG_LEVEL_DEBUG, "Purged %d non-contributing passes\n", originalSize - passes.size());
        }

        void detectDependencies() {
            // for each pass
            for (int passIdx = 0; passIdx < passes.size(); passIdx++) {
                // for each input of the current pass
                for (auto &input: passes[passIdx].inputs) {
                    // find which passes write to the input
                    for (int i = 0; i < passes.size(); i++) {
                        for (auto &write: passes[i].outputs) {
                            if (write.resourceName == input.resourceName) {
                                passes[passIdx].dependencies.push_back(&passes[i]);
                            }
                        }
                    }
                }
            }
        }

        void topologicallySortPasses() {
            // Clear existing sorted passes
            sortedPasses.clear();

            // First pass: Add Graphics passes
            for (int passIdx = 0; passIdx < passes.size(); passIdx++) {
                RenderPassNode &passNode = passes[passIdx];
                if (passNode.type == CommandQueueFamily::Graphics) {
                    sortedPasses.push_back(&passNode);
                }
            }

            // Second pass: Add Compute passes
            for (int passIdx = 0; passIdx < passes.size(); passIdx++) {
                RenderPassNode &passNode = passes[passIdx];
                if (passNode.type == CommandQueueFamily::Compute) {
                    sortedPasses.push_back(&passNode);
                }
            }

            // Third pass: Add Transfer passes
            for (int passIdx = 0; passIdx < passes.size(); passIdx++) {
                RenderPassNode &passNode = passes[passIdx];
                if (passNode.type == CommandQueueFamily::Transfer) {
                    sortedPasses.push_back(&passNode);
                }
            }
            Logger::log(LOG_LEVEL_DEBUG, "Topological order: ");
            for (int i = 0; i < sortedPasses.size(); i++) {
                Logger::log(LOG_LEVEL_DEBUG, " \"%s\" -> ", sortedPasses[i]->name.c_str());
            }
            Logger::log(LOG_LEVEL_DEBUG, " PRESENT\n");
        }

        bool needsNewGroup(const SubmissionGroup &currentGroup,
                           RenderPassNode *newPass,
                           size_t globalIndex) {
            // For the very first pass (globalIndex==0), no group exists yet so no need to split.
            if (globalIndex == 0)
                return false;

            // Rule 1: Check if the immediately preceding pass in global order is of a different queue.
            // If so, newPass is not contiguous with the previous work on the same queue.
            if (sortedPasses[globalIndex - 1]->type != newPass->type)
                return true;

            // Rule 2: Check if newPass depends on any pass that is NOT in the current submission group.
            // (This would signal that newPass is waiting on work that was separated by an inter-queue dependency.)
            for (RenderPassNode *dep: newPass->dependencies) {
                // If a dependency is not found among the passes already recorded in the current group,
                // then we require a new submission group.
                if (std::find(currentGroup.renderPassNodes.begin(),
                              currentGroup.renderPassNodes.end(),
                              dep) == currentGroup.renderPassNodes.end()) {
                    return true;
                }
            }

            // Otherwise, the candidate pass can be merged into the current group.
            return false;
        }

        void groupPassesBySubmissionGroup() {
            // Iterate through the global sorted list.
            for (size_t i = 0; i < sortedPasses.size(); ++i) {
                RenderPassNode *pass = sortedPasses[i];
                CommandQueueFamily qf = pass->type;

                // Create an initial group for this queue family if needed.
                if (groupsByQueue[qf].empty()) {
                    SubmissionGroup newGroup;
                    newGroup.queueFamily = qf;
                    newGroup.globalStartIndex = i; // first pass in this group.
                    groupsByQueue[qf].push_back(newGroup);
                } else {
                    // Get the last submission group for this queue family.
                    SubmissionGroup &currentGroup = groupsByQueue[qf].back();
                    if (needsNewGroup(currentGroup, pass, i)) {
                        SubmissionGroup newGroup;
                        newGroup.queueFamily = qf;
                        newGroup.globalStartIndex = i;
                        groupsByQueue[qf].push_back(newGroup);
                    }
                }

                // Add the current pass to the latest submission group for its queue.
                groupsByQueue[qf].back().renderPassNodes.push_back(pass);
            }
        }

        void executionOrderSortPasses() {
            for (auto &queueFamily: groupsByQueue) {
                for (auto &group: queueFamily.second) {
                    // Gather all submission groups (from all queue families) into a single vector.
                    for (auto &kv: groupsByQueue) {
                        for (auto &group: kv.second) {
                            allGroups.push_back({&group, group.globalStartIndex});
                        }
                    }
                }

                // Sort the groups by the global start index so that they follow the topological order.
                std::sort(allGroups.begin(), allGroups.end(),
                          [](const GroupWithGlobalIndex &a, const GroupWithGlobalIndex &b) {
                              return a.globalStartIndex < b.globalStartIndex;
                          });
            }

            Logger::log(LOG_LEVEL_DEBUG, "Execution order: ");
            for (int i = 0; i < allGroups.size(); i++) {
                Logger::log(LOG_LEVEL_DEBUG, " EXECUTION_GROUP{ ");
                for (int j = 0; j < allGroups[i].group->renderPassNodes.size(); j++) {
                    Logger::log(LOG_LEVEL_DEBUG, " \"%s\" -> ", allGroups[i].group->renderPassNodes[j]->name.c_str());
                }
                Logger::log(LOG_LEVEL_DEBUG, " } -> ");
            }
            Logger::log(LOG_LEVEL_DEBUG, " \"present\"\n");

            VkSemaphoreCreateInfo semaphoreInfo = {};
            semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

            // Iterate over the groups in order of execution
            for (const auto &groupWithIndex: allGroups) {
                SubmissionGroup *group = groupWithIndex.group;

                group->commandBuffers.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
                group->waitSemaphores.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
                group->signalSemaphores.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);

                for (int frameIdx = 0; frameIdx < SwapChain::MAX_FRAMES_IN_FLIGHT; frameIdx++) {
                    // First, create a command buffer for the group
                    // conservative approach for wait stage
                    group->waitStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

                    // Allocate the command buffers
                    if (group->queueFamily == CommandQueueFamily::Graphics) {
                        group->commandBuffers[frameIdx] = fm.createCommandBuffer<CommandQueueFamily::Graphics>();
                    }

                    if (group->queueFamily == CommandQueueFamily::Compute) {
                        group->commandBuffers[frameIdx] = fm.createCommandBuffer<CommandQueueFamily::Compute>();
                    }

                    if (group->queueFamily == CommandQueueFamily::Transfer) {
                        group->commandBuffers[frameIdx] = fm.createCommandBuffer<CommandQueueFamily::Transfer>();
                    }

                    // If this is a first and only group, we don!t create any semaphores
                    if (group->globalStartIndex == 0 && allGroups.size() == 1) {
                        continue;
                    }

                    // If this is the first group, we only create a signal semaphore
                    if (group->globalStartIndex == 0) {
                        ASSERT(
                            vkCreateSemaphore(device.device(), &semaphoreInfo, nullptr, &group->signalSemaphores[
                                frameIdx]) == VK_SUCCESS, "Could not create semaphore");
                        continue;
                    }

                    // If this is the last group, we only create wait semaphore
                    if (group->globalStartIndex == allGroups.size() - 1) {
                        ASSERT(
                            vkCreateSemaphore(device.device(), &semaphoreInfo, nullptr, &group->waitSemaphores[frameIdx]
                            ) == VK_SUCCESS, "Could not create semaphore");
                        continue;
                    }

                    // Else we create both wait and signal semaphores
                    ASSERT(
                        vkCreateSemaphore(device.device(), &semaphoreInfo, nullptr, &group->waitSemaphores[frameIdx]) ==
                        VK_SUCCESS, "Could not create semaphore");
                    ASSERT(
                        vkCreateSemaphore(device.device(), &semaphoreInfo, nullptr, &group->signalSemaphores[frameIdx])
                        == VK_SUCCESS, "Could not create semaphore");
                }
            }
        }

        /**
         * Compiles the render graph - analyzes dependencies and makes optimization
         */
        void build() {
            // First we purge passes that do not contribute to the final image
            purgeNonContributingPasses();

            // Topologically sort the passes
            topologicallySortPasses();

            // Detect and fill dependencies between passes
            detectDependencies();

            // Group passes into submission groups
            groupPassesBySubmissionGroup();

            // Sort passes in optimal execution order
            executionOrderSortPasses();

            // Transition all input images to required layouts and map all uniform buffers
            prepareResources();

            // Finally we need to build the descriptor layouts for each pass
            buildDescriptors();
        }


        /**
         * Creates all descriptor layouts and sets. One set per frame in flight is created. One layout per set is created
         * TODO cache the descriptor layouts
         */
        void buildDescriptors() {
            // Create descriptors
            for (int passIdx = 0; passIdx < passes.size(); passIdx++) {
                RenderPassNode &passNode = passes[passIdx];
                for (auto &descInfo: passNode.descriptorLayoutInfos) {
                    // Create a descriptor set layout
                    uint32_t setIndex = descInfo.first;
                    auto descriptorSetLayoutBuilder = DescriptorSetLayout::Builder(device);
                    for (auto &binding: descInfo.second) {
                        // No arrays yet
                        ASSERT(binding.bindingNames.size() == 1,
                               "Only one binding name per binding supported as of now!");
                        ASSERT(resources.contains(binding.bindingNames.at(0)),
                               "Binding name does not reference existing resource!");

                        ResourceNode &resourceNode = resources[binding.bindingNames.at(0)];

                        VkDescriptorType type = resourceNode.isBuffer()
                                                    ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
                                                    : VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

                        descriptorSetLayoutBuilder.addBinding(binding.bindingIndex, type, binding.stageFlags,
                                                              binding.bindingNames.size(), binding.bindingFlags);
                    }

                    auto descriptorSetLayout = descriptorSetLayoutBuilder.build();
                    passNode.descriptors.emplace(setIndex, RenderPassNode::Descriptor{
                                                     .layout = std::move(descriptorSetLayout), .setCache = {}
                                                 });

                    // Create descriptor set for each frame in flight
                    passNode.descriptors.at(setIndex).setCache.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
                    for (int frameInFlight = 0; frameInFlight < SwapChain::MAX_FRAMES_IN_FLIGHT; frameInFlight++) {
                        auto &desc = passNode.descriptorLayoutInfos.at(setIndex);
                        auto writer = DescriptorWriter(*passNode.descriptors[setIndex].layout, pool);
                        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
                        std::vector<VkDescriptorBufferInfo> bufferInfos;
                        std::vector<VkDescriptorImageInfo> imageInfos;

                        for (auto &binding: desc) {
                            ResourceNode &resourceNode = resources[binding.bindingNames.at(0)];

                            if (resourceNode.isBuffer()) {
                                auto *buffer = rm.getResource<experimental::Buffer>(
                                    resourceNode.resolve(rm, frameInFlight));
                                bufferInfos.push_back(buffer->descriptorInfo());
                                writer.writeBuffer(binding.bindingIndex, &bufferInfos.back());
                            }

                            if (resourceNode.isRenderingAttachment()) {
                                const auto *image = rm.getResource<experimental::Image>(
                                    resourceNode.resolve(rm, frameInFlight));
                                VkSampler sampler = image->createAndGetSampler();
                                passNode.samplers.push_back(sampler);
                                imageInfos.push_back(image->getDescriptorImageInfo(sampler));
                                writer.writeImage(binding.bindingIndex, &imageInfos.back());
                            }
                        }

                        ASSERT(writer.build(descriptorSet), "Failed to build descriptor set!");
                        passNode.descriptors.at(setIndex).setCache[frameInFlight] = descriptorSet;
                    }
                }
            }
        }

        /**
         * Applies pipeline barrier transition where it is needed based on the stage
         * @param pass Render pass
         * @param stage Stage of the barrier
         */
        void applyPipelineBarriers(RenderPassNode *pass, VkCommandBuffer commandBuffer,
                                   PipelineBarrier::TransitionStage stage) {
            for (auto &access: pass->outputs) {
                ASSERT(resources.find(access.resourceName) != resources.end(),
                       "Could not find the output");

                ResourceNode &resourceNode = resources[access.resourceName];
                PipelineBarrier barrier(rm, fm, commandBuffer, resourceNode, access, stage);
                if (barrier.isNeeded()) {
                    barrier.apply();
                }
            }
            for (auto &access: pass->inputs) {
                ASSERT(resources.find(access.resourceName) != resources.end(),
                       "Could not find the output");

                ResourceNode &resourceNode = resources[access.resourceName];
                PipelineBarrier barrier(rm, fm, commandBuffer, resourceNode, access, stage);
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
        std::vector<VkRenderingAttachmentInfo> collectColorAttachmentInfos(RenderPassNode *pass) {
            std::vector<VkRenderingAttachmentInfo> colorAttachments;
            for (auto &access: pass->outputs) {
                ResourceNode &node = resources[access.resourceName];
                if (node.isSwapChainAttachment() && node.isColorAttachment()) {
                    VkRenderingAttachmentInfo attachmentInfo{};
                    attachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
                    attachmentInfo.imageView = fm.getSwapChain()->
                            getImageView(fm.getSwapChainImageIndex());
                    attachmentInfo.imageLayout = access.requiredLayout;
                    attachmentInfo.clearValue = {};
                    attachmentInfo.loadOp = access.loadOp;
                    attachmentInfo.storeOp = access.storeOp;
                    colorAttachments.push_back(attachmentInfo);
                } else if (node.isColorAttachment()) {
                    ASSERT(node.resolver, "Resolver is nullptr!");
                    experimental::ResourceHandle handle = node.resolve(rm, fm.getFrameIndex());
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
        std::optional<VkRenderingAttachmentInfo> collectDepthStencilAttachmentInfo(RenderPassNode *pass) {
            for (auto &access: pass->outputs) {
                ResourceNode &node = resources[access.resourceName];
                if (node.isSwapChainAttachment() && node.isDepthAttachment()) {
                    VkRenderingAttachmentInfo attachmentInfo{};
                    attachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
                    attachmentInfo.imageView = fm.getSwapChain()->
                            getDepthImageView(fm.getSwapChainImageIndex());
                    attachmentInfo.imageLayout = access.requiredLayout;
                    attachmentInfo.clearValue = {};
                    attachmentInfo.loadOp = access.loadOp;
                    attachmentInfo.storeOp = access.storeOp;
                    return attachmentInfo;
                }

                if (node.isDepthAttachment()) {
                    ASSERT(node.resolver, "Resolver is nullptr!");
                    experimental::ResourceHandle handle = node.resolve(rm, fm.getFrameIndex());
                    experimental::Image *image = rm.getResource<experimental::Image>(handle);
                    VkRenderingAttachmentInfo attachmentInfo = image->getRenderingAttachmentInfo();
                    attachmentInfo.loadOp = access.loadOp;
                    attachmentInfo.storeOp = access.storeOp;
                    return attachmentInfo;
                }
            }

            return std::nullopt;
        }


        RenderPassContext createRenderPassContext(RenderPassNode *pass, VkCommandBuffer commandBuffer) {
            RenderPassContext context{};

            // Fill the context with input resources
            for (auto &access: pass->inputs) {
                ASSERT(resources.find(access.resourceName) != resources.end(),
                       "Could not find the input");
                ResourceNode &resourceNode = resources[access.resourceName];
                // TODO automatically find out next render pass using this resource and set final layout of the attachment to the requiredLayout to save one barrier
                context.inputs.emplace(resourceNode.name, &resources[access.resourceName]);
            }

            // fill context with input resources
            for (auto &access: pass->outputs) {
                ASSERT(resources.find(access.resourceName) != resources.end(),
                       "Could not find the output");
                ResourceNode &resourceNode = resources[access.resourceName];
                context.outputs.emplace(resourceNode.name, &resources[access.resourceName]);
            }

            // fill context with descriptor sets
            for (const auto &desc: pass->descriptors) {
                context.descriptorSets.push_back(pass->resolveDescriptorSet(desc.first, fm.getFrameIndex()));
            }

            // Add the command buffer and frame index to the context
            context.commandBuffer = commandBuffer;
            context.frameIndex = fm.getFrameIndex();

            return context;
        }

        /**
         * Begins the rendering using the dynamic rendering extension
         * @param pass Current Render pass
         */
        void beginRendering(RenderPassNode *pass, VkCommandBuffer commandBuffer) {
            // Collect attachments
            std::vector<VkRenderingAttachmentInfo> colorAttachments = collectColorAttachmentInfos(pass);
            auto depthStencilOptional = collectDepthStencilAttachmentInfo(pass);

            VkRenderingInfo renderingInfo{VK_STRUCTURE_TYPE_RENDERING_INFO_KHR};
            renderingInfo.renderArea = {
                0, 0, fm.getSwapChain()->getSwapChainExtent().width,
                fm.getSwapChain()->getSwapChainExtent().height
            };
            renderingInfo.layerCount = 1;
            renderingInfo.colorAttachmentCount = colorAttachments.size();
            renderingInfo.pColorAttachments = colorAttachments.data();
            renderingInfo.pDepthAttachment = depthStencilOptional.has_value()
                                                 ? &depthStencilOptional.value()
                                                 : VK_NULL_HANDLE;
            renderingInfo.pStencilAttachment = VK_NULL_HANDLE;

            // Start a dynamic rendering
            vkCmdBeginRendering(commandBuffer, &renderingInfo);

            // Set viewport and scissors
            VkViewport viewport = Init::viewport(pass->viewport.X, pass->viewport.Y, pass->viewport.Z,
                                                 pass->viewport.W);
            if (pass->viewportSize == RelativeViewPortSize::SwapChainRelative) {
                viewport = Init::viewport(pass->viewport.X * fm.getSwapChain()->getSwapChainExtent().width,
                                          pass->viewport.Y * fm.getSwapChain()->getSwapChainExtent().height,
                                          pass->viewport.Z, pass->viewport.W);
            }
            VkRect2D scissor{
                {0, 0}, VkExtent2D{static_cast<uint32_t>(viewport.width), static_cast<uint32_t>(viewport.height)}
            };
            vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
        }

        /**
         * Ends the rendering for current renderpass
         */
        void endRendering(VkCommandBuffer commandBuffer) {
            vkCmdEndRendering(commandBuffer);
        }

        void recordRenderPass(RenderPassNode *pass, VkCommandBuffer commandBuffer) {
            if (pass->type == CommandQueueFamily::Graphics) {
                beginRendering(pass, commandBuffer);
            }
            // Create context
            RenderPassContext renderPassContext = createRenderPassContext(pass, commandBuffer);

            // Dispatch the render pass callback
            ASSERT(pass->executeFunc, "Execute function is not set!");
            pass->executeFunc(renderPassContext);

            if (pass->type == CommandQueueFamily::Graphics) {
                endRendering(commandBuffer);
            }
        }


        /**
         * Executes the graph and makes necessary barrier transitions.
         */
        void execute() {
            // Begin frame
            if (fm.beginFrame()) {
                // Iterate over the groups in order of execution
                for (const auto &groupWithIndex: allGroups) {
                    SubmissionGroup *group = groupWithIndex.group;
                    VkCommandBuffer commandBuffer = group->commandBuffers[fm.getFrameIndex()];

                    // Begin command buffer for the group
                    fm.beginCommandBuffer(commandBuffer);

                    // Exectue all passes from the group
                    for (auto &pass: group->renderPassNodes) {
                        // Apply required barriers
                        applyPipelineBarriers(pass, commandBuffer, PipelineBarrier::TransitionStage::RequiredLayout);

                        recordRenderPass(pass, commandBuffer);
                        // post rendering barrier
                        applyPipelineBarriers(pass, commandBuffer, PipelineBarrier::TransitionStage::FinalLayout);
                    }

                    // Submit command buffer
                    // If this is the last group, we submit the command buffer for present
                    if (group->globalStartIndex == allGroups.size() - 1) {
                        fm.submitPresentCommandBuffer(commandBuffer);
                        continue;
                    }

                    // If not, we submit command buffer to corresponding queue

                    if (group->queueFamily == CommandQueueFamily::Graphics) {
                        fm.submitCommandBuffer<CommandQueueFamily::Graphics>(
                            commandBuffer, {group->waitSemaphores[fm.getFrameIndex()]},
                            {group->signalSemaphores[fm.getFrameIndex()]}, group->waitStage);
                    }

                    if (group->queueFamily == CommandQueueFamily::Compute) {
                        fm.submitCommandBuffer<CommandQueueFamily::Compute>(
                            commandBuffer, {group->waitSemaphores[fm.getFrameIndex()]},
                            {group->signalSemaphores[fm.getFrameIndex()]}, group->waitStage);
                    }

                    if (group->queueFamily == CommandQueueFamily::Transfer) {
                        fm.submitCommandBuffer<CommandQueueFamily::Transfer>(
                            commandBuffer, {group->waitSemaphores[fm.getFrameIndex()]},
                            {group->signalSemaphores[fm.getFrameIndex()]}, group->waitStage);
                    }
                }

                fm.endFrame();
            }
        }
    };
};
