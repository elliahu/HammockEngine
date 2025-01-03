#pragma once
#include <memory>
#include <vector>
#include <chrono>

#include "IApp.h"

namespace Hmck {
    class VolumeApp final: public IApp {
    public:
        VolumeApp();

        void run() override;

    protected:
        void load() override;

        void draw(int frameIndex, float elapsedTime, VkCommandBuffer commandBuffer);

        void destroy();

        void ui();



        RenderContext renderContext{window, device};

        ResourceHandle<Buffer> vertexBuffer;
        ResourceHandle<Buffer> indexBuffer;

        std::vector<ResourceHandle<VkDescriptorSet>> descriptorSets{};
        ResourceHandle<DescriptorSetLayout> descriptorSetLayout;
        std::vector<ResourceHandle<Buffer>> buffers{};

        std::unique_ptr<GraphicsPipeline> pipeline{};

        struct BufferData {
            HmckMat4 inverseProjection{1};
            HmckMat4 view{1};
            HmckMat4 inverseView{1};
            HmckVec4 textureDim{1.f, 1.f, 1.f, 1.0f};
            HmckVec4 sunPosition{-10.f, -10.f, 10.f, 1.f};
            HmckVec4 baseSkyColor{0.043f, 0.043f, 0.043f, 0.0f};
            HmckVec4 tissueColor{0.8f, 0.5f, 0.4f, 0.2f};
            HmckVec4 fatColor{1.0f, 0.8f, 0.6f, 0.4f};
            HmckVec4 boneColor{1.0f, 1.0f, 1.0f, 0.8f};
            HmckVec4 cameraPosition{0.f, 0.f, 0.f, 0.f};
        } bufferData;


        struct PushData {
            float resX = IApp::WINDOW_WIDTH;
            float resY = IApp::WINDOW_HEIGHT;
            float elapsedTime = 0.0f;
            float maxSteps = 1000.f;
            float marchSize = 0.01f;
            float airTrheshold = 0.1f;
            float tissueThreshold = 0.3f;
            float fatThreshold = 0.6f;
            int  nDotL = false;
        } pushData;

        ResourceHandle<Texture3D> texture{};

        float radius = 2.0f, azimuth = 0.0f, elevation = 0.0f;
        Vec3Padded cameraPosition{0.0f, 0.0f, 2.0f};
        Vec3Padded cameraTarget{0.0f, 0.0f, -0.8f};
    };
}