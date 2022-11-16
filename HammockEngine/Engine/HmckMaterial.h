#pragma once

#include <vulkan/vulkan.h>
#include <memory>

#include "HmckDevice.h"
#include "HmckBuffer.h"

namespace Hmck
{
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
	};

	struct HmckTexture
	{
		// format: VK_FORMAT_R8G8B8A8_SRGB
		HmckImage texture;
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
		~HmckMaterial() {};

		// delete copy constructor and copy destructor
		HmckMaterial(const HmckMaterial&) = delete;
		HmckMaterial& operator=(const HmckMaterial&) = delete;

		std::unique_ptr<HmckTexture> texture{};

	private:
		HmckDevice& hmckDevice;
	};
}