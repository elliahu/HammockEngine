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

void Hmck::EnvironmentLoader::loadHDR(std::string filepath, MapUsage mapUsage)
{
	stbi_set_flip_vertically_on_load(true);
	float* pixels = stbi_loadf(filepath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
	stbi_set_flip_vertically_on_load(false);
	if (!pixels)
	{
		throw std::runtime_error("Failed to load image from disk");
	}

	channels = 4;

    if (mapUsage == MapUsage::Envirinment)
    {
        environment->evironmentMap = memory.createHDRTexture2DFromBuffer({
                .buffer = pixels,
                .bufferSize = static_cast<uint32_t>(width * height * channels),
                .width = static_cast<uint32_t>(width), .height = static_cast<uint32_t>(height),
                .format = VK_FORMAT_R32G32B32A32_SFLOAT
            });
    }

    else if (mapUsage == MapUsage::Prefiltered)
    {
        environment->prefilterMap = memory.createHDRTexture2DFromBuffer({
                .buffer = pixels,
                .bufferSize = static_cast<uint32_t>(width * height * channels),
                .width = static_cast<uint32_t>(width), .height = static_cast<uint32_t>(height),
                .format = VK_FORMAT_R32G32B32A32_SFLOAT
            });
    }

    else if (mapUsage == MapUsage::Irradiance)
    {
        environment->irradianceMap = memory.createHDRTexture2DFromBuffer({
                .buffer = pixels,
                .bufferSize = static_cast<uint32_t>(width * height * channels),
                .width = static_cast<uint32_t>(width), .height = static_cast<uint32_t>(height),
                .format = VK_FORMAT_R32G32B32A32_SFLOAT
            });
    }

    else if (mapUsage == MapUsage::BRDFLUT)
    {
        environment->brdfLUT = memory.createHDRTexture2DFromBuffer({
                .buffer = pixels,
                .bufferSize = static_cast<uint32_t>(width * height * channels),
                .width = static_cast<uint32_t>(width), .height = static_cast<uint32_t>(height),
                .format = VK_FORMAT_R32G32B32A32_SFLOAT
            });
    }

	// Free the memory allocated by stb_image
	stbi_image_free(pixels);
}

void Hmck::EnvironmentLoader::calculateIrradianceMap(float * buffer)
{
    // Assuming some constants for irradiance map resolution and PI
    const int irradianceMapResolution = 64;
    const float pi = 3.14159265359f;

    // Allocate memory for the irradiance map buffer
    size_t bufferSize = irradianceMapResolution * irradianceMapResolution * channels * sizeof(float);
    float * irradianceBuffer = new float[bufferSize];

    // Iterate over each pixel of the irradiance map
    for (int y = 0; y < irradianceMapResolution; ++y) {
        for (int x = 0; x < irradianceMapResolution; ++x) {
            // Calculate the direction of the current pixel on the irradiance map
            // These directions will be used to shoot rays towards the environment map
            float u = (2.0f * x) / irradianceMapResolution - 1.0f;
            float v = (2.0f * y) / irradianceMapResolution - 1.0f;
            float phi = u * pi;
            float theta = v * (pi / 2.0f);

            // Calculate the direction vector in spherical coordinates
            // This direction vector will be used to sample the environment map
            float dirX = std::cos(phi) * std::sin(theta);
            float dirY = std::cos(theta);
            float dirZ = std::sin(phi) * std::sin(theta);

            // Accumulate incoming radiance by shooting a ray towards the environment map
            // For simplicity, let's assume the environment map is a simple 2D array of pixels
            float irradianceSum[3] = { 0.0f };
            for (int i = 0; i < width; ++i) {
                for (int j = 0; j < height; ++j) {
                    // Sample the environment map at the given direction
                    // For each pixel in the environment map, calculate its contribution to the current pixel on the irradiance map
                    // This could involve various techniques like importance sampling or Monte Carlo integration
                    // Here, we simply sample the environment map directly
                    float* pixel = reinterpret_cast<float*>(buffer) + (j * width + i) * channels;
                    irradianceSum[0] += pixel[0];
                    irradianceSum[1] += pixel[1];
                    irradianceSum[2] += pixel[2];
                }
            }

            // Average the accumulated radiance
            int numSamples = width * height;
            float irradiance[3] = {
                irradianceSum[0] / numSamples,
                irradianceSum[1] / numSamples,
                irradianceSum[2] / numSamples
            };

            // Assign the computed irradiance to the current pixel of the irradiance map
            // Assuming a simple linear layout of the irradiance map for simplicity
            int index = (y * irradianceMapResolution + x) * 3;
            irradianceBuffer[index] = irradiance[0];
            irradianceBuffer[index + 1] = irradiance[1];
            irradianceBuffer[index + 2] = irradiance[2];
        }
    }

    environment->irradianceMap = memory.createHDRTexture2DFromBuffer({
        .buffer = irradianceBuffer,
        .bufferSize = static_cast<uint32_t>(irradianceMapResolution * irradianceMapResolution * channels),
        .width = static_cast<uint32_t>(irradianceMapResolution), .height = static_cast<uint32_t>(irradianceMapResolution),
        .format = VK_FORMAT_R32G32B32A32_SFLOAT
        });

    // Don't forget to deallocate memory for the irradiance map buffer
    delete[] irradianceBuffer;
}

void Hmck::EnvironmentLoader::calculatePrefilterMap(float* buffer)
{
	// TOD
}

void Hmck::EnvironmentLoader::calculateBrdfLUT(float* buffer)
{
	// TODO
}

void Hmck::Environment::destroy(MemoryManager& memory)
{
    if(evironmentMap > 0)
	    memory.destroyTexture2D(evironmentMap);
    if (prefilterMap > 0)
        memory.destroyTexture2D(prefilterMap);
    if (irradianceMap > 0)
        memory.destroyTexture2D(irradianceMap);
    if (brdfLUT > 0)
        memory.destroyTexture2D(brdfLUT);
}
