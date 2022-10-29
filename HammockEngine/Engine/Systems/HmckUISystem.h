#pragma once
#include "HmckPipeline.h"
#include "HmckDevice.h"
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"
#include "HmckGameObject.h"

#include <exception>
#include <deque>
#include <map>

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
		void showDebugStats(HmckGameObject& camera);
		void showGameObjectComponents(HmckGameObject& gameObject, bool * close = (bool*)0);
		void showGameObjectsInspector(HmckGameObject::Map& gameObjects);

		// forwarding events to ImGUI
		static void forward(int button, bool state);

	private:
		void init();

		VkCommandBuffer beginSingleTimeCommands();
		void endSingleTimeCommands(VkCommandBuffer commandBuffer);

		void beginWindow(const char* title,bool * open = (bool*) 0, ImGuiWindowFlags flags = 0);
		void endWindow();
		void gameObjectComponets(HmckGameObject& gameObject);


		HmckDevice& hmckDevice;
		HmckWindow& hmckWindow;
		VkRenderPass renderPass;
		VkDescriptorPool imguiPool;
	};
}