#include "HmckGameObject.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>


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
	gameObj.colorComponent = color;
	gameObj.transformComponent.scale.x = radius;
	gameObj.pointLightComponent = std::make_unique<HmckPointLightComponent>();
	gameObj.pointLightComponent->lightIntensity = intensity;
	return gameObj;
}

Hmck::HmckGameObject Hmck::HmckGameObject::createDirectionalLight(
	glm::vec3 position, glm::vec3 target,
	glm::vec4 directionalLightColor,
	float nearClip, float farClip, float fov)
{
	HmckGameObject gameObj = HmckGameObject::createGameObject();
	gameObj.directionalLightComponent = std::make_unique<HmckDirectionalLightComponent>();
	gameObj.directionalLightComponent->lightIntensity = directionalLightColor.w;
	gameObj.directionalLightComponent->target = target;
	gameObj.directionalLightComponent->_near = nearClip;
	gameObj.directionalLightComponent->_far = farClip;
	gameObj.directionalLightComponent->fov = fov;
	gameObj.colorComponent = glm::vec3(directionalLightColor);
	gameObj.transformComponent.translation = position;
	return gameObj;
}

Hmck::HmckGameObject Hmck::HmckGameObject::createFromGLTF(std::string filepath, HmckDevice& device, HmckGLTF::Config config)
{
	HmckGameObject obj = HmckGameObject::createGameObject();
	obj.glTFComponent = std::make_unique<HmckGLTFComponent>();
	auto glTF = std::make_unique<HmckGLTF>(device);
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

void Hmck::HmckGameObject::fitBoundingBox(HmckBoundingBoxComponent::HmckBoundingBoxAxis x, HmckBoundingBoxComponent::HmckBoundingBoxAxis y, HmckBoundingBoxComponent::HmckBoundingBoxAxis z)
{
	if (boundingBoxComponent == nullptr)
		this->boundingBoxComponent = std::make_unique<HmckBoundingBoxComponent>();
	this->boundingBoxComponent->x = x;
	this->boundingBoxComponent->y = y;
	this->boundingBoxComponent->z = z;
}

void Hmck::HmckGameObject::fitBoundingBox(HmckMesh::MeshInfo& modelInfo)
{
	fitBoundingBox(
		{ modelInfo.x.min * transformComponent.scale.x, modelInfo.x.max * transformComponent.scale.x },
		{ modelInfo.y.min * transformComponent.scale.y, modelInfo.y.max * transformComponent.scale.y },
		{ modelInfo.z.min * transformComponent.scale.z, modelInfo.z.max * transformComponent.scale.z });
}

void Hmck::HmckGameObject::setMtlMaterial(std::shared_ptr<HmckMaterial>& material)
{
	assert(glTFComponent == nullptr && "Cannot use Wavefront OBJ, glTF is used.");
	if (this->wavefrontObjComponent == nullptr) {
		this->wavefrontObjComponent = std::make_unique<HmckWavefrontObjComponent>();
	}
	this->wavefrontObjComponent->material = material;
}

void Hmck::HmckGameObject::setObjMesh(std::shared_ptr<HmckMesh>& model)
{
	assert(glTFComponent == nullptr && "Cannot use Wavefront OBJ, glTF is used.");
	if (this->wavefrontObjComponent == nullptr) {
		this->wavefrontObjComponent = std::make_unique<HmckWavefrontObjComponent>();
	}
	this->wavefrontObjComponent->mesh = model;
}

void Hmck::HmckGameObject::bindMtlDescriptorSet(
	std::unique_ptr<HmckDescriptorPool>& pool,
	std::unique_ptr<HmckDescriptorSetLayout>& setLayout)
{
	descriptorSetComponent = std::make_unique<HmckDescriptorSetComponent>();

	auto descriptorWriter = HmckDescriptorWriter(*setLayout, *pool);
	descriptorWriter.writeImage(0, &wavefrontObjComponent->material->color->descriptor);
	descriptorWriter.writeImage(1, &wavefrontObjComponent->material->normal->descriptor);
	descriptorWriter.writeImage(2, &wavefrontObjComponent->material->occlusionRoughnessMetalness->descriptor);
	descriptorWriter.writeImage(3, &wavefrontObjComponent->material->occlusionRoughnessMetalness->descriptor);
	descriptorWriter.build(descriptorSetComponent->set);
}


