#pragma once

#include <hammock/hammock.h>

namespace Hammock {
    class IApp {
    public:
        static constexpr int WINDOW_WIDTH = 1920;
        static constexpr int WINDOW_HEIGHT = 1080;

        IApp()
            : device(instance, window.getSurface()) {
        }

        virtual ~IApp() {
        }

        // delete copy constructor and copy destructor
        IApp(const IApp &) = delete;

        IApp &operator=(const IApp &) = delete;

        virtual void run() = 0;

    protected:
        virtual void load() = 0;

        static std::string assetPath(const std::string& asset) {
            return "../../../data/" + asset;
        }

        static std::string compiledShaderPath(const std::string& shader) {
            return "../../../src/hammock/shaders/compiled/" + shader + ".spv";
        }

        VulkanInstance instance;
        Window window{instance, "Hammock Engine", WINDOW_WIDTH, WINDOW_HEIGHT};
        Device device;
        DeviceStorage deviceStorage{device};
        Geometry geometry{};
    };
}
