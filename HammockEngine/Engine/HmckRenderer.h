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

// black clear color
#define HMCK_CLEAR_COLOR { 0.f,0.f,0.f,1.f }

constexpr auto OFFSCREEN_RES_WIDTH = 2048;
constexpr auto OFFSCREEN_RES_HEIGHT = 2048;

namespace Hmck
{
	class HmckRenderer
	{

		struct FrameBufferAttachment {
			VkImage image;
			VkDeviceMemory mem;
			VkImageView view;
		};

		struct OffscreenRenderPass {
			int32_t width, height;
			VkFramebuffer frameBuffer;
			FrameBufferAttachment depth;
			VkRenderPass renderPass;
			VkSampler sampler;
			VkDescriptorImageInfo descriptor;
		};

	public:

		HmckRenderer(HmckWindow& window, HmckDevice& device);
		~HmckRenderer();

		// delete copy constructor and copy destructor
		HmckRenderer(const HmckRenderer&) = delete;
		HmckRenderer& operator=(const HmckRenderer&) = delete;

		VkRenderPass getSwapChainRenderPass() const { return hmckSwapChain->getRenderPass(); }
		VkRenderPass getOffscreenRenderPass() const { return offscreenRenderPass.renderPass; }
		float getAspectRatio() const { return hmckSwapChain->extentAspectRatio(); }
		bool isFrameInProgress() const { return isFrameStarted; }

		VkDescriptorImageInfo getOffscreenDescriptorImageInfo() { return offscreenRenderPass.descriptor; }

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
		void freeOffscreenRenderPass();
		void recreateSwapChain();
		void recreateOffscreenRenderPass();

		HmckWindow& hmckWindow;
		HmckDevice& hmckDevice;
		std::unique_ptr<HmckSwapChain> hmckSwapChain;
		std::vector<VkCommandBuffer> commandBuffers;
		OffscreenRenderPass offscreenRenderPass;

		uint32_t currentImageIndex;
		int currentFrameIndex{ 0 };
		bool isFrameStarted{false};
	};

}
