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
#include "HmckDescriptors.h"
#include "HmckEntity3D.h"
#include "HmckSwapChain.h"
#include "HmckFrameInfo.h"
#include "HmckCamera.h"

namespace Hmck
{
	// TODO add runtime object addition and removal
	class Scene
	{
	public:

		struct SceneLoadFileInfo
		{
			std::string filename;
			std::string name{};
			glm::vec3 translation{ 0 };
			glm::vec3 rotation{ 0 };
			glm::vec3 scale{ 1 };
		};

		struct SkyboxLoadSkyboxInfo
		{
			std::vector<std::string> textures;
		};

		struct SceneCreateInfo
		{
			Device& device;
			MemoryManager& memory;
			std::string name;
			SkyboxLoadSkyboxInfo loadSkybox;
		};

		// TODO make builder as well
		Scene(SceneCreateInfo createInfo);

		~Scene();
		void destroy();

		// delete copy constructor and copy destructor
		Scene(const Scene&) = delete;
		Scene& operator=(const Scene&) = delete;

		void update(); // TODO


		std::shared_ptr<Entity> getRoot() { return entities[root]; }
		std::shared_ptr<Entity> getEntity(EntityId id)
		{
			if (entities.contains(id))
			{
				return entities[id];
			}

			throw std::runtime_error("Entity with provided handle does not exist!");
		}
		Device& getDevice() { return device; }

		EntityId getLastAdded() { return lastAdded; };

		void add(std::shared_ptr<Entity> entity);


		void loadSkyboxTexture(SkyboxLoadSkyboxInfo loadInfo);

		

		std::unordered_map<EntityId, std::shared_ptr<Entity>> entities{}; // TODO change EntityId to EntityHandle

		

		std::vector<EntityId> cameras;
		

		std::vector<Image> images;
		std::vector<Texture> textures;
		std::vector<Material> materials;

		std::vector<Vertex> vertices{};
		std::vector<uint32_t> indices{};
		uint32_t vertexCount;
		uint32_t indexCount;

		TextureCubeMap skyboxTexture;
		std::vector<Vertex> skyboxVertices{};
		std::vector<uint32_t> skyboxIndices{};
		uint32_t skyboxVertexCount;
		uint32_t skyboxIndexCount;

		Camera camera{};
		bool hasSkybox = false;
		

	private:
		Device& device;
		MemoryManager& memory;

		EntityId lastAdded = 0;

		EntityId root;
		uint32_t activeCamera;
	};
}