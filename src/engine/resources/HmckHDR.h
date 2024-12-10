#pragma once
#include <string>
#include <filesystem>


#include "core/HmckDevice.h"
#include "core/HmckResourceManager.h"


namespace Hmck {
    class Environment {
    public:
        void readEnvironmentMap(const void *buffer, uint32_t instanceSize, uint32_t width, uint32_t height, uint32_t channels, const ResourceManager &resources, VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT);

        void readIrradianceMap(const void *buffer, uint32_t instanceSize, uint32_t width, uint32_t height, uint32_t channels, const ResourceManager &resources, VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT);

        void readBRDFLookUpTable(const void *buffer, uint32_t instanceSize, uint32_t width, uint32_t height, uint32_t channels, const ResourceManager &resources, VkFormat format  = VK_FORMAT_R32G32_SFLOAT);

        void generatePrefilteredMap(Device &device, ResourceManager &resources,
                                       VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT);
        void generatePrefilteredMapWithStaticRoughness(Device &device, ResourceManager &resources,
                                       VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT);

        void generateIrradianceMap(Device &device, ResourceManager &resources,
                                      VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT,
                                      float _deltaPhi = 180.0f, float _deltaTheta = 64.0f);

        void generateBRDFLookUpTable(Device &device, ResourceManager &resources, uint32_t dim = 512,
                             VkFormat format = VK_FORMAT_R32G32_SFLOAT);

        void destroy(ResourceManager &memory) const;

        // texture handles
        Texture2DHandle environmentMap;
        Texture2DHandle prefilteredMap; // contains mip maps, cannot be saved as standard image
        Texture2DHandle irradianceMap;
        Texture2DHandle brdfLookUpTable;
    };
}
