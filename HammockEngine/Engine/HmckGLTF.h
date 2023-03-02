#pragma once
#include <string>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <tiny_gltf.h>

#include "HmckGameObject.h"

namespace GLTF = tinygltf;

namespace Hmck
{
	class HmckGLTF
	{
	public:

		struct MeshVertex
		{
			glm::vec3 position{};
			glm::vec3 color{};
			glm::vec3 normal{};
			glm::vec2 uv{};
			glm::vec3 tangent{};
		};

		struct MeshBuilder
		{
			std::vector<MeshVertex> vertices{};
			std::vector<uint32_t> indices{};
		};


		void loadGameObject(const GLTF::Node& inputNode, const GLTF::Model& input);

		void loadScene(std::string& filepath);
	};
}


