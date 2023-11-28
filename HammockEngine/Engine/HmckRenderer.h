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

// black clear color
#define HMCK_CLEAR_COLOR { 0.f,171.f / 255.f,231.f / 255.f,1.f }

#define SHADOW_RES_WIDTH 2048
#define SHADOW_RES_HEIGHT 2048
#define SHADOW_TEX_FILTER VK_FILTER_LINEAR

#define SSAO_RES_MULTIPLIER 1.0

namespace Hmck
{
	struct SceneBufferData
	{
		
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
		uint32_t baseColorTextureIndex = TextureHandle::Invalid;
		uint32_t normalTextureIndex = TextureHandle::Invalid;
		uint32_t metallicRoughnessTextureIndex = TextureHandle::Invalid;
		uint32_t occlusionTextureIndex = TextureHandle::Invalid;
		float alphaCutoff = 1.0f;
	};

	class Renderer
	{
	public:
		Renderer(Window& window, Device& device, std::unique_ptr<Scene>&);
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
			uint32_t frameIndex,
			VkCommandBuffer commandBuffer);

		void writeSceneData(std::vector<Image>& images, SceneBufferData data);
		void bindSceneData(VkCommandBuffer commandBuffer);
		void updateFrameBuffer(uint32_t index, FrameBufferData data);
		void updateEntityBuffer(uint32_t index, EntityBufferData data);
		void updatePrimitiveBuffer(uint32_t index, PrimitiveBufferData data);


	private:
		void createCommandBuffer();
		void freeCommandBuffers();
		void recreateSwapChain();
		void renderEntity(
			uint32_t frameIndex, 
			VkCommandBuffer commandBuffer, 
			std::shared_ptr<Entity>& entity);

		Window& window;
		Device& device;
		std::unique_ptr<SwapChain> hmckSwapChain;
		std::vector<VkCommandBuffer> commandBuffers;

		std::unique_ptr<Scene>& scene;

		// descriptors
		std::unique_ptr<DescriptorPool> descriptorPool{};

		// per scene (bound once when scene is initialized)
		VkDescriptorSet environmentDescriptorSet;
		std::unique_ptr<DescriptorSetLayout> environmentDescriptorSetLayout;
		std::unique_ptr<Buffer> environmentBuffer;

		// per frame
		std::vector<VkDescriptorSet> frameDescriptorSets{ SwapChain::MAX_FRAMES_IN_FLIGHT };
		std::unique_ptr<DescriptorSetLayout> frameDescriptorSetLayout;
		std::vector<std::unique_ptr<Buffer>> frameBuffers{ SwapChain::MAX_FRAMES_IN_FLIGHT };

		// per entity
		std::vector<VkDescriptorSet> entityDescriptorSets;
		std::unique_ptr<DescriptorSetLayout> entityDescriptorSetLayout;
		std::vector<std::unique_ptr<Buffer>> entityBuffers;
		
		// per primitive
		std::vector<VkDescriptorSet> primitiveDescriptorSets;
		std::unique_ptr<DescriptorSetLayout> primitiveDescriptorSetLayout;
		std::vector<std::unique_ptr<Buffer>> primitiveBuffers;

		std::unique_ptr<GraphicsPipeline> pipeline{};

		uint32_t currentImageIndex;
		int currentFrameIndex{ 0 };
		bool isFrameStarted{false};
	};

}
