#pragma once
#include <string>
#include <filesystem>


#include "core/HmckDevice.h"
#include "core/HmckResourceManager.h"


namespace Hmck {
    class Environment {
    public:
        void load(Device &device, const ResourceManager &memory, const std::string &filepath, VkFormat format);

        void generatePrefilteredSphere(Device &device, ResourceManager &memory,
                                       VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT);

        void generateIrradianceSphere(Device &device, ResourceManager &memory,
                                      VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT);

        // TODO should load a mipmap chain for different roughness values
        void generateBRDFLUT(Device &device, ResourceManager &memory, uint32_t dim = 512,
                             VkFormat format = VK_FORMAT_R16G16_SFLOAT);

        void destroy(ResourceManager &memory) const;

        // texture handles
        Texture2DHandle environmentSphere;
        Texture2DHandle prefilteredSphere;
        Texture2DHandle irradianceSphere;
        Texture2DHandle brdfLUT;
    };
}
