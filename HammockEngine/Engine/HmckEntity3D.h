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

	enum TextureIndex : uint32_t { Invalid = 4294967295 };

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

	struct Material
	{
		std::string name;
		glm::vec4 baseColorFactor = glm::vec4(1.0f);
		uint32_t baseColorTextureIndex = TextureIndex::Invalid;
		uint32_t normalTextureIndex = TextureIndex::Invalid;
		uint32_t metallicRoughnessTextureIndex = TextureIndex::Invalid;
		uint32_t occlusionTextureIndex = TextureIndex::Invalid;
		std::string alphaMode = "OPAQUE";
		float alphaCutOff;
		float metallicFactor = 1.f;
		float roughnessFactor = 1.f;
		bool doubleSided = false;
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

		Entity3D() : Entity() { visible = true; };

		Mesh mesh{};
	};
}