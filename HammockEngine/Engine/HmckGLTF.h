#pragma once
#include <string>
#include <iostream>
#include <memory>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <tiny_gltf.h>
#include "HmckTexture.h"
#include "HmckEntity.h"
#include "HmckEntity3D.h"
#include "HmckFrameInfo.h"

namespace gltf = tinygltf;
namespace Hmck
{
	class Gltf
	{
	public:

		static std::vector<std::shared_ptr<Entity>> load(
			std::string filename,
			Device& device,
			std::unique_ptr<DescriptorPool>& descriptorPool,
			std::vector<Image>& images,
			uint32_t imagesOffset,
			std::vector<Material>& materials,
			uint32_t materialsOffset,
			std::vector<Texture>& textures,
			uint32_t texturesOffset,
			std::vector<Vertex>& vertices,
			std::vector<uint32_t>& indices,
			std::shared_ptr<Entity>& root,
			bool binary = true
		);

	private:

		static void loadImages(gltf::Model& input, Device& device, std::vector<Image>& images, uint32_t imagesOffset);
		static void loadMaterials(gltf::Model& input, Device& device, std::unique_ptr<DescriptorPool>& descriptorPool, std::vector<Material>& materials, uint32_t materialsOffset, uint32_t texturesOffset);
		static void loadTextures(gltf::Model& input, Device& device, std::unique_ptr<DescriptorPool>& descriptorPool, std::vector<Texture>& textures, uint32_t texturesOffset);
		static std::shared_ptr<Entity> loadNode(
			Device& device,
			std::unique_ptr<DescriptorPool>& descriptorPool,
			const tinygltf::Node& inputNode,
			const tinygltf::Model& input, 
			uint32_t materialsOffset,
			std::shared_ptr<Entity> parent,
			std::vector<Vertex>& vertices,
			std::vector<uint32_t>& indices,
			std::shared_ptr<Entity>& root
			);

		
	};
}


