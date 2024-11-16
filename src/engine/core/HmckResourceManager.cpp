#include "HmckResourceManager.h"

namespace Hmck
{
	std::unordered_map<BufferHandle, std::unique_ptr<Buffer>> ResourceManager::buffers;
	std::unordered_map<DescriptorSetHandle, VkDescriptorSet> ResourceManager::descriptorSets;
	std::unordered_map<DescriptorSetLayoutHandle, std::unique_ptr<DescriptorSetLayout>> ResourceManager::descriptorSetLayouts;
	std::unordered_map<Texture2DHandle, std::unique_ptr<Texture2D>> ResourceManager::texture2Ds;
	std::unordered_map<Texture2DHandle, std::unique_ptr<TextureCubeMap>> ResourceManager::textureCubeMaps;

	const uint32_t ResourceManager::INVALID_HANDLE = 0;
	uint32_t ResourceManager::buffersLastHandle = 1;
	uint32_t ResourceManager::descriptorSetsLastHandle = 1;
	uint32_t ResourceManager::descriptorSetLayoutsLastHandle = 1;
	uint32_t ResourceManager::texture2DsLastHandle = 1;
	uint32_t ResourceManager::textureCubeMapsLastHandle = 1;
}

Hmck::ResourceManager::ResourceManager(Device& device) :device{ device }
{
	descriptorPool = DescriptorPool::Builder(device)
		.setMaxSets(20000)
		.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10000)
		.addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10000)
		.addPoolSize(VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 10000)
		.build();
}

Hmck::BufferHandle Hmck::ResourceManager::createBuffer(BufferCreateInfo createInfo) const {
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


Hmck::BufferHandle Hmck::ResourceManager::createVertexBuffer(const VertexBufferCreateInfo &createInfo)
{
	Buffer stagingBuffer{
		device,
		createInfo.vertexSize,
		createInfo.vertexCount,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT ,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	};

	stagingBuffer.map();
	stagingBuffer.writeToBuffer(createInfo.data);

	BufferHandle handle = createBuffer({
		.instanceSize = createInfo.vertexSize,
		.instanceCount = createInfo.vertexCount,
		.usageFlags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | createInfo.usageFlags,
		.memoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		.map = false });

	device.copyBuffer(stagingBuffer.getBuffer(), getBuffer(handle)->getBuffer(), createInfo.vertexCount * createInfo.vertexSize);

	return handle;
}

Hmck::BufferHandle Hmck::ResourceManager::createIndexBuffer(const IndexBufferCreateInfo &createInfo)
{
	Buffer stagingBuffer{
		device,
		createInfo.indexSize,
		createInfo.indexCount,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT ,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	};

	stagingBuffer.map();
	stagingBuffer.writeToBuffer(createInfo.data);

	BufferHandle handle = createBuffer({
		.instanceSize = createInfo.indexSize,
		.instanceCount = createInfo.indexCount,
		.usageFlags = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | createInfo.usageFlags,
		.memoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		.map = false });

	device.copyBuffer(stagingBuffer.getBuffer(), getBuffer(handle)->getBuffer(), createInfo.indexCount * createInfo.indexSize);

	return handle;
}

Hmck::DescriptorSetLayoutHandle Hmck::ResourceManager::createDescriptorSetLayout(const DescriptorSetLayoutCreateInfo& createInfo) const {
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

Hmck::DescriptorSetHandle Hmck::ResourceManager::createDescriptorSet(const DescriptorSetCreateInfo& createInfo)
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

	for (auto& accelerationStructure : createInfo.accelerationStructureWrites)
	{
		writer.writeAccelerationStructure(accelerationStructure.binding, &accelerationStructure.accelerationStructureInfo);
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

Hmck::Texture2DHandle Hmck::ResourceManager::createTexture2DFromFile(Texture2DCreateFromFileInfo createInfo) const {
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

Hmck::Texture2DHandle Hmck::ResourceManager::createTexture2DFromBuffer(const Texture2DCreateFromBufferInfo &createInfo) const {
	std::unique_ptr<Texture2D> texture = std::make_unique<Texture2D>();
	texture->loadFromBuffer(
		createInfo.buffer,
		createInfo.bufferSize,
		createInfo.width, createInfo.height,
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

Hmck::Texture2DHandle Hmck::ResourceManager::createHDRTexture2DFromBuffer(const HDRTexture2DCreateFromBufferInfo &createInfo) const {
	std::unique_ptr<Texture2D> texture = std::make_unique<Texture2D>();
	texture->loadFromBuffer(
		createInfo.buffer,
		createInfo.bufferSize,
		createInfo.width, createInfo.height,
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

Hmck::Texture2DHandle Hmck::ResourceManager::createTexture2D()
{
	std::unique_ptr<Texture2D> texture = std::make_unique<Texture2D>();
	texture2Ds.emplace(texture2DsLastHandle, std::move(texture));
	Texture2DHandle handle = texture2DsLastHandle;
	texture2DsLastHandle++;
	return handle;
}

Hmck::TextureCubeMapHandle Hmck::ResourceManager::createTextureCubeMapFromFiles(const TextureCubeMapCreateFromFilesInfo &createInfo) const {
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

Hmck::DescriptorSetLayout& Hmck::ResourceManager::getDescriptorSetLayout(const DescriptorSetLayoutHandle handle)
{
	if (descriptorSetLayouts.contains(handle))
	{
		return *descriptorSetLayouts[handle];
	}

	throw std::runtime_error("Descriptor set layout with provided handle does not exist!");
}

VkDescriptorSet Hmck::ResourceManager::getDescriptorSet(const DescriptorSetHandle handle)
{
	if (descriptorSets.contains(handle))
	{
		return descriptorSets[handle];
	}

	throw std::runtime_error("Descriptor with provided handle does not exist!");
}

std::unique_ptr<Hmck::Buffer>& Hmck::ResourceManager::getBuffer(const BufferHandle handle)
{
	if (buffers.contains(handle))
	{
		return buffers[handle];
	}

	throw std::runtime_error("Uniform buffer with provided handle does not exist!");
}

std::unique_ptr<Hmck::Texture2D>& Hmck::ResourceManager::getTexture2D(const Texture2DHandle handle)
{
	if (texture2Ds.contains(handle))
	{
		return texture2Ds[handle];
	}

	throw std::runtime_error("Texture2D with provided handle does not exist!");
}

VkDescriptorImageInfo Hmck::ResourceManager::getTexture2DDescriptorImageInfo(const Texture2DHandle handle)
{
	return getTexture2D(handle)->descriptor;
}

std::unique_ptr<Hmck::TextureCubeMap>& Hmck::ResourceManager::getTextureCubeMap(const TextureCubeMapHandle handle)
{
	if (textureCubeMaps.contains(handle))
	{
		return textureCubeMaps[handle];
	}

	throw std::runtime_error("TextureCubeMap with provided handle does not exist!");
}

VkDescriptorImageInfo Hmck::ResourceManager::getTextureCubeMapDescriptorImageInfo(const TextureCubeMapHandle handle)
{
	return getTextureCubeMap(handle)->descriptor;
}

void Hmck::ResourceManager::bindDescriptorSet(
	const VkCommandBuffer commandBuffer,
	const VkPipelineBindPoint bindPoint,
	const VkPipelineLayout pipelineLayout,
	const uint32_t firstSet,
	const uint32_t descriptorCount,
	const DescriptorSetHandle descriptorSet,
	const uint32_t dynamicOffsetCount,
	const uint32_t* pDynamicOffsets)
{
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

void Hmck::ResourceManager::destroyBuffer(const BufferHandle handle)
{
	buffers.erase(handle);
}

void Hmck::ResourceManager::destroyDescriptorSetLayout(const DescriptorSetLayoutHandle handle)
{
	descriptorSetLayouts.erase(handle);
}

void Hmck::ResourceManager::destroyTexture2D(const Texture2DHandle handle) const {
	getTexture2D(handle)->destroy(device);
	texture2Ds.erase(handle);
}

void Hmck::ResourceManager::bindVertexBuffer(const BufferHandle handle, const VkCommandBuffer commandBuffer)
{
	VkDeviceSize offsets[] = { 0 };
	VkBuffer buffers[] = { getBuffer(handle)->getBuffer() };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
}

void Hmck::ResourceManager::bindVertexBuffer(const BufferHandle vertexBuffer, const BufferHandle indexBuffer, const VkCommandBuffer commandBuffer, VkIndexType indexType)
{
	bindVertexBuffer(vertexBuffer, commandBuffer);
	bindIndexBuffer(indexBuffer, commandBuffer);
}

void Hmck::ResourceManager::bindIndexBuffer(const BufferHandle handle, const VkCommandBuffer commandBuffer, const VkIndexType indexType)
{
	vkCmdBindIndexBuffer(commandBuffer, getBuffer(handle)->getBuffer(), 0, indexType);
}

void Hmck::ResourceManager::destroyTextureCubeMap(const TextureCubeMapHandle handle) const {
	getTextureCubeMap(handle)->destroy(device);
	textureCubeMaps.erase(handle);
}

void Hmck::ResourceManager::copyBuffer(const BufferHandle from, const BufferHandle to) const {
	auto& fromBuffer = getBuffer(from);
	auto& toBuffer = getBuffer(to);

	device.copyBuffer(fromBuffer->getBuffer(), toBuffer->getBuffer(), fromBuffer->getBufferSize());
}


