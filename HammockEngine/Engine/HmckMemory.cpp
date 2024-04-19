#include "HmckMemory.h"

namespace Hmck
{
	std::unordered_map<UniformBufferHandle, std::unique_ptr<Buffer>> DescriptorManager::uniformBuffers;
	std::unordered_map<DescriptorSetHandle, VkDescriptorSet> DescriptorManager::descriptorSets;
	std::unordered_map<DescriptorSetLayoutHandle, std::unique_ptr<DescriptorSetLayout>> DescriptorManager::descriptorSetLayouts;

	uint32_t DescriptorManager::uniformBuffersLastHandle = 0;
	uint32_t DescriptorManager::descriptorSetsLastHandle = 0;
	uint32_t DescriptorManager::descriptorSetLayoutsLastHandle = 0;
}

Hmck::DescriptorManager::DescriptorManager(Device& device) :device{ device }
{
	descriptorPool = DescriptorPool::Builder(device)
		.setMaxSets(10000)
		.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 5000)
		.addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 5000)
		.build();
}

Hmck::DescriptorSetLayoutHandle Hmck::DescriptorManager::createDescriptorSetLayout(DescriptorSetLayoutCreateInfo createInfo)
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

Hmck::DescriptorSetHandle Hmck::DescriptorManager::createDescriptorSet(DescriptorSetCreateInfo createInfo)
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

Hmck::DescriptorSetLayout& Hmck::DescriptorManager::getDescriptorSetLayout(DescriptorSetLayoutHandle handle)
{
	if (descriptorSetLayouts.contains(handle))
	{
		return *descriptorSetLayouts[handle];
	}

	throw std::runtime_error("Descriptor set layout with provided handle does not exist!");
}

VkDescriptorSet Hmck::DescriptorManager::getDescriptorSet(DescriptorSetHandle handle)
{
	if (descriptorSets.contains(handle))
	{
		return descriptorSets[handle];
	}

	throw std::runtime_error("Descriptor with provided handle does not exist!");
}

std::unique_ptr<Hmck::Buffer>& Hmck::DescriptorManager::getUniformBuffer(UniformBufferHandle handle)
{
	if (uniformBuffers.contains(handle))
	{
		return uniformBuffers[handle];
	}

	throw std::runtime_error("Uniform buffer with provided handle does not exist!");
}

Hmck::UniformBufferHandle Hmck::DescriptorManager::createUniformBuffer(UniformBufferCreateInfo createInfo)
{
	auto buffer = std::make_unique<Buffer>(
		device,
		createInfo.instanceSize,
		createInfo.instanceCount,
		createInfo.usageFlags,
		createInfo.memoryPropertyFlags);

	uniformBuffers.emplace(uniformBuffersLastHandle, std::move(buffer));
	UniformBufferHandle handle = uniformBuffersLastHandle;
	uniformBuffersLastHandle++;
	return handle;
}
