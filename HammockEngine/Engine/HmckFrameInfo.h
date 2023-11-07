#pragma once

#include "HmckCamera.h"

#include <vulkan/vulkan.h>

namespace Hmck
{
#define MAX_LIGHTS 10

	/*
		PointLight struct defines how 
		point light is composed in shader
	*/
	struct PointLight
	{
		glm::vec4 position{}; // ingnore w
		glm::vec4 color{}; // w is intensity
		// x = quadratic term
		// y = linear term
		// z = constant term
		glm::vec4 lightTerms{}; // w is ignored
	};

	/*
		DirectionalLight represents single
		light source that is infinitely far away (sun)
	*/
	struct DirectionalLight
	{
		glm::vec4 direction{}; // w is fov
		glm::vec4 color{}; // w is intensity
	};

	/*
		This is the format of the data sent to the UBO
	*/
	struct GlobalUbo
	{
		glm::mat4 projection{ 1.f };
		glm::mat4 view{ 1.f };
		glm::mat4 inverseView{ 1.f };
		glm::mat4 depthBiasMVP{ 1.f };
		glm::vec4 ambientLightColor = { 1.f, 1.f, 1.f, 0.005f }; // w is intensity
		DirectionalLight directionalLight;
		PointLight pointLights[MAX_LIGHTS];
		int numLights;
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