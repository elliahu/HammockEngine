#pragma once
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "HmckEntity.h"
#include "HmckFramebuffer.h"
#include "HmckPipeline.h"
#include "HmckMemory.h"

namespace Hmck
{
	class ILight : public Entity
	{
	public:
		ILight() : Entity() {};
		glm::vec3 color{ 1.f,1.f,1.f };
	};

	class OmniLight :public ILight
	{
	public:
		OmniLight() : ILight() {};
		~OmniLight(){}

		float zNear{ 0.1f };
		float zfar{ 1024.0f };
		const uint32_t shadowCubeMapFaceSize{ 1024 };
		const VkFormat shadwoCubeMapFaceImageFormat{ VK_FORMAT_R32_SFLOAT };

		ITexture shadowCubeMap;
		std::array<VkImageView, 6> shadowCubeMapFaceImageViews{};

		// Framebuffer for offscreen rendering
		struct FrameBufferAttachment {
			VkImage image;
			VkDeviceMemory mem;
			VkImageView view;
		};
		struct OffscreenPass {
			int32_t width, height;
			std::array<VkFramebuffer, 6> frameBuffers;
			FrameBufferAttachment depth;
			VkRenderPass renderPass;
			VkSampler sampler;
			VkDescriptorImageInfo descriptor;
		} renderPass;

		static inline std::unique_ptr<GraphicsPipeline> pipeline{};

		static inline DescriptorSetHandle descriptorSet;
		static inline DescriptorSetLayoutHandle descriptorSetLayout;
		static inline BufferHandle buffer;

		struct PushData 
		{
			glm::mat4 view;
			glm::vec4 lightPosition;
		};

		struct UniformData
		{
			glm::mat4 projection{ 1.0f };
			glm::mat4 model{ 1.0f };
		};

		void prepareRenderPass(Device& device);
		void prepareFrameBuffer(Device& device);
		void prepareCubeMap(Device& device);
		void prepareDescriptor(MemoryManager& memory);
		void preparePipeline(Device& device, MemoryManager& memory);
		void beginCubeMapFaceUpdate(uint32_t faceIndex, VkCommandBuffer commandBuffer);
		void endCubeMapFaceUpdate(VkCommandBuffer commandBuffer);
	};

}

