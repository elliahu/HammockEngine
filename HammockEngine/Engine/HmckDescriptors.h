#pragma once

#include "HmckDevice.h"

// std
#include <memory>
#include <unordered_map>
#include <vector>

namespace Hmck {

    class HmckDescriptorSetLayout {
    public:
        class Builder {
        public:
            Builder(HmckDevice& hmckDevice) : hmckDevice{ hmckDevice } {}

            Builder& addBinding(
                uint32_t binding,
                VkDescriptorType descriptorType,
                VkShaderStageFlags stageFlags,
                uint32_t count = 1
            );
            std::unique_ptr<HmckDescriptorSetLayout> build() const;

        private:
            HmckDevice& hmckDevice;
            std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings{};
        };

        HmckDescriptorSetLayout(
            HmckDevice& hmckDevice, std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings);
        ~HmckDescriptorSetLayout();
        HmckDescriptorSetLayout(const HmckDescriptorSetLayout&) = delete;
        HmckDescriptorSetLayout& operator=(const HmckDescriptorSetLayout&) = delete;

        VkDescriptorSetLayout getDescriptorSetLayout() const { return descriptorSetLayout; }

    private:
        HmckDevice& hmckDevice;
        VkDescriptorSetLayout descriptorSetLayout;
        std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings;

        friend class HmckDescriptorWriter;
    };

    class HmckDescriptorPool {
    public:
        class Builder {
        public:
            Builder(HmckDevice& hmckDevice) : hmckDevice{ hmckDevice } {}

            Builder& addPoolSize(VkDescriptorType descriptorType, uint32_t count);
            Builder& setPoolFlags(VkDescriptorPoolCreateFlags flags);
            Builder& setMaxSets(uint32_t count);
            std::unique_ptr<HmckDescriptorPool> build() const;

        private:
            HmckDevice& hmckDevice;
            std::vector<VkDescriptorPoolSize> poolSizes{};
            uint32_t maxSets = 1000;
            VkDescriptorPoolCreateFlags poolFlags = 0;
        };

        HmckDescriptorPool(
            HmckDevice& hmckDevice,
            uint32_t maxSets,
            VkDescriptorPoolCreateFlags poolFlags,
            const std::vector<VkDescriptorPoolSize>& poolSizes);
        ~HmckDescriptorPool();
        HmckDescriptorPool(const HmckDescriptorPool&) = delete;
        HmckDescriptorPool& operator=(const HmckDescriptorPool&) = delete;

        bool allocateDescriptor(
            const VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet& descriptor) const;

        void freeDescriptors(std::vector<VkDescriptorSet>& descriptors) const;

        void resetPool();
        VkDescriptorPool descriptorPool;

    private:
        HmckDevice& hmckDevice;
        

        friend class HmckDescriptorWriter;
    };

    class HmckDescriptorWriter {
    public:
        HmckDescriptorWriter(HmckDescriptorSetLayout& setLayout, HmckDescriptorPool& pool);

        HmckDescriptorWriter& writeBuffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo);
        HmckDescriptorWriter& writeImage(uint32_t binding, VkDescriptorImageInfo* imageInfo);

        bool build(VkDescriptorSet& set);
        void overwrite(VkDescriptorSet& set);

    private:
        HmckDescriptorSetLayout& setLayout;
        HmckDescriptorPool& pool;
        std::vector<VkWriteDescriptorSet> writes;
    };

}  // namespace hmck