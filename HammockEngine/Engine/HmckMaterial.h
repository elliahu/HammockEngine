#pragma once

#include <vulkan/vulkan.h>
#include <memory>

#include "HmckDevice.h"
#include "HmckBuffer.h"

namespace Hmck
{
	struct HmckCreateMaterialInfo 
	{
		std::string texture{};
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

	struct HmckTexture
	{
		// format: VK_FORMAT_R8G8B8A8_SRGB
		HmckImage image{};
		static VkSampler sampler;

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

		std::unique_ptr<HmckTexture> texture;

		void destroy();

	private:
		void createMaterial(HmckCreateMaterialInfo& materialInfo);

		HmckDevice& hmckDevice;
	};
}