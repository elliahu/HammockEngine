#pragma once
#include <string>
#include <iostream>
#include <memory>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <tiny_gltf.h>
#include "HmckTexture.h"
#include "HmckDescriptors.h"
#include "HmckEntityComponents.h"

namespace Hmck
{
	typedef unsigned int Id;

	class Entity
	{
	public:

		struct TransformUbo
		{
			glm::mat4 model{ 1.f };
			glm::mat4 normal{ 1.f };
		};

		Entity(Device& device, std::unique_ptr<DescriptorPool>& descriptorPool) : id{ currentId++ }, device{ device }
		{
			// prepare layout
			descriptorSetLayout = DescriptorSetLayout::Builder(device)
				.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
				.build();

			// prepare buffer
			buffer = std::make_unique<Buffer>(
				device,
				sizeof(TransformUbo),
				1,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
				);
			buffer->map();

			// write to descriptor
			auto bufferInfo = buffer->descriptorInfo();
			DescriptorWriter(*descriptorSetLayout, *descriptorPool)
				.writeBuffer(0, &bufferInfo)
				.build(descriptorSet);
		}

		virtual ~Entity() 
		{
			buffer = nullptr;
			descriptorSetLayout = nullptr;
			// ensures that before parent Entity is deleted, all of its children are deleted as well
			for (auto& child : children) {
				child = nullptr;
			}
		}

		void updateBuffer()
		{
			TransformUbo transformData{
				.model = transform.mat4(),
				.normal = transform.normalMatrix()
			};
			buffer->writeToBuffer(&transformData);
		}

		Device& device;

		static Id currentId;
		Id id;
		std::string name{};
		std::shared_ptr<Entity> parent{};
		std::vector<std::shared_ptr<Entity>> children{};
		TransformComponent transform{};
		bool visible = false;

		std::unique_ptr<DescriptorSetLayout> descriptorSetLayout;
		std::unique_ptr<Buffer> buffer;
		VkDescriptorSet descriptorSet;

	};
}