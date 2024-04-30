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

		struct SceneCreateInfo
		{
			Device& device;
			MemoryManager& memory;
			std::string name;
			std::string environment = "";
		};

		// TODO make builder as well
		Scene(SceneCreateInfo createInfo);

		~Scene();
		void destroy();

		// delete copy constructor and copy destructor
		Scene(const Scene&) = delete;
		Scene& operator=(const Scene&) = delete;

		void update(); // TODO
		void add(std::shared_ptr<Entity> entity);


		std::shared_ptr<Entity> getRoot() { return entities[root]; }
		std::shared_ptr<Entity> getEntity(EntityHandle id)
		{
			if (entities.contains(id))
			{
				return entities[id];
			}

			throw std::runtime_error("Entity with provided handle does not exist!");
		}
		Device& getDevice() { return device; }
		EntityHandle getLastAdded() { return lastAdded; };
		std::shared_ptr<Camera> getCamera(EntityHandle handle) { return cast<Entity, Camera>(getEntity(handle)); }
		std::shared_ptr<Camera> getActiveCamera() { return getCamera(activeCamera); }


		
		void setActiveCamera(EntityHandle handle) { activeCamera = activeCamera; }

		std::unordered_map<EntityHandle, std::shared_ptr<Entity>> entities{}; 

		std::vector<Image> images;
		std::vector<Texture> textures;
		std::vector<Material> materials;

		std::vector<Vertex> vertices{};
		std::vector<uint32_t> indices{};
		uint32_t vertexCount;
		uint32_t indexCount;

		Texture2D environment;

		EntityHandle activeCamera = 0;
		

	private:
		void addDefaultCamera();

		Device& device;
		MemoryManager& memory;

		std::vector<EntityHandle> cameras{};
		EntityHandle lastAdded = 0;
		EntityHandle root;
	};
}