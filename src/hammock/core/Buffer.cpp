#include "hammock/core/Buffer.h"

#include <cstring>

void hammock::rendergraph::Buffer::load() {
    alignmentSize = getAlignment(desc.instanceSize, desc.minOffsetAlignment);
    bufferSize = alignmentSize * desc.instanceCount;

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = desc.usageFlags;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

    vmaCreateBuffer(device.allocator(), &bufferInfo, &allocInfo, &buffer, &allocation, nullptr);
}

void hammock::rendergraph::Buffer::unload() {
    vmaDestroyBuffer(device.allocator(), buffer, allocation);
}

/**
    * Map a memory range of this buffer. If successful, mapped points to the specified buffer range.
    *
    * @param size (Optional) Size of the memory range to map. Pass VK_WHOLE_SIZE to map the complete
    * buffer range.
    * @param offset (Optional) Byte offset from beginning
    *
    * @return VkResult of the buffer mapping call
    */
VkResult hammock::rendergraph::Buffer::map(VkDeviceSize size, VkDeviceSize offset) {
    assert(buffer && allocation && "Called map on buffer before create");
    return vmaMapMemory(device.allocator(), allocation, &mapped);
}

/**
    * Unmap a mapped memory range
    *
    * @note Does not return a result as vkUnmapMemory can't fail
    */
void hammock::rendergraph::Buffer::unmap() {
    if (mapped) {
        vmaUnmapMemory(device.allocator(), allocation);
        mapped = nullptr;
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
void hammock::rendergraph::Buffer::writeToBuffer(const void *data, VkDeviceSize size, VkDeviceSize offset) const {
    assert(mapped && "Cannot copy to unmapped buffer");

    if (size == VK_WHOLE_SIZE) {
        memcpy(mapped, data, bufferSize);
    } else {
        char *memOffset = static_cast<char *>(mapped);
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
VkResult hammock::rendergraph::Buffer::flush(VkDeviceSize size, VkDeviceSize offset) const {
    vmaFlushAllocation(device.allocator(), allocation, offset, size);
}

/**
     * Create a buffer info descriptor
     *
     * @param size (Optional) Size of the memory range of the descriptor
     * @param offset (Optional) Byte offset from beginning
     *
     * @return VkDescriptorBufferInfo of specified offset and range
     */
VkDescriptorBufferInfo hammock::rendergraph::Buffer::descriptorInfo(VkDeviceSize size, VkDeviceSize offset) const {
    return VkDescriptorBufferInfo{
        buffer,
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
VkResult hammock::rendergraph::Buffer::invalidate(VkDeviceSize size, VkDeviceSize offset) const {
    vmaInvalidateAllocation(device.allocator(), allocation, offset, size);
}

/**
    * Copies "instanceSize" bytes of data to the mapped buffer at an offset of index * alignmentSize
    *
    * @param data Pointer to the data to copy
    * @param index Used in offset calculation
    *
    */
void hammock::rendergraph::Buffer::writeToIndex(const void *data, int index) const {
    writeToBuffer(data, desc.instanceSize, index * alignmentSize);
}

/**
     *  Flush the memory range at index * alignmentSize of the buffer to make it visible to the device
     *
     * @param index Used in offset calculation
     *
     */
VkResult hammock::rendergraph::Buffer::flushIndex(int index) const {
    return flush(alignmentSize, index * alignmentSize);
}

/**
    * Create a buffer info descriptor
    *
    * @param index Specifies the region given by index * alignmentSize
    *
    * @return VkDescriptorBufferInfo for instance at index
    */
VkDescriptorBufferInfo hammock::rendergraph::Buffer::descriptorInfoForIndex(int index) const {
    return descriptorInfo(alignmentSize, index * alignmentSize);
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
VkResult hammock::rendergraph::Buffer::invalidateIndex(int index) const {
    return invalidate(alignmentSize, index * alignmentSize);
}

/**
     * Returns the minimum instance size required to be compatible with devices minOffsetAlignment
     *
     * @param instanceSize The size of an instance
     * @param minOffsetAlignment The minimum required alignment, in bytes, for the offset member (eg
     * minUniformBufferOffsetAlignment)
     *
     * @return VkResult of the buffer mapping call
     */
VkDeviceSize hammock::rendergraph::Buffer::getAlignment(const VkDeviceSize instanceSize,
                                                        const VkDeviceSize minOffsetAlignment) {
    if (minOffsetAlignment > 0) {
        return (instanceSize + minOffsetAlignment - 1) & ~(minOffsetAlignment - 1);
    }
    return instanceSize;
}
