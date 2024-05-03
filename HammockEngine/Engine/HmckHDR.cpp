#include "HmckHDR.h"

void Hmck::EnvironmentLoader::load(std::string filepath, Texture2DHandle& texture, VkFormat format)
{
	stbi_set_flip_vertically_on_load(true);
	unsigned char* pixels = stbi_load(filepath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
	stbi_set_flip_vertically_on_load(false);
	if (!pixels)
	{
		throw std::runtime_error("Failed to load image from disk");
	}

	channels = 4;

    texture = memory.createTexture2DFromBuffer({
		.buffer = pixels,
		.bufferSize = static_cast<uint32_t>(width * height * channels),
		.width = static_cast<uint32_t>(width), .height = static_cast<uint32_t>(height),
		.format = format
		});


	// Free the memory allocated by stb_image
	stbi_image_free(pixels);
}

void Hmck::EnvironmentLoader::loadHDR(std::string filepath, Texture2DHandle& texture, VkFormat format)
{
	stbi_set_flip_vertically_on_load(true);
	float* pixels = stbi_loadf(filepath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
	stbi_set_flip_vertically_on_load(false);
	if (!pixels)
	{
		throw std::runtime_error("Failed to load image from disk");
	}

	channels = 4;

	texture = memory.createHDRTexture2DFromBuffer({
		.buffer = pixels,
		.bufferSize = static_cast<uint32_t>(width * height * channels),
		.width = static_cast<uint32_t>(width), .height = static_cast<uint32_t>(height),
		.format = format
	});

	// Free the memory allocated by stb_image
	stbi_image_free(pixels);
}

void Hmck::Environment::generateBRDFLUT(Device& device, MemoryManager& memory)
{
    VkImageCreateInfo imageCI = Init::imageCreateInfo();
    imageCI.imageType = VK_IMAGE_TYPE_2D;
    imageCI.format = brdfLUTformat;
    imageCI.extent.width = brdfLUTdim;
    imageCI.extent.height = brdfLUTdim;
    imageCI.extent.depth = 1;
    imageCI.mipLevels = 1;
    imageCI.arrayLayers = 1;
    imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCI.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

    //checkResult(vkCreateImage(device.device(), &imageCI, nullptr, &textures.lutBrdf.image))
}

void Hmck::Environment::destroy(MemoryManager& memory)
{
    if(evironmentMap > 0)
	    memory.destroyTexture2D(evironmentMap);
    if (prefilteredMap > 0)
        memory.destroyTexture2D(prefilteredMap);
    if (irradianceMap > 0)
        memory.destroyTexture2D(irradianceMap);
    if (brdfLUT > 0)
        memory.destroyTexture2D(brdfLUT);
}
