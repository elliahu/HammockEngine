#include "HmckGameObject.h"


// Matrix corrsponds to Translate * Ry * Rx * Rz * Scale
// Rotations correspond to Tait-bryan angles of Y(1), X(2), Z(3)
// https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix
glm::mat4 Hmck::HmckTransformComponent::mat4() {
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

glm::mat3 Hmck::HmckTransformComponent::normalMatrix()
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

Hmck::HmckGameObject Hmck::HmckGameObject::createPointLight(float intensity,float radius,glm::vec3 color)
{
	HmckGameObject gameObj = HmckGameObject::createGameObject();
	gameObj.color = color;
	gameObj.transform.scale.x = radius;
	gameObj.pointLight = std::make_unique<HmckPointLightComponent>();
	gameObj.pointLight->lightIntensity = intensity;
	return gameObj;
}

Hmck::HmckGameObject Hmck::HmckGameObject::createDirectionalLight(float yaw, float pitch, glm::vec4 directionalLightColor)
{
	HmckGameObject gameObj = HmckGameObject::createGameObject();
	gameObj.directionalLight = std::make_unique<HmckDirectionalLightComponent>();
	gameObj.directionalLight->lightIntensity = directionalLightColor.w;
	gameObj.color = glm::vec3(directionalLightColor);
	gameObj.transform.rotation = { // TODO fix math here
		glm::cos(yaw) * glm::cos(pitch),
		glm::sin(yaw) * glm::cos(pitch),
		glm::sin(pitch)
	};
	return gameObj;
}

void Hmck::HmckGameObject::fitBoundingBox(HmckBoundingBoxComponent::HmckBoundingBoxAxis x, HmckBoundingBoxComponent::HmckBoundingBoxAxis y, HmckBoundingBoxComponent::HmckBoundingBoxAxis z)
{
	if (boundingBox == nullptr)
		this->boundingBox = std::make_unique<HmckBoundingBoxComponent>();
	this->boundingBox->x = x;
	this->boundingBox->y = y;
	this->boundingBox->z = z;
}

void Hmck::HmckGameObject::fitBoundingBox(HmckModel::ModelInfo& modelInfo)
{
	fitBoundingBox(
		{ modelInfo.x.min * transform.scale.x, modelInfo.x.max * transform.scale.x },
		{ modelInfo.y.min * transform.scale.y, modelInfo.y.max * transform.scale.y },
		{ modelInfo.z.min * transform.scale.z, modelInfo.z.max * transform.scale.z });
}

void Hmck::HmckGameObject::setMaterial(std::shared_ptr<HmckMaterial>& material)
{
	this->material = std::make_unique<HmckMaterialComponent>();
	this->material->hmckMaterial = material;
}

void Hmck::HmckGameObject::setModel(std::shared_ptr<HmckModel>& model)
{
	this->model = std::make_unique<HmckModelComponent>();
	this->model->hmckModel = model;
}


