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

        IApp() : device(instance, window.getSurface()) {
#ifndef NDEBUG
            window.printMonitorInfo();
#endif
        }

        virtual ~IApp() {
        }

        // delete copy constructor and copy destructor
        IApp(const IApp &) = delete;

        IApp &operator=(const IApp &) = delete;

        virtual void run() = 0;

    protected:
        virtual void load() = 0;

        VulkanInstance instance;
        Window window{instance, WINDOW_WIDTH, WINDOW_HEIGHT, "Hammock Engine"};
        Device device;
        DeviceStorage deviceStorage{device};
        Geometry state{};
    };
}
