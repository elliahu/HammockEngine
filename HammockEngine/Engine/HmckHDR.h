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
		Texture2DHandle evironmentMap;

		// irradiance map
		Texture2DHandle irradianceMap;

		// prefiltered map
		Texture2DHandle prefilteredMap;

		// BRDFLUT
		Texture2DHandle brdfLUT;
		void generateBRDFLUT(Device& device, MemoryManager& memory);

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
