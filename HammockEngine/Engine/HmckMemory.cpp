#include "HmckMemory.h"

namespace Hmck
{
	std::unordered_map<UniformBufferHandle, std::unique_ptr<Buffer>> MemoryManager::uniformBuffers;
	std::unordered_map<DescriptorSetHandle, VkDescriptorSet> MemoryManager::descriptorSets;
	std::unordered_map<DescriptorSetLayoutHandle, std::unique_ptr<DescriptorSetLayout>> MemoryManager::descriptorSetLayouts;
	std::unordered_map<Texture2DHandle, std::unique_ptr<Texture2D>> MemoryManager::texture2Ds;
	std::unordered_map<Texture2DHandle, std::unique_ptr<TextureCubeMap>> MemoryManager::textureCubeMaps;

	const uint32_t MemoryManager::INVALID_HANDLE = 0;
	uint32_t MemoryManager::uniformBuffersLastHandle = 1;
	uint32_t MemoryManager::descriptorSetsLastHandle = 1;
	uint32_t MemoryManager::descriptorSetLayoutsLastHandle = 1;
	uint32_t MemoryManager::texture2DsLastHandle = 1;
	uint32_t MemoryManager::textureCubeMapsLastHandle = 1;
}

Hmck::MemoryManager::MemoryManager(Device& device) :device{ device }
{
	descriptorPool = DescriptorPool::Builder(device)
		.setMaxSets(10000)
		.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 5000)
		.addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 5000)
		.build();
}

Hmck::UniformBufferHandle Hmck::MemoryManager::createUniformBuffer(UniformBufferCreateInfo createInfo)
{
	auto buffer = std::make_unique<Buffer>(
		device,
		createInfo.instanceSize,
		createInfo.instanceCount,
		createInfo.usageFlags,
		createInfo.memoryPropertyFlags);
	buffer->map();

	uniformBuffers.emplace(uniformBuffersLastHandle, std::move(buffer));
	UniformBufferHandle handle = uniformBuffersLastHandle;
	uniformBuffersLastHandle++;
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

std::unique_ptr<Hmck::Buffer>& Hmck::MemoryManager::getUniformBuffer(UniformBufferHandle handle)
{
	if (uniformBuffers.contains(handle))
	{
		return uniformBuffers[handle];
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

void Hmck::MemoryManager::destroyUniformBuffer(UniformBufferHandle handle)
{
	uniformBuffers.erase(handle);
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


