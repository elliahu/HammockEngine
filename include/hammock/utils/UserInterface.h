#pragma once

#include "hammock/core/Device.h"
#include "imgui.h"
#include "hammock/core/RenderContext.h"
#include "HandmadeMath.h"
#include <memory>



namespace Hammock {
    class UserInterface {
    public:
        UserInterface(Device &device, VkRenderPass renderPass, VkDescriptorPool descriptorPool, Window &window);

        ~UserInterface();

        // Ui rendering
        static void beginUserInterface();

        static void endUserInterface(VkCommandBuffer commandBuffer);

        static void showDemoWindow() { ImGui::ShowDemoWindow(); }

        void showDebugStats(const HmckMat4 &inverseView, float frameTime);

        void showColorSettings(float *exposure, float *gamma, float *whitePoint);

        static void forwardKeyDownEvent(ImGuiKey key, bool down){ImGui::GetIO().AddKeyEvent(key,down);}
        static void forwardButtonDownEvent(int button, bool down){ImGui::GetIO().AddMouseButtonEvent(button,down);}
        static void forwardMousePosition(float x, float y){ImGui::GetIO().AddMousePosEvent(x,y);}
        static void forwardInputCharacter(char c){ImGui::GetIO().AddInputCharacter(c);}

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
