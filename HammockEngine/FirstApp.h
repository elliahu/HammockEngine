#pragma once

#include "HmckWindow.h"
#include "HmckPipeline.h"
#include "HmckDevice.h"
#include "HmckSwapChain.h"

#include <memory>
#include <vector>
#include <stdexcept>

namespace Hmck
{
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
		void createPipelineLayout();
		void createPipeline();
		void createCommandBuffer();
		void drawFrame();

		HmckWindow hmckWindow{ WINDOW_WIDTH, WINDOW_HEIGHT, "First Vulkan App" };
		HmckDevice hmckDevice{ hmckWindow };
		HmckSwapChain hmckSwapChain{ hmckDevice, hmckWindow.getExtent() };
		std::unique_ptr<HmckPipeline> hmckPipeline;
		VkPipelineLayout pipelineLayout;
		std::vector<VkCommandBuffer> commandBuffers;
	};

}
