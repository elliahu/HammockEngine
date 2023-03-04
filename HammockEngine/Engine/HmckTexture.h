#pragma once

#include <vulkan/vulkan.h>
#include <memory>
#include <stb_image.h>

#include "HmckDevice.h"
#include "HmckBuffer.h"

namespace Hmck
{
	/*
		HmckTexture represents a texture
		that can be used to sample from in a shader
	*/
	class HmckTexture
	{
	public:
		// Recommended:
		// VK_FORMAT_R8G8B8A8_UNORM for normals
		// VK_FORMAT_R8G8B8A8_SRGB for images
		VkSampler sampler;
		VkDeviceMemory memory;
		VkImage image;
		VkImageView view;
		VkImageLayout layout;
		int width, height, channels;
		VkDescriptorImageInfo descriptor;

		void updateDescriptor();
	};


	class HmckTexture2D : public HmckTexture
	{
	public:
		void loadFromFile(
			std::string& filepath,
			HmckDevice& hmckDevice,
			VkFormat format,
			VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		);

		void loadFromBuffer(
			unsigned char* buffer,
			uint32_t bufferSize,
			uint32_t width, uint32_t height,
			HmckDevice& hmckDevice,
			VkFormat format,
			VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		);

		void createSampler(HmckDevice& hmckDevice);
		void destroy(HmckDevice& hmckDevice);
	};
}
