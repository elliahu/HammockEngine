#ifndef CLOUDBOUNDINGBOXTESTSCENE_H
#define CLOUDBOUNDINGBOXTESTSCENE_H
#include <cstdint>

#include <hammock/hammock.h>
#include "../BoundingBox.h"
#include "../SignedDistanceField.h"
#include "../Noise3D.h"

using namespace Hammock;

class CloudBoundingBoxTestScene{
public:
    CloudBoundingBoxTestScene(const int32_t width, const int32_t height);

    ~CloudBoundingBoxTestScene();

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



    // Rendering passes in order of rendering

    /**
     * This pass draws a scene into color and depth attachments
     * Order: 0
     */
    struct {
        HmckVec2 resolution{1.0f, 1.0f}; // SwapChain relative, same size as SwapChain
        std::unique_ptr<Framebuffer> framebuffer;
        std::unique_ptr<GraphicsPipeline> pipeline;
        std::array<ResourceHandle<VkDescriptorSet>, SwapChain::MAX_FRAMES_IN_FLIGHT> descriptorSets;
        std::array<ResourceHandle<Buffer>, SwapChain::MAX_FRAMES_IN_FLIGHT> cameraBuffers;
        ResourceHandle<DescriptorSetLayout> descriptorSetLayout;
    } scenePass;


    /**
     * Draws a cloud into a color attachment
     * Order: 1
     */
    struct {
        HmckVec2 resolution{.75f, .75f}; // SwapChain relative, half resolution for better performance
        std::unique_ptr<Framebuffer> framebuffer;
        std::unique_ptr<GraphicsPipeline> pipeline;
        std::array<ResourceHandle<VkDescriptorSet>, SwapChain::MAX_FRAMES_IN_FLIGHT> descriptorSets;
        std::array<ResourceHandle<Buffer>, SwapChain::MAX_FRAMES_IN_FLIGHT> cloudBuffers;
        ResourceHandle<DescriptorSetLayout> descriptorSetLayout;
        ResourceHandle<Texture3D> lowFreqNoise;
        ResourceHandle<Texture3D> highFreqNoise;
        ResourceHandle<Texture2D> gradientsHandle;
    } cloudPass;

    /**
     * This is a final composition pass that places cloud into a scene and color attachment of this pass is submitted to SwapChain
     * Order: Last
     */
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
        HmckVec4 lightDir = {-1.4f, 1.3f, 0.7f, -3.2f}; // W is padding
        HmckVec4 lightColor = {1.0f, 0.9f, 0.8f, 1.0f};
        HmckVec4 baseSkyColor = {1.f, 1.f, 1.f, 1.0f};
        HmckVec4 gradientSkyColor = {189.f / 255.f, 196.f / 255.f, 228.f / 255.f, 1.0f};
        HmckVec4 groundColor = {0.f / 255.f, 29.f / 255.f, 58.f / 255.f, 1.0f};
        int width;
        int height;
        float sunFactor = 0.3;
        float sunExp = 20.0;
    } cameraBuffer;

    struct CloudBuffer {
        // Noise
        HmckVec4 lowFreqWeights = {1.0f, 1.0f, 1.0f, 1.0f};
        HmckVec4 highFreqWeights = {1.0f, 1.0f, 1.0f, 0.0f}; // A(W) is padding
        HmckVec4 freqSamplOffset = {0.0f, 0.0f, 0.0f, 0.0f};
        float lowFreqScale = 1.0f;
        float highFreqScale = 1.0f;
        float freqSampleScale = .1f;
        float freqOffset = -.82f;
        float densityMultiplier = 6.f;

        // Raymarching
        int numSteps = 25;     // max steps
        int numLightSteps = 6;  // max light steps

        float phaseG = -0.45;
    } cloudBuffer;

    struct PushConstants {
        HmckVec4 bb1 = {2.5f, 0.f, 4.6f};
        HmckVec4 bb2 = {-4.3f, 2.5f, -3.9f};
    } pushConstants;

    static std::string assetPath(const std::string &asset) {
        return "../../../data/" + asset;
    }

    static std::string compiledShaderPath(const std::string &shader) {
        return "../../../src/hammock/shaders/compiled/" + shader + ".spv";
    }

    // Camera
    float radius{10.0f}, azimuth{0.35f}, elevation{0.35f};
    float frameTime;
    HmckVec4 cameraPosition = {0.f, 0.f, 6.f, 0.f};
    HmckVec4 cloudRotation = {0.f, 0.f, 0.f, 1.f};
};


#endif //CLOUDRENDERER_H
