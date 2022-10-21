#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string>
#include <stdexcept>

namespace Hmck
{
	class HmckWindow
	{
	private:
		GLFWwindow* window;
		int width;
		int height;
		bool framebufferResized = false;
		std::string windowName;

		void initWindow();
		static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

	public:
		HmckWindow(int windowWidth, int windowHeight, std::string _windowName);
		~HmckWindow();

		// delete copy constructor and copy destructor
		HmckWindow(const HmckWindow&) = delete;
		HmckWindow& operator=(const HmckWindow&) = delete;

		bool shouldClose();
		VkExtent2D getExtent() { return { static_cast<uint32_t>(width), static_cast<uint32_t>(height) }; }
		void createWindowSurface(VkInstance instance, VkSurfaceKHR* surface);
		bool wasWindowResized(){ return framebufferResized; }
		void resetWindowResizedFlag() { framebufferResized = false; }
	};
} 


