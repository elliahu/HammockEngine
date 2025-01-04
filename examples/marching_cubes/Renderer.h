#pragma once
#include <hammock/hammock.h>
#include "Particle.h"


class Renderer {
public:
    struct PushData {
        float x;
    };
    struct BufferData {
        HmckMat4 projection;
        HmckMat4 view;
    };

    Renderer();


private:

    static std::string assetPath(const std::string& asset) {
        return "../../../data/" + asset;
    }

    static std::string compiledShaderPath(const std::string& shader) {
        return "../../../src/hammock/shaders/compiled/" + shader + ".spv";
    }

    void init();

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

    Hammock::Vec3Padded cameraPosition{0.0f, 2.0f, 2.0f};
    Hammock::Vec3Padded cameraTarget{0.0f, 0.0f, 0.0f};
};