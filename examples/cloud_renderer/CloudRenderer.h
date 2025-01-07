#ifndef CLOUDRENDERER_H
#define CLOUDRENDERER_H
#include <cstdint>

#include <hammock/hammock.h>
#include "PerlinNoise3D.h"

using namespace Hammock;

class CloudRenderer {
public:
    CloudRenderer(const int32_t width, const int32_t height);
    ~CloudRenderer();

    void run();

protected:

    void load();
    void prepareDescriptors();
    void preparePipelines();
    void update();
    void draw();
    void drawUi();

private:
    // Memory and context
    VulkanInstance instance;
    Hammock::Window window;
    Device device;
    DeviceStorage deviceStorage{device};
    Geometry geometry{};
    RenderContext renderContext{window, device};
    UserInterface ui{device, renderContext.getSwapChainRenderPass(), deviceStorage.getDescriptorPool(), window};

    // Rendering
    struct {
        std::unique_ptr<GraphicsPipeline> pipeline;
    } compositionPass;

    // descriptors
    std::array<ResourceHandle<VkDescriptorSet>, SwapChain::MAX_FRAMES_IN_FLIGHT> descriptorSets;
    std::array<ResourceHandle<Buffer>, SwapChain::MAX_FRAMES_IN_FLIGHT> buffers;
    ResourceHandle<DescriptorSetLayout> descriptorSetLayout;

    // Resources
    ResourceHandle<Buffer> vertexBuffer;
    ResourceHandle<Buffer> indexBuffer;
    ResourceHandle<Texture3D> noiseVolumeHandle;

    // Shader data
    struct BufferData {
        HmckMat4 projection;
        HmckMat4 view;
        HmckVec4 cameraPosition{0.f, 0.f, 2.f, 0.0f};
        HmckVec4 lightDirection{1.f, 1.f, -1.f};
        HmckVec4 lightColor{1.f, .9f, .7f};
    } bufferData;

    struct PushData {
        float stepSize = 0.01f;
        int maxSteps = 128;
        float minDensityThreshold = 0.1f;
        float time;
    } pushData;

    static std::string assetPath(const std::string& asset) {
        return "../../../data/" + asset;
    }

    static std::string compiledShaderPath(const std::string& shader) {
        return "../../../src/hammock/shaders/compiled/" + shader + ".spv";
    }

};



#endif //CLOUDRENDERER_H
