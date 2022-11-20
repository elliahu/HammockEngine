#pragma once

#include <vulkan/vulkan.h>
#include <memory>

#include "HmckDevice.h"
#include "HmckBuffer.h"
#include "HmckDescriptors.h"

namespace Hmck
{
	struct HmckCreateMaterialInfo 
	{
		std::string color{};
		std::string normal{};
		std::string roughness{};
		std::string ambientOcclusion{};
	};

	struct HmckImage
	{
		VkDeviceMemory imageMemory;
		VkImage image;
		VkImageView imageView;

		void loadImage(
			std::string& filepath, 
			HmckDevice& hmckDevice,
			bool flip = false
		);

		void createImageView(HmckDevice& hmckDevice, VkFormat format);
		void destroyImage(HmckDevice& hmckDevice);
	};

	struct HmckMaterialDescriptorSetLayoutInfo
	{
		uint32_t binding;
		VkDescriptorType type;
		VkShaderStageFlags shaderStageFlags;
		uint32_t count;
	};

	struct HmckTexture
	{
		// format: VK_FORMAT_R8G8B8A8_SRGB
		HmckImage image{};
		VkSampler sampler;

		void createSampler(HmckDevice& hmckDevice);
		void destroySampler(HmckDevice& hmckDevice);
	};

	class HmckMaterial
	{
	public:
		HmckMaterial(HmckDevice& device);

		// delete copy constructor and copy destructor
		HmckMaterial(const HmckMaterial&) = delete;
		HmckMaterial& operator=(const HmckMaterial&) = delete;

		static std::unique_ptr<HmckMaterial> createMaterial(HmckDevice& hmckDevice, HmckCreateMaterialInfo& materialInfo);
		void destroy();

		std::unique_ptr<HmckTexture> color;
		std::unique_ptr<HmckTexture> normal;
		std::unique_ptr<HmckTexture> roughness;
		std::unique_ptr<HmckTexture> ambientOcclusion;
	private:
		void createMaterial(HmckCreateMaterialInfo& materialInfo);

		// TODO think about removing device reference here as it is not really needed
		// most of the function require device as argument anyway
		HmckDevice& hmckDevice;
		
	};
}