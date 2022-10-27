#pragma once

#include "HmckModel.h"

#include <glm/gtc/matrix_transform.hpp>

#include <memory>
#include <unordered_map>

namespace Hmck
{
	// good reading:
	// https://www.raywenderlich.com/2806-introduction-to-component-based-architecture-in-games
	// https://austinmorlan.com/posts/entity_component_system/

	/*
		In this case:
		GameObject: represents a general-purpose object
		Component: A component labels an GameObject as possessing a particular aspect, and holds the data needed to model that aspect
		System: A system is a process which acts on all GameObjects with the desired components. Systems are defined in the Systems folder
	*/


	/*
		TransformComponent
	*/
	struct HmckTransformComponent
	{
		glm::vec3 translation{};
		glm::vec3 scale{ 1.0f, 1.0f, 1.0f };
		glm::vec3 rotation{};

		// Matrix corrsponds to Translate * Ry * Rx * Rz * Scale
		// Rotations correspond to Tait-bryan angles of Y(1), X(2), Z(3)
		// https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix
		glm::mat4 mat4();
		glm::mat3 normalMatrix();
	};

	/*
		PointLightComponent
		If this component is set, model should NOT be set
		Set this component to make a spot light from the GameObject
		color property is used to make a colored light
	*/
	struct HmckPointLightComponent
	{
		float lightIntensity = 1.0f;
	};
	

	/*
		BoundingBoxComponent
		Axis-Aligned Bounding Box is used for collision detection
	*/
	struct HmckBoundingBoxComponent
	{
		struct HmckBoundingBoxAxis
		{
			float min;
			float max;
		};

		HmckBoundingBoxAxis x;
		HmckBoundingBoxAxis y;
		HmckBoundingBoxAxis z;
	};


	/*
		GameObject
		Class representing a single entity in the game world
		Uses Entity Component System architecture (sort of)
	*/
	class HmckGameObject
	{
	public:
		using id_t = unsigned int;
		using Map = std::unordered_map<id_t, HmckGameObject>;

		static HmckGameObject createGameObject()
		{
			static id_t currentId = 0;
			return HmckGameObject(currentId++);
		}

		static HmckGameObject createPointLight(
			float intensity = 10.f, 
			float radius = 0.1f, 
			glm::vec3 color = glm::vec3(1));

		void fitBoundingBox(
			HmckBoundingBoxComponent::HmckBoundingBoxAxis x,
			HmckBoundingBoxComponent::HmckBoundingBoxAxis y,
			HmckBoundingBoxComponent::HmckBoundingBoxAxis z);

		HmckGameObject(const HmckGameObject&) = delete;
		HmckGameObject& operator =(const HmckGameObject&) = delete;
		HmckGameObject(HmckGameObject&&) = default;
		HmckGameObject& operator=(HmckGameObject&&) = default;

		const id_t getId() { return id; }

		// Components
		std::shared_ptr<HmckModel> model{};

		glm::vec3 color{};

		HmckTransformComponent transform{}; // every game object has this

		std::unique_ptr<HmckPointLightComponent> pointLight = nullptr;

		std::unique_ptr<HmckBoundingBoxComponent> boundingBox = nullptr;

	private:
		HmckGameObject(id_t objId) : id{ objId } {}

		id_t id;

	};
}