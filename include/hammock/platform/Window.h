#pragma once

#include <memory>
#include <string>
#if defined(_WIN32)
#define SURFER_PLATFORM_WIN32
#endif
#if defined(__linux__)
#define SURFER_PLATFORM_X11
#endif
#include <VulkanSurfer.h>

#include "hammock/core/VulkanInstance.h"

namespace hammock
{
    class Window
    {
    public:
        Window(VulkanInstance &instance, const std::string &_windowName, int windowWidth, int windowHeight);

        ~Window();

        // delete copy constructor and copy destructor
        Window(const Window &) = delete;
        Window &operator=(const Window &) = delete;

        VkSurfaceKHR getSurface() const { return window->getSurface(); }
        std::string getWindowName() const { return windowName; }
        VkExtent2D getExtent() const
        {
            return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
        }
        bool wasWindowResized() const { return framebufferResized; }
        void resetWindowResizedFlag() { framebufferResized = false; }


        bool shouldClose() const;
        void pollEvents();

        Surfer::Window * window;
        VulkanInstance &instance;
        int width;
        int height;
        bool framebufferResized = false;
        bool _shouldClose = false;
        std::string windowName;
    };
}
