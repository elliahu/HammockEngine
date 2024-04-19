#pragma once
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <memory>
#include <vector>
#include <stdexcept>
#include <chrono>

#include "Platform/HmckWindow.h"
#include "HmckDevice.h"
#include "HmckRenderer.h"
#include "Systems/HmckUserInterface.h"
#include "HmckCamera.h"
#include "Controllers/KeyboardMovementController.h"
#include "HmckBuffer.h"
#include "HmckDescriptors.h"
#include "Utils/HmckLogger.h"
#include "IApp.h"
#include "HmckEntity.h"
#include "HmckScene.h"
#include "HmckLights.h"

#ifndef MODELS_DIR
#define MODELS_DIR "../../Resources/Models/"
#endif // !MODELS_DIR

#ifndef MATERIALS_DIR
#define MATERIALS_DIR "../../Resources/Materials/"
#endif // !MATERIALS_DIR


namespace Hmck 
{
	class PBRApp : public IApp
	{
	public:

		PBRApp();

		// Inherited via IApp
		virtual void run() override;
		virtual void load() override;

		void init();

		void renderEntity(
			uint32_t frameIndex,
			VkCommandBuffer commandBuffer,
			std::unique_ptr<GraphicsPipeline>& pipeline,
			std::shared_ptr<Entity>& entity);

		

	private:
		void createPipelines(Renderer& renderer);


		std::unique_ptr<Scene> scene{};

		// Descriptors
		std::unique_ptr<DescriptorPool> descriptorPool{};
		// per scene (bound once when scene is initialized)
		VkDescriptorSet environmentDescriptorSet;
		std::unique_ptr<DescriptorSetLayout> environmentDescriptorSetLayout;
		std::unique_ptr<Buffer> environmentBuffer;
		// per frame
		std::vector<VkDescriptorSet> frameDescriptorSets{ SwapChain::MAX_FRAMES_IN_FLIGHT };
		std::unique_ptr<DescriptorSetLayout> frameDescriptorSetLayout;
		std::vector<std::unique_ptr<Buffer>> frameBuffers{ SwapChain::MAX_FRAMES_IN_FLIGHT }; // TODO this is misleading as these ara data buffers but name suggests these are actual framebbuffers
		// per entity
		std::vector<VkDescriptorSet> entityDescriptorSets{ SwapChain::MAX_FRAMES_IN_FLIGHT };
		std::unique_ptr<DescriptorSetLayout> entityDescriptorSetLayout;
		std::vector<std::unique_ptr<Buffer>> entityBuffers{ SwapChain::MAX_FRAMES_IN_FLIGHT };
		// per material
		std::vector<VkDescriptorSet> materialDescriptorSets{ SwapChain::MAX_FRAMES_IN_FLIGHT };
		std::unique_ptr<DescriptorSetLayout> materialDescriptorSetLayout;
		std::vector<std::unique_ptr<Buffer>> materialBuffers{ SwapChain::MAX_FRAMES_IN_FLIGHT };

		// gbuffer descriptors
		std::vector<VkDescriptorSet> gbufferDescriptorSets{ SwapChain::MAX_FRAMES_IN_FLIGHT };
		std::unique_ptr<DescriptorSetLayout> gbufferDescriptorSetLayout;


		// renderpasses and framebuffers
		std::unique_ptr<Framebuffer> gbufferFramebuffer{}; // TODO probably shoul be bufferd as well

		// pipelines
		std::unique_ptr<GraphicsPipeline> skyboxPipeline{}; // uses gbufferFramebuffer render pass
		std::unique_ptr<GraphicsPipeline> gbufferPipeline{}; // uses gbufferFramebuffer render pass
		std::unique_ptr<GraphicsPipeline> defferedPipeline{};// uses swapchain render pass
	};

}

