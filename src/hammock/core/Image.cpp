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
    imageInfo.initialLayout = desc.layout;
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
    imageInfo.initialLayout = desc.layout;
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
}

void hammock::rendergraph::SampledImage::unload() {
    // Clean up Vulkan resources
    vmaDestroyImage(device.allocator(), image, allocation);
    vkDestroyImageView(device.device(), view, nullptr);
    vkDestroySampler(device.device(), sampler, nullptr);
    resident = false;
}
