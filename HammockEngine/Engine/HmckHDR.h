#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>

#include <string>

#include "HmckTexture.h"
#include "HmckDevice.h"
#include "HmckMemory.h"
#include "HmckPipeline.h"
#include "HmckFramebuffer.h"

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

		// BRDFLUT
		const VkFormat brdfLUTformat = VK_FORMAT_R16G16_SFLOAT;	// R16G16 is supported pretty much everywhere
		const int32_t brdfLUTdim = 512;
		std::unique_ptr<GraphicsPipeline> brdfLUTPipeline{}; 
		std::unique_ptr<Framebuffer> brdfLUTframebuffer{};

		
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
