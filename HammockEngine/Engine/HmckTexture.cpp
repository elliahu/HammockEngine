#include "HmckTexture.h"
#include <stb_image.h>

void Hmck::HmckTexture2D::destroy(HmckDevice& hmckDevice)
{
	// if image have sampler, destroy it
	if (sampler != VK_NULL_HANDLE)
	{
		vkDestroySampler(hmckDevice.device(), sampler, nullptr);
		sampler = VK_NULL_HANDLE;
	}

	vkDestroyImageView(hmckDevice.device(), view, nullptr);
	vkDestroyImage(hmckDevice.device(), image, nullptr);
	vkFreeMemory(hmckDevice.device(), memory, nullptr);
}

void Hmck::HmckTexture2D::loadFromFile(
	std::string& filepath,
	HmckDevice& hmckDevice,
	VkFormat format,
	VkImageLayout imageLayout
)
{
	stbi_set_flip_vertically_on_load(true);
	stbi_uc* pixels = stbi_load(filepath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
	stbi_set_flip_vertically_on_load(false);
	if (!pixels)
	{
		throw std::runtime_error("Could not load image from disk");
	}

	VkDeviceSize imageSize = static_cast<VkDeviceSize>(width) * height * 4;

	// create staging buffer and copy image data to the staging buffer

	HmckBuffer stagingBuffer
	{
		hmckDevice,
		sizeof(pixels[0]),
		static_cast<uint32_t>(width * height * 4),
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	};

	stagingBuffer.map();
	stagingBuffer.writeToBuffer((void*)pixels);

	stbi_image_free(pixels);

	// create VkImage
	format = format;

	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = static_cast<uint32_t>(width);
	imageInfo.extent.height = static_cast<uint32_t>(height);
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.flags = 0; // Optional

	hmckDevice.createImageWithInfo(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, memory);

	// copy data from staging buffer to VkImage
	hmckDevice.transitionImageLayout(
		image,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
	);
	hmckDevice.copyBufferToImage(
		stagingBuffer.getBuffer(),
		image, static_cast<uint32_t>(width),
		static_cast<uint32_t>(height),
		1
	);
	hmckDevice.transitionImageLayout(
		image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		imageLayout
	);
	layout = imageLayout;

	// create image view
	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	if (vkCreateImageView(hmckDevice.device(), &viewInfo, nullptr, &view) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create image view!");
	}
}

void Hmck::HmckTexture2D::loadFromBuffer(unsigned char* buffer, uint32_t bufferSize, uint32_t width, uint32_t height, HmckDevice& hmckDevice, VkFormat format, VkImageLayout imageLayout)
{
	HmckBuffer stagingBuffer
	{
		hmckDevice,
		sizeof(buffer[0]),
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	};

	stagingBuffer.map();
	stagingBuffer.writeToBuffer(buffer);

	// create VkImage
	format = format;

	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = static_cast<uint32_t>(width);
	imageInfo.extent.height = static_cast<uint32_t>(height);
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.flags = 0; // Optional

	hmckDevice.createImageWithInfo(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, memory);

	// copy data from staging buffer to VkImage
	hmckDevice.transitionImageLayout(
		image,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
	);
	hmckDevice.copyBufferToImage(
		stagingBuffer.getBuffer(),
		image, static_cast<uint32_t>(width),
		static_cast<uint32_t>(height),
		1
	);
	hmckDevice.transitionImageLayout(
		image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		imageLayout
	);
	layout = imageLayout;

	// create image view
	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	if (vkCreateImageView(hmckDevice.device(), &viewInfo, nullptr, &view) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create image view!");
	}
}

void Hmck::HmckTexture2D::createSampler(HmckDevice& hmckDevice, VkFilter filter)
{
	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = filter;
	samplerInfo.minFilter = filter;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_TRUE;

	// retrieve max anisotropy from physical device
	VkPhysicalDeviceProperties properties{};
	vkGetPhysicalDeviceProperties(hmckDevice.getPhysicalDevice(), &properties);

	samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	if (vkCreateSampler(hmckDevice.device(), &samplerInfo, nullptr, &sampler) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create image sampler!");
	}
}

void Hmck::HmckTexture::updateDescriptor()
{
	descriptor.sampler = sampler;
	descriptor.imageView = view;
	descriptor.imageLayout = layout;
}
