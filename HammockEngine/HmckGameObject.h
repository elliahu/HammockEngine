#pragma once

#include "HmckModel.h"

#include <memory>

namespace Hmck
{
	// good reading:
	// https://www.raywenderlich.com/2806-introduction-to-component-based-architecture-in-games
	// https://austinmorlan.com/posts/entity_component_system/


	struct HmckTransform2dComponent
	{
		glm::vec2 translation{};
		glm::vec2 scale{ 1.0f, 1.0f };
		float rotation;

		glm::mat2 mat2() 
		{
			const float s = glm::sin(rotation);
			const float c = glm::cos(rotation);

			glm::mat2 rotMat{ {c,s},{-s, c} };
			glm::mat2 scaleMat{ {scale.x, .0f},{.0f, scale.y} };
			return rotMat * scaleMat;
		}
	};
	
	class HmckGameObject
	{
	public:
		using id_t = unsigned int;
		
		static HmckGameObject createGameObject()
		{
			static id_t currentId = 0;
			return HmckGameObject(currentId++);
		}

		HmckGameObject(const HmckGameObject&) = delete;
		HmckGameObject& operator =(const HmckGameObject&) = delete;
		HmckGameObject(HmckGameObject&&) = default;
		HmckGameObject& operator=(HmckGameObject&&) = default;

		const id_t getId() { return id; }

		std::shared_ptr<HmckModel> model{};
		glm::vec3 color{};
		HmckTransform2dComponent transform2d{};

	private:
		HmckGameObject(id_t objId) : id{ objId } {}

		id_t id;

	};
}