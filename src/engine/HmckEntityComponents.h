#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>

namespace Hmck
{
	typedef std::string EntityComponentLabel;

	struct EntityComponent
	{
		EntityComponentLabel label{};
	};

	struct TransformComponent: EntityComponent
	{
		glm::vec3 translation{ 0.0f };
		glm::vec3 scale{ 1.0f, 1.0f, 1.0f };
		glm::vec3 rotation{ 0.0f };

		// Matrix corrsponds to Translate * Ry * Rx * Rz * Scale
		// Rotations correspond to Tait-bryan angles of Y(1), X(2), Z(3)
		// https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix
		inline glm::mat4 mat4()
		{
			const float c3 = glm::cos(rotation.z);
			const float s3 = glm::sin(rotation.z);
			const float c2 = glm::cos(rotation.x);
			const float s2 = glm::sin(rotation.x);
			const float c1 = glm::cos(rotation.y);
			const float s1 = glm::sin(rotation.y);
			return glm::mat4{
				{
					scale.x * (c1 * c3 + s1 * s2 * s3),
					scale.x * (c2 * s3),
					scale.x * (c1 * s2 * s3 - c3 * s1),
					0.0f,
				},
				{
					scale.y * (c3 * s1 * s2 - c1 * s3),
					scale.y * (c2 * c3),
					scale.y * (c1 * c3 * s2 + s1 * s3),
					0.0f,
				},
				{
					scale.z * (c2 * s1),
					scale.z * (-s2),
					scale.z * (c1 * c2),
					0.0f,
				},
				{translation.x, translation.y, translation.z, 1.0f} };
		}
		inline glm::mat3 normalMatrix()
		{
			const float c3 = glm::cos(rotation.z);
			const float s3 = glm::sin(rotation.z);
			const float c2 = glm::cos(rotation.x);
			const float s2 = glm::sin(rotation.x);
			const float c1 = glm::cos(rotation.y);
			const float s1 = glm::sin(rotation.y);
			const glm::vec3 inverseScale = 1.0f / scale;

			return glm::mat3{
				{
					inverseScale.x * (c1 * c3 + s1 * s2 * s3),
					inverseScale.x * (c2 * s3),
					inverseScale.x * (c1 * s2 * s3 - c3 * s1),
				},
				{
					inverseScale.y * (c3 * s1 * s2 - c1 * s3),
					inverseScale.y * (c2 * c3),
					inverseScale.y * (c1 * c3 * s2 + s1 * s3),
				},
				{
					inverseScale.z * (c2 * s1),
					inverseScale.z * (-s2),
					inverseScale.z * (c1 * c2),
				} };
		}
	};

	/*
	* @deprecated
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
	* @deprecated
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
	* @deprecated
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