#include "HmckWindow.h"

void Hmck::HmckWindow::initWindow(int width, int height)
{
	if (!glfwInit())
	{
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

void Hmck::HmckWindow::framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
	auto hmckWindow = reinterpret_cast<HmckWindow*>(glfwGetWindowUserPointer(window));
	hmckWindow->framebufferResized = true;
	hmckWindow->width = width;
	hmckWindow->height = height;
}

Hmck::HmckWindow::HmckWindow(int windowWidth, int windowHeight, std::string _windowName) : width{ windowWidth }, height{ windowHeight }, windowName{ _windowName }
{
	initWindow(width, height);
	inputHandler.setWindow(window);
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

void Hmck::HmckWindow::setCursorVisibility(bool visible)
{
	glfwSetInputMode(window, GLFW_CURSOR, (visible)? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
}

void Hmck::HmckWindow::getCursorPosition(double& x, double& y)
{
	glfwGetCursorPos(window, &x, &y);
}

void Hmck::HmckWindow::setWindowMode(HmckWindowMode mode)
{
	const GLFWvidmode* vidmode = glfwGetVideoMode(monitors[monitorIndex]);
	if (mode == HMCK_WINDOW_MODE_FULLSCREEN)
	{
		// fullscreen
		glfwSetWindowMonitor(window, monitors[monitorIndex], 0, 0 ,width, height, vidmode->refreshRate);
	}
	else if (mode == HMCK_WINDOW_MODE_BORDERLESS)
	{
		// TODO doesn't work
		// borderless fullscreen
		glfwWindowHint(GLFW_RED_BITS, vidmode->redBits);
		glfwWindowHint(GLFW_GREEN_BITS, vidmode->greenBits);
		glfwWindowHint(GLFW_BLUE_BITS, vidmode->blueBits);
		glfwWindowHint(GLFW_REFRESH_RATE, vidmode->refreshRate);
		glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
		glfwSetWindowMonitor(window, nullptr, 0, 0, vidmode->width, vidmode->height, vidmode->refreshRate);
		
	}
	else if (mode == HMCK_WINDOW_MODE_WINDOWED)
	{
		// windowed
		glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
		glfwSetWindowMonitor(window, nullptr, 450, 450, width, height, vidmode->refreshRate);
	}

	
}

void Hmck::HmckWindow::setWindowResolution(uint32_t resX, uint32_t resY)
{
	width = resX;
	height = resY;
	glfwSetWindowSize(window, resX, resY);
}
