#pragma once
#include "IScene.h"

class NoiseEditor final : public IScene {

    float frametime{0.f};
    uint32_t texWidth, texHeight, texDepth;
    float animateStepsize = 0.1f;
    bool animate = true;

    int32_t seed = 42;

    // Resource release queue
    std::queue<ResourceHandle> releaseQueue;

    // Storage buffer

    bool valuesChanged{false};

    int32_t gridSizeX = 10, gridSizeY = 10, gridSizeZ = 10;
    const HmckVec3 maxGridSize = {200.f, 200.f, 200.f};
    ResourceHandle pointsBuffer;

    // Compute pipeline
    std::unique_ptr<ComputePipeline> computePipeline;
    std::unique_ptr<GraphicsPipeline> graphicsPipeline;


    struct GraphicsPushData {
        HmckVec4 displayChannels{1.f, 1.f, 1.f, 1.f};
        float slice{0.f};
        float scale{1.0};
    } graphicsPushData;

    struct ComputePushData {
        HmckVec4 cellSizeR;
        HmckVec4 gridSizeR;
        HmckVec4 cellSizeG;
        HmckVec4 gridSizeG;
        HmckVec4 cellSizeB;
        HmckVec4 gridSizeB;
        HmckVec4 cellSizeA;
        HmckVec4 gridSizeA;
    } computePushData;


    ResourceHandle generateWorleyPointsBuffer();


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
