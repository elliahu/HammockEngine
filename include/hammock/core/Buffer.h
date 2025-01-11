#pragma once
#include <string>
#include <vulkan/vulkan.h>

#include "hammock/core/Resource.h"

namespace hammock {
    namespace rendergraph {
        struct BufferInfo {
            VkDeviceSize instanceSize;
            uint32_t instanceCount;
            VkBufferUsageFlags usageFlags;
            VkMemoryPropertyFlags memoryPropertyFlags;
            VkDeviceSize minOffsetAlignment;
        };

        class Buffer : public Resource {
        protected:
            explicit Buffer(Device &device, uint64_t uid, const std::string &name,  const BufferInfo &info): Resource(device, uid, name) {
            }
        };
    }
}