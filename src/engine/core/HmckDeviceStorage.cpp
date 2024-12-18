#include "HmckDeviceStorage.h"
#include <stdint.h>

namespace Hmck {
    const uint32_t DeviceStorage::INVALID_HANDLE = UINT32_MAX;
}

Hmck::DeviceStorage::DeviceStorage(Device &device) : device{device}{
    descriptorPool = DescriptorPool::Builder(device)
            .setMaxSets(20000)
            .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10000)
            .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10000)
            .build();
}

Hmck::DeviceStorage::~DeviceStorage() {
}

Hmck::BufferHandle Hmck::DeviceStorage::createBuffer(BufferCreateInfo createInfo) {
    auto buffer = std::make_unique<Buffer>(
        device,
        createInfo.instanceSize,
        createInfo.instanceCount,
        createInfo.usageFlags,
        createInfo.memoryPropertyFlags);

    if (createInfo.map) buffer->map();

    buffers.emplace(buffersLastHandle, std::move(buffer));
    BufferHandle handle = buffersLastHandle;
    buffersLastHandle++;
    return handle;
}


Hmck::BufferHandle Hmck::DeviceStorage::createVertexBuffer(const VertexBufferCreateInfo &createInfo) {
    Buffer stagingBuffer{
        device,
        createInfo.vertexSize,
        createInfo.vertexCount,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    };

    stagingBuffer.map();
    stagingBuffer.writeToBuffer(createInfo.data);

    BufferHandle handle = createBuffer({
        .instanceSize = createInfo.vertexSize,
        .instanceCount = createInfo.vertexCount,
        .usageFlags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | createInfo.usageFlags,
        .memoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        .map = false
    });

    device.copyBuffer(stagingBuffer.getBuffer(), getBuffer(handle)->getBuffer(),
                      createInfo.vertexCount * createInfo.vertexSize);

    return handle;
}

Hmck::BufferHandle Hmck::DeviceStorage::createIndexBuffer(const IndexBufferCreateInfo &createInfo) {
    Buffer stagingBuffer{
        device,
        createInfo.indexSize,
        createInfo.indexCount,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    };

    stagingBuffer.map();
    stagingBuffer.writeToBuffer(createInfo.data);

    BufferHandle handle = createBuffer({
        .instanceSize = createInfo.indexSize,
        .instanceCount = createInfo.indexCount,
        .usageFlags = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | createInfo.usageFlags,
        .memoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        .map = false
    });

    device.copyBuffer(stagingBuffer.getBuffer(), getBuffer(handle)->getBuffer(),
                      createInfo.indexCount * createInfo.indexSize);

    return handle;
}

Hmck::DescriptorSetLayoutHandle Hmck::DeviceStorage::createDescriptorSetLayout(
    const DescriptorSetLayoutCreateInfo &createInfo) {
    auto descriptorSetLayoutBuilder = DescriptorSetLayout::Builder(device);

    for (auto &binding: createInfo.bindings) {
        descriptorSetLayoutBuilder.addBinding(binding.binding, binding.descriptorType, binding.stageFlags,
                                              binding.count, binding.bindingFlags);
    }
    auto descriptorSetLayout = descriptorSetLayoutBuilder.build();

    descriptorSetLayouts.emplace(descriptorSetLayoutsLastHandle, std::move(descriptorSetLayout));
    DescriptorSetLayoutHandle handle = descriptorSetLayoutsLastHandle;
    descriptorSetLayoutsLastHandle++;
    return handle;
}

Hmck::DescriptorSetHandle Hmck::DeviceStorage::createDescriptorSet(const DescriptorSetCreateInfo &createInfo) {
    VkDescriptorSet descriptorSet;

    auto writer = DescriptorWriter(getDescriptorSetLayout(createInfo.descriptorSetLayout), *descriptorPool);

    for (auto &buffer: createInfo.bufferWrites) {
        writer.writeBuffer(buffer.binding, &buffer.bufferInfo);
    }

    for (auto &bufferArray: createInfo.bufferArrayWrites) {
        writer.writeBufferArray(bufferArray.binding, bufferArray.bufferInfos);
    }

    for (auto &image: createInfo.imageWrites) {
        writer.writeImage(image.binding, &image.imageInfo);
    }

    for (auto &imageArray: createInfo.imageArrayWrites) {
        writer.writeImageArray(imageArray.binding, imageArray.imageInfos);
    }

    for (auto &accelerationStructure: createInfo.accelerationStructureWrites) {
        writer.writeAccelerationStructure(accelerationStructure.binding,
                                          &accelerationStructure.accelerationStructureInfo);
    }

    if (writer.build(descriptorSet)) {
        descriptorSets[descriptorSetsLastHandle] = descriptorSet;
        DescriptorSetHandle handle = descriptorSetsLastHandle;
        descriptorSetsLastHandle++;
        return handle;
    }

    throw std::runtime_error("Faild to create descriptor!");
}

Hmck::Texture2DHandle Hmck::DeviceStorage::createTexture2D(
    const Texture2DCreateFromBufferInfo &createInfo) {
    std::unique_ptr<Texture2D> texture = std::make_unique<Texture2D>(device);

    texture->loadFromBuffer(
        createInfo.buffer,
        createInfo.instanceSize,
        createInfo.width, createInfo.height, createInfo.channels,
        device,
        createInfo.format,
        createInfo.imageLayout,
        createInfo.samplerInfo.maxLod);

    if (createInfo.samplerInfo.createSampler) {
        texture->createSampler(device,
                               createInfo.samplerInfo.filter,
                               createInfo.samplerInfo.addressMode,
                               createInfo.samplerInfo.borderColor,
                               createInfo.samplerInfo.mipmapMode,
                               createInfo.samplerInfo.maxLod);
    }

    if (createInfo.samplerInfo.maxLod > 1) {
        texture->generateMipMaps(device,createInfo.samplerInfo.maxLod);
    }

    texture->updateDescriptor();
    texture2Ds.emplace(texture2DsLastHandle, std::move(texture));
    Texture2DHandle handle = texture2DsLastHandle;
    texture2DsLastHandle++;
    return handle;
}

Hmck::Texture2DHandle Hmck::DeviceStorage::createEmptyTexture2D() {
    std::unique_ptr<Texture2D> texture = std::make_unique<Texture2D>(device);
    texture2Ds.emplace(texture2DsLastHandle, std::move(texture));
    Texture2DHandle handle = texture2DsLastHandle;
    texture2DsLastHandle++;
    return handle;
}


Hmck::Texture3DHandle Hmck::DeviceStorage::createTexture3D(
    const Texture3DCreateFromBufferInfo &createInfo) {
    std::unique_ptr<Texture3D> texture = std::make_unique<Texture3D>(device);
    texture->loadFromBuffer(
        device,
        createInfo.buffer,
        createInfo.instanceSize,
        createInfo.width, createInfo.height, createInfo.channels, createInfo.depth,
        createInfo.format,
        createInfo.imageLayout
    );
    if (createInfo.samplerInfo.createSampler) {
        texture->createSampler(device, createInfo.samplerInfo.filter, createInfo.samplerInfo.addressMode);
    }
    texture->updateDescriptor();
    texture3Ds.emplace(texture3DsLastHandle, std::move(texture));
    Texture3DHandle handle = texture3DsLastHandle;
    texture3DsLastHandle++;
    return handle;
}

Hmck::DescriptorSetLayout &Hmck::DeviceStorage::getDescriptorSetLayout(const DescriptorSetLayoutHandle handle) {
    if (descriptorSetLayouts.contains(handle)) {
        return *descriptorSetLayouts[handle];
    }

    throw std::runtime_error("Descriptor set layout with provided handle does not exist!");
}

VkDescriptorSet Hmck::DeviceStorage::getDescriptorSet(const DescriptorSetHandle handle) {
    if (descriptorSets.contains(handle)) {
        return descriptorSets[handle];
    }

    throw std::runtime_error("Descriptor with provided handle does not exist!");
}

std::unique_ptr<Hmck::Buffer> &Hmck::DeviceStorage::getBuffer(const BufferHandle handle) {
    if (buffers.contains(handle)) {
        return buffers[handle];
    }

    throw std::runtime_error("Uniform buffer with provided handle does not exist!");
}

std::unique_ptr<Hmck::Texture2D> &Hmck::DeviceStorage::getTexture2D(const Texture2DHandle handle) {
    if (texture2Ds.contains(handle)) {
        return texture2Ds[handle];
    }

    throw std::runtime_error("Texture2D with provided handle does not exist!");
}

VkDescriptorImageInfo Hmck::DeviceStorage::getTexture2DDescriptorImageInfo(const Texture2DHandle handle) {
    return getTexture2D(handle)->descriptor;
}


std::unique_ptr<Hmck::Texture3D> &Hmck::DeviceStorage::getTexture3D(Texture3DHandle handle) {
    if (texture3Ds.contains(handle)) {
        return texture3Ds[handle];
    }

    throw std::runtime_error("Texture3D with provided handle does not exist!");
}

VkDescriptorImageInfo Hmck::DeviceStorage::getTexture3DDescriptorImageInfo(Texture3DHandle handle) {
    return getTexture3D(handle)->descriptor;
}

void Hmck::DeviceStorage::bindDescriptorSet(
    const VkCommandBuffer commandBuffer,
    const VkPipelineBindPoint bindPoint,
    const VkPipelineLayout pipelineLayout,
    const uint32_t firstSet,
    const uint32_t descriptorCount,
    const DescriptorSetHandle descriptorSet,
    const uint32_t dynamicOffsetCount,
    const uint32_t *pDynamicOffsets) {
    const auto descriptor = getDescriptorSet(descriptorSet);

    vkCmdBindDescriptorSets(
        commandBuffer,
        bindPoint,
        pipelineLayout,
        firstSet, descriptorCount,
        &descriptor,
        dynamicOffsetCount,
        pDynamicOffsets);
}

void Hmck::DeviceStorage::destroyBuffer(const BufferHandle handle) {
    buffers.erase(handle);
}

void Hmck::DeviceStorage::destroyDescriptorSetLayout(const DescriptorSetLayoutHandle handle) {
    descriptorSetLayouts.erase(handle);
}

void Hmck::DeviceStorage::destroyTexture2D(const Texture2DHandle handle){
    texture2Ds.erase(handle);
}

void Hmck::DeviceStorage::bindVertexBuffer(const BufferHandle handle, const VkCommandBuffer commandBuffer) {
    VkDeviceSize offsets[] = {0};
    VkBuffer buffers[] = {getBuffer(handle)->getBuffer()};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
}

void Hmck::DeviceStorage::bindVertexBuffer(const BufferHandle vertexBuffer, const BufferHandle indexBuffer,
                                             const VkCommandBuffer commandBuffer, VkIndexType indexType) {
    bindVertexBuffer(vertexBuffer, commandBuffer);
    bindIndexBuffer(indexBuffer, commandBuffer);
}

void Hmck::DeviceStorage::bindIndexBuffer(const BufferHandle handle, const VkCommandBuffer commandBuffer,
                                            const VkIndexType indexType) {
    vkCmdBindIndexBuffer(commandBuffer, getBuffer(handle)->getBuffer(), 0, indexType);
}

void Hmck::DeviceStorage::destroyTexture3D(Texture3DHandle handle) {
    texture3Ds.erase(handle);
}

void Hmck::DeviceStorage::copyBuffer(const BufferHandle from, const BufferHandle to) {
    auto &fromBuffer = getBuffer(from);
    auto &toBuffer = getBuffer(to);

    device.copyBuffer(fromBuffer->getBuffer(), toBuffer->getBuffer(), fromBuffer->getBufferSize());
}
