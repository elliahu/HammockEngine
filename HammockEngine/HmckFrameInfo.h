#pragma once

#include "HmckCamera.h"
#include "HmckGameObject.h"

#include <vulkan/vulkan.h>

namespace Hmck
{
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