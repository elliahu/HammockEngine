#pragma once

#include <memory>
#include <string>

#include "core/HmckVulkanInstance.h"
#include "utils/HmckEventEmitter.h"
#define TW_USE_VULKAN
#include <TinyWindow.h>

namespace Hmck {
    class Window : public EventEmitter {
    public:

        typedef TinyWindow::keyState_t KeyState;
        typedef TinyWindow::key_t Key;
        typedef TinyWindow::buttonState_t ButtonState;
        typedef TinyWindow::mouseButton_t MouseButton;
        typedef TinyWindow::mouseScroll_t MouseScroll;


        Window(VulkanInstance& vulkanInstance, int windowWidth, int windowHeight, const std::string &_windowName);

        // delete copy constructor and copy destructor
        Window(const Window &) = delete;

        Window &operator=(const Window &) = delete;

        ~Window();

        void printMonitorInfo();


        std::string getWindowName() { return windowName; }
        std::unique_ptr<TinyWindow::tWindow> &getWindowPtr() { return window; }
        VkSurfaceKHR getSurface() const { return window->GetVulkanSurface(); }
        VkExtent2D getExtent() const {
            return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
        }


        void pollEvents();
        bool shouldClose() const { return window->shouldClose; }
        bool wasWindowResized() const { return framebufferResized; }

        void resetWindowResizedFlag() { framebufferResized = false; }

        bool isKeyDown(unsigned int key) const;
        bool isButtonDown(MouseButton button) const;

        bool isKeyUp(unsigned int key) const;
        bool isButtonUp(MouseButton button) const;

    private:
        VulkanInstance& instance;
        static std::unique_ptr<TinyWindow::windowManager> manager;
        std::unique_ptr<TinyWindow::tWindow> window;

        int width;
        int height;
        bool framebufferResized = false;
        std::string windowName;

        std::vector<std::function<void(TinyWindow::tWindow *window, const TinyWindow::vec2_t<unsigned int> &res)> >
        resizeCallbacks;
        std::vector<std::function<void(TinyWindow::tWindow *window, unsigned int key, TinyWindow::keyState_t state)> >
        keyCallbacks;
        std::vector<std::function<void(TinyWindow::tWindow *window, TinyWindow::mouseButton_t button,
                                       TinyWindow::buttonState_t state)> > mouseClickCallbacks;
        std::vector<std::function<void(TinyWindow::tWindow *window, TinyWindow::mouseScroll_t mouseScrollDirection)> >
        scrollCallbacks;

        std::vector<std::function<void(TinyWindow::tWindow *window, const TinyWindow::vec2_t<int> &windowMousePosition,
                                       const TinyWindow::vec2_t<int> &screenMousePosition)> > mouseMoveCallbacks;

        std::vector<std::function<void(TinyWindow::tWindow *window)> > shutDownCallbacks;
        std::vector<std::function<void(TinyWindow::tWindow *window)> > maximizedCallbacks;
        std::vector<std::function<void(TinyWindow::tWindow *window)> > minimizedCallbacks;
        std::vector<std::function<void(TinyWindow::tWindow *window, bool isFocused)> > focusCallbacks;
        std::vector<std::function<void(TinyWindow::tWindow *window, const TinyWindow::vec2_t<int> &windowPosition)> >
        windowMoveCallbacks;
        std::vector<std::function<void(TinyWindow::tWindow *window, const std::vector<std::string> &files,
                                       const TinyWindow::vec2_t<int> &windowMousePosition)> > fileDropCallbacks;

        std::unordered_map<unsigned int, KeyState> keyMap;
        std::unordered_map<MouseButton, ButtonState> buttonMap;
    };
}
