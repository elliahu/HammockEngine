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
#include "HmckTexture.h"

#ifndef MODELS_DIR
#define MODELS_DIR "../../Resources/Models/"
#endif // !MODELS_DIR

#ifndef MATERIALS_DIR
#define MATERIALS_DIR "../../Resources/Materials/"
#endif // !MATERIALS_DIR


namespace Hmck
{
	class RaymarchingDemoApp : public IApp
	{
		// There is so much code in this class 
		// TODO make more abstraction layers over the IApp
	public:

		struct BufferData
		{
			glm::mat4 projection{};
			glm::mat4 view{};
			glm::mat4 inverseView{};
		};

		struct PushData
		{
			glm::vec2 resolution{};
			float elapsedTime;
		};

		RaymarchingDemoApp();

		// Inherited via IApp
		virtual void run() override;
		virtual void load() override;

		void init();
		void draw(int frameIndex, float elapsedTime, VkCommandBuffer commandBuffer);
		void destroy();

	private:
		std::unique_ptr<Scene> scene{};

		// DESCRIPTORS
		std::unique_ptr<DescriptorPool> descriptorPool{};
		
		std::vector<VkDescriptorSet> descriptorSets{ SwapChain::MAX_FRAMES_IN_FLIGHT };
		std::unique_ptr<DescriptorSetLayout> descriptorSetLayout;
		std::vector<std::unique_ptr<Buffer>> uniformBuffers{ SwapChain::MAX_FRAMES_IN_FLIGHT };

		std::unique_ptr<GraphicsPipeline> pipeline{}; // uses swapchain render pass

		Texture2D noiseTexture;
	};

}

