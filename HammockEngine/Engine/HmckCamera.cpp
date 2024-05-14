#include "HmckCamera.h"

void Hmck::Camera::setOrthographicProjection(float left, float right, float top, float bottom, float _near, float _far)
{
	projectionType = ProjectionType::Orthographic;
	projectionMatrix = glm::mat4{ 1.0f };
	projectionMatrix[0][0] = 2.f / (right - left);
	projectionMatrix[1][1] = 2.f / (bottom - top);
	projectionMatrix[2][2] = 1.f / (_far - _near);
	projectionMatrix[3][0] = -(right + left) / (right - left);
	projectionMatrix[3][1] = -(bottom + top) / (bottom - top);
	projectionMatrix[3][2] = -_near / (_far - _near);
}

void Hmck::Camera::setPerspectiveProjection(float fovy, float aspect, float _near, float _far)
{
	assert(glm::abs(aspect - std::numeric_limits<float>::epsilon()) > 0.0f);
	projectionType = ProjectionType::Perspective;
	const float tanHalfFovy = tan(fovy / 2.f);
	projectionMatrix = glm::mat4{ 0.0f };
	projectionMatrix[0][0] = 1.f / (aspect * tanHalfFovy);
	projectionMatrix[1][1] = 1.f / (tanHalfFovy);
	projectionMatrix[2][2] = _far / (_far - _near);
	projectionMatrix[2][3] = 1.f;
	projectionMatrix[3][2] = -(_far * _near) / (_far - _near);
	if (flipY)
	{
		projectionMatrix[0][0] = - 1.f / (aspect * tanHalfFovy);
		projectionMatrix[1][1] = - 1.f / (tanHalfFovy);
	}
	
	this->fovy = fovy;
	this->aspect = aspect;
	this->_near = _near;
	this->_far = _far;
}

void Hmck::Camera::setView(glm::vec3 position, glm::vec3 rotation)
{
	transform.translation = position;
	transform.rotation = rotation;

	const float c3 = glm::cos(rotation.z);
	const float s3 = glm::sin(rotation.z);
	const float c2 = glm::cos(rotation.x);
	const float s2 = glm::sin(rotation.x);
	const float c1 = glm::cos(rotation.y);
	const float s1 = glm::sin(rotation.y);
	const glm::vec3 u{ (c1 * c3 + s1 * s2 * s3), (c2 * s3), (c1 * s2 * s3 - c3 * s1) };
	const glm::vec3 v{ (c3 * s1 * s2 - c1 * s3), (c2 * c3), (c1 * c3 * s2 + s1 * s3) };
	const glm::vec3 w{ (c2 * s1), (-s2), (c1 * c2) };
	viewMatrix = glm::mat4{ 1.f };
	viewMatrix[0][0] = u.x;
	viewMatrix[1][0] = u.y;
	viewMatrix[2][0] = u.z;
	viewMatrix[0][1] = v.x;
	viewMatrix[1][1] = v.y;
	viewMatrix[2][1] = v.z;
	viewMatrix[0][2] = w.x;
	viewMatrix[1][2] = w.y;
	viewMatrix[2][2] = w.z;
	viewMatrix[3][0] = -glm::dot(u, position);
	viewMatrix[3][1] = -glm::dot(v, position);
	viewMatrix[3][2] = -glm::dot(w, position);

	inverseViewMatrix = glm::mat4{ 1.f };
	inverseViewMatrix[0][0] = u.x;
	inverseViewMatrix[0][1] = u.y;
	inverseViewMatrix[0][2] = u.z;
	inverseViewMatrix[1][0] = v.x;
	inverseViewMatrix[1][1] = v.y;
	inverseViewMatrix[1][2] = v.z;
	inverseViewMatrix[2][0] = w.x;
	inverseViewMatrix[2][1] = w.y;
	inverseViewMatrix[2][2] = w.z;
	inverseViewMatrix[3][0] = position.x;
	inverseViewMatrix[3][1] = position.y;
	inverseViewMatrix[3][2] = position.z;

}

void Hmck::Camera::update()
{
	// Update projectionMatrix
	if (projectionType == ProjectionType::Orthographic)
	{
		setOrthographicProjection(left, right, top, bottom, _near, _far);
	}
	else if (projectionType == ProjectionType::Perspective)
	{
		setPerspectiveProjection(fovy, aspect, _near, _far);
	}

	// Update the view matrix
	setView(transform.translation, transform.rotation);
}

