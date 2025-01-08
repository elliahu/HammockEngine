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

    void update(float frameTime);

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
        HmckMat4 model{
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        };
        HmckVec4 cameraPosition{0.f, 0.f, 6.f, 0.0f};
        HmckVec4 lightDirection{100.f, 100.f, 0.f};
        HmckVec4 lightColor{1.f, 1.0f, 1.0f};
    } bufferData;

    struct PushData {
        float stepSize = 0.01f;
        int maxSteps = 128;
        float minDensityThreshold = 0.1f;
        float time;
    } pushData;

    static std::string assetPath(const std::string &asset) {
        return "../../../data/" + asset;
    }

    static std::string compiledShaderPath(const std::string &shader) {
        return "../../../src/hammock/shaders/compiled/" + shader + ".spv";
    }

    // Camera
    float radius{2.0f}, azimuth, elevation;
};


#endif //CLOUDRENDERER_H
