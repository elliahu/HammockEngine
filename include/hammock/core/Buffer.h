#pragma once
#include <cstring>
#include "hammock/core/Types.h"

namespace hammock {
    class Buffer : public Resource {
    protected:
        void *m_mapped = nullptr;
        VkBuffer m_buffer = VK_NULL_HANDLE;
        VmaAllocation m_allocation = VK_NULL_HANDLE;

        VkDeviceSize m_alignmentSize;
        VkDeviceSize m_bufferSize;
        uint32_t m_instanceCount;
        VkDeviceSize m_instanceSize;
        VkBufferUsageFlags m_usageFlags;
        VmaAllocationCreateFlags m_memoryPropertyFlags;

        CommandQueueFamily m_queueFamily;
        VkSharingMode m_sharingMode;


        /**
             * Returns the minimum instance size required to be compatible with devices minOffsetAlignment
             *
             * @param instanceSize The size of an instance
             * @param minOffsetAlignment The minimum required alignment, in bytes, for the offset member (eg
             * minUniformBufferOffsetAlignment)
             *
             * @return VkResult of the buffer mapping call
             */
        VkDeviceSize getAlignment(const VkDeviceSize instanceSize, const VkDeviceSize minOffsetAlignment) {
            if (minOffsetAlignment > 0) {
                return (instanceSize + minOffsetAlignment - 1) & ~(minOffsetAlignment - 1);
            }
            return instanceSize;
        }

    public:
        Buffer(Device &device, uint64_t id, const std::string &name, const BufferDesc &desc) : Resource(
            device, id, name) {
            m_alignmentSize = getAlignment(desc.instanceSize, desc.minOffsetAlignment);
            m_bufferSize = m_alignmentSize * desc.instanceCount;
            m_instanceCount = desc.instanceCount;
            m_instanceSize = desc.instanceSize;
            m_usageFlags = desc.usageFlags;
            m_memoryPropertyFlags = desc.allocationFlags;
            m_queueFamily = desc.queueFamily;
            m_sharingMode = desc.sharingMode;
        }

        ~Buffer() override {
            if (isResident()) {
                Buffer::release();
            }
        }

        /**
         * Creates the actual resource and loads it into memory
         */
        void create() override {
            Logger::log(LOG_LEVEL_DEBUG, "Creating buffer %s\n", getName().c_str());
            VkBufferCreateInfo bufferInfo{};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = m_bufferSize;
            bufferInfo.usage = m_usageFlags;
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            VmaAllocationCreateInfo allocInfo = {};
            allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
            allocInfo.flags = m_memoryPropertyFlags;

            ASSERT(
                vmaCreateBuffer(device.allocator(), &bufferInfo, &allocInfo, &m_buffer, &m_allocation, nullptr) ==
                VK_SUCCESS, "Could not create buffer!");
            resident = true;
        }

        /**
         * Frees the resource
         */
        void release() override {
            Logger::log(LOG_LEVEL_DEBUG, "Releasing buffer %s\n", getName().c_str());
            unmap();
            vmaDestroyBuffer(device.allocator(), m_buffer, m_allocation);
            resident = false;
        }

        [[nodiscard]] VkBuffer getBuffer() const { return m_buffer; }
        [[nodiscard]] void *getMappedMemory() const { return m_mapped; }
        [[nodiscard]] uint32_t getInstanceCount() const { return m_instanceCount; }
        [[nodiscard]] VkDeviceSize getInstanceSize() const { return m_instanceSize; }
        [[nodiscard]] VkDeviceSize getAlignmentSize() const { return m_alignmentSize; }
        [[nodiscard]] VkBufferUsageFlags getUsageFlags() const { return m_usageFlags; }
        [[nodiscard]] VkMemoryPropertyFlags getMemoryPropertyFlags() const { return m_memoryPropertyFlags; }
        [[nodiscard]] VkDeviceSize getBufferSize() const { return m_bufferSize; }
        [[nodiscard]] CommandQueueFamily getQueueFamily() const {return m_queueFamily;}

        /**
        * Map a memory range of this buffer. If successful, mapped points to the specified buffer range.
        *
        * @param size (Optional) Size of the memory range to map. Pass VK_WHOLE_SIZE to map the complete
        * buffer range.
        * @param offset (Optional) Byte offset from beginning
        *
        * @return VkResult of the buffer mapping call
        */
        VkResult map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) {
            assert(m_buffer && m_allocation && "Called map on buffer before create");
            return vmaMapMemory(device.allocator(), m_allocation, &m_mapped);
        }

        /**
        * Unmap a mapped memory range
        *
        * @note Does not return a result as vkUnmapMemory can't fail
        */
        void unmap() {
            if (m_mapped) {
                vmaUnmapMemory(device.allocator(), m_allocation);
                m_mapped = nullptr;
            }
        }

        /**
        * Copies the specified data to the mapped buffer. Default value writes whole buffer range
        *
        * @param data Pointer to the data to copy
        * @param size (Optional) Size of the data to copy. Pass VK_WHOLE_SIZE to flush the complete buffer
        * range.
        * @param offset (Optional) Byte offset from beginning of mapped region
        *
        */
        void writeToBuffer(const void *data, VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) const {
            assert(m_mapped && "Cannot copy to unmapped buffer");

            if (size == VK_WHOLE_SIZE) {
                memcpy(m_mapped, data, m_bufferSize);
            } else {
                char *memOffset = static_cast<char *>(m_mapped);
                memOffset += offset;
                memcpy(memOffset, data, size);
            }
        }

        /**
         * Flush a memory range of the buffer to make it visible to the device
         *
         * @note Only required for non-coherent memory
         *
         * @param size (Optional) Size of the memory range to flush. Pass VK_WHOLE_SIZE to flush the
         * complete buffer range.
         * @param offset (Optional) Byte offset from beginning
         *
         * @return VkResult of the flush call
         */
        VkResult flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) const {
            return vmaFlushAllocation(device.allocator(), m_allocation, offset, size);
        }

        /**
         * Create a buffer info descriptor
         *
         * @param size (Optional) Size of the memory range of the descriptor
         * @param offset (Optional) Byte offset from beginning
         *
         * @return VkDescriptorBufferInfo of specified offset and range
         */
        VkDescriptorBufferInfo descriptorInfo(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) const {
            return VkDescriptorBufferInfo{
                m_buffer,
                offset,
                size,
            };
        }

        /**
         * Invalidate a memory range of the buffer to make it visible to the host
         *
         * @note Only required for non-coherent memory
         *
         * @param size (Optional) Size of the memory range to invalidate. Pass VK_WHOLE_SIZE to invalidate
         * the complete buffer range.
         * @param offset (Optional) Byte offset from beginning
         *
         * @return VkResult of the invalidate call
         */
        VkResult invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) const {
            return vmaInvalidateAllocation(device.allocator(), m_allocation, offset, size);
        }

        /**
        * Copies "instanceSize" bytes of data to the mapped buffer at an offset of index * alignmentSize
        *
        * @param data Pointer to the data to copy
        * @param index Used in offset calculation
        *
        */
        void writeToIndex(const void *data, int index) const {
            writeToBuffer(data, m_instanceSize, index * m_alignmentSize);
        }

        /**
        *  Flush the memory range at index * alignmentSize of the buffer to make it visible to the device
        *
        * @param index Used in offset calculation
        *
        */
        VkResult flushIndex(int index) const {
            return flush(m_alignmentSize, index * m_alignmentSize);
        }

        /**
        * Create a buffer info descriptor
        *
        * @param index Specifies the region given by index * alignmentSize
        *
        * @return VkDescriptorBufferInfo for instance at index
         */
        VkDescriptorBufferInfo descriptorInfoForIndex(int index) const {
            return descriptorInfo(m_alignmentSize, index * m_alignmentSize);
        }

        /**
        * Invalidate a memory range of the buffer to make it visible to the host
        *
        * @note Only required for non-coherent memory
        *
        * @param index Specifies the region to invalidate: index * alignmentSize
        *
        * @return VkResult of the invalidate call
        */
        VkResult invalidateIndex(int index) const {
            return invalidate(m_alignmentSize, index * m_alignmentSize);
        }
    };

    template<>
    struct ResourceTypeTraits<Buffer> {
        static constexpr ResourceType type = ResourceType::Buffer;
    };
}
