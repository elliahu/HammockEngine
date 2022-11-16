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
		VkSampler imageSampler;

		void loadImage(
			std::string& filepath, 
			HmckDevice& hmckDevice,
			bool flip = false
		);

		void createImageView(HmckDevice& hmckDevice, VkFormat format);

		void createImageSampler(HmckDevice& hmckDevice);

		void destroy(HmckDevice& hmckDevice);
	};

	struct HmckTexture
	{
		// format: VK_FORMAT_R8G8B8A8_SRGB
		HmckImage image{};
	};

	class HmckMaterial
	{
	public:
		HmckMaterial(HmckDevice& device);
		// TODO properly dispose used resources
		// For each image
		// 1. Destroy imageView
		// 2. Destroy image
		// 3. Destroy imageMemory
		// 4. Destroy imageSampler
		// Currently throw validation erros when closing the app
		~HmckMaterial() {};

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