#include "IScene.h"

class SkyScene final : public IScene {
    // Compute work group and local size computation
    static constexpr uint32_t WORKGROUP_SIZE_X = 16;
    static constexpr uint32_t WORKGROUP_SIZE_Y = 16;
    static constexpr uint32_t groupsX = (1920 + WORKGROUP_SIZE_X - 1) / WORKGROUP_SIZE_X;
    static constexpr uint32_t groupsY = (1080 + WORKGROUP_SIZE_Y - 1) / WORKGROUP_SIZE_Y;

    // Uniform data updated every frame
    // Lives in a GPU memory that is visible to CPU
    struct UniformBufferData {
        // Camera related data are stored here
        struct Camera {
            HmckMat4 inverseView; // 64 bytes
            HmckVec4 position{0.0f, 638.8f, 0.0f}; // 16 bytes
        } camera{};

        struct Sun {
            HmckVec4 direction{0.0f,1.0f, 0.0f, 0.0f}; // 16 bytes
            HmckVec4 color{1.0f, 1.0f, 1.0f, 1.0f}; // 16 bytes
        } sun{};
    } uniformBufferData;

    // Storage buffer data that remains static during execution
    // Lives in a dedicated GPU memory that is NOT accessible by CPU
    struct StorageBufferData {
        // Radius of out planet (earth for example)
        float surfaceRadius = 637.8f; // 4 bytes

        // radius of atmosphere surrounding out planet
        float atmosphereRadius = 1000.0f; // 4 bytes

        // Height above the surface where clouds start to appear
        float cloudMinHeight = 150.f;

        // Height above the surface where clouds stop to appear
        float cloudMaxHeight = 450.f;

    } storageBufferData;


    // Compute pass resources
    struct {
        // Compute pipeline to draw the clouds in parallel patches
        std::unique_ptr<ComputePipeline> pipeline;

        // Storage buffer resource handle
        ResourceHandle storageBuffer;

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
    float yaw{0.f}, pitch{0.f}; // This describes camera look direction relative to the planet surface normal vector (standing on surface and looking)
    float elevation{0.f}, azimuth{0.f}, radius{638.8}; // This describes camera position on the surface using spherical coordinates

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
