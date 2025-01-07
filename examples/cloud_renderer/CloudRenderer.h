#ifndef CLOUDRENDERER_H
#define CLOUDRENDERER_H
#include <cstdint>

#include <hammock/hammock.h>

class CloudRenderer {
public:
    CloudRenderer(const int32_t width, const int32_t height);

    void run();

protected:


private:
    Hammock::VulkanInstance instance;
    Hammock::Window window;
    Hammock::Device device;
    Hammock::DeviceStorage deviceStorage{device};
    Hammock::Geometry geometry{};
    Hammock::RenderContext renderContext{window, device};
    Hammock::UserInterface ui{device, renderContext.getSwapChainRenderPass(), deviceStorage.getDescriptorPool(), window};
};



#endif //CLOUDRENDERER_H
