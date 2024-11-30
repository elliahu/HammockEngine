#pragma once

#include <memory>
#include <vector>

#include <resources/HmckBuffer.h>
#include <resources/HmckDescriptors.h>
#include <resources/HmckTexture.h>

namespace Hmck {
    typedef uint32_t BufferHandle;
    typedef uint32_t DescriptorSetHandle;
    typedef uint32_t DescriptorSetLayoutHandle;
    typedef uint32_t Texture2DHandle;
    typedef uint32_t TextureCubeMapHandle;
    typedef uint32_t Texture3DHandle;

    class ResourceManager {
    public:
        static const uint32_t INVALID_HANDLE;

        explicit ResourceManager(Device &device);

        struct BufferCreateInfo {
            VkDeviceSize instanceSize;
            uint32_t instanceCount;
            VkBufferUsageFlags usageFlags;
            VkMemoryPropertyFlags memoryPropertyFlags;
            VkDeviceSize minOffsetAlignment = 1;
            bool map = true;
        };

        [[nodiscard]] BufferHandle createBuffer(BufferCreateInfo createInfo) const;

        struct VertexBufferCreateInfo {
            VkDeviceSize vertexSize;
            uint32_t vertexCount;
            void *data;
            VkBufferUsageFlags usageFlags = 0;
        };

        [[nodiscard]] BufferHandle createVertexBuffer(const VertexBufferCreateInfo &createInfo);

        struct IndexBufferCreateInfo {
            VkDeviceSize indexSize;
            uint32_t indexCount;
            void *data;
            VkBufferUsageFlags usageFlags = 0;
        };

        [[nodiscard]] BufferHandle createIndexBuffer(const IndexBufferCreateInfo &createInfo);

        struct DescriptorSetLayoutCreateInfo {
            struct DescriptorSetLayoutBindingCreateInfo {
                uint32_t binding;
                VkDescriptorType descriptorType;
                VkShaderStageFlags stageFlags;
                uint32_t count = 1;
                VkDescriptorBindingFlags bindingFlags = 0;
            };

            std::vector<DescriptorSetLayoutBindingCreateInfo> bindings;
        };

        [[nodiscard]] DescriptorSetLayoutHandle createDescriptorSetLayout(const DescriptorSetLayoutCreateInfo &createInfo) const;

        struct DescriptorSetCreateInfo {
            DescriptorSetLayoutHandle descriptorSetLayout;

            struct DescriptorSetWriteBufferInfo {
                uint32_t binding;
                VkDescriptorBufferInfo bufferInfo;
            };

            struct DescriptorSetWriteBufferArrayInfo {
                uint32_t binding;
                std::vector<VkDescriptorBufferInfo> bufferInfos;
            };

            struct DescriptorSetWriteImageInfo {
                uint32_t binding;
                VkDescriptorImageInfo imageInfo;
            };

            struct DescriptorSetWriteImageArrayInfo {
                uint32_t binding;
                std::vector<VkDescriptorImageInfo> imageInfos;
            };

            struct DescriptorSetWriteAccelerationStructureInfo {
                uint32_t binding;
                VkWriteDescriptorSetAccelerationStructureKHR accelerationStructureInfo;
            };

            std::vector<DescriptorSetWriteBufferInfo> bufferWrites;
            std::vector<DescriptorSetWriteBufferArrayInfo> bufferArrayWrites;
            std::vector<DescriptorSetWriteImageInfo> imageWrites;
            std::vector<DescriptorSetWriteImageArrayInfo> imageArrayWrites;
            std::vector<DescriptorSetWriteAccelerationStructureInfo> accelerationStructureWrites;
        };

        [[nodiscard]] DescriptorSetHandle createDescriptorSet(const DescriptorSetCreateInfo &createInfo);

        static Texture2DHandle createTexture2D();

        struct Texture2DCreateSamplerInfo {
            bool createSampler = true;
            const VkFilter filter = VK_FILTER_LINEAR;
            const VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            const VkBorderColor borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
            const VkSamplerMipmapMode mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            const float minLod = 0.0f;
            const float maxLod = 1.0f;
            const float lodBias = 0.0f;
        };

        struct Texture2DCreateFromBufferInfo {
            const void *buffer;
            uint32_t instanceSize;
            uint32_t width;
            uint32_t height;
            uint32_t channels;
            VkFormat format;
            VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            Texture2DCreateSamplerInfo samplerInfo{};
        };
        // Creates a 2D texture from buffer
        // Set createInfo.samplerInfo.maxLod > 1 to automatically generate mip maps
        [[nodiscard]] Texture2DHandle createTexture2D(const Texture2DCreateFromBufferInfo &createInfo) const;

        struct TextureCubeMapCreateFromFilesInfo {
            std::vector<std::string> filenames;
            VkFormat format;
            VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        };

        [[nodiscard]] TextureCubeMapHandle createTextureCubeMapFromFiles(const TextureCubeMapCreateFromFilesInfo &createInfo) const;

        struct Texture3DCreateSamplerInfo {
            bool createSampler = true;
            VkFilter filter = VK_FILTER_LINEAR;
            VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        };

        struct Texture3DCreateFromBufferInfo{
            const void *buffer;
            uint32_t instanceSize;
            uint32_t width;
            uint32_t height;
            uint32_t channels;
            uint32_t depth;
            VkFormat format;
            VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            Texture3DCreateSamplerInfo samplerInfo{};
        };

        [[nodiscard]] Texture3DHandle createTexture3D(const Texture3DCreateFromBufferInfo &createInfo) const;

        static DescriptorSetLayout &getDescriptorSetLayout(DescriptorSetLayoutHandle handle);

        static VkDescriptorSet getDescriptorSet(DescriptorSetHandle handle);

        static std::unique_ptr<Buffer> &getBuffer(BufferHandle handle);

        static std::unique_ptr<Texture2D> &getTexture2D(Texture2DHandle handle);

        static VkDescriptorImageInfo getTexture2DDescriptorImageInfo(Texture2DHandle handle);

        static std::unique_ptr<TextureCubeMap> &getTextureCubeMap(TextureCubeMapHandle handle);

        static VkDescriptorImageInfo getTextureCubeMapDescriptorImageInfo(TextureCubeMapHandle handle);

        static std::unique_ptr<Texture3D> &getTexture3D(Texture3DHandle handle);

        static VkDescriptorImageInfo getTexture3DDescriptorImageInfo(Texture3DHandle handle);

        static void bindDescriptorSet(
            VkCommandBuffer commandBuffer,
            VkPipelineBindPoint bindPoint,
            VkPipelineLayout pipelineLayout,
            uint32_t firstSet,
            uint32_t descriptorCount,
            DescriptorSetHandle descriptorSet,
            uint32_t dynamicOffsetCount,
            const uint32_t *pDynamicOffsets);

        static void bindVertexBuffer(BufferHandle vertexBuffer, BufferHandle indexBuffer, VkCommandBuffer commandBuffer,
                                     VkIndexType indexType = VK_INDEX_TYPE_UINT32);

        static void bindVertexBuffer(BufferHandle handle, VkCommandBuffer commandBuffer);

        static void bindIndexBuffer(BufferHandle handle, VkCommandBuffer commandBuffer,
                                    VkIndexType indexType = VK_INDEX_TYPE_UINT32);

        static void destroyBuffer(BufferHandle handle);

        static void destroyDescriptorSetLayout(DescriptorSetLayoutHandle handle);

        void destroyTexture2D(Texture2DHandle handle) const;

        void destroyTextureCubeMap(TextureCubeMapHandle handle) const;

        void destroyTexture3D(Texture3DHandle handle) const;

        void copyBuffer(BufferHandle from, BufferHandle to) const;

    private:
        Device &device;
        std::unique_ptr<DescriptorPool> descriptorPool;

        // descriptors and buffer
        static std::unordered_map<BufferHandle, std::unique_ptr<Buffer> > buffers;
        static std::unordered_map<DescriptorSetHandle, VkDescriptorSet> descriptorSets;
        static std::unordered_map<DescriptorSetLayoutHandle, std::unique_ptr<DescriptorSetLayout> >
        descriptorSetLayouts;

        // textures
        static std::unordered_map<Texture2DHandle, std::unique_ptr<Texture2D> > texture2Ds;
        static std::unordered_map<Texture2DHandle, std::unique_ptr<TextureCubeMap> > textureCubeMaps;
        static std::unordered_map<Texture3DHandle, std::unique_ptr<Texture3D> > texture3Ds;

        static uint32_t buffersLastHandle;
        static uint32_t descriptorSetsLastHandle;
        static uint32_t descriptorSetLayoutsLastHandle;
        static uint32_t texture2DsLastHandle;
        static uint32_t textureCubeMapsLastHandle;
        static uint32_t texture3DsLastHandle;
    };
}
