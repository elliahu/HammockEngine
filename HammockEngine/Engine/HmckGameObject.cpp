#include "HmckGameObject.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>


// Matrix corrsponds to Translate * Ry * Rx * Rz * Scale
// Rotations correspond to Tait-bryan angles of Y(1), X(2), Z(3)
// https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix
glm::mat4 Hmck::TransformComponent::mat4() {
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

glm::mat3 Hmck::TransformComponent::normalMatrix()
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

Hmck::GameObject Hmck::GameObject::createPointLight(float intensity,float radius,glm::vec3 color)
{
	GameObject gameObj = GameObject::createGameObject();
	gameObj.colorComponent = color;
	gameObj.transformComponent.scale.x = radius;
	gameObj.pointLightComponent = std::make_unique<PointLightComponent>();
	gameObj.pointLightComponent->lightIntensity = intensity;
	return gameObj;
}

Hmck::GameObject Hmck::GameObject::createDirectionalLight(
	glm::vec3 position, glm::vec3 target,
	glm::vec4 directionalLightColor,
	float nearClip, float farClip, float fov)
{
	GameObject gameObj = GameObject::createGameObject();
	gameObj.directionalLightComponent = std::make_unique<DirectionalLightComponent>();
	gameObj.directionalLightComponent->lightIntensity = directionalLightColor.w;
	gameObj.directionalLightComponent->target = target;
	gameObj.directionalLightComponent->_near = nearClip;
	gameObj.directionalLightComponent->_far = farClip;
	gameObj.directionalLightComponent->fov = fov;
	gameObj.colorComponent = glm::vec3(directionalLightColor);
	gameObj.transformComponent.translation = position;
	return gameObj;
}

Hmck::GameObject Hmck::GameObject::createFromGLTF(std::string filepath, Device& device, Gltf::Config config)
{
	GameObject obj = GameObject::createGameObject();
	obj.glTFComponent = std::make_unique<GltfComponent>();
	auto glTF = std::make_unique<Gltf>(device);
	glTF->load(filepath, config);
	obj.glTFComponent->glTF = std::move(glTF);
	

	// model matrix
	glm::quat rotation;
	glm::vec3 skew;
	glm::vec4 perspective;
	glm::mat4 m = obj.glTFComponent->glTF->nodes[0]->matrix;
	glm::decompose(obj.glTFComponent->glTF->nodes[0]->matrix, obj.transformComponent.scale, rotation, obj.transformComponent.translation, skew, perspective);
	obj.transformComponent.rotation = glm::eulerAngles(rotation);

	return obj;
}

void Hmck::GameObject::fitBoundingBox(BoundingBoxComponent::BoundingBoxAxis x, BoundingBoxComponent::BoundingBoxAxis y, BoundingBoxComponent::BoundingBoxAxis z)
{
	if (boundingBoxComponent == nullptr)
		this->boundingBoxComponent = std::make_unique<BoundingBoxComponent>();
	this->boundingBoxComponent->x = x;
	this->boundingBoxComponent->y = y;
	this->boundingBoxComponent->z = z;
}



