#pragma once

#include <vulkan/vulkan.h>
#include <memory>
#include <stb_image.h>

#include "core/HmckDevice.h"
#include "HmckBuffer.h"
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
        int width, height, channels;
        uint32_t layerCount;
        VkDescriptorImageInfo descriptor;

        void updateDescriptor();

        void destroy(const Device &device);
    };


    class Texture2D : public ITexture {
    public:
        void loadFromFile(
            const std::string &filepath,
            Device &device,
            VkFormat format,
            VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        );

        void loadFromBuffer(
            const unsigned char *buffer,
            uint32_t bufferSize,
            uint32_t width, uint32_t height,
            Device &device,
            VkFormat format,
            VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        );

        void loadFromBuffer(
            const float *buffer,
            uint32_t bufferSize,
            uint32_t width, uint32_t height,
            Device &device,
            VkFormat format,
            VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        );

        void createSampler(const Device &device, VkFilter filter = VK_FILTER_LINEAR);
    };

    class TextureCubeMap : public ITexture {
    public:
        void loadFromFiles(
            const std::vector<std::string> &filenames,
            VkFormat format,
            Device &device,
            VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        void createSampler(const Device &device, VkFilter filter = VK_FILTER_LINEAR);
    };
}
