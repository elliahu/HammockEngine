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
#include "HmckCamera.h"
#include "HmckHDR.h"
#include "HmckUtils.h"
#include "HmckLights.h"

namespace Hmck
{
	// TODO add runtime object addition and removal
	class Scene
	{
	public:

		struct SceneCreateInfo
		{
			Device& device;
			MemoryManager& memory;
			std::string name;
		};

		// TODO make builder as well
		Scene(SceneCreateInfo createInfo);

		~Scene();
		void destroy();

		// delete copy constructor and copy destructor
		Scene(const Scene&) = delete;
		Scene& operator=(const Scene&) = delete;

		void add(std::shared_ptr<Entity> entity, std::shared_ptr<Entity> parent);


		std::shared_ptr<Entity> getRoot() { return entities[root]; }
		std::shared_ptr<Entity> getEntity(EntityHandle id)
		{
			if (entities.contains(id))
			{
				return entities[id];
			}

			return nullptr;
		}
		Device& getDevice() { return device; }
		EntityHandle getLastAdded() { return lastAdded; };
		std::shared_ptr<Camera> getCamera(EntityHandle handle) { return cast<Entity, Camera>(getEntity(handle)); }
		std::shared_ptr<Camera> getActiveCamera() { return getCamera(activeCamera); }


		
		void setActiveCamera(EntityHandle handle) { activeCamera = handle; }

		std::unordered_map<EntityHandle, std::shared_ptr<Entity>> entities{}; 

		std::vector<Image> images;
		std::vector<Texture> textures;
		std::vector<Material> materials;

		std::vector<Vertex> vertices{};
		std::vector<uint32_t> indices{};
		uint32_t vertexCount;
		uint32_t indexCount;

		std::shared_ptr<Environment> environment;

		EntityHandle activeCamera = 0;
		std::vector<EntityHandle> cameras{};
		std::vector<EntityHandle> lights{};
		

	private:

		Device& device;
		MemoryManager& memory;
		EntityHandle lastAdded = 0;
		EntityHandle root;
	};
}