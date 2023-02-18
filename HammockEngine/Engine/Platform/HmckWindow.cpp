#include "HmckWindow.h"

void Hmck::HmckWindow::initWindow()
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	window = glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);
	glfwSetWindowUserPointer(window, this);
	glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}

void Hmck::HmckWindow::framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
	auto hmckWindow = reinterpret_cast<HmckWindow*>(glfwGetWindowUserPointer(window));
	hmckWindow->framebufferResized = true;
	hmckWindow->width = width;
	hmckWindow->height = height;
}

Hmck::HmckWindow::HmckWindow(int windowWidth, int windowHeight, std::string _windowName) : width{windowWidth}, height{windowHeight}, windowName{_windowName}
{
	initWindow();
}

Hmck::HmckWindow::~HmckWindow()
{
	glfwDestroyWindow(window);
	glfwTerminate();
}

bool Hmck::HmckWindow::shouldClose()
{
	return glfwWindowShouldClose(window);
}

void Hmck::HmckWindow::createWindowSurface(VkInstance instance, VkSurfaceKHR* surface)
{
	if (glfwCreateWindowSurface(instance, window, nullptr, surface) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create window surface");
	}
}
