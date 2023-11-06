#pragma once
#include "HmckPipeline.h"
#include "HmckDevice.h"
#include "Utils/HmckLogger.h"
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"
#include "HmckGameObject.h"
#include "HmckFrameInfo.h"

#include <exception>
#include <deque>
#include <map>

namespace Hmck
{

	class UserInterface
	{
	public:
		UserInterface(Device& device, VkRenderPass renderPass, Window& window);
		~UserInterface();

		// Ui rendering
		void beginUserInterface();
		void endUserInterface(VkCommandBuffer commandBuffer);
		void showDemoWindow() { ImGui::ShowDemoWindow(); }
		void showDebugStats(GameObject& camera);
		void showWindowControls();
		void showGameObjectComponents(GameObject& gameObject, bool * close = (bool*)0);
		void showGameObjectsInspector(GameObject::Map& gameObjects);
		void showGlobalSettings(GlobalUbo& ubo);
		void showLog();

		// forwarding events to ImGUI
		static void forward(int button, bool state);

	private:
		void init();
		void setupStyle();

		VkCommandBuffer beginSingleTimeCommands();
		void endSingleTimeCommands(VkCommandBuffer commandBuffer);

		void beginWindow(const char* title,bool * open = (bool*) 0, ImGuiWindowFlags flags = 0);
		void endWindow();
		void gameObjectComponets(GameObject& gameObject);

		Device& hmckDevice;
		Window& hmckWindow;
		VkRenderPass renderPass;
		VkDescriptorPool imguiPool;
	};
}