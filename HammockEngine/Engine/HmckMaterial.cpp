#include "HmckMaterial.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

Hmck::HmckMaterial::HmckMaterial(HmckDevice& device): hmckDevice{device} {}

std::unique_ptr<Hmck::HmckMaterial> Hmck::HmckMaterial::createMaterial(HmckDevice& hmckDevice, HmckCreateMaterialInfo& materialInfo)
{
	std::unique_ptr<HmckMaterial> material = std::make_unique<HmckMaterial>(hmckDevice);
	material->createMaterial(materialInfo);
	return material;
}

void Hmck::HmckMaterial::createMaterial(HmckCreateMaterialInfo& materialInfo)
{
	// TODO check if paths are provided
	// TODO load default value if not
	// color
	color = std::make_unique<HmckTexture>();
	color->image.loadImage(materialInfo.color, hmckDevice);
	color->image.createImageView(hmckDevice, VK_FORMAT_R8G8B8A8_SRGB);
	color->createSampler(hmckDevice);

	// normal
	normal = std::make_unique<HmckTexture>();
	normal->image.loadImage(materialInfo.normal, hmckDevice);
	normal->image.createImageView(hmckDevice, VK_FORMAT_R8G8B8A8_SRGB);
	normal->createSampler(hmckDevice);

	// roughness
	roughness = std::make_unique<HmckTexture>();
	roughness->image.loadImage(materialInfo.roughness, hmckDevice);
	roughness->image.createImageView(hmckDevice, VK_FORMAT_R8G8B8A8_SRGB);
	roughness->createSampler(hmckDevice);
}

void Hmck::HmckMaterial::destroy()
{
	// color
	color->image.destroyImage(hmckDevice);
	color->destroySampler(hmckDevice);

	// color
	normal->image.destroyImage(hmckDevice);
	normal->destroySampler(hmckDevice);

	// roughness
	roughness->image.destroyImage(hmckDevice);
	roughness->destroySampler(hmckDevice);
}

void Hmck::HmckTexture::destroySampler(HmckDevice& hmckDevice)
{
	vkDestroySampler(hmckDevice.device(), sampler, nullptr);
	sampler = VK_NULL_HANDLE;
}

void Hmck::HmckImage::loadImage(
	std::string& filepath, 
	HmckDevice& hmckDevice,  
	bool flip
)
{
	int imgWidth = 0, imgHeight = 0, imgChannels = 0;

	stbi_uc* pixels = stbi_load(filepath.c_str(), &imgWidth, &imgHeight, &imgChannels, STBI_rgb_alpha);

	if (!pixels)
	{
		throw std::runtime_error("Could not load image from disk");
	}

	VkDeviceSize imageSize = static_cast<VkDeviceSize>(imgWidth) * imgHeight * 4;

	// create staging buffer and copy image data to the staging buffer

	HmckBuffer stagingBuffer
	{
		hmckDevice,
		sizeof(pixels[0]),
		static_cast<uint32_t>(imgWidth * imgHeight * 4),
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	};

	stagingBuffer.map();
	stagingBuffer.writeToBuffer((void*)pixels);

	stbi_image_free(pixels);

	// create VkImage

	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = static_cast<uint32_t>(imgWidth);
	imageInfo.extent.height = static_cast<uint32_t>(imgHeight);
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.flags = 0; // Optional

	hmckDevice.createImageWithInfo(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, imageMemory);

	// copy data from staging buffer to VkImage
	hmckDevice.transitionImageLayout(
		image,
		VK_FORMAT_R8G8B8A8_SRGB,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
	);
	hmckDevice.copyBufferToImage(
		stagingBuffer.getBuffer(),
		image, static_cast<uint32_t>(imgWidth),
		static_cast<uint32_t>(imgHeight),
		1
	);
	hmckDevice.transitionImageLayout(
		image,
		VK_FORMAT_R8G8B8A8_SRGB,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	);
}

void Hmck::HmckImage::createImageView(HmckDevice& hmckDevice, VkFormat format)
{
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

	if (vkCreateImageView(hmckDevice.device(), &viewInfo, nullptr, &imageView) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create image view!");
	}
}

void Hmck::HmckTexture::createSampler(HmckDevice& hmckDevice)
{
	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
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

void Hmck::HmckImage::destroyImage(HmckDevice& hmckDevice)
{
	vkDestroyImageView(hmckDevice.device(), imageView, nullptr);
	vkDestroyImage(hmckDevice.device(), image, nullptr);
	vkFreeMemory(hmckDevice.device(), imageMemory, nullptr);
}
