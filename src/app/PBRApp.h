#pragma once

#include <memory>
#include <vector>
#include <chrono>

#include "core/HmckDevice.h"
#include "core/HmckRenderContext.h"
#include "resources/HmckDescriptors.h"
#include "IApp.h"
#include "core/HmckBindingTypes.h"
#include "core/HmckDeviceStorage.h"
#include "core/HmckGraphicsPipeline.h"
#include "utils/HmckBenchmarkRunner.h"

namespace Hmck {
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
            Texture2DHandle environmentMap;
            Texture2DHandle prefilteredEnvMap;
            Texture2DHandle irradianceMap;
            Texture2DHandle brdfLut;
        } environment;

        struct {
            BufferHandle vertexBuffer;
            BufferHandle indexBuffer;
            uint32_t numTriangles = 0;
        } geometry;

        // Descriptors
        // global descriptor
        struct {
            std::vector<DescriptorSetHandle> descriptorSets;
            DescriptorSetLayoutHandle descriptorSetLayout;
            std::vector<BufferHandle> buffers;
            Binding binding = 0;
        } globalDescriptors;

        struct {
            std::vector<DescriptorSetHandle> descriptorSets{};
            DescriptorSetLayoutHandle descriptorSetLayout;
            std::vector<BufferHandle> buffers{};
            Binding binding = 1;
        } projectionDescriptors;

        // composition descriptors
        struct {
            std::vector<DescriptorSetHandle> descriptorSets{};
            DescriptorSetLayoutHandle descriptorSetLayout;
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
