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
	gameObj.colorComponent = color;
	gameObj.transformComponent.scale.x = radius;
	gameObj.pointLightComponent = std::make_unique<HmckPointLightComponent>();
	gameObj.pointLightComponent->lightIntensity = intensity;
	return gameObj;
}

Hmck::HmckGameObject Hmck::HmckGameObject::createDirectionalLight(float yaw, float pitch, glm::vec4 directionalLightColor)
{
	HmckGameObject gameObj = HmckGameObject::createGameObject();
	gameObj.directionalLightComponent = std::make_unique<HmckDirectionalLightComponent>();
	gameObj.directionalLightComponent->lightIntensity = directionalLightColor.w;
	gameObj.colorComponent = glm::vec3(directionalLightColor);
	gameObj.transformComponent.rotation = { // TODO fix math here
		glm::cos(yaw) * glm::cos(pitch),
		glm::sin(yaw) * glm::cos(pitch),
		glm::sin(pitch)
	};
	return gameObj;
}

void Hmck::HmckGameObject::fitBoundingBox(HmckBoundingBoxComponent::HmckBoundingBoxAxis x, HmckBoundingBoxComponent::HmckBoundingBoxAxis y, HmckBoundingBoxComponent::HmckBoundingBoxAxis z)
{
	if (boundingBoxComponent == nullptr)
		this->boundingBoxComponent = std::make_unique<HmckBoundingBoxComponent>();
	this->boundingBoxComponent->x = x;
	this->boundingBoxComponent->y = y;
	this->boundingBoxComponent->z = z;
}

void Hmck::HmckGameObject::fitBoundingBox(HmckModel::ModelInfo& modelInfo)
{
	fitBoundingBox(
		{ modelInfo.x.min * transformComponent.scale.x, modelInfo.x.max * transformComponent.scale.x },
		{ modelInfo.y.min * transformComponent.scale.y, modelInfo.y.max * transformComponent.scale.y },
		{ modelInfo.z.min * transformComponent.scale.z, modelInfo.z.max * transformComponent.scale.z });
}

void Hmck::HmckGameObject::setMaterial(std::shared_ptr<HmckMaterial>& material)
{
	this->materialComponent = std::make_unique<HmckMaterialComponent>();
	this->materialComponent->material = material;
}

void Hmck::HmckGameObject::setModel(std::shared_ptr<HmckModel>& model)
{
	this->modelComponent = std::make_unique<HmckModelComponent>();
	this->modelComponent->model = model;
}


void Hmck::HmckGameObject::bindDescriptorSet(
	std::unique_ptr<HmckDescriptorPool>& pool,
	std::unique_ptr<HmckDescriptorSetLayout>& setLayout)
{
	descriptorSetComponent = std::make_unique<HmckDescriptorSetComponent>();

	auto descriptorWriter = HmckDescriptorWriter(*setLayout, *pool);
	
	if (materialComponent->material->color != nullptr)
	{
		VkDescriptorImageInfo colorInfo{};
		colorInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		colorInfo.imageView = materialComponent->material->color->image.imageView;
		colorInfo.sampler = materialComponent->material->color->sampler;

		descriptorWriter.writeImage(0, &colorInfo);
	}

	if (materialComponent->material->normal != nullptr)
	{
		VkDescriptorImageInfo normalInfo{};
		normalInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		normalInfo.imageView = materialComponent->material->normal->image.imageView;
		normalInfo.sampler = materialComponent->material->normal->sampler;

		descriptorWriter.writeImage(1, &normalInfo);
	}

	if (materialComponent->material->roughness != nullptr)
	{
		VkDescriptorImageInfo roughnessInfo{};
		roughnessInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		roughnessInfo.imageView = materialComponent->material->roughness->image.imageView;
		roughnessInfo.sampler = materialComponent->material->roughness->sampler;

		descriptorWriter.writeImage(2, &roughnessInfo);
	}

	if (materialComponent->material->metalness != nullptr)
	{
		VkDescriptorImageInfo metalnessInfo{};
		metalnessInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		metalnessInfo.imageView = materialComponent->material->metalness->image.imageView;
		metalnessInfo.sampler = materialComponent->material->metalness->sampler;

		descriptorWriter.writeImage(3, &metalnessInfo);
	}

	if (materialComponent->material->ambientOcclusion != nullptr)
	{
		VkDescriptorImageInfo aoInfo{};
		aoInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		aoInfo.imageView = materialComponent->material->ambientOcclusion->image.imageView;
		aoInfo.sampler = materialComponent->material->ambientOcclusion->sampler;

		descriptorWriter.writeImage(4, &aoInfo);
	}

	if (materialComponent->material->displacement != nullptr)
	{
		VkDescriptorImageInfo displacementInfo{};
		displacementInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		displacementInfo.imageView = materialComponent->material->displacement->image.imageView;
		displacementInfo.sampler = materialComponent->material->displacement->sampler;

		descriptorWriter.writeImage(5, &displacementInfo);
	}

	descriptorWriter.build(descriptorSetComponent->set);
}


