#pragma once
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <memory>
#include <vector>
#include <stdexcept>
#include <chrono>

#include "platforms/HmckWindow.h"
#include "HmckDevice.h"
#include "HmckRenderer.h"
#include "systems/HmckUserInterface.h"
#include "HmckCamera.h"
#include "controllers/KeyboardMovementController.h"
#include "HmckBuffer.h"
#include "HmckDescriptors.h"
#include "utils/HmckLogger.h"
#include "IApp.h"
#include "HmckEntity.h"
#include "HmckScene.h"
#include "HmckLights.h"
#include "HmckTexture.h"
#include "HmckMemory.h"
#include "HmckGLTF.h"




namespace Hmck
{
	class RaymarchingDemoApp : public IApp
	{
	public:

		struct BufferData
		{
			glm::mat4 projection{};
			glm::mat4 view{};
			glm::mat4 inverseView{};
			glm::vec4 sunPosition{ 2.0f, .3f, 2.0f, 0.0f };
			glm::vec4 sunColor{ 1.0f, 0.5f, 0.3f, 0.0f };
			glm::vec4 baseSkyColor{ 0.7f, 0.7f, 0.90, 0.0f };
			glm::vec4 gradientSkyColor{ 0.90f, 0.75f, 0.90f, 0.8f };
		};

		struct PushData
		{
			float resX = IApp::WINDOW_WIDTH;
			float resY = IApp::WINDOW_HEIGHT;
			float elapsedTime = 0.0f;
			float maxSteps = 100.f;
			float marchSize = 0.08f;

		};

		RaymarchingDemoApp();

		// Inherited via IApp
		virtual void run() override;
		virtual void load() override;

		void draw(int frameIndex, float elapsedTime, VkCommandBuffer commandBuffer);
		void destroy();
		void ui();

	private:
		std::unique_ptr<Scene> scene{};

		BufferHandle vertexBuffer;
		BufferHandle indexBuffer;
		
		std::vector<DescriptorSetHandle> descriptorSets{};
		DescriptorSetLayoutHandle descriptorSetLayout;
		std::vector<BufferHandle> buffers{};

		std::unique_ptr<GraphicsPipeline> pipeline{}; // uses swapchain render pass

		Texture2DHandle noiseTexture;

		PushData pushData;
		BufferData bufferData;
	};

}

