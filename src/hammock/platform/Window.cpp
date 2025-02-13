#include "hammock/platform/Window.h"

hammock::Window::Window(VulkanInstance &instance, const std::string &_windowName, int windowWidth,
                     int windowHeight) : instance{instance}, width{windowWidth}, height {windowHeight}, windowName{_windowName} {
    window = Surfer::Window::createWindow(_windowName, instance.getInstance(), windowWidth, windowHeight , 100, 100 );
}

hammock::Window::~Window()
{
    Surfer::Window::destroyWindow(window);
}

bool hammock::Window::shouldClose() const
{
    return _shouldClose;
}

void hammock::Window::pollEvents()
{
}
