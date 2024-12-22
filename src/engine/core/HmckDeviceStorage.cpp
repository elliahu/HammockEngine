#include "HmckDeviceStorage.h"
#include <stdint.h>

namespace Hmck {
    const uint32_t DeviceStorage::INVALID_HANDLE = UINT32_MAX;
}

Hmck::DeviceStorage::DeviceStorage(Device &device) : device{device}, buffers(), descriptorSets(),
                                                     descriptorSetLayouts(),
                                                     texture2Ds(),
                                                     texture3Ds() {
    descriptorPool = DescriptorPool::Builder(device)
            .setMaxSets(20000)
            .setPoolFlags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)
            .addPoolSize(VK_DESCRIPTOR_TYPE_SAMPLER, 10000)
            .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10000)
            .addPoolSize(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 10000)
            .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 10000)
            .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 10000)
            .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 10000)
            .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10000)
            .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10000)
            .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 10000)
            .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 10000)
            .addPoolSize(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 10000)
            .build();
}

Hmck::ResourceHandle<Hmck::Buffer> Hmck::DeviceStorage::createBuffer(BufferCreateInfo createInfo) {
    auto buffer = std::make_unique<Buffer>(
        device,
        createInfo.instanceSize,
        createInfo.instanceCount,
        createInfo.usageFlags,
        createInfo.memoryPropertyFlags);

    if (createInfo.map) buffer->map();

    ResourceHandle<Buffer> handle(static_cast<id_t>(buffers.size()));
    buffers.emplace(handle.id(), std::move(buffer));
    return handle;
}


Hmck::ResourceHandle<Hmck::Buffer> Hmck::DeviceStorage::createVertexBuffer(const VertexBufferCreateInfo &createInfo) {
    Buffer stagingBuffer{
        device,
        createInfo.vertexSize,
        createInfo.vertexCount,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    };

    stagingBuffer.map();
    stagingBuffer.writeToBuffer(createInfo.data);

    const ResourceHandle<Buffer> handle = createBuffer({
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

Hmck::ResourceHandle<Hmck::Buffer> Hmck::DeviceStorage::createIndexBuffer(const IndexBufferCreateInfo &createInfo) {
    Buffer stagingBuffer{
        device,
        createInfo.indexSize,
        createInfo.indexCount,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    };

    stagingBuffer.map();
    stagingBuffer.writeToBuffer(createInfo.data);

    const ResourceHandle<Buffer> handle = createBuffer({
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

Hmck::ResourceHandle<Hmck::DescriptorSetLayout> Hmck::DeviceStorage::createDescriptorSetLayout(
    const DescriptorSetLayoutCreateInfo &createInfo) {
    auto descriptorSetLayoutBuilder = DescriptorSetLayout::Builder(device);

    for (auto &binding: createInfo.bindings) {
        descriptorSetLayoutBuilder.addBinding(binding.binding, binding.descriptorType, binding.stageFlags,
                                              binding.count, binding.bindingFlags);
    }
    auto descriptorSetLayout = descriptorSetLayoutBuilder.build();

    ResourceHandle<DescriptorSetLayout> handle(static_cast<id_t>(descriptorSetLayouts.size()));
    descriptorSetLayouts.emplace(handle.id(), std::move(descriptorSetLayout));
    return handle;
}

Hmck::ResourceHandle<VkDescriptorSet_T *> Hmck::DeviceStorage::createDescriptorSet(
    const DescriptorSetCreateInfo &createInfo) {
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
        ResourceHandle<VkDescriptorSet> handle(static_cast<id_t>(descriptorSets.size()));
        descriptorSets.emplace(handle.id(), descriptorSet);
        return handle;
    }

    throw std::runtime_error("Faild to create descriptor!");
}

Hmck::ResourceHandle<Hmck::Texture2D> Hmck::DeviceStorage::createTexture2D(
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
        texture->generateMipMaps(device, createInfo.samplerInfo.maxLod);
    }

    texture->updateDescriptor();
    ResourceHandle<Texture2D> handle(static_cast<id_t>(texture2Ds.size()));
    texture2Ds.emplace(handle.id(), std::move(texture));
    return handle;
}

Hmck::ResourceHandle<Hmck::Texture2D> Hmck::DeviceStorage::createEmptyTexture2D() {
    std::unique_ptr<Texture2D> texture = std::make_unique<Texture2D>(device);
    ResourceHandle<Texture2D> handle(static_cast<id_t>(texture2Ds.size()));
    texture2Ds.emplace(handle.id(), std::move(texture));
    return handle;
}


Hmck::ResourceHandle<Hmck::Texture3D> Hmck::DeviceStorage::createTexture3D(
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
    ResourceHandle<Texture3D> handle(static_cast<id_t>(texture3Ds.size()));
    texture3Ds.emplace(handle.id(), std::move(texture));
    return handle;
}

Hmck::DescriptorSetLayout &Hmck::DeviceStorage::getDescriptorSetLayout(ResourceHandle<DescriptorSetLayout> handle) {
    if (descriptorSetLayouts.contains(handle.id())) {
        return *descriptorSetLayouts[handle.id()];
    }

    throw std::runtime_error("Descriptor set layout with provided handle does not exist!");
}

VkDescriptorSet Hmck::DeviceStorage::getDescriptorSet(ResourceHandle<VkDescriptorSet> handle) {
    if (descriptorSets.contains(handle.id())) {
        return descriptorSets[handle.id()];
    }

    throw std::runtime_error("Descriptor with provided handle does not exist!");
}

std::unique_ptr<Hmck::Buffer> &Hmck::DeviceStorage::getBuffer(ResourceHandle<Buffer> handle) {
    if (buffers.contains(handle.id())) {
        return buffers[handle.id()];
    }

    throw std::runtime_error("Uniform buffer with provided handle does not exist!");
}

std::unique_ptr<Hmck::Texture2D> &Hmck::DeviceStorage::getTexture2D(ResourceHandle<Texture2D> handle) {
    if (texture2Ds.contains(handle.id())) {
        return texture2Ds[handle.id()];
    }

    throw std::runtime_error("Texture2D with provided handle does not exist!");
}

VkDescriptorImageInfo Hmck::DeviceStorage::getTexture2DDescriptorImageInfo(ResourceHandle<Texture2D> handle) {
    return getTexture2D(handle)->descriptor;
}


std::unique_ptr<Hmck::Texture3D> &Hmck::DeviceStorage::getTexture3D(ResourceHandle<Texture3D> handle) {
    if (texture3Ds.contains(handle.id())) {
        return texture3Ds[handle.id()];
    }

    throw std::runtime_error("Texture3D with provided handle does not exist!");
}

VkDescriptorImageInfo Hmck::DeviceStorage::getTexture3DDescriptorImageInfo(ResourceHandle<Texture3D> handle) {
    return getTexture3D(handle)->descriptor;
}

void Hmck::DeviceStorage::bindDescriptorSet(
    const VkCommandBuffer commandBuffer,
    const VkPipelineBindPoint bindPoint,
    const VkPipelineLayout pipelineLayout,
    const uint32_t firstSet,
    const uint32_t descriptorCount,
    ResourceHandle<VkDescriptorSet> descriptorSet,
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

void Hmck::DeviceStorage::destroyBuffer(ResourceHandle<Buffer> handle) {
    buffers.erase(handle.id());
}

void Hmck::DeviceStorage::destroyDescriptorSetLayout(ResourceHandle<DescriptorSetLayout> handle) {
    descriptorSetLayouts.erase(handle.id());
}

void Hmck::DeviceStorage::destroyTexture2D(ResourceHandle<Texture2D> handle) {
    texture2Ds.erase(handle.id());
}

void Hmck::DeviceStorage::destroyTexture3D(ResourceHandle<Texture3D> handle) {
    texture3Ds.erase(handle.id());
}

void Hmck::DeviceStorage::bindVertexBuffer(ResourceHandle<Buffer> handle, const VkCommandBuffer commandBuffer) {
    VkDeviceSize offsets[] = {0};
    VkBuffer buffers[] = {getBuffer(handle)->getBuffer()};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
}

void Hmck::DeviceStorage::bindVertexBuffer(ResourceHandle<Buffer> vertexBuffer, ResourceHandle<Buffer> indexBuffer,
                                           const VkCommandBuffer commandBuffer, VkIndexType indexType) {
    bindVertexBuffer(vertexBuffer, commandBuffer);
    bindIndexBuffer(indexBuffer, commandBuffer);
}

void Hmck::DeviceStorage::bindIndexBuffer(ResourceHandle<Buffer> handle, const VkCommandBuffer commandBuffer,
                                          const VkIndexType indexType) {
    vkCmdBindIndexBuffer(commandBuffer, getBuffer(handle)->getBuffer(), 0, indexType);
}

void Hmck::DeviceStorage::copyBuffer(ResourceHandle<Buffer> from, ResourceHandle<Buffer> to) {
    auto &fromBuffer = getBuffer(from);
    auto &toBuffer = getBuffer(to);

    device.copyBuffer(fromBuffer->getBuffer(), toBuffer->getBuffer(), fromBuffer->getBufferSize());
}
