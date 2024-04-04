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
#include "HmckGLTF.h"
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
			bool binary = true;
		};

		struct SkyboxLoadSkyboxInfo
		{
			std::vector<std::string> filenames;
		};

		struct SceneCreateInfo
		{
			Device& device;
			std::string name;
			std::vector<SceneLoadFileInfo> loadFiles;
			SkyboxLoadSkyboxInfo loadSkybox;
		};

		// TODO make builder as well
		Scene(SceneCreateInfo createInfo);

		~Scene();
		void destroy();

		// delete copy constructor and copy destructor
		Scene(const Scene&) = delete;
		Scene& operator=(const Scene&) = delete;

		std::shared_ptr<Entity> getRoot() { return root; }
		Device& getDevice() { return device; }
		void addChildOfRoot(std::shared_ptr<Entity> child)
		{
			child->parent = getRoot();
			getRoot()->children.push_back(child);
		}


		std::shared_ptr<Entity> root;
		std::vector<Image> images;
		std::vector<Texture> textures;
		std::vector<Material> materials;

		TextureCubeMap skybox;

		std::vector<Vertex> vertices{};
		std::vector<uint32_t> indices{};
		uint32_t vertexCount;
		uint32_t indexCount;

		Camera camera{};

	private:

		void loadFile(SceneLoadFileInfo loadInfo);
		void loadSkybox(SkyboxLoadSkyboxInfo loadInfo);

		bool hasSkybox = false;
		Device& device;
		


	};
}