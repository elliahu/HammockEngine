#pragma once

#include <vulkan/vulkan.h>
#include <memory>
#include "core/HmckDevice.h"
#include "utils/HmckUtils.h"

namespace Hmck {
    /*
        Texture represents a texture
        that can be used to sample from in a shader
    */
    class ITexture {
    public:
        // Recommended:
        // VK_FORMAT_R8G8B8A8_UNORM for normals
        // VK_FORMAT_R8G8B8A8_SRGB for images
        VkSampler sampler;
        VkDeviceMemory memory;
        VkImage image;
        VkImageView view;
        VkImageLayout layout;
        int width{0}, height{0}, channels{0}, mipLevels{1};
        uint32_t layerCount{1};
        VkDescriptorImageInfo descriptor;

        void updateDescriptor();

        void destroy(const Device &device);
    };


    class Texture2D final: public ITexture{
    public:
        void loadFromFile(
            const std::string &filepath,
            Device &device,
            VkFormat format,
            VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            uint32_t mipLevels = 1
        );

        // TODO to be removed
         [[deprecated("Use the standard float buffer. To be removed")]]void loadFromBuffer(
            const unsigned char *buffer,
            uint32_t bufferSize,
            uint32_t width, uint32_t height,
            Device &device,
            VkFormat format,
            VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            uint32_t mipLevels = 1
        );

        void loadFromBuffer(
            const float *buffer,
            uint32_t bufferSize,
            uint32_t width, uint32_t height,
            Device &device,
            VkFormat format,
            VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            uint32_t mipLevels = 1
        );

        void createSampler(const Device &device,
            VkFilter filter = VK_FILTER_LINEAR,
            VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            VkBorderColor borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
            VkSamplerMipmapMode mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
            float numMips = 0.0f
            );

        void generateMipMaps(const Device &device, uint32_t mipLevels) const;
    };

    class TextureCubeMap : public ITexture {
    public:
        // TODO loadFromBuffer

        void loadFromFiles(
            const std::vector<std::string> &filenames,
            VkFormat format,
            Device &device,
            VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        void createSampler(const Device &device, VkFilter filter = VK_FILTER_LINEAR);
    };

    class Texture3D final: public ITexture {
    public:
        int depth{0};

        // Recommended format VK_FORMAT_R8_UNORM
        void loadFromBuffer(Device &device,
            const void * buffer,
            VkDeviceSize instanceSize,
            uint32_t width,
            uint32_t height,
            uint32_t channels,
            uint32_t depth,
            VkFormat format,
            VkImageLayout imageLayout);

        void createSampler(Device& device,
            VkFilter filter = VK_FILTER_LINEAR,
            VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    };
}
