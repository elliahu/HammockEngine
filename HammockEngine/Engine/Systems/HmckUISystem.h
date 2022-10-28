#pragma once
#include "HmckPipeline.h"
#include "HmckDevice.h"
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"

#include <exception>
#include <deque>

namespace Hmck
{

	class HmckUISystem
	{
	public:
		HmckUISystem(HmckDevice& device, VkRenderPass renderPass, HmckWindow& window);
		~HmckUISystem();

		void renderUI();

		void endRenderUI(VkCommandBuffer commandBuffer);

	private:
		void init();

		VkCommandBuffer beginSingleTimeCommands();
		void endSingleTimeCommands(VkCommandBuffer commandBuffer);

		HmckDevice& hmckDevice;
		HmckWindow& hmckWindow;
		VkRenderPass renderPass;
		VkDescriptorPool imguiPool;
	};
}