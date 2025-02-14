#include "IScene.h"

class SkyScene final : public IScene {
    // Compute work group and local size computation
    static constexpr uint32_t WORKGROUP_SIZE_X = 16;
    static constexpr uint32_t WORKGROUP_SIZE_Y = 16;
    static constexpr uint32_t groupsX = (1920 + WORKGROUP_SIZE_X - 1) / WORKGROUP_SIZE_X;
    static constexpr uint32_t groupsY = (1080 + WORKGROUP_SIZE_Y - 1) / WORKGROUP_SIZE_Y;

    // Uniform data updated every frame
    struct UniformBufferData {
        struct Camera {
            HmckMat4 inverseView; // 64 bytes
            HmckVec4 position{0.0f, 638.8f, 0.0f}; // 16 bytes
            HmckVec2 viewport; // 8 bytes
        } camera{};

        struct Sun {
            HmckVec4 direction{0.0f,1.0f, 0.0f, 0.0f}; // 16 bytes
            HmckVec4 color{1.0f, 1.0f, 1.0f, 1.0f}; // 16 bytes
        } sun{}; // Total 128 bytes (multiple of 16) no padding needed
    } uniformBufferData;

    // Storage buffer data that remains static during execution
    struct StorageBufferData {
        float surfaceRadius = 637.8f; // 4 bytes
        float atmosphereRadius = 800.8f; // 4 bytes
    } storageBufferData;


    // Compute pass resources
    struct {
        std::unique_ptr<ComputePipeline> pipeline;
        ResourceHandle storageBuffer;
    } compute;

    // Composition pass resources
    struct {
        std::unique_ptr<GraphicsPipeline> pipeline;
    } composition;


public:
    SkyScene(const std::string &name, const uint32_t width, const uint32_t height)
        : IScene(name, width, height) {
        init();
        // Some of the calls in inti may submit a command buffer so we need to wait for it to finish before we start rendering
        device.waitIdle();
    }

    // Initializes all resources
    void init() override;

    // Builds the rendergraph
    void buildRenderGraph();

    // Builds pipelines for each pass
    void buildPipelines();
3
    // This gets called every frame and updates the data that is then passed to the uniform buffer
    void updateUniformBuffer();

    // Render loop
    void render() override;
};
