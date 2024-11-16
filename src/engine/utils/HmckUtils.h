#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>

#include <functional>
#include <fstream>
#include <iostream>
#include <string>
#include <random>
#include <cmath>
#include <memory>

#include "resources/HmckDescriptors.h"
#include "resources/HmckBuffer.h"

namespace Hmck {
    namespace Init {
        inline VkImageCreateInfo imageCreateInfo() {
            VkImageCreateInfo imageCreateInfo{};
            imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            return imageCreateInfo;
        }

        inline VkMemoryAllocateInfo memoryAllocateInfo() {
            VkMemoryAllocateInfo memAllocInfo{};
            memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            return memAllocInfo;
        }

        inline VkImageViewCreateInfo imageViewCreateInfo() {
            VkImageViewCreateInfo imageViewCreateInfo{};
            imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            return imageViewCreateInfo;
        }

        /** @brief Initialize an image memory barrier with no image transfer ownership */
        inline VkImageMemoryBarrier imageMemoryBarrier() {
            VkImageMemoryBarrier imageMemoryBarrier{};
            imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            return imageMemoryBarrier;
        }


        inline VkSamplerCreateInfo samplerCreateInfo() {
            VkSamplerCreateInfo samplerCreateInfo{};
            samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            samplerCreateInfo.maxAnisotropy = 1.0f;
            return samplerCreateInfo;
        }

        inline VkFramebufferCreateInfo framebufferCreateInfo() {
            VkFramebufferCreateInfo framebufferCreateInfo{};
            framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            return framebufferCreateInfo;
        }

        inline VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState(
            VkColorComponentFlags colorWriteMask,
            VkBool32 blendEnable) {
            VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState{};
            pipelineColorBlendAttachmentState.colorWriteMask = colorWriteMask;
            pipelineColorBlendAttachmentState.blendEnable = blendEnable;
            pipelineColorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            pipelineColorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            pipelineColorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
            pipelineColorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            pipelineColorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            pipelineColorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
            pipelineColorBlendAttachmentState.colorWriteMask =
                    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                    VK_COLOR_COMPONENT_A_BIT;
            return pipelineColorBlendAttachmentState;
        }

        inline VkViewport viewport(const float width, const float height, const float minDepth, const float maxDepth) {
            VkViewport viewport{};
            viewport.width = width;
            viewport.height = height;
            viewport.minDepth = minDepth;
            viewport.maxDepth = maxDepth;
            return viewport;
        }

        inline VkViewport viewportFlipped(const float width, const float height, const float minDepth, const float maxDepth) {
            VkViewport viewport{};
            viewport.width = width;
            viewport.height = -height;
            viewport.minDepth = minDepth;
            viewport.maxDepth = maxDepth;
            viewport.x = 0.0f;
            viewport.y = height;
            return viewport;
        }

        inline VkRect2D rect2D(const int32_t width, const int32_t height, const int32_t offsetX, const int32_t offsetY) {
            VkRect2D rect2D{};
            rect2D.extent.width = width;
            rect2D.extent.height = height;
            rect2D.offset.x = offsetX;
            rect2D.offset.y = offsetY;
            return rect2D;
        }

        /** @brief Initialize a map entry for a shader specialization constant */
        inline VkSpecializationMapEntry specializationMapEntry(const uint32_t constantID, const uint32_t offset, const size_t size) {
            VkSpecializationMapEntry specializationMapEntry{};
            specializationMapEntry.constantID = constantID;
            specializationMapEntry.offset = offset;
            specializationMapEntry.size = size;
            return specializationMapEntry;
        }

        /** @brief Initialize a specialization constant info structure to pass to a shader stage */
        inline VkSpecializationInfo specializationInfo(const uint32_t mapEntryCount,
                                                       const VkSpecializationMapEntry *mapEntries, const size_t dataSize,
                                                       const void *data) {
            VkSpecializationInfo specializationInfo{};
            specializationInfo.mapEntryCount = mapEntryCount;
            specializationInfo.pMapEntries = mapEntries;
            specializationInfo.dataSize = dataSize;
            specializationInfo.pData = data;
            return specializationInfo;
        }

        /** @brief Initialize a specialization constant info structure to pass to a shader stage */
        inline VkSpecializationInfo specializationInfo(const std::vector<VkSpecializationMapEntry> &mapEntries,
                                                       const size_t dataSize, const void *data) {
            VkSpecializationInfo specializationInfo{};
            specializationInfo.mapEntryCount = static_cast<uint32_t>(mapEntries.size());
            specializationInfo.pMapEntries = mapEntries.data();
            specializationInfo.dataSize = dataSize;
            specializationInfo.pData = data;
            return specializationInfo;
        }

        inline VkRenderPassBeginInfo renderPassBeginInfo() {
            VkRenderPassBeginInfo renderPassBeginInfo{};
            renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            return renderPassBeginInfo;
        }

        inline VkRenderPassCreateInfo renderPassCreateInfo() {
            VkRenderPassCreateInfo renderPassCreateInfo{};
            renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            return renderPassCreateInfo;
        }

        inline VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo(
            const std::vector<VkDescriptorSetLayoutBinding> &bindings) {
            VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
            descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            descriptorSetLayoutCreateInfo.pBindings = bindings.data();
            descriptorSetLayoutCreateInfo.bindingCount = static_cast<uint32_t>(bindings.size());
            return descriptorSetLayoutCreateInfo;
        }

        inline VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo(
            const VkDescriptorSetLayout *pSetLayouts,
            uint32_t setLayoutCount = 1) {
            VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
            pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutCreateInfo.setLayoutCount = setLayoutCount;
            pipelineLayoutCreateInfo.pSetLayouts = pSetLayouts;
            return pipelineLayoutCreateInfo;
        }

        inline VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo(
            uint32_t setLayoutCount = 1) {
            VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
            pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutCreateInfo.setLayoutCount = setLayoutCount;
            return pipelineLayoutCreateInfo;
        }

        inline VkDescriptorSetAllocateInfo descriptorSetAllocateInfo(
            const VkDescriptorPool descriptorPool,
            const VkDescriptorSetLayout *pSetLayouts,
            const uint32_t descriptorSetCount) {
            VkDescriptorSetAllocateInfo descriptorSetAllocateInfo{};
            descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            descriptorSetAllocateInfo.descriptorPool = descriptorPool;
            descriptorSetAllocateInfo.pSetLayouts = pSetLayouts;
            descriptorSetAllocateInfo.descriptorSetCount = descriptorSetCount;
            return descriptorSetAllocateInfo;
        }

        inline VkDescriptorPoolCreateInfo descriptorPoolCreateInfo(
            const uint32_t poolSizeCount,
            const VkDescriptorPoolSize *pPoolSizes,
            const uint32_t maxSets) {
            VkDescriptorPoolCreateInfo descriptorPoolInfo{};
            descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            descriptorPoolInfo.poolSizeCount = poolSizeCount;
            descriptorPoolInfo.pPoolSizes = pPoolSizes;
            descriptorPoolInfo.maxSets = maxSets;
            return descriptorPoolInfo;
        }

        inline VkDescriptorPoolCreateInfo descriptorPoolCreateInfo(
            const std::vector<VkDescriptorPoolSize> &poolSizes,
            uint32_t maxSets) {
            VkDescriptorPoolCreateInfo descriptorPoolInfo{};
            descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
            descriptorPoolInfo.pPoolSizes = poolSizes.data();
            descriptorPoolInfo.maxSets = maxSets;
            return descriptorPoolInfo;
        }

        inline VkDescriptorPoolSize descriptorPoolSize(
            VkDescriptorType type,
            uint32_t descriptorCount) {
            VkDescriptorPoolSize descriptorPoolSize{};
            descriptorPoolSize.type = type;
            descriptorPoolSize.descriptorCount = descriptorCount;
            return descriptorPoolSize;
        }

        inline VkCommandBufferAllocateInfo commandBufferAllocateInfo(
            VkCommandPool commandPool,
            VkCommandBufferLevel level,
            uint32_t bufferCount) {
            VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
            commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            commandBufferAllocateInfo.commandPool = commandPool;
            commandBufferAllocateInfo.level = level;
            commandBufferAllocateInfo.commandBufferCount = bufferCount;
            return commandBufferAllocateInfo;
        }

        inline VkCommandPoolCreateInfo commandPoolCreateInfo() {
            VkCommandPoolCreateInfo cmdPoolCreateInfo{};
            cmdPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            return cmdPoolCreateInfo;
        }

        inline VkCommandBufferBeginInfo commandBufferBeginInfo() {
            VkCommandBufferBeginInfo cmdBufferBeginInfo{};
            cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            return cmdBufferBeginInfo;
        }

        inline VkCommandBufferInheritanceInfo commandBufferInheritanceInfo() {
            VkCommandBufferInheritanceInfo cmdBufferInheritanceInfo{};
            cmdBufferInheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
            return cmdBufferInheritanceInfo;
        }

        inline VkDescriptorSetLayoutBinding descriptorSetLayoutBinding(
            VkDescriptorType type,
            VkShaderStageFlags stageFlags,
            uint32_t binding,
            uint32_t descriptorCount = 1) {
            VkDescriptorSetLayoutBinding setLayoutBinding{};
            setLayoutBinding.descriptorType = type;
            setLayoutBinding.stageFlags = stageFlags;
            setLayoutBinding.binding = binding;
            setLayoutBinding.descriptorCount = descriptorCount;
            return setLayoutBinding;
        }

        inline VkWriteDescriptorSet writeDescriptorSet(
            VkDescriptorSet dstSet,
            VkDescriptorType type,
            uint32_t binding,
            VkDescriptorBufferInfo *bufferInfo,
            uint32_t descriptorCount = 1) {
            VkWriteDescriptorSet writeDescriptorSet{};
            writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeDescriptorSet.dstSet = dstSet;
            writeDescriptorSet.descriptorType = type;
            writeDescriptorSet.dstBinding = binding;
            writeDescriptorSet.pBufferInfo = bufferInfo;
            writeDescriptorSet.descriptorCount = descriptorCount;
            return writeDescriptorSet;
        }

        inline VkWriteDescriptorSet writeDescriptorSet(
            VkDescriptorSet dstSet,
            VkDescriptorType type,
            uint32_t binding,
            VkDescriptorImageInfo *imageInfo,
            uint32_t descriptorCount = 1) {
            VkWriteDescriptorSet writeDescriptorSet{};
            writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeDescriptorSet.dstSet = dstSet;
            writeDescriptorSet.descriptorType = type;
            writeDescriptorSet.dstBinding = binding;
            writeDescriptorSet.pImageInfo = imageInfo;
            writeDescriptorSet.descriptorCount = descriptorCount;
            return writeDescriptorSet;
        }

        // Ray tracing related
        inline VkAccelerationStructureGeometryKHR accelerationStructureGeometryKHR() {
            VkAccelerationStructureGeometryKHR accelerationStructureGeometryKHR{};
            accelerationStructureGeometryKHR.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
            return accelerationStructureGeometryKHR;
        }

        inline VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfoKHR() {
            VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfoKHR{};
            accelerationStructureBuildGeometryInfoKHR.sType =
                    VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
            return accelerationStructureBuildGeometryInfoKHR;
        }

        inline VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfoKHR() {
            VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfoKHR{};
            accelerationStructureBuildSizesInfoKHR.sType =
                    VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
            return accelerationStructureBuildSizesInfoKHR;
        }

        inline VkRayTracingShaderGroupCreateInfoKHR rayTracingShaderGroupCreateInfoKHR() {
            VkRayTracingShaderGroupCreateInfoKHR rayTracingShaderGroupCreateInfoKHR{};
            rayTracingShaderGroupCreateInfoKHR.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
            return rayTracingShaderGroupCreateInfoKHR;
        }

        inline VkRayTracingPipelineCreateInfoKHR rayTracingPipelineCreateInfoKHR() {
            VkRayTracingPipelineCreateInfoKHR rayTracingPipelineCreateInfoKHR{};
            rayTracingPipelineCreateInfoKHR.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
            return rayTracingPipelineCreateInfoKHR;
        }

        inline VkWriteDescriptorSetAccelerationStructureKHR writeDescriptorSetAccelerationStructureKHR() {
            VkWriteDescriptorSetAccelerationStructureKHR writeDescriptorSetAccelerationStructureKHR{};
            writeDescriptorSetAccelerationStructureKHR.sType =
                    VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
            return writeDescriptorSetAccelerationStructureKHR;
        }
    } // namespace Init

    // dark magic from: https://stackoverflow.com/a/57595105
    template<typename T, typename... Rest>
    void hashCombine(std::size_t &seed, const T &v, const Rest &... rest) {
        seed ^= std::hash<T>{}(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        (hashCombine(seed, rest), ...);
    };

    inline void checkResult(VkResult result) {
        assert(result == VK_SUCCESS && "Vulkan API assertion fail");
    }

    // Check if enity of type P is derived from T
    template<typename T, typename P>
    bool isInstanceOf(std::shared_ptr<T> entity) {
        std::shared_ptr<P> derived = std::dynamic_pointer_cast<P>(entity);
        return derived != nullptr;
    }

    // tries cast T to P
    template<typename T, typename P>
    std::shared_ptr<P> cast(std::shared_ptr<T> entity) {
        std::shared_ptr<P> derived = std::dynamic_pointer_cast<P>(entity);
        if (!derived) {
            throw std::runtime_error("Dynamic cast failed: Cannot cast object to specified derived type.");
        }
        return derived;
    }

    inline bool hasExtension(std::string fullString, std::string ending) {
        if (fullString.length() >= ending.length()) {
            return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
        } else {
            return false;
        }
    }

    inline void setImageLayout(
        VkCommandBuffer cmdbuffer,
        VkImage image,
        VkImageLayout oldImageLayout,
        VkImageLayout newImageLayout,
        VkImageSubresourceRange subresourceRange,
        VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT) {
        // Create an image barrier object
        VkImageMemoryBarrier imageMemoryBarrier = Init::imageMemoryBarrier();
        imageMemoryBarrier.oldLayout = oldImageLayout;
        imageMemoryBarrier.newLayout = newImageLayout;
        imageMemoryBarrier.image = image;
        imageMemoryBarrier.subresourceRange = subresourceRange;

        // Source layouts (old)
        // Source access mask controls actions that have to be finished on the old layout
        // before it will be transitioned to the new layout
        switch (oldImageLayout) {
            case VK_IMAGE_LAYOUT_UNDEFINED:
                // Image layout is undefined (or does not matter)
                // Only valid as initial layout
                // No flags required, listed only for completeness
                imageMemoryBarrier.srcAccessMask = 0;
                break;

            case VK_IMAGE_LAYOUT_PREINITIALIZED:
                // Image is preinitialized
                // Only valid as initial layout for linear images, preserves memory contents
                // Make sure host writes have been finished
                imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                // Image is a color attachment
                // Make sure any writes to the color buffer have been finished
                imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                // Image is a depth/stencil attachment
                // Make sure any writes to the depth/stencil buffer have been finished
                imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                // Image is a transfer source
                // Make sure any reads from the image have been finished
                imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                break;

            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                // Image is a transfer destination
                // Make sure any writes to the image have been finished
                imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                // Image is read by a shader
                // Make sure any shader reads from the image have been finished
                imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
                break;
            default:
                // Other source layouts aren't handled (yet)
                break;
        }

        // Target layouts (new)
        // Destination access mask controls the dependency for the new image layout
        switch (newImageLayout) {
            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                // Image will be used as a transfer destination
                // Make sure any writes to the image have been finished
                imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                // Image will be used as a transfer source
                // Make sure any reads from the image have been finished
                imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                break;

            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                // Image will be used as a color attachment
                // Make sure any writes to the color buffer have been finished
                imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                // Image layout will be used as a depth/stencil attachment
                // Make sure any writes to depth/stencil buffer have been finished
                imageMemoryBarrier.dstAccessMask =
                        imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                // Image will be read in a shader (sampler, input attachment)
                // Make sure any writes to the image have been finished
                if (imageMemoryBarrier.srcAccessMask == 0) {
                    imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
                }
                imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                break;
            default:
                // Other source layouts aren't handled (yet)
                break;
        }

        // Put barrier inside setup command buffer
        vkCmdPipelineBarrier(
            cmdbuffer,
            srcStageMask,
            dstStageMask,
            0,
            0, nullptr,
            0, nullptr,
            1, &imageMemoryBarrier);
    }

    // Fixed sub resource on first mip level and layer
    inline void setImageLayout(
        VkCommandBuffer cmdbuffer,
        VkImage image,
        VkImageAspectFlags aspectMask,
        VkImageLayout oldImageLayout,
        VkImageLayout newImageLayout,
        VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT) {
        VkImageSubresourceRange subresourceRange = {};
        subresourceRange.aspectMask = aspectMask;
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = 1;
        subresourceRange.layerCount = 1;
        setImageLayout(cmdbuffer, image, oldImageLayout, newImageLayout, subresourceRange, srcStageMask, dstStageMask);
    }


    namespace Filesystem {
        inline bool fileExists(const std::string &filename) {
            std::ifstream f(filename.c_str());
            return !f.fail();
        }

        inline std::vector<char> readFile(const std::string &filePath) {
            std::ifstream file{filePath, std::ios::ate | std::ios::binary};

            if (!file.is_open()) {
                throw std::runtime_error("failed to open file: " + filePath);
            }

            size_t fileSize = static_cast<size_t>(file.tellg());
            std::vector<char> buffer(fileSize);

            file.seekg(0);
            file.read(buffer.data(), fileSize);

            file.close();
            return buffer;
        }

        inline void dump(const std::string &filename, const std::string &data) {
            std::ofstream outFile(filename);
            if (outFile.is_open()) {
                outFile << data;
                outFile.close();
            } else {
                throw std::runtime_error("could not dump into file!");
            }
        }
    } // namespace Filesystem

    namespace Math {
        inline float lerp(float a, float b, float f) {
            return a + f * (b - a);
        }

        inline glm::mat4 normal(glm::mat4 &model) {
            return glm::transpose(glm::inverse(model));
        }

        inline glm::vec3 decomposeTranslation(glm::mat4 &transform) {
            return transform[3];
        }

        inline glm::vec3 decomposeRotation(glm::mat4 &transform) {
            glm::vec3 scale, translation, skew;
            glm::vec4 perspective;
            glm::quat orientation;
            glm::decompose(transform, scale, orientation, translation, skew, perspective);
            return glm::eulerAngles(orientation);
        }

        inline glm::vec3 decomposeScale(glm::mat4 &transform) {
            return {
                glm::length(transform[0]),
                glm::length(transform[1]),
                glm::length(transform[2])
            };
        }

        inline uint32_t padSizeToMinAlignment(uint32_t originalSize, uint32_t minAlignment) {
            return (originalSize + minAlignment - 1) & ~(minAlignment - 1);
        }
    }

    namespace Rnd {
        inline std::unique_ptr<Buffer> createSSAOKernel(Device &device, uint32_t kernelSize) {
            std::default_random_engine rndEngine((unsigned) time(nullptr));
            std::uniform_real_distribution<float> rndDist(0.0f, 1.0f);

            std::vector<glm::vec4> ssaoKernel{kernelSize};
            std::unique_ptr<Buffer> ssaoKernelBuffer{};

            for (uint32_t i = 0; i < kernelSize; ++i) {
                glm::vec3 sample(rndDist(rndEngine) * 2.0 - 1.0, rndDist(rndEngine) * 2.0 - 1.0, rndDist(rndEngine));
                sample = glm::normalize(sample);
                sample *= rndDist(rndEngine);
                float scale = float(i) / float(kernelSize);
                scale = Hmck::Math::lerp(0.1f, 1.0f, scale * scale);
                ssaoKernel[i] = glm::vec4(sample * scale, 0.0f);
            }

            ssaoKernelBuffer = std::make_unique<Buffer>(
                device,
                ssaoKernel.size() * sizeof(glm::vec4),
                1,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            );
            ssaoKernelBuffer->map();
            ssaoKernelBuffer->writeToBuffer(ssaoKernel.data());
            return ssaoKernelBuffer;
        }
    }
} // namespace Hmck
