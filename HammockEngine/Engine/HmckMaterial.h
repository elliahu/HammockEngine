#pragma once

#include <vulkan/vulkan.h>
#include <memory>
#include <glm/glm.hpp>

#include "HmckDevice.h"
#include "HmckBuffer.h"
#include "HmckDescriptors.h"
#include "HmckTexture.h"

namespace Hmck
{
	struct HmckCreateMaterialInfo 
	{
		std::string color{};
		std::string normal{};
		std::string roughnessMetalness{};
	};

	/*
		HmckMaterial class consists of logic surrounding creation and usage
		of material in the HammockEngine
	*/
	class HmckMaterial
	{
	public:
		HmckMaterial(HmckDevice& device);
		~HmckMaterial();

		// delete copy constructor and copy destructor
		HmckMaterial(const HmckMaterial&) = delete;
		HmckMaterial& operator=(const HmckMaterial&) = delete;

		static std::unique_ptr<HmckMaterial> createMaterial(HmckDevice& hmckDevice, HmckCreateMaterialInfo& materialInfo);
		void destroy();

		std::unique_ptr<HmckTexture2D> color;
		std::unique_ptr<HmckTexture2D> normal;
		std::unique_ptr<HmckTexture2D> roughnessMetalness;
	private:
		void createMaterial(HmckCreateMaterialInfo& materialInfo);

		// TODO think about removing device reference here as it is not really needed
		// most of the function require device as argument anyway
		HmckDevice& hmckDevice;
		
	};
}