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

    void init();
    void drawUi();

    Hammock::VulkanInstance instance;
    Hammock::Window window;
    Hammock::Device device;
    Hammock::DeviceStorage deviceStorage{device};
    Hammock::Geometry geometry{};
    Hammock::RenderContext renderContext{window, device};
    Hammock::UserInterface ui{device, renderContext.getSwapChainRenderPass(), deviceStorage.getDescriptorPool(), window};


    Hammock::ResourceHandle<Hammock::Buffer> vertexBuffer;

    std::vector<Hammock::ResourceHandle<VkDescriptorSet>> descriptorSets{};
    Hammock::ResourceHandle<Hammock::DescriptorSetLayout> descriptorSetLayout;
    std::vector<Hammock::ResourceHandle<Hammock::Buffer>> buffers{};

    std::unique_ptr<Hammock::GraphicsPipeline> pipeline{};

    HmckVec3 cameraPosition{-24.0f , 40.f, 40.f};
    HmckVec3 cameraTarget{0.0f, 0.0f, 0.0f};
    float azimuth{2.4f}, radius{40.0f}, elevation{0.56f};
    bool orbit{true};
    float isovalue = 7.0f; // Threshold value for surface extraction
    float cubeSize = .01f; // Size of the cubes in the marching cubes algorithm
};