#pragma once
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <memory>
#include <vector>
#include <stdexcept>
#include <numbers> 
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
#include "HmckGLTF.h"
#include "HmckRTX.h"

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

		struct SceneBufferData
		{
			glm::mat4 projection{ 1.f };
			glm::mat4 view{ 1.f };
			glm::mat4 inverseView{ 1.f };

			float exposure = 4.5f;
			float gamma = 1.0f;
			float whitePoint = 11.2f;

			struct OmniLight
			{
				glm::vec4 position;
				glm::vec4 color;
			} omniLights[1000];
			uint32_t numOmniLights = 0;

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
			float metallicFactor = 0.0f;
			float roughnessFactor = 1.0f;
		};

		PBRApp();
		~PBRApp();

		// Inherited via IApp
		virtual void run() override;
		virtual void load() override;

		void init();

		void renderEntity(
			uint32_t frameIndex,
			VkCommandBuffer commandBuffer,
			std::unique_ptr<GraphicsPipeline>& pipeline,
			std::shared_ptr<Entity> entity);

	private:
		void createPipelines(Renderer& renderer);

		std::unique_ptr<Scene> scene{};


		struct {
			BufferHandle vertexBuffer;
			BufferHandle indexBuffer;
			uint32_t numTriangles = 0;
		} geometry;
		

		// Descriptors
		// per frame (scene info updated every frame)
		struct {
			std::vector<DescriptorSetHandle> descriptorSets{};
			DescriptorSetLayoutHandle descriptorSetLayout;
			std::vector<BufferHandle> sceneBuffers{};
			Binding binding = 0;
		} sceneDescriptors;

		// per entity
		struct {
			std::unordered_map<EntityHandle, DescriptorSetHandle> descriptorSets{};
			DescriptorSetLayoutHandle descriptorSetLayout;
			std::unordered_map<EntityHandle, BufferHandle> entityBuffers{};
			Binding binding = 1;
		} entityDescriptors;
		
		// per primitive
		struct {
			std::vector<DescriptorSetHandle> descriptorSets{};
			DescriptorSetLayoutHandle descriptorSetLayout;
			std::vector<BufferHandle> primitiveBuffers{};
			Binding binding = 2;
		} primitiveDescriptors;
		
		// gbuffer descriptors
		struct {
			std::vector<DescriptorSetHandle> descriptorSets{}; 
			DescriptorSetLayoutHandle descriptorSetLayout;
			Binding binding = 3;
		} gBufferDescriptors;



		// Render passes
		// renders environment sphere into gbuffer color attachments and clears depth buffer
		struct {
			std::unique_ptr<GraphicsPipeline> pipeline{}; 
			// uses gbuffer render pass
		} environmentPass;

		// constructs gbuffer
		struct {
			std::unique_ptr<GraphicsPipeline> pipeline{}; 
			std::unique_ptr<Framebuffer> framebuffer{}; // TODO probably shoul be bufferd as well
		} gbufferPass;

		// composes image from gbuffer attachments
		struct {
			std::unique_ptr<GraphicsPipeline> pipeline{};
			std::unique_ptr<Framebuffer> framebuffer{};
		} compositionPass;

		// forward transparency pass
		struct {
			std::unique_ptr<GraphicsPipeline> pipeline{};
		} transparencyPass;

	};

}

