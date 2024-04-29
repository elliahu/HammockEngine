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
	typedef uint32_t EntityId;

	class Entity
	{
	public:

		Entity() : id{ currentId++ }{}
		virtual ~Entity(){}

		static void restIds() { currentId == 1; };

		static EntityId currentId;
		EntityId id;
		std::string name = "";
		EntityId parent{};
		std::vector<EntityId> children{};
		TransformComponent transform{};
		bool visible = false;

	};
}