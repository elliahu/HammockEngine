#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include <functional>
#include <fstream>
#include <iostream>
#include <string>
#include <random>
#include <cmath>

#include "HmckDescriptors.h"
#include "HmckTexture.h"

namespace Hmck
{
	// dark magic from: https://stackoverflow.com/a/57595105
	template <typename T, typename... Rest>
	void hashCombine(std::size_t& seed, const T& v, const Rest&... rest) {
		seed ^= std::hash<T>{}(v)+0x9e3779b9 + (seed << 6) + (seed >> 2);
		(hashCombine(seed, rest), ...);
	};

	inline void checkResult(VkResult result)
	{
		assert(result == VK_SUCCESS && "Failed to check result. Result is not VK_SUCCESS!");
	}

	namespace Init 
	{
		inline VkImageCreateInfo imageCreateInfo()
		{
			VkImageCreateInfo imageCreateInfo{};
			imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			return imageCreateInfo;
		}

		inline VkMemoryAllocateInfo memoryAllocateInfo()
		{
			VkMemoryAllocateInfo memAllocInfo{};
			memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			return memAllocInfo;
		}

		inline VkImageViewCreateInfo imageViewCreateInfo()
		{
			VkImageViewCreateInfo imageViewCreateInfo{};
			imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			return imageViewCreateInfo;
		}

		inline VkSamplerCreateInfo samplerCreateInfo()
		{
			VkSamplerCreateInfo samplerCreateInfo{};
			samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			samplerCreateInfo.maxAnisotropy = 1.0f;
			return samplerCreateInfo;
		}

		inline VkFramebufferCreateInfo framebufferCreateInfo()
		{
			VkFramebufferCreateInfo framebufferCreateInfo{};
			framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			return framebufferCreateInfo;
		}

		inline VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState(
			VkColorComponentFlags colorWriteMask,
			VkBool32 blendEnable)
		{
			VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState{};
			pipelineColorBlendAttachmentState.colorWriteMask = colorWriteMask;
			pipelineColorBlendAttachmentState.blendEnable = blendEnable;
			return pipelineColorBlendAttachmentState;
		}

		inline VkViewport viewport(float width, float height, float minDepth, float maxDepth)
		{
			VkViewport viewport{};
			viewport.width = width;
			viewport.height = height;
			viewport.minDepth = minDepth;
			viewport.maxDepth = maxDepth;
			return viewport;
		}

		inline VkRect2D rect2D(int32_t width, int32_t height, int32_t offsetX, int32_t offsetY)
		{
			VkRect2D rect2D{};
			rect2D.extent.width = width;
			rect2D.extent.height = height;
			rect2D.offset.x = offsetX;
			rect2D.offset.y = offsetY;
			return rect2D;
		}

		/** @brief Initialize a map entry for a shader specialization constant */
		inline VkSpecializationMapEntry specializationMapEntry(uint32_t constantID, uint32_t offset, size_t size)
		{
			VkSpecializationMapEntry specializationMapEntry{};
			specializationMapEntry.constantID = constantID;
			specializationMapEntry.offset = offset;
			specializationMapEntry.size = size;
			return specializationMapEntry;
		}

		/** @brief Initialize a specialization constant info structure to pass to a shader stage */
		inline VkSpecializationInfo specializationInfo(uint32_t mapEntryCount, const VkSpecializationMapEntry* mapEntries, size_t dataSize, const void* data)
		{
			VkSpecializationInfo specializationInfo{};
			specializationInfo.mapEntryCount = mapEntryCount;
			specializationInfo.pMapEntries = mapEntries;
			specializationInfo.dataSize = dataSize;
			specializationInfo.pData = data;
			return specializationInfo;
		}

		/** @brief Initialize a specialization constant info structure to pass to a shader stage */
		inline VkSpecializationInfo specializationInfo(const std::vector<VkSpecializationMapEntry>& mapEntries, size_t dataSize, const void* data)
		{
			VkSpecializationInfo specializationInfo{};
			specializationInfo.mapEntryCount = static_cast<uint32_t>(mapEntries.size());
			specializationInfo.pMapEntries = mapEntries.data();
			specializationInfo.dataSize = dataSize;
			specializationInfo.pData = data;
			return specializationInfo;
		}

	} // namespace Init

	namespace Filesystem
	{
		inline bool fileExists(const std::string& filename)
		{
			std::ifstream f(filename.c_str());
			return !f.fail();
		}

		inline std::vector<char> readFile(const std::string& filePath)
		{
			std::ifstream file{ filePath, std::ios::ate | std::ios::binary };

			if (!file.is_open())
			{
				throw std::runtime_error("failed to open file: " + filePath);
			}

			size_t fileSize = static_cast<size_t>(file.tellg());
			std::vector<char> buffer(fileSize);

			file.seekg(0);
			file.read(buffer.data(), fileSize);

			file.close();
			return buffer;
		}

	} // namespace Filesystem

	namespace Math
	{
		inline float lerp(float a, float b, float f)
		{
			return a + f * (b - a);
		}

		inline glm::mat4 normal(glm::mat4& model)
		{
			return glm::transpose(glm::inverse(model));
		}

		inline glm::vec3 decomposeTranslation(glm::mat4& transform)
		{
			return transform[3];
		}

		inline glm::vec3 decomposeRotation(glm::mat4& transform)
		{
			glm::vec3 scale, translation, skew;
			glm::vec4 perspective;
			glm::quat orientation;
			glm::decompose(transform, scale, orientation, translation, skew, perspective);
			return glm::eulerAngles(orientation);
		}

		inline glm::vec3 decomposeScale(glm::mat4& transform)
		{
			return {
				glm::length(transform[0]),
				glm::length(transform[1]),
				glm::length(transform[2])
			};
		}

		inline uint32_t padSizeToMinAlignment(uint32_t originalSize, uint32_t minAlignment) {
			return (originalSize + minAlignment - 1) & ~(minAlignment - 1);
		}
	}

	namespace Rnd 
	{
		inline std::unique_ptr<Buffer> createSSAOKernel(Device& device, uint32_t kernelSize) 
		{
			std::default_random_engine rndEngine((unsigned)time(nullptr));
			std::uniform_real_distribution<float> rndDist(0.0f, 1.0f);

			std::vector<glm::vec4> ssaoKernel{ kernelSize };
			std::unique_ptr<Buffer> ssaoKernelBuffer{};

			for (uint32_t i = 0; i < kernelSize; ++i)
			{
				glm::vec3 sample(rndDist(rndEngine) * 2.0 - 1.0, rndDist(rndEngine) * 2.0 - 1.0, rndDist(rndEngine));
				sample = glm::normalize(sample);
				sample *= rndDist(rndEngine);
				float scale = float(i) / float(kernelSize);
				scale = Hmck::Math::lerp(0.1f, 1.0f, scale * scale);
				ssaoKernel[i] = glm::vec4(sample * scale, 0.0f);
			}

			ssaoKernelBuffer = std::make_unique<Buffer>(
				device,
				ssaoKernel.size() * sizeof(glm::vec4),
				1,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
				);
			ssaoKernelBuffer->map();
			ssaoKernelBuffer->writeToBuffer(ssaoKernel.data());
			return ssaoKernelBuffer;
		}

		
	}
} // namespace Hmck
