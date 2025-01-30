#ifndef CLOUDRENDERER_H
#define CLOUDRENDERER_H
#include <cstdint>

#include <hammock/hammock.h>
#include "PerlinNoise3D.h"
#include "BoundingBox.h"
#include "SignedDistanceField.h"

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
    std::array<ResourceHandle<Buffer>, SwapChain::MAX_FRAMES_IN_FLIGHT> cameraBuffers;
    std::array<ResourceHandle<Buffer>, SwapChain::MAX_FRAMES_IN_FLIGHT> cloudBuffers;
    ResourceHandle<DescriptorSetLayout> descriptorSetLayout;

    // Resources
    ResourceHandle<Buffer> vertexBuffer;
    ResourceHandle<Buffer> indexBuffer;
    ResourceHandle<Texture3D> noiseVolumeHandle;
    ResourceHandle<Texture3D> cloudSdfHandle;


    // Shader data
    struct CameraBuffer {
        HmckMat4 view;
        HmckMat4 proj;
        HmckVec4 cameraPosition;
        int width;
        int height;
    } cameraBuffer;

    struct CloudBuffer {
        HmckVec4 lightDir = {1.0f, 1.0f, -1.0f, 0.0f}; // W is padding
        HmckVec4 lightColor = {1.0f, 0.9f, 0.8f, 1.0f}; // W is padding

        float density = 1.0f;
        float absorption = 0.5f;
        float phase = 0.3f;
        float stepSize = 0.01f;
        int maxSteps = 100;
        float lsMul = 5.0f;
        int maxLs = 6;
    } cloudBuffer;

    struct PushConstants {
        HmckMat4 cloudTransform;
    } pushConstants;

    static std::string assetPath(const std::string &asset) {
        return "../../../data/" + asset;
    }

    static std::string compiledShaderPath(const std::string &shader) {
        return "../../../src/hammock/shaders/compiled/" + shader + ".spv";
    }

    // Camera
    float radius{2.0f}, azimuth, elevation;
    float frameTime;
    HmckVec4 cameraPosition = {0.f, 0.f, 6.f, 0.f};
    HmckVec3 cloudTranslation = {0.f, 0.f, 0.f};
    HmckVec4 cloudRotation = {0.f, 0.f, 0.f, 1.f};
};


#endif //CLOUDRENDERER_H
