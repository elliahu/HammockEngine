#pragma once
#include <vulkan/vulkan.h>
#include "HmckCamera.h"
#include "HmckEntity3D.h"


namespace Hmck
{
	struct SceneUbo
	{
		glm::mat4 projection{ 1.f };
		glm::mat4 view{ 1.f };
		glm::mat4 inverseView{ 1.f };
		glm::mat4 depthBiasMVP{ 1.f };
		glm::vec4 ambientLightColor = { 1.f, 1.f, 1.f, 0.005f }; // w is intensity
	};

	struct TransformUbo
	{
		glm::mat4 model{ 1.f };
		glm::mat4 normal{ 1.f };
	};

	struct MaterialPropertyUbo
	{
		glm::vec4 baseColorFactor;
		uint32_t baseColorTextureIndex;
		uint32_t normalTextureIndex;
		uint32_t metallicRoughnessTextureIndex;
		uint32_t occlusionTextureIndex;
		float alphaCutoff;
	};


	/*
		This information is passed to renderSystem every frame 
	*/
	struct FrameInfo
	{
		int frameIndex;
		float frameTime;
		VkCommandBuffer commandBuffer;
		Camera& camera;
		VkDescriptorSet globalDescriptorSet;
	};
}