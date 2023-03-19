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
#define HMCK_CLEAR_COLOR { 1.f,1.f,1.f,1.f }

constexpr auto OFFSCREEN_RES_WIDTH = 4096;
constexpr auto OFFSCREEN_RES_HEIGHT = 4096;
#define TEX_FILTER VK_FILTER_LINEAR;

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
		VkRenderPass getSSAORenderPass() const { return ssaoFramebuffer->renderPass; }
		VkRenderPass getSSAOBlurRenderPass() const { return ssaoBlurFramebuffer->renderPass; }
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

		VkDescriptorImageInfo getGbufferDescriptorDepthImageInfo()
		{
			VkDescriptorImageInfo descriptorImageInfo{};
			descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			descriptorImageInfo.imageView = gbufferFramebuffer->attachments[4].view;
			descriptorImageInfo.sampler = gbufferFramebuffer->sampler;

			return descriptorImageInfo;
		}

		VkDescriptorImageInfo getSSAODescriptorImageInfo()
		{
			VkDescriptorImageInfo descriptorImageInfo{};
			descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			descriptorImageInfo.imageView = ssaoFramebuffer->attachments[0].view;
			descriptorImageInfo.sampler = ssaoFramebuffer->sampler;

			return descriptorImageInfo;
		}

		VkDescriptorImageInfo getSSAOBlurDescriptorImageInfo()
		{
			VkDescriptorImageInfo descriptorImageInfo{};
			descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			descriptorImageInfo.imageView = ssaoBlurFramebuffer->attachments[0].view;
			descriptorImageInfo.sampler = ssaoBlurFramebuffer->sampler;

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
		void beginSSAORenderPass(VkCommandBuffer commandBuffer);
		void beginSSAOBlurRenderPass(VkCommandBuffer commandBuffer);
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
		void recreateOmniShadowmapFramebuffer();
		void recreateGbufferRenderPass();
		void recreateSSAORenderPasses();
		void createShadowCubeMap();

		HmckWindow& hmckWindow;
		HmckDevice& hmckDevice;
		std::unique_ptr<HmckSwapChain> hmckSwapChain;
		std::vector<VkCommandBuffer> commandBuffers;
		std::unique_ptr<HmckFramebuffer> shadowmapFramebuffer;
		std::unique_ptr<HmckFramebuffer> omniShadowmapFramebuffer;
		std::unique_ptr<HmckFramebuffer> gbufferFramebuffer;
		std::unique_ptr<HmckFramebuffer> ssaoFramebuffer;
		std::unique_ptr<HmckFramebuffer> ssaoBlurFramebuffer;

		HmckTexture shadowCubeMap;
		std::array<VkImageView, 6> shadowCubeMapFaceImageViews;
		

		uint32_t currentImageIndex;
		int currentFrameIndex{ 0 };
		bool isFrameStarted{false};
	};

}
