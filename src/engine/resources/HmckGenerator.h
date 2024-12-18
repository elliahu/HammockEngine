#pragma once
#include <string>
#include <filesystem>


#include "core/HmckDevice.h"
#include "core/HmckDeviceStorage.h"


namespace Hmck {
    class Generator {
    public:
        Texture2DHandle generatePrefilteredMap(Device &device, Texture2DHandle environmentMap, DeviceStorage &resources,
                                    VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT);

        Texture2DHandle generatePrefilteredMapWithStaticRoughness(Device &device, Texture2DHandle environmentMap, DeviceStorage &resources,
                                                       VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT);

        Texture2DHandle generateIrradianceMap(Device &device, Texture2DHandle environmentMap, DeviceStorage &resources,
                                   VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT,
                                   float _deltaPhi = 180.0f, float _deltaTheta = 64.0f);

        Texture2DHandle generateBRDFLookUpTable(Device &device, DeviceStorage &resources, uint32_t dim = 512,
                                     VkFormat format = VK_FORMAT_R32G32_SFLOAT);

    };
}
