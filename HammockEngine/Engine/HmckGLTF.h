#pragma once
#include <string>
#include <iostream>
#include <memory>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp> // Include for glm::eulerAngleYXZ function
#include <tiny_gltf.h>
#include "HmckTexture.h"
#include "HmckEntity.h"
#include "HmckEntity3D.h"
#include "HmckFrameInfo.h"
#include "HmckMemory.h"
#include "HmckLights.h"
#include "HmckScene.h"

namespace gltf = tinygltf;
namespace Hmck
{
	class GltfLoader
	{
	public:
		GltfLoader(Device& device, MemoryManager& memory, std::unique_ptr<Scene>& scene);

		void load(std::string filename);

	private:
		Device& device;
		MemoryManager& memory;
		std::unique_ptr<Scene>& scene;

		uint32_t imagesOffset;
		uint32_t texturesOffset;
		uint32_t materialsOffset;


		bool isBinary(std::string& filename);
		bool isLight(gltf::Node& node, gltf::Model& model);
		bool isSolid(gltf::Node& node);
		bool isCamera(gltf::Node& node);

		void loadImages(gltf::Model& model);
		void loadTextures(gltf::Model& model);
		void loadMaterials(gltf::Model& model);

		void loadEntities(gltf::Model& model);
		void loadEntitiesRecursive(gltf::Node& node, gltf::Model& model, std::shared_ptr<Entity> parent);

		void loadEntity(gltf::Node& node, gltf::Model& model, std::shared_ptr<Entity> parent);
		void loadEntity3D(gltf::Node& node, gltf::Model& model, std::shared_ptr<Entity> parent);
		void loadIOmniLight(gltf::Node& node, gltf::Model& model, std::shared_ptr<Entity> parent);
		void loadCamera(gltf::Node& node, gltf::Model& model, std::shared_ptr<Entity> parent);
		
	};
}


