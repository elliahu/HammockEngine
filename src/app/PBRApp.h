#pragma once
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <memory>
#include <vector>
#include <chrono>

#include "core/HmckDevice.h"
#include "core/HmckRenderer.h"
#include "resources/HmckDescriptors.h"
#include "IApp.h"
#include "scene/HmckEntity.h"
#include "scene/HmckScene.h"
#include "core/HmckMemory.h"
#include "scene/HmckGLTF.h"
#include "utils/HmckBenchmarkRunner.h"
#include "io/KeyboardMovementController.h"


namespace Hmck {
    class PBRApp final : public IApp, public BenchmarkRunner {
    public:
        struct SceneBufferData {
            glm::mat4 projection{1.f};
            glm::mat4 view{1.f};
            glm::mat4 inverseView{1.f};

            float exposure = 4.5f;
            float gamma = 1.0f;
            float whitePoint = 11.2f;

            struct OmniLight {
                glm::vec4 position;
                glm::vec4 color;
            } omniLights[1000];

            uint32_t numOmniLights = 0;
        };

        struct EntityBufferData {
            glm::mat4 model{1.f};
            glm::mat4 normal{1.f};
        };

        struct PrimitiveBufferData {
            glm::vec4 baseColorFactor{1.0f, 1.0f, 1.0f, 1.0f};
            uint32_t baseColorTextureIndex = TextureIndex::Invalid;
            uint32_t normalTextureIndex = TextureIndex::Invalid;
            uint32_t metallicRoughnessTextureIndex = TextureIndex::Invalid;
            uint32_t occlusionTextureIndex = TextureIndex::Invalid;
            float alphaCutoff = 1.0f;
            float metallicFactor = 0.0f;
            float roughnessFactor = 1.0f;
        };

        PBRApp();

        ~PBRApp() override;

        // Inherited via IApp
        void run() override;

        void load() override;

        void init();

        void renderEntity(
            uint32_t frameIndex,
            VkCommandBuffer commandBuffer,
            std::unique_ptr<GraphicsPipeline> &pipeline,
            const std::shared_ptr<Entity> &entity);

    private:
        void createPipelines(const Renderer &renderer);

        std::unique_ptr<Scene> scene{};

        bool blend = false;

        struct {
            BufferHandle vertexBuffer;
            BufferHandle indexBuffer;
            uint32_t numTriangles = 0;
        } geometry;


        // Descriptors
        // per frame (scene info updated every frame)
        struct {
            std::vector<DescriptorSetHandle> descriptorSets{};
            DescriptorSetLayoutHandle descriptorSetLayout;
            std::vector<BufferHandle> sceneBuffers{};
            Binding binding = 0;
        } sceneDescriptors;

        // per entity
        struct {
            std::unordered_map<EntityHandle, DescriptorSetHandle> descriptorSets{};
            DescriptorSetLayoutHandle descriptorSetLayout;
            std::unordered_map<EntityHandle, BufferHandle> entityBuffers{};
            Binding binding = 1;
        } entityDescriptors;

        // per primitive
        struct {
            std::vector<DescriptorSetHandle> descriptorSets{};
            DescriptorSetLayoutHandle descriptorSetLayout;
            std::vector<BufferHandle> primitiveBuffers{};
            Binding binding = 2;
        } primitiveDescriptors;

        // gbuffer descriptors
        struct {
            std::vector<DescriptorSetHandle> descriptorSets{};
            DescriptorSetLayoutHandle descriptorSetLayout;
            Binding binding = 3;
        } gBufferDescriptors;

        // Render passes
        // renders environment sphere into gbuffer color attachments and clears depth buffer
        struct {
            std::unique_ptr<GraphicsPipeline> pipeline{};
            // uses gbuffer render pass
        } environmentPass;

        // constructs gbuffer
        struct {
            std::unique_ptr<GraphicsPipeline> pipeline{};
            std::unique_ptr<Framebuffer> framebuffer{}; // TODO probably shoul be bufferd as well
        } gbufferPass;

        // composes image from gbuffer attachments
        struct {
            std::unique_ptr<GraphicsPipeline> pipeline{};
        } compositionPass;

        // forward transparency pass
        struct {
            std::unique_ptr<GraphicsPipeline> pipeline{};
            // uses swapchain render pass
        } transparencyPass;
    };
}
