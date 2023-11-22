#pragma once
#include <vulkan/vulkan.h>
#include "HmckCamera.h"
#include "HmckEntity3D.h"


namespace Hmck
{
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