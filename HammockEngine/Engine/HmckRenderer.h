#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <memory>
#include <vector>
#include <stdexcept>
#include <cassert>
#include <functional>

#include "Platform/HmckWindow.h"
#include "HmckDevice.h"
#include "HmckSwapChain.h"
#include "Systems/HmckUserInterface.h"
#include "HmckFramebuffer.h"
#include "HmckScene.h"
#include "HmckPipeline.h"
#include "Apps/IApp.h"

// black clear color
#define HMCK_CLEAR_COLOR {0.f,0.f,0.f} //{ 0.f,171.f / 255.f,231.f / 255.f,1.f }

#define SHADOW_RES_WIDTH 2048
#define SHADOW_RES_HEIGHT 2048
#define SHADOW_TEX_FILTER VK_FILTER_LINEAR

#define SSAO_RES_MULTIPLIER 1.0

namespace Hmck
{
	struct EnvironmentBufferData
	{
		struct OmniLight
		{
			glm::vec4 position;
			glm::vec4 color;
		};

		OmniLight omniLights[10]; // TODO lights should be in frame buffer data
		uint32_t numOmniLights = 0;
	};

	struct FrameBufferData
	{
		glm::mat4 projection{ 1.f };
		glm::mat4 view{ 1.f };
		glm::mat4 inverseView{ 1.f };
	};

	struct EntityBufferData
	{
		glm::mat4 model{ 1.f };
		glm::mat4 normal{ 1.f };
	};

	struct PrimitiveBufferData
	{
		glm::vec4 baseColorFactor{ 1.0f,1.0f,1.0f,1.0f };
		uint32_t baseColorTextureIndex = TextureIndex::Invalid;
		uint32_t normalTextureIndex = TextureIndex::Invalid;
		uint32_t metallicRoughnessTextureIndex = TextureIndex::Invalid;
		uint32_t occlusionTextureIndex = TextureIndex::Invalid;
		float alphaCutoff = 1.0f;
	};

	class Renderer
	{
	public:


		struct PushConstantData
		{
			glm::vec2 resolution;
			float elapsedTime;
		};


		Renderer(Window& window, Device& device, std::unique_ptr<Scene>& scene);
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


		void bindVertexBuffer(VkCommandBuffer commandBuffer);
		void bindSkyboxVertexBuffer(VkCommandBuffer commandBuffer);

		


	private:
		void createCommandBuffer();
		void freeCommandBuffers();
		void createVertexBuffer();
		void createIndexBuffer();
		void recreateSwapChain();
		

		Window& window;
		Device& device;
		std::unique_ptr<SwapChain> hmckSwapChain;
		std::vector<VkCommandBuffer> commandBuffers;

		std::unique_ptr<Buffer> sceneVertexBuffer;
		std::unique_ptr<Buffer> sceneIndexBuffer;

		std::unique_ptr<Buffer> skyboxVertexBuffer;
		std::unique_ptr<Buffer> skyboxIndexBuffer;

		std::unique_ptr<Scene>& scene;


		uint32_t currentImageIndex;
		int currentFrameIndex{ 0 };
		bool isFrameStarted{false};
	};

}
