#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string>
#include <stdexcept>

#include "HmckInputHandler.h"

namespace Hmck
{
	enum HmckWindowMode 
	{
		HMCK_WINDOW_MODE_FULLSCREEN,
		HMCK_WINDOW_MODE_BORDERLESS,
		HMCK_WINDOW_MODE_WINDOWED
	};

	class HmckWindow
	{
	public:
		HmckWindow(int windowWidth, int windowHeight, std::string _windowName);
		~HmckWindow();

		// delete copy constructor and copy destructor
		HmckWindow(const HmckWindow&) = delete;
		HmckWindow& operator=(const HmckWindow&) = delete;

		std::string getWindowName() { return windowName; }
		bool shouldClose();
		VkExtent2D getExtent() { return { static_cast<uint32_t>(width), static_cast<uint32_t>(height) }; }
		void createWindowSurface(VkInstance instance, VkSurfaceKHR* surface);
		bool wasWindowResized(){ return framebufferResized; }
		void resetWindowResizedFlag() { framebufferResized = false; }
		GLFWwindow* getGLFWwindow() const { return window; }
		void pollEvents() { glfwPollEvents(); }
		HmckInputManager& getInputManager() { return inputHandler; }
		void setCursorVisibility(bool visible);
		void getCursorPosition(double& x, double& y);
		void setWindowMode(HmckWindowMode mode);
		

	private:
		GLFWwindow* window;
		int monitorsCount;
		GLFWmonitor** monitors = nullptr;
		int monitorIndex = 0;
		int width;
		int height;
		bool framebufferResized = false;
		std::string windowName;

		HmckInputManager inputHandler;

		void initWindow(int width, int height);
		static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
	};
} 


