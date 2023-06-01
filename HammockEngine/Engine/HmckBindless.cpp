#include "HmckBindless.h"

void Hmck::HmckBindless::destroy(HmckDevice& device)
{
    vkDestroyDescriptorSetLayout(device.device(), bindlessLayout, nullptr);
}

Hmck::TextureHandle Hmck::HmckBindless::addTexture(HmckDevice& device, HmckTexture& texture)
{
    uint32_t handle = currentWriteIndex;

    VkWriteDescriptorSet descriptor_write{};
    descriptor_write = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
    descriptor_write.descriptorCount = 1;
    descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptor_write.dstSet = bindlessDescriptorSet;
    descriptor_write.dstBinding = TEXTURE_BINDING;

    VkDescriptorImageInfo descriptor_image_info{};
    descriptor_image_info.sampler = texture.sampler;
    descriptor_image_info.imageView = texture.view;
    descriptor_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    descriptor_write.pImageInfo = &descriptor_image_info;

    vkUpdateDescriptorSets(device.device(), handle, &descriptor_write, 0, nullptr);
    
    currentWriteIndex++;

    return static_cast<TextureHandle>(handle);
}

void Hmck::HmckBindless::createBindlessLayout(HmckDevice& device)
{
    VkDescriptorBindingFlags bindless_flags =  VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT | VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT;

    VkDescriptorSetLayoutBinding vk_binding;
    vk_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    vk_binding.descriptorCount = MAX_BINDLESS_RESOURCES;
    vk_binding.binding = TEXTURE_BINDING;

    vk_binding.stageFlags = VK_SHADER_STAGE_ALL;
    vk_binding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo layout_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    layout_info.bindingCount = 1;
    layout_info.pBindings = &vk_binding;
    layout_info.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT;

    VkDescriptorSetLayoutBindingFlagsCreateInfoEXT extended_info{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT, nullptr };
    extended_info.bindingCount = 1;
    extended_info.pBindingFlags = &bindless_flags;

    layout_info.pNext = &extended_info;

    checkResult(vkCreateDescriptorSetLayout(device.device(), &layout_info, nullptr, &bindlessLayout));
}


void Hmck::HmckBindless::createBindlessDescriptorSet(HmckDevice& device)
{
    VkDescriptorSetAllocateInfo alloc_info{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    alloc_info.descriptorPool = pool->descriptorPool;
    alloc_info.descriptorSetCount = 1;
    alloc_info.pSetLayouts = &bindlessLayout;

    VkDescriptorSetVariableDescriptorCountAllocateInfoEXT count_info{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT };
    uint32_t max_binding = MAX_BINDLESS_RESOURCES - 1;
    count_info.descriptorSetCount = 1;
    // This number is the max allocatable count
    count_info.pDescriptorCounts = &max_binding;
    alloc_info.pNext = &count_info;

    checkResult(vkAllocateDescriptorSets(device.device(), &alloc_info, &bindlessDescriptorSet));
}
