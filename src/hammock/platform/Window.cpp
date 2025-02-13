#include "hammock/platform/Window.h"

#include <iostream>

hammock::Window::Window(VulkanInstance &instance, const std::string &_windowName, int windowWidth,
                        int windowHeight) : instance{instance}, width{windowWidth}, height {windowHeight}, windowName{_windowName} {
    window = Surfer::Window::createWindow(_windowName, instance.getInstance(), windowWidth, windowHeight , 100, 100 );

    window->registerKeyPressCallback([this](Surfer::KeyCode key) {
        keysDown[key] = true;
    });

    window->registerKeyReleaseCallback([this](Surfer::KeyCode key) {
        keysDown[key] = false;
    });

    window->registerMouseMotionCallback([this](unsigned int x, unsigned int y) {

    });

    window->registerResizeCallback([this](unsigned int width, unsigned int height) {
       framebufferResized = true;
    });

    window->registerMoveCallback([this](int x, int y) {

    });

    window->registerCloseCallback([this]() {
        std::cout << "Closing..." << std::endl;
    });

    window->registerMouseEnterExitCallback([this](bool enter) {
    });

    window->registerFocusCallback([this](bool focused) {
    });
}

hammock::Window::~Window()
{
    Surfer::Window::destroyWindow(window);
}

bool hammock::Window::isKeyDown(Surfer::KeyCode keyCode) {
    if (keysDown.contains(keyCode)) {
        return keysDown[keyCode];
    }
    return false;
}

HmckVec2 hammock::Window::getMousePosition() const {
    unsigned int x,y;
    window->getCursorPosition(x,y);
    return {static_cast<float>(x),static_cast<float>(y)};
}

bool hammock::Window::shouldClose() const
{
    return window->shouldClose();
}

void hammock::Window::pollEvents()
{
    window->pollEvents();
}
