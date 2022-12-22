#pragma once

#include "HmckCamera.h"
#include "HmckGameObject.h"

#include <vulkan/vulkan.h>

namespace Hmck
{
#define MAX_LIGHTS 10

	/*
		HmckPointLight struct defines how 
		point light is composed in shader
	*/
	struct HmckPointLight
	{
		glm::vec4 position{}; // ingnore w
		glm::vec4 color{}; // w is intensity
		// x = quadratic term
		// y = linear term
		// z = constant term
		glm::vec4 lightTerms{}; // w is ignored
	};

	/*
		HmckDirectionalLight represents single
		light source that is infinitely far away (sun)
	*/
	struct HmckDirectionalLight
	{
		glm::vec4 direction{}; // w is ignored
		glm::vec4 color{}; // w is intensity
	};

	/*
		This is the format of the data sent to the UBO
	*/
	struct HmckGlobalUbo
	{
		glm::mat4 projection{ 1.f };
		glm::mat4 view{ 1.f };
		glm::mat4 inverseView{ 1.f };
		glm::vec4 ambientLightColor = { 1.f, 1.f, 1.f, 0.01f }; // w is intensity
		HmckDirectionalLight directionalLight;
		HmckPointLight pointLights[MAX_LIGHTS];
		int numLights;
	};

	/*
		This information is passed to renderSystem every frame 
	*/
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