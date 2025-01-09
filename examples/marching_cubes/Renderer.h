#pragma once
#include <hammock/hammock.h>
#include "Particle.h"


class Renderer {
public:
    struct PushData {
        float elapsedTime;
    } pushData;
    struct BufferData {
        alignas(16) HmckMat4 mvp{};
        alignas(16) HmckVec4 lightPos {100.f,100.f, 100.f};
    } bufferData;

    Renderer();

    void draw();


private:

    static std::string assetPath(const std::string& asset) {
        return "../../../data/" + asset;
    }

    static std::string compiledShaderPath(const std::string& shader) {
        return "../../../src/hammock/shaders/compiled/" + shader + ".spv";
    }

    void loadSph();
    void init();
    void drawUi();

    hammock::VulkanInstance instance;
    hammock::Window window;
    hammock::Device device;
    hammock::DeviceStorage deviceStorage{device};
    hammock::RenderContext renderContext{window, device};
    hammock::UserInterface ui{device, renderContext.getSwapChainRenderPass(), deviceStorage.getDescriptorPool(), window};


    std::vector<hammock::ResourceHandle<hammock::Buffer>> vertexBuffers;
    std::vector<std::vector<hammock::Vertex>> vertices;

    std::vector<hammock::ResourceHandle<VkDescriptorSet>> descriptorSets{};
    hammock::ResourceHandle<hammock::DescriptorSetLayout> descriptorSetLayout;
    std::vector<hammock::ResourceHandle<hammock::Buffer>> buffers{};

    std::unique_ptr<hammock::GraphicsPipeline> pipeline{};

    HmckVec3 cameraPosition{-24.0f , 40.f, 40.f};
    HmckVec3 cameraTarget{0.0f, 0.0f, 0.0f};
    float azimuth{2.4f}, radius{40.0f}, elevation{0.56f};
    bool orbit{true};
    float isovalue = .001f; // Threshold value for surface extraction
    float cubeSize = .00001f; // Size of the cubes in the marching cubes algorithm
    int vertexBufferId = 0;
    bool loop{true};
    int framing = 30;
};