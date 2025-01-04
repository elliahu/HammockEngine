#pragma once

#include <memory>
#include <vector>
#include <chrono>

#include "IApp.h"

namespace Hammock {
    class PBRApp final : public IApp, public BenchmarkRunner {
    public:
        PBRApp();

        ~PBRApp() override;

        // Inherited via IApp
        void run() override;

        void load() override;

        void init();

    private:
        void createPipelines(const RenderContext &renderer);

        // Data
        GlobalDataBuffer globalBuffer{};
        FrameDataBuffer projectionBuffer{};
        PushBlockDataBuffer meshPushBlock{};

        float radius = 1.0f, azimuth = 0.0f, elevation = 0.0f;

        struct {
            ResourceHandle<Texture2D> environmentMap;
            ResourceHandle<Texture2D> prefilteredEnvMap;
            ResourceHandle<Texture2D> irradianceMap;
            ResourceHandle<Texture2D> brdfLut;
        } environment;

        struct {
            ResourceHandle<Buffer> vertexBuffer;
             ResourceHandle<Buffer> indexBuffer;
            uint32_t numTriangles = 0;
        } geomBuffers;

        // Descriptors
        // global descriptor
        struct {
            std::vector<ResourceHandle<VkDescriptorSet>> descriptorSets;
            ResourceHandle<DescriptorSetLayout> descriptorSetLayout;
            std::vector<ResourceHandle<Buffer>> buffers;
            Binding binding = 0;
        } globalDescriptors;

        struct {
            std::vector<ResourceHandle<VkDescriptorSet>> descriptorSets{};
            ResourceHandle<DescriptorSetLayout> descriptorSetLayout;
            std::vector<ResourceHandle<Buffer>> buffers{};
            Binding binding = 1;
        } projectionDescriptors;

        // composition descriptors
        struct {
            std::vector<ResourceHandle<VkDescriptorSet>> descriptorSets{};
            ResourceHandle<DescriptorSetLayout> descriptorSetLayout;
            Binding binding = 2;
        } compositionDescriptors;


        // Render passes
        // renders environment sphere into gbuffer color attachments and clears depth buffer
        struct {
            std::unique_ptr<GraphicsPipeline> pipeline{};
            // uses gbuffer render pass
        } environmentPass;

        // constructs gbuffer
        struct {
            std::unique_ptr<GraphicsPipeline> pipeline{};
            std::unique_ptr<Framebuffer> framebuffer{};
        } opaqueGeometryPass;

        // construct wb-oit buffer
        struct {
            std::unique_ptr<GraphicsPipeline> pipeline{};
            std::unique_ptr<Framebuffer> framebuffer{};
        } transparentGeometryPass;

        // composes image from gbuffer attachments
        struct {
            std::unique_ptr<GraphicsPipeline> pipeline{};
        } compositionPass;
    };
}
