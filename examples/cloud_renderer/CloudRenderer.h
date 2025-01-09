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
    hammock::VulkanInstance instance;
    hammock::Window window;
    hammock::Device device;
    hammock::DeviceStorage deviceStorage{device};
    hammock::Geometry geometry{};
    hammock::RenderContext renderContext{window, device};
    hammock::UserInterface ui{device, renderContext.getSwapChainRenderPass(), deviceStorage.getDescriptorPool(), window};
};



#endif //CLOUDRENDERER_H
