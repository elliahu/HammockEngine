#pragma once

#include "HmckCamera.h"

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
	};
}