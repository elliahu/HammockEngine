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
        HmckVec2 resolution{0.5f, 0.5f}; // SwapChain relative, half resolution for better performance
        std::unique_ptr<Framebuffer> framebuffer;
        std::unique_ptr<GraphicsPipeline> pipeline;
        std::array<ResourceHandle<VkDescriptorSet>, SwapChain::MAX_FRAMES_IN_FLIGHT> descriptorSets;
        std::array<ResourceHandle<Buffer>, SwapChain::MAX_FRAMES_IN_FLIGHT> cloudBuffers;
        ResourceHandle<DescriptorSetLayout> descriptorSetLayout;
        ResourceHandle<Texture3D> noiseVolumeHandle;
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
        HmckVec4 lightDir = {-1.f, 1.0f, 0.5f, 0.0f}; // W is padding
        HmckVec4 lightColor = {1.0f, 0.9f, 0.8f, 1.0f};
        int width;
        int height;
    } cameraBuffer;

    struct CloudBuffer {
        HmckVec4 offset = {0.0f, 0.0f, 0.0f, 0.0f}; // W is padding
        float scale = 72.0f;
        float densityThreshold = 0.5f;
        int numSteps = 100;     // max steps
        float lightStepMult = 1.0f;     // light step multiplier
        int maxLightSteps = 4;  // max light steps
        float elapsedTime = 0.0f;   // elapsed time
        float density = 5.0f; // density multiplier
        float absorption = 0.1f;    // absorption coef
        float forwardScattering= 0.3f;   // scattering coef
        float backwardScattering= -0.3f;
    } cloudBuffer;

    struct PushConstants {
        HmckVec4 bb1 = {1.f, 0.f, 1.f};
        HmckVec4 bb2 = {-1.f, 1.f, -1.f};
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
