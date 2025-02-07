#ifndef PARTICIPATINGMEDIUMTESTSCENE_H
#define PARTICIPATINGMEDIUMTESTSCENE_H
#include <cstdint>

#include <hammock/hammock.h>
#include "../BoundingBox.h"
#include "../SignedDistanceField.h"
#include "../Noise3D.h"

using namespace Hammock;

class ParticipatingMediumTestScene {
public:
    ParticipatingMediumTestScene(const int32_t width, const int32_t height);

    ~ParticipatingMediumTestScene();

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
        HmckVec4 lightDir = {-1.4f, -0.3f, -0.8f, 0.0f}; // W is padding
        HmckVec4 lightColor = {1.0f, 0.9f, 0.8f, 1.0f};
        HmckVec4 baseSkyColor = {0.7f, 0.7f, 0.90, 1.0f};
        HmckVec4 gradientSkyColor = {0.90f, 0.75f, 0.90f, 0.2f};
        int width;
        int height;
        float sunFactor = 0.3;
        float sunExp = 20.0;
    } cameraBuffer;

    struct CloudBuffer {


        float stepSize = 0.01f; // size of single raymarching step
        int maxSteps = 300;     // max steps
        float lsMul = 1.0f;     // light step multiplier
        int maxLs = 4;  // max light steps
        float elapsedTime = 0.0f;   // elapsed time
        float noiseScale = 2.0f;
        float noiseLowerCutoff = 0.3f;
        float noiseHigherCutoff = 1.0f;
        float density = 1.0f; // density multiplier
        float absorption = 0.1f;    // absorption coef
        float scatteringAniso = 0.3f;   // scattering coef
        float scatteringIso = -0.3f;   // scattering coef
        float scatteringBlend = 0.7;
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
    float radius{2.0f}, azimuth{0.0f}, elevation{0.0f};
    float frameTime;
    HmckVec4 cameraPosition = {0.f, 0.f, 6.f, 0.f};
    HmckVec3 cloudTranslation = {0.f, 0.f, 0.f};
    HmckVec4 cloudRotation = {0.f, 0.f, 0.f, 1.f};
};


#endif //CLOUDRENDERER_H
