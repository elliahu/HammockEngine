#pragma once

#include <memory>
#include <vector>

#include <hammock/resources/Buffer.h>
#include <hammock/resources/Descriptors.h>
#include <hammock/resources/Texture.h>

namespace Hammock {

    typedef int32_t id_t;

    template<typename ResourceType>
    class ResourceHandle {
    public:

        explicit ResourceHandle(id_t id = 0) : id_(id) {
        }


        id_t id() const { return id_; }


        bool isValid() const { return id_ != -1; }


        bool operator==(const ResourceHandle &other) const { return id_ == other.id_; }

        bool operator!=(const ResourceHandle &other) const { return id_ != other.id_; }

    private:
        id_t id_;
    };

    class DeviceStorage {
    public:
        static const uint32_t INVALID_HANDLE;

        explicit DeviceStorage(Device &device);

        struct BufferCreateInfo {
            VkDeviceSize instanceSize;
            uint32_t instanceCount;
            VkBufferUsageFlags usageFlags;
            VkMemoryPropertyFlags memoryPropertyFlags;
            VkDeviceSize minOffsetAlignment = 1;
            bool map = true;
        };

        [[nodiscard]] ResourceHandle<Buffer> createBuffer(BufferCreateInfo createInfo);

        struct VertexBufferCreateInfo {
            VkDeviceSize vertexSize;
            uint32_t vertexCount;
            void *data;
            VkBufferUsageFlags usageFlags = 0;
        };

        [[nodiscard]] ResourceHandle<Buffer> createVertexBuffer(const VertexBufferCreateInfo &createInfo);

        struct IndexBufferCreateInfo {
            VkDeviceSize indexSize;
            uint32_t indexCount;
            void *data;
            VkBufferUsageFlags usageFlags = 0;
        };

        [[nodiscard]] ResourceHandle<Buffer> createIndexBuffer(const IndexBufferCreateInfo &createInfo);

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

        [[nodiscard]] ResourceHandle<DescriptorSetLayout> createDescriptorSetLayout(
            const DescriptorSetLayoutCreateInfo &createInfo);

        struct DescriptorSetCreateInfo {
            ResourceHandle<DescriptorSetLayout> descriptorSetLayout;

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

        [[nodiscard]] ResourceHandle<VkDescriptorSet> createDescriptorSet(const DescriptorSetCreateInfo &createInfo);

        ResourceHandle<Texture2D> createEmptyTexture2D();

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
        [[nodiscard]] ResourceHandle<Texture2D> createTexture2D(const Texture2DCreateFromBufferInfo &createInfo);

        struct Texture3DCreateSamplerInfo {
            bool createSampler = true;
            VkFilter filter = VK_FILTER_LINEAR;
            VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        };

        struct Texture3DCreateFromBufferInfo {
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

        [[nodiscard]] ResourceHandle<Texture3D> createTexture3D(const Texture3DCreateFromBufferInfo &createInfo);

        DescriptorSetLayout &getDescriptorSetLayout(ResourceHandle<DescriptorSetLayout> handle);

        VkDescriptorSet getDescriptorSet(ResourceHandle<VkDescriptorSet> handle);

        std::unique_ptr<Buffer> &getBuffer(ResourceHandle<Buffer> handle);

        std::unique_ptr<Texture2D> &getTexture2D(ResourceHandle<Texture2D> handle);

        VkDescriptorImageInfo getTexture2DDescriptorImageInfo(ResourceHandle<Texture2D> handle);

        VkDescriptorImageInfo getTextureCubeMapDescriptorImageInfo(ResourceHandle<TextureCubeMap> handle);

        std::unique_ptr<Texture3D> &getTexture3D(ResourceHandle<Texture3D> handle);

        VkDescriptorImageInfo getTexture3DDescriptorImageInfo(ResourceHandle<Texture3D> handle);

        void bindDescriptorSet(
            VkCommandBuffer commandBuffer,
            VkPipelineBindPoint bindPoint,
            VkPipelineLayout pipelineLayout,
            uint32_t firstSet,
            uint32_t descriptorCount,
            ResourceHandle<VkDescriptorSet> descriptorSet,
            uint32_t dynamicOffsetCount,
            const uint32_t *pDynamicOffsets);

        void bindVertexBuffer(ResourceHandle<Buffer> vertexBuffer, ResourceHandle<Buffer> indexBuffer, VkCommandBuffer commandBuffer,
                              VkIndexType indexType = VK_INDEX_TYPE_UINT32);

        void bindVertexBuffer(ResourceHandle<Buffer> handle, VkCommandBuffer commandBuffer);

        void bindIndexBuffer(ResourceHandle<Buffer> handle, VkCommandBuffer commandBuffer,
                             VkIndexType indexType = VK_INDEX_TYPE_UINT32);

        void destroyBuffer(ResourceHandle<Buffer> handle);

        void destroyDescriptorSetLayout(ResourceHandle<DescriptorSetLayout> handle);

        void destroyTexture2D(ResourceHandle<Texture2D> handle);


        void destroyTexture3D(ResourceHandle<Texture3D> handle);

        void copyBuffer(ResourceHandle<Buffer> from, ResourceHandle<Buffer> to);

        VkDescriptorPool getDescriptorPool() {return descriptorPool->descriptorPool;}

    private:
        Device &device;
        std::unique_ptr<DescriptorPool> descriptorPool;

        std::unordered_map<id_t, std::unique_ptr<Buffer> > buffers;
        std::unordered_map<id_t, VkDescriptorSet> descriptorSets;
        std::unordered_map<id_t, std::unique_ptr<DescriptorSetLayout> >
        descriptorSetLayouts;
        std::unordered_map<id_t, std::unique_ptr<Texture2D> > texture2Ds;
        std::unordered_map<id_t, std::unique_ptr<Texture3D> > texture3Ds;

    };
}
