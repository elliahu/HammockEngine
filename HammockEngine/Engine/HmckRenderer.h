#pragma once

#include "Platform/HmckWindow.h"
#include "HmckDevice.h"
#include "HmckSwapChain.h"
#include "Systems/HmckUISystem.h"
#include "HmckFramebuffer.h"

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

	public:

		HmckRenderer(HmckWindow& window, HmckDevice& device);
		~HmckRenderer();

		// delete copy constructor and copy destructor
		HmckRenderer(const HmckRenderer&) = delete;
		HmckRenderer& operator=(const HmckRenderer&) = delete;

		VkRenderPass getSwapChainRenderPass() const { return hmckSwapChain->getRenderPass(); }
		VkRenderPass getOffscreenRenderPass() const { return shadowmapFramebuffer->renderPass; }
		VkRenderPass getGbufferRenderPass() const { return gbufferFramebuffer->renderPass; }
		float getAspectRatio() const { return hmckSwapChain->extentAspectRatio(); }
		bool isFrameInProgress() const { return isFrameStarted; }

		VkDescriptorImageInfo getShadowmapDescriptorImageInfo() 
		{  
			VkDescriptorImageInfo descriptorImageInfo{};
			descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			descriptorImageInfo.imageView = shadowmapFramebuffer->attachments[0].view;
			descriptorImageInfo.sampler = shadowmapFramebuffer->sampler;

			return descriptorImageInfo;
		}

		VkDescriptorImageInfo getGbufferDescriptorImageInfo(int index)
		{
			VkDescriptorImageInfo descriptorImageInfo{};
			descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			descriptorImageInfo.imageView = gbufferFramebuffer->attachments[index].view;
			descriptorImageInfo.sampler = gbufferFramebuffer->sampler;

			return descriptorImageInfo;
		}


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
		void beginShadowmapRenderPass(VkCommandBuffer commandBuffer);
		void beginGbufferRenderPass(VkCommandBuffer commandBuffer);
		void beginSwapChainRenderPass(VkCommandBuffer commandBuffer);
		void beginRenderPass(
			std::unique_ptr<HmckFramebuffer>& framebuffer,
			VkCommandBuffer commandBuffer,
			std::vector<VkClearValue> clearValues);
		void endRenderPass(VkCommandBuffer commandBuffer);


	private:
		void createCommandBuffer();
		void freeCommandBuffers();
		void recreateSwapChain();
		void recreateShadowmapRenderPass();
		void recreateGbufferRenderPass();

		HmckWindow& hmckWindow;
		HmckDevice& hmckDevice;
		std::unique_ptr<HmckSwapChain> hmckSwapChain;
		std::vector<VkCommandBuffer> commandBuffers;
		std::unique_ptr<HmckFramebuffer> shadowmapFramebuffer;
		std::unique_ptr<HmckFramebuffer> gbufferFramebuffer;
		

		uint32_t currentImageIndex;
		int currentFrameIndex{ 0 };
		bool isFrameStarted{false};
	};

}
