#pragma once
#include <string>
#include <filesystem>


#include "core/HmckDevice.h"
#include "core/HmckDeviceStorage.h"


namespace Hmck {
    class Generator {
    public:
         ResourceHandle<Texture2D> generatePrefilteredMap(Device &device, ResourceHandle<Texture2D> environmentMap, DeviceStorage &resources,
                                    VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT);

         ResourceHandle<Texture2D> generatePrefilteredMapWithStaticRoughness(Device &device, ResourceHandle<Texture2D> environmentMap, DeviceStorage &resources,
                                                       VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT);

         ResourceHandle<Texture2D> generateIrradianceMap(Device &device,  ResourceHandle<Texture2D> environmentMap, DeviceStorage &resources,
                                   VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT,
                                   float _deltaPhi = 180.0f, float _deltaTheta = 64.0f);

         ResourceHandle<Texture2D> generateBRDFLookUpTable(Device &device, DeviceStorage &resources, uint32_t dim = 512,
                                     VkFormat format = VK_FORMAT_R32G32_SFLOAT);

    };
}
