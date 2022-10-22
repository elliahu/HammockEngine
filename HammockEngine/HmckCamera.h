#pragma once


#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <cassert>
#include <limits>

namespace Hmck
{
	class HmckCamera
	{
	public:
		void setOrthographicProjection(float left, float right, float top, float bottom, float near, float far);
		void setPerspectiveProjection(float fovy, float aspect, float near, float far);

		const glm::mat4& getProjection() const { return projectionMatrix; }

	private:
		glm::mat4 projectionMatrix{ 1.f };

	};
}