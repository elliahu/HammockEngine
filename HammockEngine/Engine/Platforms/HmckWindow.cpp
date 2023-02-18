#include "HmckWindow.h"
#include <iostream>
#include <windows.h>

void Hmck::HmckWindow::initWindow()
{
	// TODO make platform independent
	// this make sure that on windows, when scaling is enabled, window doesn look ugly
	SetProcessDPIAware();

	// Doesn't work on Windows ???
	//bool result = SDL_SetHintWithPriority(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "1", SDL_HINT_OVERRIDE);
	SDL_Init(SDL_INIT_EVERYTHING);
	window = SDL_CreateWindow(
		windowName.c_str(), 
		SDL_WINDOWPOS_CENTERED, 
		SDL_WINDOWPOS_CENTERED, 
		width, height, 
		SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
}

Hmck::HmckWindow::HmckWindow(
	int windowWidth, 
	int windowHeight, 
	std::string _windowName) : width{windowWidth}, height{windowHeight}, windowName{_windowName}
{
	initWindow();
}

Hmck::HmckWindow::~HmckWindow()
{
	SDL_DestroyWindow(window);
	window = nullptr;

	SDL_Quit();
}

void Hmck::HmckWindow::createWindowSurface(VkInstance instance, VkSurfaceKHR* surface)
{
	SDL_Vulkan_CreateSurface(window, instance, surface);
}

void Hmck::HmckWindow::pollEvents()
{
	inputManager.clearEvents();

	while (SDL_PollEvent(&event))
	{
		// Send event to input manager to handle
		inputManager.addEvent(event);

		// send event to UI library as well
		ImGui_ImplSDL2_ProcessEvent(&event);

		// request to quit
		if (event.type == SDL_QUIT)
		{
			running = false;
			break;
		}
	}
}

void Hmck::HmckWindow::waitEvents()
{
	SDL_WaitEvent(&event);
}

void Hmck::HmckWindow::setCursorVisibility(bool visible)
{
	SDL_ShowCursor((visible) ? SDL_ENABLE : SDL_DISABLE);
}

void Hmck::HmckWindow::getCursorPosition(int& x, int& y)
{
	SDL_GetMouseState(&x,&y);
}

bool Hmck::HmckWindow::isMinimized()
{
	Uint32 flags = SDL_GetWindowFlags(window);
	return (flags & SDL_WINDOW_MINIMIZED);
}

bool Hmck::HmckWindow::isMaximized()
{
	Uint32 flags = SDL_GetWindowFlags(window);
	return (flags & SDL_WINDOW_MAXIMIZED);
}
