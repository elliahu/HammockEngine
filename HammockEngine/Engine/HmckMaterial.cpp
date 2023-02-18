#include "HmckMaterial.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#ifndef MATERIALS_DIR
#define MATERIALS_DIR "../../Resources/Materials/"
#endif // !MATERIALS_DIR

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

	VkImageCreateInfo defaultImageInfo{};
	defaultImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	defaultImageInfo.imageType = VK_IMAGE_TYPE_2D;
	defaultImageInfo.extent.width = static_cast<uint32_t>(1024);
	defaultImageInfo.extent.height = static_cast<uint32_t>(1024);
	defaultImageInfo.extent.depth = 1;
	defaultImageInfo.mipLevels = 1;
	defaultImageInfo.arrayLayers = 1;
	defaultImageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
	defaultImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	defaultImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	defaultImageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	defaultImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	defaultImageInfo.flags = 0; // Optional

	// TODO make this load once and reuse
	HmckCreateMaterialInfo defaultInfo{
		std::string(MATERIALS_DIR) + "empty_white.jpg", // color
		std::string(MATERIALS_DIR) + "empty_normal.jpg", // normal
		std::string(MATERIALS_DIR) + "empty_black.jpg", // roughness
		std::string(MATERIALS_DIR) + "empty_black.jpg", // metalness
		std::string(MATERIALS_DIR) + "empty_white.jpg", // ao
		std::string(MATERIALS_DIR) + "empty_white.jpg" // displacement
	};
	

	color = std::make_unique<HmckTexture>();
	color->image.loadImage(materialInfo.color.length() != 0 ? materialInfo.color : defaultInfo.color, hmckDevice);
	color->image.createImageView(hmckDevice);
	color->createSampler(hmckDevice);

	// normal
	normal = std::make_unique<HmckTexture>();
	normal->image.loadImage(materialInfo.normal.length() != 0 ? materialInfo.normal : defaultInfo.normal, hmckDevice, VK_FORMAT_R8G8B8A8_UNORM); // !!!
	normal->image.createImageView(hmckDevice, VK_FORMAT_R8G8B8A8_UNORM);
	normal->createSampler(hmckDevice);

	// roughness
	roughness = std::make_unique<HmckTexture>();
	roughness->image.loadImage(materialInfo.roughness.length() != 0 ? materialInfo.roughness: defaultInfo.roughness, hmckDevice);
	roughness->image.createImageView(hmckDevice);
	roughness->createSampler(hmckDevice);

	// metalness
	metalness = std::make_unique<HmckTexture>();
	metalness->image.loadImage(materialInfo.metalness.length() != 0 ? materialInfo.metalness: defaultInfo.metalness, hmckDevice);
	metalness->image.createImageView(hmckDevice);
	metalness->createSampler(hmckDevice);

	// ambient occlusion
	ambientOcclusion = std::make_unique<HmckTexture>();
	ambientOcclusion->image.loadImage(materialInfo.ambientOcclusion.length() != 0 ? materialInfo.ambientOcclusion: defaultInfo.ambientOcclusion, hmckDevice);
	ambientOcclusion->image.createImageView(hmckDevice);
	ambientOcclusion->createSampler(hmckDevice);

	// displacement
	displacement = std::make_unique<HmckTexture>();
	displacement->image.loadImage(materialInfo.displacement.length() != 0 ? materialInfo.displacement: defaultInfo.displacement, hmckDevice);
	displacement->image.createImageView(hmckDevice);
	displacement->createSampler(hmckDevice);
}

Hmck::HmckMaterial::~HmckMaterial()
{
	destroy();
}

void Hmck::HmckMaterial::destroy()
{
	if (color != nullptr)
	{
		color->image.destroyImage(hmckDevice);
		color->destroySampler(hmckDevice);
	}
	
	if (normal != nullptr)
	{
		normal->image.destroyImage(hmckDevice);
		normal->destroySampler(hmckDevice);
	}
	
	if (roughness != nullptr)
	{
		roughness->image.destroyImage(hmckDevice);
		roughness->destroySampler(hmckDevice);
	}
	
	if (metalness != nullptr)
	{
		metalness->image.destroyImage(hmckDevice);
		metalness->destroySampler(hmckDevice);
	}
	
	if (ambientOcclusion != nullptr)
	{
		ambientOcclusion->image.destroyImage(hmckDevice);
		ambientOcclusion->destroySampler(hmckDevice);
	}
	
	if (displacement != nullptr)
	{
		displacement->image.destroyImage(hmckDevice);
		displacement->destroySampler(hmckDevice);
	}
}

void Hmck::HmckTexture::destroySampler(HmckDevice& hmckDevice)
{
	vkDestroySampler(hmckDevice.device(), sampler, nullptr);
	sampler = VK_NULL_HANDLE;
}

void Hmck::HmckImage::loadImage(
	std::string& filepath, 
	HmckDevice& hmckDevice,
	VkFormat format,
	bool flip
)
{
	int imgWidth = 0, imgHeight = 0, imgChannels = 0;

	stbi_set_flip_vertically_on_load(flip);
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
	imageInfo.format = format;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.flags = 0; // Optional

	hmckDevice.createImageWithInfo(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, imageMemory);

	// copy data from staging buffer to VkImage
	hmckDevice.transitionImageLayout(
		image,
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


void Hmck::HmckImage::clearImage(
	HmckDevice& hmckDevice,
	VkClearColorValue clearColor,
	VkFormat format)
{
	assert(image != VK_NULL_HANDLE && "Cannot clear uninitialized image!");

	hmckDevice.transitionImageLayout(
		image,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
	);

	VkCommandBuffer commandBuffer = hmckDevice.beginSingleTimeCommands();

	VkImageSubresourceRange imageSubresourceRange;
	imageSubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageSubresourceRange.baseMipLevel = 0;
	imageSubresourceRange.levelCount = 1;
	imageSubresourceRange.baseArrayLayer = 0;
	imageSubresourceRange.layerCount = 1;

	vkCmdClearColorImage(
		commandBuffer,
		image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		&clearColor,
		1,
		&imageSubresourceRange
	);

	hmckDevice.transitionImageLayout(
		image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	);

	hmckDevice.endSingleTimeCommands(commandBuffer);
}

void Hmck::HmckTexture::createDepthSampler(HmckDevice& hmckDevice)
{
	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_NEAREST; // TODO https://github.com/SaschaWillems/Vulkan/blob/master/examples/shadowmapping/shadowmapping.cpp#L237
	samplerInfo.minFilter = VK_FILTER_NEAREST; // TODO above
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = 1.0f;
	samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 1.0f;

	if (vkCreateSampler(hmckDevice.device(), &samplerInfo, nullptr, &sampler) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create depth sampler!");
	}
}
