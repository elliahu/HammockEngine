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

		Entity() : id{ currentId++ } {}

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

		static Id currentId;
		Id id;
		std::string name{};
		std::shared_ptr<Entity> parent{};
		std::vector<std::shared_ptr<Entity>> children{};
		TransformComponent transform{};
		EntityComponentMap components{};
		bool visible = false;

	};
}