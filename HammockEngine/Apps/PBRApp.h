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
#include "HmckMemory.h"

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
		void clean();


		std::unique_ptr<Scene> scene{};

		BufferHandle vertexBuffer;
		BufferHandle indexBuffer;

		BufferHandle skyboxVertexBuffer;
		BufferHandle skyboxIndexBuffer;

		// Descriptors
		// per scene (bound once when scene is initialized)
		DescriptorSetHandle environmentDescriptorSet;
		DescriptorSetLayoutHandle environmentDescriptorSetLayout;
		BufferHandle environmentBuffer;
		
		// per frame
		std::vector<DescriptorSetHandle> frameDescriptorSets{};
		DescriptorSetLayoutHandle frameDescriptorSetLayout;
		std::vector<BufferHandle> frameBuffers{}; // TODO this is misleading as these ara data buffers but name suggests these are actual framebbuffers
		
		// per entity
		std::vector<DescriptorSetHandle> entityDescriptorSets{};
		DescriptorSetLayoutHandle entityDescriptorSetLayout;
		std::vector<BufferHandle> entityBuffers{};

		// per material
		std::vector<DescriptorSetHandle> materialDescriptorSets{};
		DescriptorSetLayoutHandle materialDescriptorSetLayout;
		std::vector<BufferHandle> materialBuffers{};

		// gbuffer descriptors
		std::vector<DescriptorSetHandle> gbufferDescriptorSets{};
		DescriptorSetLayoutHandle gbufferDescriptorSetLayout;


		// renderpasses and framebuffers
		std::unique_ptr<Framebuffer> gbufferFramebuffer{}; // TODO probably shoul be bufferd as well

		// pipelines
		std::unique_ptr<GraphicsPipeline> skyboxPipeline{}; // uses gbufferFramebuffer render pass
		std::unique_ptr<GraphicsPipeline> gbufferPipeline{}; // uses gbufferFramebuffer render pass
		std::unique_ptr<GraphicsPipeline> defferedPipeline{};// uses swapchain render pass
	};

}

