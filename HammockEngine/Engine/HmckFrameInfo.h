#pragma once

#include "HmckCamera.h"
#include "HmckGameObject.h"

#include <vulkan/vulkan.h>

namespace Hmck
{
#define MAX_LIGHTS 10

	struct HmckPointLight
	{
		glm::vec4 position{}; // ingnore w
		glm::vec4 color{}; // w is intensity
	};

	struct HmckDirectionalLight
	{
		glm::vec4 direction{}; // w is ignored
		glm::vec4 color{}; // w is intensity
	};

	struct HmckGlobalUbo
	{
		glm::mat4 projection{ 1.f };
		glm::mat4 view{ 1.f };
		glm::mat4 inverseView{ 1.f };
		glm::vec4 ambientLightColor = { 1.f, 1.f, 1.f, 0.07f }; // w is intensity
		HmckDirectionalLight directionalLight;
		HmckPointLight pointLights[MAX_LIGHTS];
		int numLights;
	};

	struct HmckFrameInfo
	{
		int frameIndex;
		float frameTime;
		VkCommandBuffer commandBuffer;
		HmckCamera& camera;
		VkDescriptorSet globalDescriptorSet;
		HmckGameObject::Map& gameObjects;
	};
}