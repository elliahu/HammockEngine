#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>
#include <chrono>
#include <string>

#include "HmckTexture.h"
#include "HmckDevice.h"
#include "HmckMemory.h"
#include "HmckPipeline.h"
#include "HmckFramebuffer.h"
#include "Utils/HmckLogger.h"

namespace Hmck
{
	class Environment
	{
	public:
		// environment map
		Texture2DHandle environmentSphere;

		// irradiance map
		Texture2DHandle irradianceSphere;
		void generateIrradianceSphere(Device& device, MemoryManager& memory, VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT);

		// BRDFLUT
		Texture2DHandle brdfLUT;
		void generateBRDFLUT(Device& device, MemoryManager& memory, uint32_t dim = 512, VkFormat format = VK_FORMAT_R16G16_SFLOAT);

		void destroy(MemoryManager& memory);

	private:
	
	};

	class EnvironmentLoader
	{
	public:
		// TODO totaly change this haha
		EnvironmentLoader(Device& device, MemoryManager& memory) :device{ device }, memory{ memory } {}

		
		void load(std::string filepath, Texture2DHandle& texture, VkFormat format);
		void loadHDR(std::string filepath, Texture2DHandle& texture, VkFormat format);

	private:
		Device& device;
		MemoryManager& memory;
		int width, height, channels;
	};
}
