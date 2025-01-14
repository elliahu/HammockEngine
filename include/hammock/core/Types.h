#pragma once
#include <variant>

namespace hammock {
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
}
