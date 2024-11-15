#pragma once
#include "HmckPipeline.h"
#include "HmckDevice.h"
#include "utils/HmckLogger.h"
#include "imgui.h"
#include "backends/imgui_impl_glfw.h" // !important
#include "backends/imgui_impl_vulkan.h"
#include "HmckEntity.h"
#include "HmckScene.h"
#include "HmckRenderer.h"
#include "HmckUtils.h"
#include "HmckLights.h"
#include "HmckUtils.h"

#include <exception>
#include <deque>
#include <map>
#include <memory>
#include <string>


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
		void showEntityInspector(std::unique_ptr<Scene>& scene);
		void showColorSettings(float* exposure, float* gamma, float * whitePoint);
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
		void entityComponets(std::shared_ptr<Entity> entity);
		void inspectEntity(std::shared_ptr<Entity> entity, std::unique_ptr<Scene>& scene);

		Device& device;
		Window& window;
		VkRenderPass renderPass;
		VkDescriptorPool imguiPool;
	};
}