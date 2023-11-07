#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <memory>
#include <vector>
#include <stdexcept>
#include <cassert>

#include "Platform/HmckWindow.h"
#include "HmckDevice.h"
#include "HmckSwapChain.h"
#include "Systems/HmckUserInterface.h"
#include "HmckFramebuffer.h"
#include "HmckScene.h"

// black clear color
#define HMCK_CLEAR_COLOR { 0.f,171.f / 255.f,231.f / 255.f,1.f }

#define SHADOW_RES_WIDTH 2048
#define SHADOW_RES_HEIGHT 2048
#define SHADOW_TEX_FILTER VK_FILTER_LINEAR

#define SSAO_RES_MULTIPLIER 1.0

namespace Hmck
{
	class Renderer
	{

	public:

		Renderer(Window& window, Device& device);
		~Renderer();

		// delete copy constructor and copy destructor
		Renderer(const Renderer&) = delete;
		Renderer& operator=(const Renderer&) = delete;

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
		void beginSwapChainRenderPass(VkCommandBuffer commandBuffer);
		void beginRenderPass(
			std::unique_ptr<Framebuffer>& framebuffer,
			VkCommandBuffer commandBuffer,
			std::vector<VkClearValue> clearValues);
		void beginRenderPass(
			Framebuffer& framebuffer,
			VkCommandBuffer commandBuffer,
			std::vector<VkClearValue> clearValues);
		void endRenderPass(VkCommandBuffer commandBuffer);
		void render(
			std::unique_ptr<Scene>& scene, 
			VkCommandBuffer commandBuffer, 
			VkPipelineLayout pipelineLayout,
			std::unique_ptr<Buffer>& transformBuffer,
			std::unique_ptr<Buffer>& materialPropertyBuffer);


	private:
		void createCommandBuffer();
		void freeCommandBuffers();
		void recreateSwapChain();
		void renderEntity(
			std::unique_ptr<Scene>& scene, 
			VkCommandBuffer commandBuffer, 
			std::shared_ptr<Entity>& entity, 
			VkPipelineLayout pipelineLayout,
			std::unique_ptr<Buffer>& transformBuffer,
			std::unique_ptr<Buffer>& materialPropertyBuffer);

		Window& window;
		Device& device;
		std::unique_ptr<SwapChain> hmckSwapChain;
		std::vector<VkCommandBuffer> commandBuffers;
		

		uint32_t currentImageIndex;
		int currentFrameIndex{ 0 };
		bool isFrameStarted{false};
	};

}
