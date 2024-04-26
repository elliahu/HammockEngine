#include "HmckMemory.h"

namespace Hmck
{
	std::unordered_map<BufferHandle, std::unique_ptr<Buffer>> MemoryManager::buffers;
	std::unordered_map<DescriptorSetHandle, VkDescriptorSet> MemoryManager::descriptorSets;
	std::unordered_map<DescriptorSetLayoutHandle, std::unique_ptr<DescriptorSetLayout>> MemoryManager::descriptorSetLayouts;
	std::unordered_map<Texture2DHandle, std::unique_ptr<Texture2D>> MemoryManager::texture2Ds;
	std::unordered_map<Texture2DHandle, std::unique_ptr<TextureCubeMap>> MemoryManager::textureCubeMaps;

	const uint32_t MemoryManager::INVALID_HANDLE = 0;
	uint32_t MemoryManager::buffersLastHandle = 1;
	uint32_t MemoryManager::descriptorSetsLastHandle = 1;
	uint32_t MemoryManager::descriptorSetLayoutsLastHandle = 1;
	uint32_t MemoryManager::texture2DsLastHandle = 1;
	uint32_t MemoryManager::textureCubeMapsLastHandle = 1;
}

Hmck::MemoryManager::MemoryManager(Device& device) :device{ device }
{
	descriptorPool = DescriptorPool::Builder(device)
		.setMaxSets(20000)
		.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10000)
		.addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10000)
		.build();
}

Hmck::BufferHandle Hmck::MemoryManager::createBuffer(BufferCreateInfo createInfo)
{
	auto buffer = std::make_unique<Buffer>(
		device,
		createInfo.instanceSize,
		createInfo.instanceCount,
		createInfo.usageFlags,
		createInfo.memoryPropertyFlags);

	if(createInfo.map) buffer->map();

	buffers.emplace(buffersLastHandle, std::move(buffer));
	BufferHandle handle = buffersLastHandle;
	buffersLastHandle++;
	return handle;
}


Hmck::BufferHandle Hmck::MemoryManager::createVertexBuffer(VertexBufferCreateInfo createInfo)
{
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
		.usageFlags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		.memoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		.map = false});

	device.copyBuffer(stagingBuffer.getBuffer(), getBuffer(handle)->getBuffer(), createInfo.vertexCount * createInfo.vertexSize);

	return handle;
}

Hmck::BufferHandle Hmck::MemoryManager::createIndexBuffer(IndexBufferCreateInfo createInfo)
{
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
		.usageFlags = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		.memoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		.map = false });

	device.copyBuffer(stagingBuffer.getBuffer(), getBuffer(handle)->getBuffer(), createInfo.indexCount * createInfo.indexSize);

	return handle;
}

Hmck::DescriptorSetLayoutHandle Hmck::MemoryManager::createDescriptorSetLayout(DescriptorSetLayoutCreateInfo createInfo)
{
	auto descriptorSetLayoutBuilder = DescriptorSetLayout::Builder(device);

	for (auto& binding : createInfo.bindings)
	{
		descriptorSetLayoutBuilder.addBinding(binding.binding, binding.descriptorType, binding.stageFlags, binding.count, binding.bindingFlags);
	}
	auto descriptorSetLayout = descriptorSetLayoutBuilder.build();

	descriptorSetLayouts.emplace(descriptorSetLayoutsLastHandle, std::move(descriptorSetLayout));
	DescriptorSetLayoutHandle handle = descriptorSetLayoutsLastHandle;
	descriptorSetLayoutsLastHandle++;
	return handle;
}

Hmck::DescriptorSetHandle Hmck::MemoryManager::createDescriptorSet(DescriptorSetCreateInfo createInfo)
{
	VkDescriptorSet descriptorSet;

	auto writer = DescriptorWriter(getDescriptorSetLayout(createInfo.descriptorSetLayout), *descriptorPool);
	
	for (auto& buffer : createInfo.bufferWrites)
	{
		writer.writeBuffer(buffer.binding, &buffer.bufferInfo);
	}

	for (auto& bufferArray : createInfo.bufferArrayWrites)
	{
		writer.writeBufferArray(bufferArray.binding, bufferArray.bufferInfos);
	}

	for (auto& image : createInfo.imageWrites)
	{
		writer.writeImage(image.binding, &image.imageInfo);
	}

	for (auto& imageArray : createInfo.imageArrayWrites)
	{
		writer.writeImageArray(imageArray.binding, imageArray.imageInfos);
	}

	if (writer.build(descriptorSet))
	{
		descriptorSets[descriptorSetsLastHandle] = descriptorSet;
		DescriptorSetHandle handle = descriptorSetsLastHandle;
		descriptorSetsLastHandle++;
		return handle;
	}

	throw std::runtime_error("Faild to create descriptor!");
}

Hmck::Texture2DHandle Hmck::MemoryManager::createTexture2DFromFile(Texture2DCreateFromFileInfo createInfo)
{
	std::unique_ptr<Texture2D> texture = std::make_unique<Texture2D>();
	texture->loadFromFile(
		createInfo.filepath,
		device,
		createInfo.format,
		createInfo.imageLayout);
	texture->createSampler(device);
	texture->updateDescriptor();
	texture2Ds.emplace(texture2DsLastHandle, std::move(texture));
	Texture2DHandle handle = texture2DsLastHandle;
	texture2DsLastHandle++;
	return handle;
}

Hmck::Texture2DHandle Hmck::MemoryManager::createTexture2DFromBuffer(Texture2DCreateFromBufferInfo createInfo)
{
	std::unique_ptr<Texture2D> texture = std::make_unique<Texture2D>();
	texture->loadFromBuffer(
		createInfo.buffer,
		createInfo.bufferSize,
		createInfo.width,createInfo.height,
		device,
		createInfo.format,
		createInfo.imageLayout);
	texture->createSampler(device);
	texture->updateDescriptor();
	texture2Ds.emplace(texture2DsLastHandle, std::move(texture));
	Texture2DHandle handle = texture2DsLastHandle;
	texture2DsLastHandle++;
	return handle;
}

Hmck::TextureCubeMapHandle Hmck::MemoryManager::createTextureCubeMapFromFiles(TextureCubeMapCreateFromFilesInfo createInfo)
{
	std::unique_ptr<TextureCubeMap> texture = std::make_unique<TextureCubeMap>();
	texture->loadFromFiles(
		createInfo.filenames,
		createInfo.format,
		device,
		createInfo.imageLayout);
	texture->createSampler(device);
	texture->updateDescriptor();
	textureCubeMaps.emplace(textureCubeMapsLastHandle, std::move(texture));
	TextureCubeMapHandle handle = textureCubeMapsLastHandle;
	textureCubeMapsLastHandle++;
	return handle;
}

Hmck::DescriptorSetLayout& Hmck::MemoryManager::getDescriptorSetLayout(DescriptorSetLayoutHandle handle)
{
	if (descriptorSetLayouts.contains(handle))
	{
		return *descriptorSetLayouts[handle];
	}

	throw std::runtime_error("Descriptor set layout with provided handle does not exist!");
}

VkDescriptorSet Hmck::MemoryManager::getDescriptorSet(DescriptorSetHandle handle)
{
	if (descriptorSets.contains(handle))
	{
		return descriptorSets[handle];
	}

	throw std::runtime_error("Descriptor with provided handle does not exist!");
}

std::unique_ptr<Hmck::Buffer>& Hmck::MemoryManager::getBuffer(BufferHandle handle)
{
	if (buffers.contains(handle))
	{
		return buffers[handle];
	}

	throw std::runtime_error("Uniform buffer with provided handle does not exist!");
}

std::unique_ptr<Hmck::Texture2D>& Hmck::MemoryManager::getTexture2D(Texture2DHandle handle)
{
	if (texture2Ds.contains(handle))
	{
		return texture2Ds[handle];
	}

	throw std::runtime_error("Texture2D with provided handle does not exist!");
}

VkDescriptorImageInfo Hmck::MemoryManager::getTexture2DDescriptorImageInfo(Texture2DHandle handle)
{
	return getTexture2D(handle)->descriptor;
}

std::unique_ptr<Hmck::TextureCubeMap>& Hmck::MemoryManager::getTextureCubeMap(TextureCubeMapHandle handle)
{
	if (textureCubeMaps.contains(handle))
	{
		return textureCubeMaps[handle];
	}

	throw std::runtime_error("TextureCubeMap with provided handle does not exist!");
}

VkDescriptorImageInfo Hmck::MemoryManager::getTextureCubeMapDescriptorImageInfo(TextureCubeMapHandle handle)
{
	return getTextureCubeMap(handle)->descriptor;
}

void Hmck::MemoryManager::bindDescriptorSet(
	VkCommandBuffer commandBuffer, 
	VkPipelineBindPoint bindPoint, 
	VkPipelineLayout pipelineLayout, 
	uint32_t firstSet, 
	uint32_t descriptorCount, 
	DescriptorSetHandle descriptorSet,
	uint32_t dynamicOffsetCount, 
	const uint32_t* pDynamicOffsets)
{
	auto descriptor = getDescriptorSet(descriptorSet);

	vkCmdBindDescriptorSets(
		commandBuffer,
		bindPoint,
		pipelineLayout,
		firstSet, descriptorCount,
		&descriptor,
		dynamicOffsetCount,
		pDynamicOffsets);
}

void Hmck::MemoryManager::destroyBuffer(BufferHandle handle)
{
	buffers.erase(handle);
}

void Hmck::MemoryManager::destroyDescriptorSetLayout(DescriptorSetLayoutHandle handle)
{
	descriptorSetLayouts.erase(handle);
}

void Hmck::MemoryManager::destroyTexture2D(Texture2DHandle handle)
{
	getTexture2D(handle)->destroy(device);
	texture2Ds.erase(handle);
}

void Hmck::MemoryManager::bindVertexBuffer(BufferHandle handle, VkCommandBuffer commandBuffer)
{
	VkDeviceSize offsets[] = { 0 };
	VkBuffer buffers[] = { getBuffer(handle)->getBuffer()};
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
}

void Hmck::MemoryManager::bindVertexBuffer(BufferHandle vertexBuffer, BufferHandle indexBuffer, VkCommandBuffer commandBuffer, VkIndexType indexType)
{
	bindVertexBuffer(vertexBuffer, commandBuffer);
	bindIndexBuffer(indexBuffer, commandBuffer);
}

void Hmck::MemoryManager::bindIndexBuffer(BufferHandle handle, VkCommandBuffer commandBuffer, VkIndexType indexType)
{
	vkCmdBindIndexBuffer(commandBuffer, getBuffer(handle)->getBuffer(), 0, indexType);
}

void Hmck::MemoryManager::destroyTextureCubeMap(TextureCubeMapHandle handle)
{
	getTextureCubeMap(handle)->destroy(device);
	textureCubeMaps.erase(handle);
}

void Hmck::MemoryManager::copyBuffer(BufferHandle from, BufferHandle to)
{
	auto& fromBuffer = getBuffer(from);
	auto& toBuffer = getBuffer(to);

	device.copyBuffer(fromBuffer->getBuffer(), toBuffer->getBuffer(), fromBuffer->getBufferSize());
}


