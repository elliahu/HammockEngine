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
#include "HmckEntity3D.h"
#include "HmckGLTF.h"


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

		struct SceneCreateInfo
		{
			Device& device;
			std::string name;
			std::vector<SceneLoadFileInfo> loadFiles;
		};
		// TODO make builder as well
		Scene(SceneCreateInfo createInfo);

		~Scene();

		// delete copy constructor and copy destructor
		Scene(const Scene&) = delete;
		Scene& operator=(const Scene&) = delete;

		void draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout);

		std::shared_ptr<Entity> root() { return entities[0]; }
		void addChildOfRoot(std::shared_ptr<Entity> child)
		{
			child->parent = root();
			entities.push_back(child);
		}

		std::vector<std::shared_ptr<Entity>> entities;
		std::vector<Image> images;
		std::vector<Texture> textures;
		std::vector<Material> materials;

	private:

		void loadFile(SceneLoadFileInfo loadInfo);
		void drawEntity(VkCommandBuffer commandBuffer, std::shared_ptr<Entity>& entity, VkPipelineLayout pipelineLayout);
		void createVertexBuffer();
		void createIndexBuffer();

		std::vector<Vertex> vertices{};
		std::vector<uint32_t> indices{};
		std::unique_ptr<Buffer> vertexBuffer;
		uint32_t vertexCount;
		std::unique_ptr<Buffer> indexBuffer;
		uint32_t indexCount;

		Device& device;
	};
}