#pragma once

#include "Platforms/HmckWindow.h"
#include "HmckDevice.h"
#include "HmckSwapChain.h"
#include "Systems/HmckUISystem.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <memory>
#include <vector>
#include <stdexcept>
#include <cassert>

#ifndef SHADOWMAP_RES
#define SHADOWMAP_RES 2048
#endif // !SHADOWMAP_RES

#ifndef DEPTH_FORMAT
#define DEPTH_FORMAT VK_FORMAT_D16_UNORM
#endif // !DEPTH_FORMAT

#define HMCK_CLEAR_COLOR { 0.f,0.f,0.f,1.f }

namespace Hmck
{
	class HmckRenderer
	{
	public:

		HmckRenderer(HmckWindow& window, HmckDevice& device);
		~HmckRenderer();

		// delete copy constructor and copy destructor
		HmckRenderer(const HmckRenderer&) = delete;
		HmckRenderer& operator=(const HmckRenderer&) = delete;

		VkRenderPass getSwapChainRenderPass() const { return hmckSwapChain->getRenderPass(); }
		float getAspectRatio() const { return hmckSwapChain->extentAspectRatio(); }
		bool isFrameInProgress() const { return isFrameStarted; }

		VkCommandBuffer getCurrentCommandBuffer() const
		{
			assert(isFrameStarted && "Cannot get command buffer when frame not in progress");
			return commandBuffers[getFrameIndex()];
		}

		int getFrameIndex() const
		{
			assert(isFrameStarted && "Cannot get frame index when frame not in progress");
			return currentFrameIndex;
		}

		VkCommandBuffer beginFrame();
		void endFrame();
		void beginOffscreenRenderPass(VkCommandBuffer commandBuffer);
		void endOffscreenRenderPass(VkCommandBuffer commandBuffer);
		void beginSwapChainRenderPass(VkCommandBuffer commandBuffer);
		void endSwapChainRenderPass(VkCommandBuffer commandBuffer);


	private:
		void createCommandBuffer();
		void freeCommandBuffers();
		void recreateSwapChain();

		HmckWindow& hmckWindow;
		HmckDevice& hmckDevice;
		std::unique_ptr<HmckSwapChain> hmckSwapChain;
		std::vector<VkCommandBuffer> commandBuffers;

		uint32_t currentImageIndex;
		int currentFrameIndex{ 0 };
		bool isFrameStarted{false};
	};

}
