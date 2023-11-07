#pragma once
#include <string>
#include <iostream>

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
	typedef std::unordered_map<EntityComponentLabel, EntityComponent> EntityComponentMap;

	class Entity
	{
	public:

		// scalar float: N = 4 Bytes
		// vec2: 2N = 8 Bytes
		// vec3 (or vec4): 4N = 16 Bytes
		// taken from 15.6.4 Offset and Stride Assignment
		struct TransformPushConstantData {
			glm::mat4 modelMatrix{ 1.f };
			glm::mat4 normalMatrix{ 1.f };
			//uint32_t albedo_index{}; // TODO remove this later as this does not belong here and is over the minimum size
		};

		Entity() : id{ currentId++ } {};

		virtual ~Entity() {
			// ensures that before parent Entity is deleted, all of its children are deleted as well
			for (auto& child : children) {
				child = nullptr;
			}
		}

		static std::shared_ptr<Entity> createEntity()
		{
			return std::make_unique<Entity>();
		}

		float scalingFactor();
		glm::vec3 translation();
		glm::mat3 rotationMat();
		glm::vec3 rotation();

		void translate(glm::vec3 position);
		void rotate(glm::vec3 rotation);

		static Id currentId;
		Id id;
		std::string name{};
		glm::mat4 transform{ 1 }; // model matrix
		std::shared_ptr<Entity> parent{};
		std::vector<std::shared_ptr<Entity>> children{};
		EntityComponentMap components{};
		bool visible = false;

	};
}