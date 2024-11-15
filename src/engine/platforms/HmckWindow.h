#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string>
#include <stdexcept>

#include "HmckInputHandler.h"

namespace Hmck
{
	enum WindowMode 
	{
		HMCK_WINDOW_MODE_FULLSCREEN,
		HMCK_WINDOW_MODE_BORDERLESS,
		HMCK_WINDOW_MODE_WINDOWED
	};

	class Window
	{
	public:
		Window(int windowWidth, int windowHeight, std::string _windowName);
		~Window();

		// delete copy constructor and copy destructor
		Window(const Window&) = delete;
		Window& operator=(const Window&) = delete;

		std::string getWindowName() { return windowName; }
		bool shouldClose();
		VkExtent2D getExtent() { return { static_cast<uint32_t>(width), static_cast<uint32_t>(height) }; }
		void createWindowSurface(VkInstance instance, VkSurfaceKHR* surface);
		bool wasWindowResized(){ return framebufferResized; }
		void resetWindowResizedFlag() { framebufferResized = false; }
		GLFWwindow* getGLFWwindow() const { return window; }
		void pollEvents() { glfwPollEvents(); }
		InputManager& getInputManager() { return inputHandler; }
		void setCursorVisibility(bool visible);
		void getCursorPosition(double& x, double& y);
		void setWindowMode(WindowMode mode);
		void setWindowResolution(uint32_t resX, uint32_t resY);

	private:
		GLFWwindow* window;
		int monitorsCount;
		GLFWmonitor** monitors = nullptr;
		int monitorIndex = 0;
		int width;
		int height;
		bool framebufferResized = false;
		std::string windowName;

		InputManager inputHandler;

		void initWindow(int width, int height);
		static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
	};
} 


