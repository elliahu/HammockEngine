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

	channels = 4;

	environment->evironmentMap = memory.createTexture2DFromBuffer({
		.buffer = pixels,
		.bufferSize = static_cast<uint32_t>(width * height * channels),
		.width = static_cast<uint32_t>(width), .height = static_cast<uint32_t>(height),
		.format = VK_FORMAT_R8G8B8A8_UNORM
		});


	// Free the memory allocated by stb_image
	stbi_image_free(pixels);
}

void Hmck::EnvironmentLoader::loadHDR(std::string filepath)
{
	stbi_set_flip_vertically_on_load(true);
	float* pixels = stbi_loadf(filepath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
	stbi_set_flip_vertically_on_load(false);
	if (!pixels)
	{
		throw std::runtime_error("Failed to load image from disk");
	}

	channels = 4;

	environment->evironmentMap = memory.createHDRTexture2DFromBuffer({
		.buffer = pixels,
		.bufferSize = static_cast<uint32_t>(width * height * channels),
		.width = static_cast<uint32_t>(width), .height = static_cast<uint32_t>(height),
		.format = VK_FORMAT_R32G32B32A32_SFLOAT
		});

	// Free the memory allocated by stb_image
	stbi_image_free(pixels);
}

void Hmck::EnvironmentLoader::calculateIrradianceMap(unsigned char* buffer)
{
	// TODO implement the irradiance map calculation
	// buffer is an enviroment map
	// create a new buffer that will be irradiance map
}

void Hmck::EnvironmentLoader::calculatePrefilterMap(unsigned char* buffer)
{
	// TOD
}

void Hmck::EnvironmentLoader::calculateBrdfLUT(unsigned char* buffer)
{
	// TODO
}

void Hmck::Environment::destroy(MemoryManager& memory)
{
	memory.destroyTexture2D(evironmentMap);
}
