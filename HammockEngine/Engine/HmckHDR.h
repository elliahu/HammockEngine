#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>

#include <string>

#include "HmckTexture.h"
#include "HmckDevice.h"
#include "HmckMemory.h"

namespace Hmck
{
	struct Environment
	{
		Texture2DHandle evironmentMap;
		Texture2DHandle irradianceMap;
		Texture2DHandle prefilterMap;
		Texture2DHandle brdfLUT;

		void destroy(MemoryManager& memory);
	};

	class EnvironmentLoader
	{
	public:

		enum class MapUsage
		{
			Envirinment, Prefiltered, Irradiance, BRDFLUT
		};
		
		EnvironmentLoader(Device& device, MemoryManager& memory, std::shared_ptr<Environment> environment) :device{ device }, memory{ memory }, environment { environment } {}

		void load(std::string filepath);
		void loadHDR(std::string filepath, MapUsage mapUsage);

	private:
		Device& device;
		MemoryManager& memory;
		std::shared_ptr<Environment> environment;
		int width, height, channels;

		void calculateIrradianceMap(float* buffer);
		void calculatePrefilterMap(float* buffer);
		void calculateBrdfLUT(float* buffer);
	};

	
}
