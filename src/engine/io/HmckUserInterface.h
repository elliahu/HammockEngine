#pragma once
#include "core/HmckPipeline.h"
#include "core/HmckDevice.h"
#include "utils/HmckLogger.h"
#include "imgui.h"
#include "backends/imgui_impl_glfw.h" // !important
#include "backends/imgui_impl_vulkan.h"
#include "scene/HmckEntity.h"
#include "scene/HmckScene.h"
#include "core/HmckRenderer.h"
#include "utils/HmckUtils.h"
#include "scene/HmckLights.h"

#include <exception>
#include <deque>
#include <map>
#include <memory>
#include <string>


namespace Hmck {
    class UserInterface {
    public:
        UserInterface(Device &device, VkRenderPass renderPass, Window &window);

        ~UserInterface();

        // Ui rendering
        static void beginUserInterface();

        static void endUserInterface(VkCommandBuffer commandBuffer);

        static void showDemoWindow() { ImGui::ShowDemoWindow(); }

        void showDebugStats(const std::shared_ptr<Entity> &camera) const;

        void showWindowControls() const;

        void showEntityInspector(std::unique_ptr<Scene> &scene);

        void showColorSettings(float *exposure, float *gamma, float *whitePoint);

        static void showLog();

        // forwarding events to ImGUI
        static void forward(int button, bool state);

    private:
        void init();

        static void setupStyle();

        VkCommandBuffer beginSingleTimeCommands() const;

        void endSingleTimeCommands(VkCommandBuffer commandBuffer) const;

        static void beginWindow(const char *title, bool *open = (bool *) 0, ImGuiWindowFlags flags = 0);

        static void endWindow();

        static void entityComponents(const std::shared_ptr<Entity> &entity);

        static void inspectEntity(const std::shared_ptr<Entity> &entity, std::unique_ptr<Scene> &scene);

        Device &device;
        Window &window;
        VkRenderPass renderPass;
        VkDescriptorPool imguiPool;
    };
}
