#pragma once
#include <io/HmckWindow.h>
#include <core/HmckDevice.h>
#include <core/HmckDeviceStorage.h>

#include "core/HmckVulkanInstance.h"
#include "scene/HmckGeometry.h"

namespace Hmck {
    class IApp {
    public:

        static constexpr int WINDOW_WIDTH = 1920;
        static constexpr int WINDOW_HEIGHT = 1080;

        IApp()
            : device(instance, [&] {
                  window.createWindowSurface(instance);
                  return window.getSurface();
              }()) {
        }

        virtual ~IApp() {
            vkDestroySurfaceKHR(instance.getInstance(), window.getSurface(), nullptr);
        }

        // delete copy constructor and copy destructor
        IApp(const IApp &) = delete;

        IApp &operator=(const IApp &) = delete;

        virtual void run() = 0;

    protected:
        virtual void load() = 0;
        Window window{WINDOW_WIDTH, WINDOW_HEIGHT, "Hammock Engine"};
        VulkanInstance instance;
        Device device;
        DeviceStorage deviceStorage{device};
        Geometry state{};
    };
}
