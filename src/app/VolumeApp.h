#pragma once
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <memory>
#include <vector>
#include <chrono>


#include "core/HmckDevice.h"
#include "core/HmckPipeline.h"
#include "IApp.h"
#include "scene/HmckEntity.h"
#include "scene/HmckScene.h"
#include "core/HmckResourceManager.h"
#include "utils/HmckUtils.h"

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

        std::unique_ptr<Scene> scene{};

        BufferHandle vertexBuffer;
        BufferHandle indexBuffer;

        std::vector<DescriptorSetHandle> descriptorSets{};
        DescriptorSetLayoutHandle descriptorSetLayout;
        std::vector<BufferHandle> buffers{};

        std::unique_ptr<GraphicsPipeline> pipeline{};

        struct BufferData {
            glm::mat4 projection{1};
            glm::mat4 view{1};
            glm::mat4 inverseView{1};
            glm::vec4 textureDim{1.f, 1.f, 1.f, 1.0f};
            glm::vec4 sunPosition{-10.f, -10.f, 10.f, 1.f};
            glm::vec4 baseSkyColor{0.043f, 0.043f, 0.043f, 0.0f};
            glm::vec4 tissueColor{0.8f, 0.5f, 0.4f, 0.2f};
            glm::vec4 fatColor{1.0f, 0.8f, 0.6f, 0.4f};
            glm::vec4 boneColor{1.0f, 1.0f, 1.0f, 0.8f};
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

        Texture3DHandle texture{};
    };
}