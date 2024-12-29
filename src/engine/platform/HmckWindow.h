#pragma once

#include <memory>
#include <string>
#if defined(_WIN32)
#include <windows.h>
#endif
#if defined(__linux__)
#include <X11/Xlib.h>
#endif

#include "HmckKeycodes.h"
#include "core/HmckVulkanInstance.h"
#include "utils/HmckEventEmitter.h"
#include "HandmadeMath.h"

namespace Hmck
{
    class Window : public EventEmitter
    {
    public:
        Window(VulkanInstance &instance, const std::string &_windowName, int windowWidth, int windowHeight);

        ~Window();

        // delete copy constructor and copy destructor
        Window(const Window &) = delete;
        Window &operator=(const Window &) = delete;

        VkSurfaceKHR getSurface() const { return surface; }
        std::string getWindowName() const { return windowName; }
        VkExtent2D getExtent() const
        {
            return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
        }
        bool wasWindowResized() const { return framebufferResized; }
        void resetWindowResizedFlag() { framebufferResized = false; }

        KeyState getKeyState(Keycode key);
        ButtonState getButtonState(Keycode button);
        HmckVec2 getCursorPosition() const { return mousePosition; }

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
        std::unordered_map<Keycode, ButtonState> buttonMap;
        HmckVec2 mousePosition{0.f, 0.f};

#if defined(_WIN32)
        void Win32_onKeyDown(WPARAM key);
        void Win32_onKeyUp(WPARAM key);
        void Win32_onClose();
        void Win32_onDpiChange(HWND hWnd, WPARAM wParam, LPARAM lParam);
        HWND hWnd;
        MSG msg{};
        bool resizing = false;
#endif

#if defined(__linux__)
        void X11_onClose();
        void X11_processEvent(XEvent event);
        void X11_onKeyDown(KeySym key);
        void X11_onKeyUp(KeySym key);
        Display* display;
        ::Window window;
        ::Window root;
        Atom wmDeleteMessage;
#endif
    };
}
