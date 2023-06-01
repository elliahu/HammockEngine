#pragma once
#include <vulkan/vulkan.h>
#include <cstdint>
#include <vector>
#include <span>
#include <memory>
#include <cassert>

#include "HmckDevice.h"
#include "HmckDescriptors.h"
#include "HmckUtils.h"
#include "HmckTexture.h"

namespace Hmck
{
	enum class TextureHandle : uint32_t { Invalid = UINT32_MAX };
	enum class BufferHandle : uint32_t { Invalid = UINT32_MAX };

	class HmckBindless
	{
	public:

        HmckBindless(HmckDevice& device) 
        {
            pool = HmckDescriptorPool::Builder(device)
                .setPoolFlags(VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT)
                .setMaxSets(MAX_BINDLESS_RESOURCES)
                .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MAX_BINDLESS_RESOURCES)
                .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_BINDLESS_RESOURCES)
                .build();
            createBindlessLayout(device);
            createBindlessDescriptorSet(device);
        };

        // delete copy constructor and copy destructor
        HmckBindless(const HmckBindless&) = delete;
        HmckBindless& operator=(const HmckBindless&) = delete;

        void destroy(HmckDevice& device);

        TextureHandle addTexture(HmckDevice& device, HmckTexture& texture);

        VkDescriptorSetLayout bindlessLayout;
        VkDescriptorSet bindlessDescriptorSet;

    private:
        void createBindlessLayout(HmckDevice& device);
        void createBindlessDescriptorSet(HmckDevice& device);

        static const uint32_t MAX_BINDLESS_RESOURCES = 1024;
        static const uint32_t TEXTURE_BINDING = 0;

        std::unique_ptr<HmckDescriptorPool> pool;
        
        uint32_t currentWriteIndex = 0;
	};

}
