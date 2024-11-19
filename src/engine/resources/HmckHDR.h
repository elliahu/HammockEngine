#pragma once
#include <string>
#include <filesystem>


#include "core/HmckDevice.h"
#include "core/HmckResourceManager.h"


namespace Hmck {
    class Environment {
    public:
        void load(Device &device, const ResourceManager &resources, const std::string &filepath, VkFormat format);

        void generatePrefilteredSphere(Device &device, ResourceManager &resources,
                                       VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT);
        void generatePrefilteredSphereWithStaticRoughness(Device &device, ResourceManager &resources,
                                       VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT);

        void generateIrradianceSphere(Device &device, ResourceManager &resources,
                                      VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT,
                                      float _deltaPhi = 180.0f, float _deltaTheta = 64.0f);

        void generateBRDFLUT(Device &device, ResourceManager &resources, uint32_t dim = 512,
                             VkFormat format = VK_FORMAT_R16G16_SFLOAT);

        void destroy(ResourceManager &memory) const;

        // texture handles
        Texture2DHandle environmentSphere;
        Texture2DHandle prefilteredSphere;
        Texture2DHandle irradianceSphere;
        Texture2DHandle brdfLUT;
    };
}
