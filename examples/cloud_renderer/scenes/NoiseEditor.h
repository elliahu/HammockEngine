#pragma once
#include "IScene.h"
#include <thread>

class NoiseEditor final : public IScene {

    float frametime{0.f};
    uint32_t texWidth, texHeight, texDepth;
    float animateStepsize = 0.1f;
    bool animate = false;

    int32_t seed = 42;

    // Resource release queue
    std::queue<ResourceHandle> releaseQueue;

    // Storage buffer

    bool valuesChanged{false};
    bool transferRequested{false};
    bool transferQueued{false};
    const HmckVec3 maxGridSize = {200.f, 200.f, 200.f};
    ResourceHandle pointsBuffer;
    ResourceHandle exportBufferHandle;

    // Compute pipeline
    std::unique_ptr<ComputePipeline> computePipeline;
    std::unique_ptr<GraphicsPipeline> graphicsPipeline;

    int selectedChannel = 0;

    struct GraphicsPushData {
        int channel = 0;
        float slice{0.f};
        float scale{1.0};
    } graphicsPushData;

    struct ComputePushData {
        HmckVec4 cellSize;
        HmckVec4 gridSize{10.f,10.f,10.f, 0.0f};
        int numOctaves = 1;
        float persistence = 0.5f;
        float lacunarity = 1.5f;
        float fallOff = 1.0f;
        int channel = 0;
    };

    std::unordered_map<int, ComputePushData> computePushData;


    ResourceHandle generateWorleyPointsBuffer(int gridSizeX, int gridSizeY, int gridSizeZ);


public:
    NoiseEditor(const std::string &name, uint32_t width, uint32_t height, uint32_t texWidth, uint32_t texHeight, uint32_t texDepth)
        : IScene(name, width, height), texWidth(texWidth), texHeight(texHeight), texDepth(texDepth) {
        init();
        device.waitIdle();
    }

    void init() override;

    // Builds the rendergraph
    void buildRenderGraph();

    // Builds pipelines for each pass
    void buildPipelines();

    void update();

    void render() override;
};
