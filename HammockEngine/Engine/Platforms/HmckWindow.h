#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <stdexcept>
#include <SDL.h>
#include <SDL_vulkan.h>

#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_vulkan.h"


#include "HmckInputManager.h"

namespace Hmck
{
	class HmckWindow
	{
	public:
		HmckWindow(int windowWidth, int windowHeight, std::string _windowName);
		~HmckWindow();

		// delete copy constructor and copy destructor
		HmckWindow(const HmckWindow&) = delete;
		HmckWindow& operator=(const HmckWindow&) = delete;

		std::string getWindowName() { return windowName; }
		bool shouldClose() { return running == false; }
		VkExtent2D getExtent() { return { static_cast<uint32_t>(width), static_cast<uint32_t>(height) }; }
		void createWindowSurface(VkInstance instance, VkSurfaceKHR* surface);
		bool wasWindowResized(){ return framebufferResized; }
		void resetWindowResizedFlag() { framebufferResized = false; }
		SDL_Window* getSDL_Window() const { return window; }
		void pollEvents();
		void waitEvents();
		void setCursorVisibility(bool visible);
		void getCursorPosition(int& x, int& y);
		bool isMinimized();
		bool isMaximized();

		HmckInputManager& getInputManager() { return inputManager; }


	private:
		HmckInputManager inputManager{};
		SDL_Window* window;
		SDL_Event event;
		int width;
		int height;
		bool framebufferResized = false;
		bool running = true;
		std::string windowName;

		void initWindow();
	};
} 


