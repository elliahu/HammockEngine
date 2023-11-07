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

namespace Hmck
{
	struct Vertex
	{
		glm::vec3 position{};
		glm::vec3 color{};
		glm::vec3 normal{};
		glm::vec2 uv{};
		glm::vec4 tangent{};
	};

	struct Primitive {
		uint32_t firstIndex;
		uint32_t indexCount;
		int32_t materialIndex;
	};

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
		uint32_t baseColorTextureIndex;
		uint32_t normalTextureIndex;
		uint32_t metallicRoughnessTextureIndex;
		uint32_t occlusionTexture;
		std::string alphaMode = "OPAQUE";
		float alphaCutOff;
		bool doubleSided = false;
	};

	struct Mesh
	{
		std::vector<Primitive> primitives;
		std::string name;
	};


	class Entity3D : public Entity
	{
	public:
		Mesh mesh{};
	};
}