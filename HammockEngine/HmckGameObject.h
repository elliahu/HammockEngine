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

	struct HmckPointLightComponent
	{
		float lightIntensity = 1.0f;

	};

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

		HmckGameObject(const HmckGameObject&) = delete;
		HmckGameObject& operator =(const HmckGameObject&) = delete;
		HmckGameObject(HmckGameObject&&) = default;
		HmckGameObject& operator=(HmckGameObject&&) = default;

		const id_t getId() { return id; }

		std::shared_ptr<HmckModel> model{};
		glm::vec3 color{};
		HmckTransformComponent transform{}; // every game object has this

		std::unique_ptr<HmckPointLightComponent> pointLight = nullptr;

	private:
		HmckGameObject(id_t objId) : id{ objId } {}

		id_t id;

	};
}