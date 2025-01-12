#include "hammock/core/Image.h"

VkImageType hammock::rendergraph::mapImageTypeToVulkanImageType(ImageType imageType) {
    switch (imageType) {
        case ImageType::Image1D: {
            return VK_IMAGE_TYPE_1D;
        }
        case ImageType::Image2D: {
            return VK_IMAGE_TYPE_2D;
        }
        case ImageType::Image3D: {
            return VK_IMAGE_TYPE_3D;
        }
        case ImageType::ImageCube: {
            return VK_IMAGE_TYPE_2D;
        }
    }

    return VK_IMAGE_TYPE_2D;
}

VkImageViewType hammock::rendergraph::mapImageTypeToVulkanImageViewType(ImageType imageType) {
    switch (imageType) {
        case ImageType::Image1D: {
            return VK_IMAGE_VIEW_TYPE_1D;
        }
        case ImageType::Image2D: {
            return VK_IMAGE_VIEW_TYPE_2D;
        }
        case ImageType::Image3D: {
            return VK_IMAGE_VIEW_TYPE_3D;
        }
        case ImageType::ImageCube: {
            return VK_IMAGE_VIEW_TYPE_CUBE;
        }
    }

    return VK_IMAGE_VIEW_TYPE_2D;
}

void hammock::rendergraph::Image::generateMips() {
    // we need to generate the mips
        VkCommandBuffer commandBuffer = device.beginSingleTimeCommands();

        setImageLayout(commandBuffer, image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, {
                           .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                           .baseMipLevel = 0,
                           .levelCount = desc.mips,
                           .layerCount = 1
                       });

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.image = image;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.subresourceRange.levelCount = 1;

        int32_t mipWidth = desc.width;
        int32_t mipHeight = desc.height;

        for (uint32_t i = 1; i < desc.mips; i++) {
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
                           image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
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

        barrier.subresourceRange.baseMipLevel = desc.mips - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                             0, nullptr,
                             0, nullptr,
                             1, &barrier);

        device.endSingleTimeCommands(commandBuffer);
        vkQueueWaitIdle(device.graphicsQueue());
}

/**
 * Copies a content of a staging buffer (that has to be host visible and host coherent) into the image memory. Image has to be in Undefined layout.
 * @param buffer Staging buffer, Host visible, host coherent
 * @param finalLayout Layout into which the image is then transitioned
 */
void hammock::rendergraph::Image::loadFromBuffer(Buffer &buffer, VkImageLayout finalLayout) {
    assert(layout == VK_IMAGE_LAYOUT_UNDEFINED && "Layout has to be VK_IMAGE_LAYOUT_UNDEFINED!");
    transitionLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    device.copyBufferToImage(
        buffer.getBuffer(),
        image,
        desc.width, desc.height,
        desc.layers, 0, desc.depth
    );
    transitionLayout(finalLayout);
}

/**
 * Transitions image to specified layout.
 * @param finalLayout Layout to which the image is transitioned
 */
void hammock::rendergraph::Image::transitionLayout(VkImageLayout finalLayout) {
    device.transitionImageLayout(
        image,
        layout,
        finalLayout
    );
    layout = finalLayout;
}


void hammock::rendergraph::Image::load() {
    // Create the VkImage
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = mapImageTypeToVulkanImageType(desc.imageType);
    imageInfo.extent.width = desc.width;
    imageInfo.extent.height = desc.height;
    imageInfo.extent.depth = desc.depth;
    imageInfo.mipLevels = desc.mips;
    imageInfo.arrayLayers = desc.layers;
    imageInfo.format = desc.format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = desc.usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.flags = 0;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

    // Allocate memory
    checkResult(vmaCreateImage(device.allocator(), &imageInfo, &allocInfo, &image, &allocation, nullptr));

    // create image view
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = mapImageTypeToVulkanImageViewType(desc.imageType);
    viewInfo.format = desc.format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = desc.mips;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = desc.layers;

    checkResult(vkCreateImageView(device.device(), &viewInfo, nullptr, &view));

    resident = true;

    bool generateMipmaps = desc.mips > 1;

    if (generateMipmaps) {
        generateMipmaps();
    }
}

void hammock::rendergraph::Image::unload() {
    // Clean up Vulkan resources
    vmaDestroyImage(device.allocator(), image, allocation);
    vkDestroyImageView(device.device(), view, nullptr);
    resident = false;
}


void hammock::rendergraph::SampledImage::load() {
    // Create the VkImage
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = mapImageTypeToVulkanImageType(desc.imageType);
    imageInfo.extent.width = desc.width;
    imageInfo.extent.height = desc.height;
    imageInfo.extent.depth = desc.depth;
    imageInfo.mipLevels = desc.mips;
    imageInfo.arrayLayers = desc.layers;
    imageInfo.format = desc.format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = desc.usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.flags = 0;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

    // Allocate memory
    checkResult(vmaCreateImage(device.allocator(), &imageInfo, &allocInfo, &image, &allocation, nullptr));

    // create image view
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = mapImageTypeToVulkanImageViewType(desc.imageType);
    viewInfo.format = desc.format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = desc.mips;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = desc.layers;

    checkResult(vkCreateImageView(device.device(), &viewInfo, nullptr, &view));

    // Create sampler
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = desc.filter;
    samplerInfo.minFilter = desc.filter;
    samplerInfo.addressModeU = desc.addressMode;
    samplerInfo.addressModeV = desc.addressMode;
    samplerInfo.addressModeW = desc.addressMode;
    samplerInfo.anisotropyEnable = VK_TRUE;

    // retrieve max anisotropy from physical device
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(device.getPhysicalDevice(), &properties);

    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = desc.borderColor;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = desc.mipmapMode;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = desc.mips;

    checkResult(vkCreateSampler(device.device(), &samplerInfo, nullptr, &sampler));

    resident = true;

    bool generateMipmaps = desc.mips > 1;

    if (generateMipmaps) {
        generateMipmaps();
    }
}

void hammock::rendergraph::SampledImage::unload() {
    // Clean up Vulkan resources
    vmaDestroyImage(device.allocator(), image, allocation);
    vkDestroyImageView(device.device(), view, nullptr);
    vkDestroySampler(device.device(), sampler, nullptr);
    resident = false;
}
