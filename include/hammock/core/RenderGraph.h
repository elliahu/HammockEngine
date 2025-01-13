#pragma once

#include <hammock/core/Resource.h>
#include <hammock/core/Device.h>
#include <hammock/core/SwapChain.h>
#include <hammock/core/CoreUtils.h>
#include <cassert>
#include <variant>


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
            BufferDesc desc;
        };

        /**
        * Describes general image.
        */
        struct ImageDesc {
            uint32_t width, height, channels = 4, depth = 1, layers = 1, mips = 1;
            VkFormat format;
            VkImageUsageFlags usage;
            VkImageType imageType;
            VkImageViewType imageViewType;
            VkImageUsageFlags imageUsageFlags;
            VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_STORE;
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
            ImageDesc desc;
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
                SwapChain // Special type of image. If resource is SwapChain, it is the final output resource that gets presented.
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
            std::vector<ResourceRef> refs;
        };

        /**
         * Describes access intentions of a resource. Resource is references by its name from a resource map.
         */
        struct ResourceAccess {
            std::string resourceName;
            VkImageLayout requiredLayout;
            VkAccessFlags accessFlags;
            VkPipelineStageFlags stageFlags;
        };


        /**
         * RenderGraph node representing a render pass
         */
        struct RenderPassNode {
            enum class Type {
                Graphics, // Represents a pass that draws into some image
                Compute, // Represents a compute pass
                Transfer // Represents a transfer pass - data is moved from one location to another (eg. host -> device or device -> host)
            } type; // Type of the pass

            std::string name; // Name of the pass for debug purposes and lookup
            std::vector<ResourceAccess> inputs; // Read accesses
            std::vector<ResourceAccess> outputs; // Write accesses

            // callback for rendering
            std::function<void(VkCommandBuffer, uint32_t)> executeFunc;
        };


        /**
         * Describes image barrier that will transform an image from one layout to another
         */
        struct ImageBarrier {
            std::string resourceName;
            VkImageLayout oldLayout;
            VkImageLayout newLayout;
            VkAccessFlags srcAccessFlags;
            VkAccessFlags dstAccessFlags;
            VkPipelineStageFlags srcStageFlags;
            VkPipelineStageFlags dstStageFlags;
        };

        /**
         * RenderGraph is a class representing a directed acyclic graph (DAG) that describes the process of creation of a frame.
         * It consists of resource nodes and render pass nodes. Render pass node can be dependent on some resources and
         * can itself create resources that other passes may depend on. RenderGraph analyzes these dependencies, makes adjustments when possible,
         * makes necessary transitions between resource states.
         *  TODO support for Read Modify Write - render pass writes and reads from the same resource
         */
        class RenderGraph {
            Device &device;
            uint32_t maxFramesInFlight; // Maximal numer of frames that GPU works on concurrently
            std::unordered_map<std::string, ResourceNode> resources; // Holds all the resources
            std::vector<RenderPassNode> passes; // Holds all the render passes
            uint32_t rootPass = 0; // Pass from which the rendering starts (top of the graph)

            /**
             * Creates a resource for specific node
             * @param resourceNode Node for which to create a resource
             */
            void createResource(ResourceNode &resourceNode, std::variant<BufferDesc, ImageDesc> descVariant);

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
            void createImage(ResourceRef &resourceRef, ImageDesc &desc) const;

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
            RenderGraph(Device &device, uint32_t maxFramesInFlight): device(device),
                                                                     maxFramesInFlight(maxFramesInFlight) {
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
            }


            /**
             * Executes the graph and makes necessary barrier transitions.
             * @param cmd CommandBuffer to which the render calls will be recorded
             * @param frameIndex Index of the current frame.This is used to identify which resource ref should be used of the resource node is buffered.
             */
            void execute(VkCommandBuffer cmd, uint32_t frameIndex) {

            }
        };
    }
};
