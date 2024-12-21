#include "HmckWindow.h"

#include "scene/HmckAssetDelivery.h"

Hmck::Window::Window(VulkanInstance &vulkanInstance, const int windowWidth, const int windowHeight,
                     const std::string &_windowName) : instance{vulkanInstance}, width{windowWidth},
                                                       height{windowHeight}, windowName{_windowName} {
    TinyWindow::windowSetting_t defaultSetting;
    defaultSetting.name = _windowName.c_str();
    defaultSetting.resolution = TinyWindow::vec2_t<unsigned int>(windowWidth, windowHeight);
    defaultSetting.SetProfile(TinyWindow::profile_t::core);
    defaultSetting.currentState = TinyWindow::state_t::maximized;
    defaultSetting.enableSRGB = true;

    manager = std::unique_ptr<TinyWindow::windowManager>();
    window = std::unique_ptr<TinyWindow::tWindow>(manager->AddWindow(defaultSetting));

    auto onResize = [this](TinyWindow::tWindow *window, const TinyWindow::vec2_t<unsigned int> &windowSize) {
        // do the actuall resize
        this->framebufferResized = true;
        this->width = windowSize.x;
        this->height = windowSize.y;

        // dispatch callbacks
        for (auto &callback: resizeCallbacks)
            callback(window, windowSize);
    };
    manager->resizeEvent = onResize;

    auto onKeyPress = [this](TinyWindow::tWindow *window, unsigned int key, TinyWindow::keyState_t keyState) {
        // add key into keymap
        keyMap[key] = keyState;

        // dispatch callbacks
        for (auto &callback: keyCallbacks)
            callback(window, key, keyState);
    };
    manager->keyEvent = onKeyPress;

    auto onMouseClick = [this](TinyWindow::tWindow *window, TinyWindow::mouseButton_t button,
                               TinyWindow::buttonState_t state) {
        // add button into buttonMap
        buttonMap[button] = state;

        // dispatch callbacks
        for (auto &callback: mouseClickCallbacks)
            callback(window, button, state);
    };
    manager->mouseButtonEvent = onMouseClick;

    auto onMouseWheel = [this](TinyWindow::tWindow *window, TinyWindow::mouseScroll_t mouseScrollDirection) {
        // dispatch callbacks
        for (auto &callback: scrollCallbacks)
            callback(window, mouseScrollDirection);
    };
    manager->mouseWheelEvent = onMouseWheel;

    auto onMouseMove = [this](TinyWindow::tWindow *window, const TinyWindow::vec2_t<int> &windowMousePosition,
                              const TinyWindow::vec2_t<int> &screenMousePosition) {
        // dispatch callbacks
        for (auto &callback: mouseMoveCallbacks)
            callback(window, windowMousePosition, screenMousePosition);
    };
    manager->mouseMoveEvent = onMouseMove;

    auto onShutdown = [this](TinyWindow::tWindow *window) {
        // dispatch callbacks
        for (auto &callback: shutDownCallbacks)
            callback(window);
    };
    manager->destroyedEvent = onShutdown;


    auto onMaximized = [this](TinyWindow::tWindow *window) {
        // dispatch callbacks
        for (auto &callback: maximizedCallbacks)
            callback(window);
    };
    manager->maximizedEvent = onMaximized;

    auto onMinimized = [this](TinyWindow::tWindow *window) {
        // dispatch callbacks
        for (auto &callback: minimizedCallbacks)
            callback(window);
    };
    manager->minimizedEvent = onMinimized;

    auto onFocusChanged = [this](TinyWindow::tWindow *window, bool isFocused) {
        // dispatch callbacks
        for (auto &callback: focusCallbacks)
            callback(window, isFocused);
    };
    manager->focusEvent = onFocusChanged;

    auto onWindowMoved = [this](TinyWindow::tWindow *window, const TinyWindow::vec2_t<int> &windowPosition) {
        // dispatch callbacks
        for (auto &callback: windowMoveCallbacks)
            callback(window, windowPosition);
    };
    manager->movedEvent = onWindowMoved;

    auto onFileDrop = [this](TinyWindow::tWindow *window, const std::vector<std::string> &files,
                             const TinyWindow::vec2_t<int> &windowMousePosition) {
        // dispatch callbacks
        for (auto &callback: fileDropCallbacks)
            callback(window, files, windowMousePosition);
    };
    manager->fileDropEvent = onFileDrop;
}

Hmck::Window::~Window() {
    manager->ShutDown();
    TinyWindow::tWindow *tempWindow = window.release();
    delete tempWindow;
    vkDestroySurfaceKHR(instance.getInstance(), getSurface(), nullptr);
}

void Hmck::Window::printMonitorInfo() {
    for (auto monitorIter: manager->GetMonitors()) {
        printf("%s \n", monitorIter->deviceName.c_str());
        printf("%s \n", monitorIter->monitorName.c_str());
        printf("%s \n", monitorIter->displayName.c_str());
        printf("resolution:\t current width: %i | current height: %i \n", monitorIter->currentSetting->resolution.width,
               monitorIter->currentSetting->resolution.height);
        printf("extents:\t top: %i | left: %i | bottom: %i | right: %i \n", monitorIter->extents.top,
               monitorIter->extents.left, monitorIter->extents.bottom, monitorIter->extents.right);
        for (auto settingIter: monitorIter->settings) {
            printf("width %i | height %i | frequency %i | pixel depth: %i",
                   settingIter->resolution.width, settingIter->resolution.height, settingIter->displayFrequency,
                   settingIter->bitsPerPixel);
#if defined(TW_WINDOWS)
            printf(" | flags %i", settingIter->displayFlags);
            switch (settingIter->fixedOutput) {
                case DMDFO_DEFAULT: {
                    printf(" | output: %s", "default");
                    break;
                }

                case DMDFO_CENTER: {
                    printf(" | output: %s", "center");
                    break;
                }

                case DMDFO_STRETCH: {
                    printf(" | output: %s", "stretch");
                    break;
                }
            }
#endif
            printf("\n");
        }
        printf("\n");
    }
}

void Hmck::Window::pollEvents() {
    keyMap.clear();
    buttonMap.clear();
    manager->PollForEvents();
}

bool Hmck::Window::isKeyDown(unsigned int key) const {
    return keyMap.contains(key) && keyMap.at(key) == KeyState::down;
}

bool Hmck::Window::isButtonDown(MouseButton button) const {
    return buttonMap.contains(button) && buttonMap.at(button) == ButtonState::down;
}

bool Hmck::Window::isKeyUp(unsigned int key) const {
    return keyMap.contains(key) && keyMap.at(key) == KeyState::up;
}

bool Hmck::Window::isButtonUp(MouseButton button) const {
    return buttonMap.contains(button) && buttonMap.at(button) == ButtonState::up;
}
