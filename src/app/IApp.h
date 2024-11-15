#pragma once
#include <platforms/HmckWindow.h>
#include <HmckDevice.h>
#include <HmckRenderer.h>
#include <HmckMemory.h>

namespace Hmck
{
	class IApp
	{
	public:
		static constexpr int WINDOW_WIDTH = 1920;
		static constexpr int WINDOW_HEIGHT = 1080;

		IApp(){}

		// delete copy constructor and copy destructor
		IApp(const IApp&) = delete;
		IApp& operator=(const IApp&) = delete;

		virtual void run() = 0;

	protected:
		
		virtual void load() = 0;

		Window window{ WINDOW_WIDTH, WINDOW_HEIGHT, "Hammock Engine" };
		Device device{ window }; // TODO this is wrong, device should not be dependent on window -> should be otherway around
		MemoryManager memoryManager{ device };
	};
}