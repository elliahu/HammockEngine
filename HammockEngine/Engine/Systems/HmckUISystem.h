#pragma once
#include "HmckPipeline.h"
#include "HmckDevice.h"
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"
#include "HmckGameObject.h"

#include <exception>
#include <deque>

namespace Hmck
{

	class HmckUISystem
	{
	public:
		HmckUISystem(HmckDevice& device, VkRenderPass renderPass, HmckWindow& window);
		~HmckUISystem();

		// Ui rendering
		void beginUserInterface();
		void endUserInterface(VkCommandBuffer commandBuffer);
		void showDebugStats(HmckGameObject& viewerObject);

		// forwarding events to ImGUI
		static void forward(int button, bool state);

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