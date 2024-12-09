#include "HmckWindow.h"
#include <vulkan/vulkan.h>
#include <stdexcept>
#include <utility>

#include "core/HmckVulkanInstance.h"


void Hmck::Window::initWindow(const int width, const int height) {
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    monitors = glfwGetMonitors(&monitorsCount);
    window = glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    setWindowMode(HMCK_WINDOW_MODE_WINDOWED);
}

void Hmck::Window::framebufferResizeCallback(GLFWwindow *window, int width, int height) {
    auto window_ = reinterpret_cast<Window *>(glfwGetWindowUserPointer(window));
    window_->framebufferResized = true;
    window_->width = width;
    window_->height = height;
    window_->trigger(WINDOW_RESIZED, nullptr);
}



Hmck::Window::Window(const int windowWidth, const int windowHeight, std::string _windowName) : width{windowWidth},
                                                                                               height{windowHeight}, windowName{std::move(_windowName)} {
    initWindow(width, height);
    inputHandler.setWindow(window);
}

Hmck::Window::~Window() {
    glfwDestroyWindow(window);
    glfwTerminate();
    //vkDestroySurfaceKHR(instance, surface_, nullptr);
}

bool Hmck::Window::shouldClose() const {
    return glfwWindowShouldClose(window);
}

void Hmck::Window::createWindowSurface(VulkanInstance &instance){
    if (glfwCreateWindowSurface(instance.getInstance(), window, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface");
    }
}

void Hmck::Window::setCursorVisibility(const bool visible) const {
    glfwSetInputMode(window, GLFW_CURSOR, (visible) ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
}

void Hmck::Window::getCursorPosition(double &x, double &y) const {
    glfwGetCursorPos(window, &x, &y);
}

void Hmck::Window::setWindowMode(const WindowMode mode) const {
    const GLFWvidmode *vidmode = glfwGetVideoMode(monitors[monitorIndex]);
    if (mode == HMCK_WINDOW_MODE_FULLSCREEN) {
        // fullscreen
        glfwSetWindowMonitor(window, monitors[monitorIndex], 0, 0, width, height, vidmode->refreshRate);
    } else if (mode == HMCK_WINDOW_MODE_BORDERLESS) {
        // FIXME doesn't work
        // borderless fullscreen
        glfwWindowHint(GLFW_RED_BITS, vidmode->redBits);
        glfwWindowHint(GLFW_GREEN_BITS, vidmode->greenBits);
        glfwWindowHint(GLFW_BLUE_BITS, vidmode->blueBits);
        glfwWindowHint(GLFW_REFRESH_RATE, vidmode->refreshRate);
        glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
        glfwSetWindowMonitor(window, nullptr, 0, 0, vidmode->width, vidmode->height, vidmode->refreshRate);
    } else if (mode == HMCK_WINDOW_MODE_WINDOWED) {
        // windowed
        glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
        glfwSetWindowMonitor(window, nullptr, 450, 450, width, height, vidmode->refreshRate);
    }
}

void Hmck::Window::setWindowResolution(const uint32_t resX, const uint32_t resY) {
    width = resX;
    height = resY;
    glfwSetWindowSize(window, resX, resY);
}
