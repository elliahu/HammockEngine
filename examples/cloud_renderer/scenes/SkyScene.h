#include "IScene.h"

class SkyScene final : public IScene {
    // Compute work group and local size computation
    static constexpr uint32_t WORKGROUP_SIZE_X = 32;
    static constexpr uint32_t WORKGROUP_SIZE_Y = 32;
    static constexpr uint32_t groupsX = (1920 + WORKGROUP_SIZE_X - 1) / WORKGROUP_SIZE_X;
    static constexpr uint32_t groupsY = (1080 + WORKGROUP_SIZE_Y - 1) / WORKGROUP_SIZE_Y;

    // Uniform data updated every frame
    // Lives in a GPU memory that is visible to CPU
    struct UniformBufferData {
        // Camera related data are stored here
        struct Camera {
            HmckMat4 inverseView; // 64 bytes
            HmckVec4 position{0.0f, 0.0f, 0.0f}; // 16 bytes
        } camera{};

        struct Sun {
            HmckVec4 position{0.0f,20.0f, 0.0f, 0.0f}; // 16 bytes
            HmckVec4 color{1.0f, 1.0f, 1.0f, 1.0f}; // 16 bytes
        } sun{};

        struct Rendering {
            float absorptionCoef{0.23f};
            float scatteringCoef{0.28f};
            float phase{0.3f};
        } rendering;
    } uniformBufferData;

    // Storage buffer data that remains static during execution
    // Lives in a dedicated GPU memory that is NOT accessible by CPU
    struct StorageBufferData {
        HmckVec4 bbMin{-20.f, 5.f, -20.f};
        HmckVec4 bbMax{20.f, 15.f, 20.f};
    } storageBufferData;


    // Compute pass resources
    struct {
        // Compute pipeline to draw the clouds in parallel patches
        std::unique_ptr<ComputePipeline> pipeline;

        // Storage buffer resource handle
        ResourceHandle storageBuffer;

        // Base cloud noise
        ResourceHandle baseNoise;

        // Detail cloud noise
        ResourceHandle detailNoise;

        // Curl cloud noise
        ResourceHandle curlNoise;

        // Cloud map
        ResourceHandle cloudMap;

        // Other resources are managed on-the-fly by the rendergraph
    } compute;

    // Composition pass resources
    struct {
        // Graphics pipeline that is used to compose the final image and write it to the swapchain image
        std::unique_ptr<GraphicsPipeline> pipeline;
        // Other resources are managed on-the-fly by the rendergraph
    } composition;

    // This is used to measure frame time
    float frameTime = 0.0f;
    float yaw{0.f}, pitch{0.847f}; // This describes camera look direction relative to the planet surface normal vector (standing on surface and looking)
    HmckVec3 cameraPosition{-40.0f, -35.0f, 0.0f};

    // Benchmarking
    // Constants
    static constexpr int FRAMETIME_BUFFER_SIZE = 512; // Number of frames to track
    // Variables
    float frameTimes[FRAMETIME_BUFFER_SIZE] = { 0.0f };
    int frameTimeFrameIndex = 0;

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

    // This gets called every frame and updates the data that is then passed to the uniform buffer
    void update();

    // Render loop
    void render() override;
};
