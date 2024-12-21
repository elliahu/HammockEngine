#pragma once

#include <memory>
#include <string>
#if defined(_WIN32)
#include <windows.h>
#endif

#include "HmckKeycodes.h"
#include "core/HmckVulkanInstance.h"
#include "utils/HmckEventEmitter.h"


namespace Hmck {
    class Window : public EventEmitter {
    public:

        Window(VulkanInstance &instance, const std::string &_windowName, int windowWidth, int windowHeight);

        ~Window();

        void HandleDpiChange(HWND hWnd, WPARAM wParam, LPARAM lParam);

        // delete copy constructor and copy destructor
        Window(const Window &) = delete;
        Window &operator=(const Window &) = delete;

        VkSurfaceKHR getSurface() const { return surface; }
        std::string getWindowName() const { return windowName; }
        VkExtent2D getExtent() const {
            return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
        }
        bool wasWindowResized() const { return framebufferResized; }
        void resetWindowResizedFlag() { framebufferResized = false; }

        KeyState getKeyState(Keycode key);


        bool shouldClose() const;
        void pollEvents();

        VulkanInstance &instance;
        VkSurfaceKHR surface;
        int width;
        int height;
        bool framebufferResized = false;
        bool _shouldClose = false;
        std::string windowName;

#if defined(_WIN32)
        void onKeyDown(WPARAM key);
        void onKeyUp(WPARAM key);
        HWND hWnd;
        MSG msg{};
        bool resizing = false;
        std::unordered_map<Keycode, KeyState> keymap;
#endif

    };
}
