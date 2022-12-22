#pragma once

#include <vulkan/vulkan.h>
#include <memory>
#include <glm/glm.hpp>

#include "HmckDevice.h"
#include "HmckBuffer.h"
#include "HmckDescriptors.h"

namespace Hmck
{
	/*
		This struct is used to describe the material that is gonna be loaded
		specially tha paths to each material map
	*/
	struct HmckCreateMaterialInfo 
	{
		std::string color{};
		std::string normal{};
		std::string roughness{};
		std::string metalness{};
		std::string ambientOcclusion{};
		std::string displacement{};
	};

	/*
		This struct represents general purpose image on GPU
		Does not have a sampler tho
	*/
	struct HmckImage
	{
		VkDeviceMemory imageMemory;
		VkImage image;
		VkImageView imageView;

		// Recommended:
		// VK_FORMAT_R8G8B8A8_UNORM for normals
		// VK_FORMAT_R8G8B8A8_SRGB for images

		void loadImage(
			std::string& filepath, 
			HmckDevice& hmckDevice,
			VkFormat format = VK_FORMAT_R8G8B8A8_SRGB,
			bool flip = true
		);

		void createImageView(HmckDevice& hmckDevice, VkFormat format = VK_FORMAT_R8G8B8A8_SRGB);
		void destroyImage(HmckDevice& hmckDevice);
		void clearImage(HmckDevice& hmckDevice, VkClearColorValue clearColor = { 1.0f, 1.0f, 1.0f, 1.0f }, VkFormat format = VK_FORMAT_R8G8B8A8_SRGB);
	};


	/*
		HmckTexture struct represents a texture or a map
		that can be used to sample from in a shader
	*/
	struct HmckTexture
	{
		// format: VK_FORMAT_R8G8B8A8_SRGB
		HmckImage image{};
		VkSampler sampler;

		void createSampler(HmckDevice& hmckDevice);
		void destroySampler(HmckDevice& hmckDevice);
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

		std::unique_ptr<HmckTexture> color;
		std::unique_ptr<HmckTexture> normal;
		std::unique_ptr<HmckTexture> roughness;
		std::unique_ptr<HmckTexture> metalness;
		std::unique_ptr<HmckTexture> ambientOcclusion;
		std::unique_ptr<HmckTexture> displacement;
	private:
		void createMaterial(HmckCreateMaterialInfo& materialInfo);

		// TODO think about removing device reference here as it is not really needed
		// most of the function require device as argument anyway
		HmckDevice& hmckDevice;
		
	};
}