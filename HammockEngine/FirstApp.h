#pragma once

#include "HmckWindow.h"
#include "HmckPipeline.h"
#include "HmckDevice.h"
#include "HmckSwapChain.h"
#include "HmckModel.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <memory>
#include <vector>
#include <stdexcept>

namespace Hmck
{
	/// <summary>
	/// scalar float: N = 4 Bytes
	/// vec2: 2N = 8 Bytes
	/// vec3 (or vec4): 4N = 16 Bytes
	/// taken from 15.6.4 Offset and Stride Assignment
	/// </summary>
	struct HmckSimplePushConstantData {
		glm::vec2 offset;
		alignas(16) glm::vec3 color;
	};

	class FirstApp
	{
	public:
		static constexpr int WINDOW_WIDTH = 1280;
		static constexpr int WINDOW_HEIGHT = 720;

		FirstApp();
		~FirstApp();

		// delete copy constructor and copy destructor
		FirstApp(const FirstApp&) = delete;
		FirstApp& operator=(const FirstApp&) = delete;

		void run();

	private:
		void loadModels();
		void createPipelineLayout();
		void createPipeline();
		void createCommandBuffer();
		void freeCommandBuffers();
		void drawFrame();
		void recreateSwapChain();
		void recordCommandBuffer(int imageIndex);

		HmckWindow hmckWindow{ WINDOW_WIDTH, WINDOW_HEIGHT, "First Vulkan App" };
		HmckDevice hmckDevice{ hmckWindow };
		std::unique_ptr<HmckSwapChain> hmckSwapChain;
		std::unique_ptr<HmckPipeline> hmckPipeline;
		VkPipelineLayout pipelineLayout;
		std::vector<VkCommandBuffer> commandBuffers;
		std::unique_ptr<HmckModel> hmckModel;
	};

}
