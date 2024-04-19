#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <memory>
#include <vector>
#include <stdexcept>
#include <cassert>
#include <functional>

#include <HmckBuffer.h>
#include <HmckDescriptors.h>
#include <HmckTexture.h>

namespace Hmck
{
	typedef uint32_t UniformBufferHandle;
	typedef uint32_t DescriptorSetHandle;
	typedef uint32_t DescriptorSetLayoutHandle;
	typedef uint32_t Texture2DHandle;
	typedef uint32_t TextureCubeMapHandle;

	class MemoryManager
	{
	public:

		static const uint32_t INVALID_HANDLE;

		MemoryManager(Device& device);

		struct UniformBufferCreateInfo
		{
			VkDeviceSize instanceSize;
			uint32_t instanceCount;
			VkBufferUsageFlags usageFlags;
			VkMemoryPropertyFlags memoryPropertyFlags;
			VkDeviceSize minOffsetAlignment = 1;
		};

		UniformBufferHandle createUniformBuffer(UniformBufferCreateInfo createInfo);

		struct DescriptorSetLayoutCreateInfo
		{
			struct DescriptorSetLayoutBindingCreateInfo
			{
				uint32_t binding;
				VkDescriptorType descriptorType;
				VkShaderStageFlags stageFlags;
				uint32_t count = 1;
				VkDescriptorBindingFlags bindingFlags = 0;
			};

			std::vector<DescriptorSetLayoutBindingCreateInfo> bindings;
		};

		DescriptorSetLayoutHandle createDescriptorSetLayout(DescriptorSetLayoutCreateInfo createInfo);

		struct DescriptorSetCreateInfo
		{
			DescriptorSetLayoutHandle descriptorSetLayout;

			struct DescriptorSetWriteBufferInfo
			{
				uint32_t binding;
				VkDescriptorBufferInfo bufferInfo;
			};

			struct DescriptorSetWriteBufferArrayInfo
			{
				uint32_t binding;
				std::vector <VkDescriptorBufferInfo> bufferInfos;
			};

			struct DescriptorSetWriteImageInfo
			{
				uint32_t binding;
				VkDescriptorImageInfo imageInfo;
			};

			struct DescriptorSetWriteImageArrayInfo
			{
				uint32_t binding;
				std::vector<VkDescriptorImageInfo> imageInfos;
			};

			std::vector<DescriptorSetWriteBufferInfo> bufferWrites;
			std::vector<DescriptorSetWriteBufferArrayInfo> bufferArrayWrites;
			std::vector<DescriptorSetWriteImageInfo> imageWrites;
			std::vector<DescriptorSetWriteImageArrayInfo> imageArrayWrites;
		};

		DescriptorSetHandle createDescriptorSet(DescriptorSetCreateInfo createInfo);

		struct Texture2DCreateFromFileInfo
		{
			std::string filepath;
			VkFormat format;
			VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		};

		Texture2DHandle createTexture2DFromFile(Texture2DCreateFromFileInfo createInfo);

		struct Texture2DCreateFromBufferInfo
		{
			unsigned char* buffer;
			uint32_t bufferSize;
			uint32_t width;
			uint32_t height;
			VkFormat format;
			VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		};

		Texture2DHandle createTexture2DFromBuffer(Texture2DCreateFromBufferInfo createInfo);

		struct TextureCubeMapCreateFromFilesInfo
		{
			std::vector<std::string> filenames;
			VkFormat format;
			VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		};

		TextureCubeMapHandle createTextureCubeMapFromFiles(TextureCubeMapCreateFromFilesInfo createInfo);


		DescriptorSetLayout& getDescriptorSetLayout(DescriptorSetLayoutHandle handle); // TODO make it return pointer, do not dereference
		VkDescriptorSet getDescriptorSet(DescriptorSetHandle handle);
		std::unique_ptr<Buffer>& getUniformBuffer(UniformBufferHandle handle);
		std::unique_ptr<Texture2D>& getTexture2D(Texture2DHandle handle);
		VkDescriptorImageInfo getTexture2DDescriptorImageInfo(Texture2DHandle handle);
		std::unique_ptr<TextureCubeMap>& getTextureCubeMap(TextureCubeMapHandle handle);
		VkDescriptorImageInfo getTextureCubeMapDescriptorImageInfo(TextureCubeMapHandle handle);

		void bindDescriptorSet(
			VkCommandBuffer commandBuffer,
			VkPipelineBindPoint bindPoint,
			VkPipelineLayout pipelineLayout,
			uint32_t firstSet,
			uint32_t descriptorCount,
			DescriptorSetHandle descriptorSet,
			uint32_t dynamicOffsetCount,
			const uint32_t* pDynamicOffsets);

		void destroyUniformBuffer(UniformBufferHandle handle);
		void destroyDescriptorSetLayout(DescriptorSetLayoutHandle handle);
		void destroyTexture2D(Texture2DHandle handle);
		void destroyTextureCubeMap(TextureCubeMapHandle handle);


	private:
		Device& device;
		std::unique_ptr<DescriptorPool> descriptorPool;

		static std::unordered_map<UniformBufferHandle, std::unique_ptr<Buffer>> uniformBuffers;
		static std::unordered_map<DescriptorSetHandle, VkDescriptorSet> descriptorSets;
		static std::unordered_map<DescriptorSetLayoutHandle, std::unique_ptr<DescriptorSetLayout>> descriptorSetLayouts;
		static std::unordered_map<Texture2DHandle, std::unique_ptr<Texture2D>> texture2Ds;
		static std::unordered_map<Texture2DHandle, std::unique_ptr<TextureCubeMap>> textureCubeMaps;


		static uint32_t uniformBuffersLastHandle;
		static uint32_t descriptorSetsLastHandle;
		static uint32_t descriptorSetLayoutsLastHandle;
		static uint32_t texture2DsLastHandle;
		static uint32_t textureCubeMapsLastHandle;

	};
}
