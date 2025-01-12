#pragma once
#include <string>
#include <vulkan/vulkan.h>

#include "hammock/core/Resource.h"

namespace hammock {
    namespace rendergraph {
        struct BufferDescription {
            const std::string &name;
            VkDeviceSize instanceSize;
            uint32_t instanceCount;
            VkBufferUsageFlags usageFlags;
            VkMemoryPropertyFlags memoryPropertyFlags;
            VkDeviceSize minOffsetAlignment = 1;
        };

        class Buffer : public Resource {
        protected:
            explicit Buffer(Device &device, uint64_t uid,
                            const BufferDescription &desc): Resource(device, uid, desc.name), desc(desc) {
            }
            VkDeviceSize getAlignment(const VkDeviceSize instanceSize, const VkDeviceSize minOffsetAlignment);



            void *mapped = nullptr;
            VkBuffer buffer = VK_NULL_HANDLE;
            VmaAllocation allocation = VK_NULL_HANDLE;
            VkDeviceSize bufferSize;
            VkDeviceSize alignmentSize;
            bool scoped = false; // if scoped, it gets destroyed automatically when it goes out of scope
            BufferDescription desc;

        public:

            Buffer(Device &device, const BufferDescription &desc ): Resource(device, 0, "stagin_buffer"), desc(desc) {
                scoped = true;
                Buffer::load();
            }

            ~Buffer() {
                if (scoped) {
                    Buffer::unload();
                }
            }

            void load() override;
            void unload() override;

            [[nodiscard]] VkBuffer getBuffer() const { return buffer; }
            [[nodiscard]] void *getMappedMemory() const { return mapped; }
            [[nodiscard]] uint32_t getInstanceCount() const { return desc.instanceCount; }
            [[nodiscard]] VkDeviceSize getInstanceSize() const { return desc.instanceSize; }
            [[nodiscard]] VkDeviceSize getAlignmentSize() const { return desc.instanceSize; }
            [[nodiscard]] VkBufferUsageFlags getUsageFlags() const { return desc.usageFlags; }
            [[nodiscard]] VkMemoryPropertyFlags getMemoryPropertyFlags() const { return desc.memoryPropertyFlags; }
            [[nodiscard]] VkDeviceSize getBufferSize() const { return bufferSize; }


            VkResult map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);

            void unmap();

            void writeToBuffer(const void *data, VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) const;

            VkResult flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) const;

            VkDescriptorBufferInfo descriptorInfo(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) const;

            VkResult invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) const;

            void writeToIndex(const void *data, int index) const;

            VkResult flushIndex(int index) const;

            VkDescriptorBufferInfo descriptorInfoForIndex(int index) const;

            VkResult invalidateIndex(int index) const;
        };
    }
}
