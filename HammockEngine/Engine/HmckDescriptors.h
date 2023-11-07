#pragma once

#include "HmckDevice.h"

// std
#include <memory>
#include <unordered_map>
#include <vector>

namespace Hmck {

    typedef const uint32_t Binding;

    class DescriptorSetLayout {
    public:
        class Builder {
        public:
            Builder(Device& hmckDevice) : hmckDevice{ hmckDevice } {}

            Builder& addBinding(
                uint32_t binding,
                VkDescriptorType descriptorType,
                VkShaderStageFlags stageFlags,
                uint32_t count = 1,
                VkDescriptorBindingFlags flags = 0
            );
            std::unique_ptr<DescriptorSetLayout> build() const;

        private:
            Device& hmckDevice;
            std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings{};
            std::unordered_map <uint32_t, VkDescriptorBindingFlags> bindingFlags{};
        };

        DescriptorSetLayout(
            Device& hmckDevice, 
            std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings,
            std::unordered_map <uint32_t, VkDescriptorBindingFlags> flags);
        ~DescriptorSetLayout();
        DescriptorSetLayout(const DescriptorSetLayout&) = delete;
        DescriptorSetLayout& operator=(const DescriptorSetLayout&) = delete;

        VkDescriptorSetLayout getDescriptorSetLayout() const { return descriptorSetLayout; }

    private:
        Device& hmckDevice;
        VkDescriptorSetLayout descriptorSetLayout;
        std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings;

        friend class DescriptorWriter;
    };

    class DescriptorPool {
    public:
        class Builder {
        public:
            Builder(Device& hmckDevice) : hmckDevice{ hmckDevice } {}

            Builder& addPoolSize(VkDescriptorType descriptorType, uint32_t count);
            Builder& setPoolFlags(VkDescriptorPoolCreateFlags flags);
            Builder& setMaxSets(uint32_t count);
            std::unique_ptr<DescriptorPool> build() const;

        private:
            Device& hmckDevice;
            std::vector<VkDescriptorPoolSize> poolSizes{};
            uint32_t maxSets = 1000;
            VkDescriptorPoolCreateFlags poolFlags = 0;
        };

        DescriptorPool(
            Device& hmckDevice,
            uint32_t maxSets,
            VkDescriptorPoolCreateFlags poolFlags,
            const std::vector<VkDescriptorPoolSize>& poolSizes);
        ~DescriptorPool();
        DescriptorPool(const DescriptorPool&) = delete;
        DescriptorPool& operator=(const DescriptorPool&) = delete;

        bool allocateDescriptor(
            const VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet& descriptor) const;

        void freeDescriptors(std::vector<VkDescriptorSet>& descriptors) const;

        void resetPool();
        VkDescriptorPool descriptorPool;

    private:
        Device& hmckDevice;
        

        friend class DescriptorWriter;
    };

    class DescriptorWriter {
    public:
        DescriptorWriter(DescriptorSetLayout& setLayout, DescriptorPool& pool);

        DescriptorWriter& writeBuffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo);
        DescriptorWriter& writeImage(uint32_t binding, VkDescriptorImageInfo* imageInfo);
        DescriptorWriter& writeImages(uint32_t binding, std::vector<VkDescriptorImageInfo>& imageInfo);

        bool build(VkDescriptorSet& set);
        void overwrite(VkDescriptorSet& set);

    private:
        DescriptorSetLayout& setLayout;
        DescriptorPool& pool;
        std::vector<VkWriteDescriptorSet> writes;
    };

}  // namespace hmck