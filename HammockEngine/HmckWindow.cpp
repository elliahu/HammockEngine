#include "HmckWindow.h"

void Hmck::HmckWindow::initWindow()
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	window = glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);
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
