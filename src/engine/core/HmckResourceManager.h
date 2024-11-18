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

        struct Texture2DCreateFromFileInfo {
            std::string filepath;
            VkFormat format;
            VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            uint32_t mipLevels = 1;
        };

        [[nodiscard]] Texture2DHandle createTexture2DFromFile(Texture2DCreateFromFileInfo createInfo) const;

        struct Texture2DCreateFromBufferInfo {
            unsigned char *buffer;
            uint32_t bufferSize;
            uint32_t width;
            uint32_t height;
            VkFormat format;
            VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            uint32_t mipLevels = 1 ;
        };

        [[nodiscard]] Texture2DHandle createTexture2DFromBuffer(const Texture2DCreateFromBufferInfo &createInfo) const;

        struct HDRTexture2DCreateFromBufferInfo {
            float *buffer;
            uint32_t bufferSize;
            uint32_t width;
            uint32_t height;
            VkFormat format;
            VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            uint32_t mipLevels = 1;
        };

        [[nodiscard]] Texture2DHandle createHDRTexture2DFromBuffer(const HDRTexture2DCreateFromBufferInfo &createInfo) const;


        struct TextureCubeMapCreateFromFilesInfo {
            std::vector<std::string> filenames;
            VkFormat format;
            VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        };

        [[nodiscard]] TextureCubeMapHandle createTextureCubeMapFromFiles(const TextureCubeMapCreateFromFilesInfo &createInfo) const;


        static DescriptorSetLayout &getDescriptorSetLayout(DescriptorSetLayoutHandle handle);

        static VkDescriptorSet getDescriptorSet(DescriptorSetHandle handle);

        static std::unique_ptr<Buffer> &getBuffer(BufferHandle handle);

        static std::unique_ptr<Texture2D> &getTexture2D(Texture2DHandle handle);

        static VkDescriptorImageInfo getTexture2DDescriptorImageInfo(Texture2DHandle handle);

        static std::unique_ptr<TextureCubeMap> &getTextureCubeMap(TextureCubeMapHandle handle);

        static VkDescriptorImageInfo getTextureCubeMapDescriptorImageInfo(TextureCubeMapHandle handle);

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

        static uint32_t buffersLastHandle;
        static uint32_t descriptorSetsLastHandle;
        static uint32_t descriptorSetLayoutsLastHandle;
        static uint32_t texture2DsLastHandle;
        static uint32_t textureCubeMapsLastHandle;
    };
}
