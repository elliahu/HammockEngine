#pragma once


#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "HmckEntity.h"

namespace Hmck
{
	class Camera : public Entity
	{
	public:

		enum class ProjectionType
		{
			None, Orthographic, Perspective
		};

		void setOrthographicProjection(float left, float right, float top, float bottom, float _near, float _far);
		void setPerspectiveProjection(float fovy, float aspect, float near, float far);

		void setView(glm::vec3 position, glm::vec3 rotation);

		void update();


		const glm::mat4& getProjection() const { return projectionMatrix; }
		const glm::mat4& getView() const { return viewMatrix; }
		const glm::mat4& getInverseView() const { return inverseViewMatrix; }

		bool flipY = true;

	private:
		glm::mat4 projectionMatrix{ 1.f };
		glm::mat4 viewMatrix{ 1.f };
		glm::mat4 inverseViewMatrix{ 1.f };

		float left, right, top, bottom;
		float  _near, _far;
		float fovy, aspect;

		ProjectionType projectionType = ProjectionType::None;

	};
}