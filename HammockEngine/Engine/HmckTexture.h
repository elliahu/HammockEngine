#pragma once

#include <vulkan/vulkan.h>
#include <memory>
#include <stb_image.h>

#include "HmckDevice.h"
#include "HmckBuffer.h"

namespace Hmck
{
	/*
		Texture represents a texture
		that can be used to sample from in a shader
	*/
	class Texture
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


	class Texture2D : public Texture
	{
	public:
		void loadFromFile(
			std::string& filepath,
			Device& hmckDevice,
			VkFormat format,
			VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		);

		void loadFromBuffer(
			unsigned char* buffer,
			uint32_t bufferSize,
			uint32_t width, uint32_t height,
			Device& hmckDevice,
			VkFormat format,
			VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		);

		void createSampler(Device& hmckDevice, VkFilter filter = VK_FILTER_LINEAR);
		void destroy(Device& hmckDevice);
	};

	class TextureCubeMap : public Texture
	{
	public:
		void loadFromFile(
			std::string filename,
			VkFormat format,
			Device& device,
			VkQueue copyQueue,
			VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
			VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	};
}
