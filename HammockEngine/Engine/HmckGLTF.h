#pragma once
#include <string>
#include <iostream>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <tiny_gltf.h>
#include "HmckTexture.h"
#include "HmckEntity.h"
#include "HmckEntity3D.h"

namespace gltf = tinygltf;
namespace Hmck
{
	class Gltf
	{
	public:

		static std::vector<std::shared_ptr<Entity>> load(
			std::string filename,
			Device& device,
			std::vector<Image>& images,
			std::vector<Material>& materials,
			std::vector<Texture>& textures,
			std::vector<Vertex>& vertices,
			std::vector<uint32_t>& indices,
			std::vector<std::shared_ptr<Entity>>& entities,
			bool binary = true
		);

	private:

		static void loadImages(gltf::Model& input, Device& device, std::vector<Image>& images);
		static void loadMaterials(gltf::Model& input, Device& device, std::vector<Material>& materials);
		static void loadTextures(gltf::Model& input, Device& device, std::vector<Texture>& textures);
		static std::shared_ptr<Entity> loadNode(
			const tinygltf::Node& inputNode,
			const tinygltf::Model& input, 
			std::shared_ptr<Entity> parent,
			std::vector<Vertex>& vertices,
			std::vector<uint32_t>& indices,
			std::vector<std::shared_ptr<Entity>>& entities
			);

		
	};
}


