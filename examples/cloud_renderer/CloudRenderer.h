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

    void prepareRenderPasses();


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
        HmckVec2 resolution{0.5f, 0.5f}; // SwapChain relative
        std::unique_ptr<Framebuffer> framebuffer;
        std::unique_ptr<GraphicsPipeline> pipeline;
        std::array<ResourceHandle<VkDescriptorSet>, SwapChain::MAX_FRAMES_IN_FLIGHT> descriptorSets;
        std::array<ResourceHandle<Buffer>, SwapChain::MAX_FRAMES_IN_FLIGHT> cameraBuffers;
        std::array<ResourceHandle<Buffer>, SwapChain::MAX_FRAMES_IN_FLIGHT> cloudBuffers;
        ResourceHandle<DescriptorSetLayout> descriptorSetLayout;
        ResourceHandle<Texture3D> noiseVolumeHandle;
        ResourceHandle<Texture3D> cloudSdfHandle;
    } cloudPass;

    struct {
        std::unique_ptr<GraphicsPipeline> pipeline;
        std::array<ResourceHandle<VkDescriptorSet>, SwapChain::MAX_FRAMES_IN_FLIGHT> descriptorSets;
        ResourceHandle<DescriptorSetLayout> descriptorSetLayout;
    } compositionPass;


    // Global resources
    ResourceHandle<Buffer> vertexBuffer;
    ResourceHandle<Buffer> indexBuffer;

    // Shader data
    struct CameraBuffer {
        HmckMat4 view;
        HmckMat4 proj;
        HmckVec4 cameraPosition;
        int width;
        int height;
    } cameraBuffer;

    struct CloudBuffer {
        HmckVec4 lightDir = {-26.f, 6.0f, 0.0f, 0.0f}; // W is padding
        HmckVec4 lightColor = {1.0f, 0.9f, 0.8f, 1.0f};
        HmckVec4 baseSkyColor = {0.7f, 0.7f, 0.90, 1.0f};
        HmckVec4 gradientSkyColor = {0.90f, 0.75f, 0.90f, 0.8f};
        float stepSize = 0.01f;
        int maxSteps = 300;
        float lsMul = 5.0f;
        int maxLs = 6;
    } cloudBuffer;

    struct PushConstants {
        HmckMat4 cloudTransform;
        float density = 1.0f;
        float absorption = 0.9f;
        float scatteringAniso = 0.3f;
    } pushConstants;

    static std::string assetPath(const std::string &asset) {
        return "../../../data/" + asset;
    }

    static std::string compiledShaderPath(const std::string &shader) {
        return "../../../src/hammock/shaders/compiled/" + shader + ".spv";
    }

    // Camera
    float radius{2.0f}, azimuth{0.0f}, elevation{0.0f};
    float frameTime;
    HmckVec4 cameraPosition = {0.f, 0.f, 6.f, 0.f};
    HmckVec3 cloudTranslation = {0.f, 0.f, 0.f};
    HmckVec4 cloudRotation = {0.f, 0.f, 0.f, 1.f};
};


#endif //CLOUDRENDERER_H
