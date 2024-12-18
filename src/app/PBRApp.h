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
        void createPipelines(const Renderer &renderer);

        // Data
        struct alignas(16) GlobalBuffer {
            static constexpr size_t MAX_MESHES = 256;

            alignas(16) HmckVec4 baseColorFactors[MAX_MESHES]; // w is padding
            alignas(16) HmckVec4 metallicRoughnessAlphaCutOffFactors[MAX_MESHES]; // w is padding

            // todo make this a global type
            struct alignas(16) IntPadding {
                int32_t value;
                int32_t padding[3]; // Explicit padding to 16 bytes
            };

            IntPadding baseColorTextureIndexes[MAX_MESHES];
            IntPadding normalTextureIndexes[MAX_MESHES];
            IntPadding metallicRoughnessTextureIndexes[MAX_MESHES];
            IntPadding occlusionTextureIndexes[MAX_MESHES];
            IntPadding visibilityFlags[MAX_MESHES];
        }globalBuffer{};

        // Projection buffer bound every frame
        struct ProjectionBuffer {
            HmckMat4 projectionMat;
            HmckMat4 viewMat;
            HmckMat4 inverseViewMat;
            HmckVec4 exposureGammaWhitePoint{4.5f, 1.0f, 11.0f};
        } projectionBuffer{};

        // Push block pushed for each mesh
        struct MeshPushBlock {
            HmckMat4 modelMat;
            uint32_t meshIndex;
        } meshPushBlock{};

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
