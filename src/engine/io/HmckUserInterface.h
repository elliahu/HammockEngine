#pragma once

#include "core/HmckDevice.h"
#include "imgui.h"
#include "core/HmckRenderer.h"

#include <memory>



namespace Hmck {
    class UserInterface {
    public:
        UserInterface(Device &device, VkRenderPass renderPass, Window &window);

        ~UserInterface();

        // Ui rendering
        static void beginUserInterface();

        static void endUserInterface(VkCommandBuffer commandBuffer);

        static void showDemoWindow() { ImGui::ShowDemoWindow(); }



        void showWindowControls() const;


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


        Device &device;
        Window &window;
        VkRenderPass renderPass;
        VkDescriptorPool imguiPool;
    };
}
