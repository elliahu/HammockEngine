#pragma once
#include <string>
#include <iostream>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <tiny_gltf.h>
#include "HmckTexture.h"
#include "HmckDescriptors.h"
#include "HmckEntity.h"
#include "HmckPipeline.h"
#include "HmckVertex.h"

namespace Hmck
{

	enum TextureHandle : uint32_t { Invalid = 4294967295 };

	struct Image
	{
		std::string uri;
		std::string name;
		Texture2D texture{};
	};

	struct Texture
	{
		uint32_t imageIndex;
		uint32_t scale;
	};

	struct MaterialPropertyUbo
	{
		glm::vec4 baseColorFactor;
		uint32_t baseColorTextureIndex;
		uint32_t normalTextureIndex;
		uint32_t metallicRoughnessTextureIndex;
		uint32_t occlusionTextureIndex;
		float alphaCutoff;
	};

	struct Material
	{
		std::string name;
		glm::vec4 baseColorFactor = glm::vec4(1.0f);
		uint32_t baseColorTextureIndex = TextureHandle::Invalid;
		uint32_t normalTextureIndex = TextureHandle::Invalid;
		uint32_t metallicRoughnessTextureIndex = TextureHandle::Invalid;
		uint32_t occlusionTextureIndex = TextureHandle::Invalid;
		std::string alphaMode = "OPAQUE";
		float alphaCutOff;
		bool doubleSided = false;
		std::shared_ptr<GraphicsPipeline> pipeline{};
		std::unique_ptr<DescriptorSetLayout> descriptorSetLayout{};
		std::unique_ptr<Buffer> buffer{};
		VkDescriptorSet descriptorSet;

		void updateBuffer()
		{
			MaterialPropertyUbo materialData{
				.baseColorFactor = baseColorFactor,
				.baseColorTextureIndex = baseColorTextureIndex,
				.normalTextureIndex = baseColorTextureIndex,
				.metallicRoughnessTextureIndex = metallicRoughnessTextureIndex,
				.occlusionTextureIndex = metallicRoughnessTextureIndex,
				.alphaCutoff = alphaCutOff
			};

			buffer->writeToBuffer(&materialData);
		}
	};

	struct Primitive {
		uint32_t firstIndex;
		uint32_t indexCount;
		int32_t materialIndex;
	};

	struct Mesh
	{
		std::vector<Primitive> primitives;
		std::string name;
	};


	class Entity3D : public Entity
	{
	public:

		Entity3D(Device& device, std::unique_ptr<DescriptorPool>& descriptorPool) : Entity(device, descriptorPool) { visible = true; };

		~Entity3D(){}

		Mesh mesh{};
	};
}