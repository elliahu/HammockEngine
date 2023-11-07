#pragma once
#include "HmckPipeline.h"
#include "HmckDevice.h"
#include "Utils/HmckLogger.h"
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"
#include "HmckFrameInfo.h"
#include "HmckEntity.h"

#include <exception>
#include <deque>
#include <map>
#include <memory>


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
		void showDebugStats(std::shared_ptr<Entity> camera);
		void showWindowControls();
		void showGameObjectComponents(std::shared_ptr<Entity>& gameObject, bool * close = (bool*)0);
		void showGameObjectsInspector(std::vector<std::shared_ptr<Entity>>& gameObjects);
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
		void gameObjectComponets(std::shared_ptr<Entity>& gameObject);

		Device& hmckDevice;
		Window& hmckWindow;
		VkRenderPass renderPass;
		VkDescriptorPool imguiPool;
	};
}