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
        std::unordered_map<Keycode, KeyState> keymap;

#if defined(_WIN32)
        void Win32_onKeyDown(WPARAM key);
        void Win32_onKeyUp(WPARAM key);
        void Win32_onClose();
        void Win32_onDpiChange(HWND hWnd, WPARAM wParam, LPARAM lParam);
        HWND hWnd;
        MSG msg{};
        bool resizing = false;
#endif

    };
}
