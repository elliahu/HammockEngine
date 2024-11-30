#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string>

#include "HmckInputHandler.h"
#include "core/HmckVulkanInstance.h"

namespace Hmck {
    enum WindowMode {
        HMCK_WINDOW_MODE_FULLSCREEN,
        HMCK_WINDOW_MODE_BORDERLESS,
        HMCK_WINDOW_MODE_WINDOWED
    };

    class Window {
    public:
        Window(int windowWidth, int windowHeight, std::string _windowName);

        ~Window();

        // delete copy constructor and copy destructor
        Window(const Window &) = delete;

        Window &operator=(const Window &) = delete;

        std::string getWindowName() { return windowName; }

        bool shouldClose() const;

        [[nodiscard]] VkExtent2D getExtent() const {
            return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
        }

        void createWindowSurface(VulkanInstance &instance);

        [[nodiscard]] VkSurfaceKHR getSurface() const { return surface; }

        [[nodiscard]] bool wasWindowResized() const { return framebufferResized; }
        void resetWindowResizedFlag() { framebufferResized = false; }
        [[nodiscard]] GLFWwindow *getGLFWwindow() const { return window; }
        static void pollEvents() { glfwPollEvents(); }
        InputManager &getInputManager() { return inputHandler; }

        void setCursorVisibility(bool visible) const;

        void getCursorPosition(double &x, double &y) const;

        void setWindowMode(WindowMode mode) const;

        void setWindowResolution(uint32_t resX, uint32_t resY);

    private:
        GLFWwindow *window;
        VkSurfaceKHR surface;
        int monitorsCount;
        GLFWmonitor **monitors = nullptr;
        int monitorIndex = 0;
        int width;
        int height;
        bool framebufferResized = false;
        std::string windowName;

        InputManager inputHandler;

        void initWindow(int width, int height);

        static void framebufferResizeCallback(GLFWwindow *window, int width, int height);
    };
}
