#pragma once
#include "hammock/core/Types.h"

namespace hammock {
    namespace experimental {

        class Image : public Resource {
        protected:
            // Format and usage
            VkFormat m_format;
            VkImageUsageFlags m_usage;
            VkImageType m_type;
            VkImageViewType m_viewType;

            // Image dimensions
            uint32_t m_width, m_height, m_channels, m_depth = 1, m_layers = 1, m_mips = 1;

            // Vulkan handles
            VkImage m_image = VK_NULL_HANDLE;
            VkImageView m_view = VK_NULL_HANDLE;
            VkImageLayout m_layout = VK_IMAGE_LAYOUT_UNDEFINED;
            VmaAllocation m_allocation = VK_NULL_HANDLE;

            // Attachment
            VkClearValue clearValue = {};

            Image(Device &device, uint64_t id, const std::string &name, const ImageDesc &desc) : Resource(
                device, id, name) {
                // Fromat and usage
                m_format = desc.format;
                m_usage = desc.usage;
                m_type = desc.imageType;
                m_viewType = desc.imageViewType;

                // Image dimensions
                m_width = desc.width;
                m_height = desc.height;
                m_channels = desc.channels;
                m_depth = desc.depth;
                m_layers = desc.layers;
                m_mips = desc.mips;

                // Vulkan handles are created in load function on demand

                // attachment
                clearValue = desc.clearValue;

                // Check for support
                VkFormatProperties formatProperties;
                vkGetPhysicalDeviceFormatProperties(device.getPhysicalDevice(), m_format, &formatProperties);

                ASSERT(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_TRANSFER_DST_BIT,
                       "Device does not support flag TRANSFER_DST for selected format!");

                if (m_type == VK_IMAGE_TYPE_3D) {
                    uint32_t maxImageDimension3D(device.properties.limits.maxImageDimension3D);
                    ASSERT(
                        m_width <= maxImageDimension3D && m_height <= maxImageDimension3D && m_depth <=
                        maxImageDimension3D,
                        "Requested 3D texture dimensions are greater than supported 3D texture dimension!");
                }
            }

        public:
            /**
             * Creates the resource on device. This is called when ever this resource is requested and is not resident
             */
            void load() override {
                // Create the image
                VkImageCreateInfo imageCreateInfo = Init::imageCreateInfo();
                imageCreateInfo.imageType = m_type;
                imageCreateInfo.format = m_format;
                imageCreateInfo.mipLevels = m_mips;
                imageCreateInfo.arrayLayers = m_layers;
                imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
                imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
                imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                imageCreateInfo.extent.width = m_width;
                imageCreateInfo.extent.height = m_height;
                imageCreateInfo.extent.depth = m_depth;
                imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                imageCreateInfo.usage = m_usage;

                VmaAllocationCreateInfo allocInfo = {};
                allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

                checkResult(vmaCreateImage(device.allocator(), &imageCreateInfo, &allocInfo, &m_image, &m_allocation,
                                           nullptr));

                // create image view
                VkImageViewCreateInfo viewInfo{};
                viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                viewInfo.image = m_image;
                viewInfo.viewType = m_viewType;
                viewInfo.format = m_format;
                viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                viewInfo.subresourceRange.baseMipLevel = 0;
                viewInfo.subresourceRange.levelCount = m_mips;
                viewInfo.subresourceRange.baseArrayLayer = 0;
                viewInfo.subresourceRange.layerCount = m_layers;

                checkResult(vkCreateImageView(device.device(), &viewInfo, nullptr, &m_view));

                // generate mips if needed
                if (m_mips > 1) {
                    generateMips();
                }

                resident = true;
            }

            /**
             * Clears the resource from device memory. This is called if the resource is being destroyed ot space is needed in device memory.
             */
            void unload() override {
                if (m_image != VK_NULL_HANDLE) {
                    vmaDestroyImage(device.allocator(), m_image, m_allocation);
                }

                if (m_view != VK_NULL_HANDLE) {
                    vkDestroyImageView(device.device(), m_view, nullptr);
                }
                resident = false;
            }

            /**
             * Generates mip map chain for this image
             */
            void generateMips() {
                VkCommandBuffer commandBuffer = device.beginSingleTimeCommands();

                setImageLayout(commandBuffer, m_image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, {
                                   .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                   .baseMipLevel = 0,
                                   .levelCount = m_mips,
                                   .layerCount = 1
                               });

                VkImageMemoryBarrier barrier{};
                barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                barrier.image = m_image;
                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                barrier.subresourceRange.baseArrayLayer = 0;
                barrier.subresourceRange.layerCount = 1;
                barrier.subresourceRange.levelCount = 1;

                auto mipWidth = static_cast<int32_t>(m_width);
                auto mipHeight = static_cast<int32_t>(m_height);

                for (uint32_t i = 1; i < m_mips; i++) {
                    barrier.subresourceRange.baseMipLevel = i - 1;
                    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

                    vkCmdPipelineBarrier(commandBuffer,
                                         VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                                         0, nullptr,
                                         0, nullptr,
                                         1, &barrier);

                    VkImageBlit blit{};
                    blit.srcOffsets[0] = {0, 0, 0};
                    blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
                    blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    blit.srcSubresource.mipLevel = i - 1;
                    blit.srcSubresource.baseArrayLayer = 0;
                    blit.srcSubresource.layerCount = 1;
                    blit.dstOffsets[0] = {0, 0, 0};
                    blit.dstOffsets[1] = {mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1};
                    blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    blit.dstSubresource.mipLevel = i;
                    blit.dstSubresource.baseArrayLayer = 0;
                    blit.dstSubresource.layerCount = 1;

                    vkCmdBlitImage(commandBuffer,
                                   m_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                   m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                   1, &blit,
                                   VK_FILTER_LINEAR);

                    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                    vkCmdPipelineBarrier(commandBuffer,
                                         VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                                         0, nullptr,
                                         0, nullptr,
                                         1, &barrier);

                    if (mipWidth > 1) mipWidth /= 2;
                    if (mipHeight > 1) mipHeight /= 2;
                }

                barrier.subresourceRange.baseMipLevel = m_mips - 1;
                barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                m_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                vkCmdPipelineBarrier(commandBuffer,
                                     VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                                     0, nullptr,
                                     0, nullptr,
                                     1, &barrier);

                device.endSingleTimeCommands(commandBuffer);
                vkQueueWaitIdle(device.graphicsQueue());
            }
        };

        template<>
       struct ResourceTypeTraits<Image> {
            static constexpr ResourceType type = ResourceType::Image;
        };
    }
}
