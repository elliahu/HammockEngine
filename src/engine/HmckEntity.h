#pragma once
#include <string>
#include <iostream>
#include <memory>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <tiny_gltf.h>
#include "HmckDescriptors.h"
#include "HmckEntityComponents.h"

namespace Hmck
{
	typedef uint32_t EntityHandle;

	class Entity
	{
	public:

		Entity() : id{ currentId++ }{}
		virtual ~Entity(){}

		static void resetId() { currentId = 1; };

		inline void notifyChildrenDataChanged()
		{
			for (const auto& child : children) 
			{
				child->dataChanged = true;
				child->notifyChildrenDataChanged();
			}
		}

		glm::mat4 mat4();
		glm::mat4 mat4N();

		static EntityHandle currentId;
		EntityHandle id;
		std::string name = "";

		std::shared_ptr<Entity> parent{};
		std::vector<std::shared_ptr<Entity>> children{};

		TransformComponent transform{};

		bool visible = false;
		bool dataChanged = true;

	};
}