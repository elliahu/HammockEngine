#include "HmckHDR.h"



void Hmck::EnvironmentLoader::load(std::string filepath)
{
	stbi_set_flip_vertically_on_load(true);
	unsigned char* pixels = stbi_load(filepath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
	stbi_set_flip_vertically_on_load(false);
	if (!pixels)
	{
		throw std::runtime_error("Failed to load image from disk");
	}

	environment->evironmentMap = memory.createTexture2DFromBuffer({
		.buffer = pixels,
		.bufferSize = static_cast<uint32_t>(width * height * 4),
		.width = static_cast<uint32_t>(width), .height = static_cast<uint32_t>(height),
		.format = VK_FORMAT_R8G8B8A8_UNORM
		});


	// Free the memory allocated by stb_image
	stbi_image_free(pixels);
}

void Hmck::EnvironmentLoader::calculateIrradianceMap(unsigned char* buffer)
{
}

void Hmck::EnvironmentLoader::calculatePrefilterMap(unsigned char* buffer)
{
}

void Hmck::EnvironmentLoader::calculateBrdfLUT(unsigned char* buffer)
{
}

void Hmck::Environment::destroy(MemoryManager& memory)
{
	memory.destroyTexture2D(evironmentMap);
}
