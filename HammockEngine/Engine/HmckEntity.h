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

		Entity() : id{ currentId++ }
		{
		}

		virtual ~Entity() 
		{
			// ensures that before parent Entity is deleted, all of its children are deleted as well
			for (auto& child : children) {
				child = nullptr;
			}
		}

		uint32_t numberOfEntities()
		{
			uint32_t total = 1;
			for (auto& child : children)
			{
				total += child->numberOfEntities();
			}

			return total;
		}

		std::shared_ptr<Entity> getChild(Id id);


		static Id currentId;
		Id id;
		std::string name{};
		std::shared_ptr<Entity> parent{};
		std::vector<std::shared_ptr<Entity>> children{};
		TransformComponent transform{};
		bool visible = false;

	};
}