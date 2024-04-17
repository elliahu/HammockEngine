#pragma once
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "HmckEntity.h"

namespace Hmck
{
	class ILight : public Entity
	{
	public:
		ILight() : Entity() {};
		glm::vec3 color{ 1.f,1.f,1.f };
	};

	class OmniLight :public ILight
	{
	public:
		OmniLight() : ILight() {};
	};

}

