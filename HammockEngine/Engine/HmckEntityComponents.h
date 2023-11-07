#pragma once

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include <memory>
#include <unordered_map>

#include "HmckEntity.h"

namespace Hmck
{
	typedef std::string EntityComponentLabel;

	class EntityComponent
	{
		EntityComponentLabel label{};
	};

	/*
		PointLightComponent
		If this component is set, model should NOT be set
		Set this component to make a spot light from the GameObject
		color property is used to make a colored light
	*/
	struct OmniLightEntityComponent : public EntityComponent
	{
		float lightIntensity = 1.0f;
		float quadraticTerm = 1.0f;
		float linearTerm = .0f;
		float constantTerm = .0f;
		glm::vec3 color{ 1 };
	};

	/*
		DirectionalLightComponent
		If this component is set, model should NOT be set
		Set this component to make a dorectional light from the GameObject
	*/
	struct DirectionalLightEntityComponent : public EntityComponent
	{
		float lightIntensity = 1.0f;
		float fov = 50.0f;
		float _near = 0.1f;
		float _far = 100.f;
		glm::vec3 target{ 0 };
		glm::vec3 color{ 1 };
	};


	/*
		BoundingBoxComponent
		Axis-Aligned Bounding Box is used for collision detection
		TODO make sure to resize when scale changes
	*/
	struct BoundingBoxEntityComponent : public EntityComponent
	{
		struct BoundingBoxAxis
		{
			float min;
			float max;
		};

		BoundingBoxAxis x;
		BoundingBoxAxis y;
		BoundingBoxAxis z;
	};
}