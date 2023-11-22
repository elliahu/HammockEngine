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

namespace Hmck
{
	// TODO add runtime object addition and removal
	class Scene
	{
	public:

		struct SceneUbo
		{
			glm::mat4 projection{ 1.f };
			glm::mat4 view{ 1.f };
			glm::mat4 inverseView{ 1.f };
		};

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

		std::unique_ptr<Buffer> vertexBuffer;
		std::unique_ptr<Buffer> indexBuffer;
		uint32_t vertexCount;
		uint32_t indexCount;

		std::unique_ptr<DescriptorPool> descriptorPool{};
		std::unique_ptr<DescriptorSetLayout> descriptorSetLayout{};
		std::vector<VkDescriptorSet> descriptorSets{ SwapChain::MAX_FRAMES_IN_FLIGHT };
		std::vector<std::unique_ptr<Buffer>> sceneBuffers{ SwapChain::MAX_FRAMES_IN_FLIGHT };

	private:

		void loadFile(SceneLoadFileInfo loadInfo);
		void createVertexBuffer();
		void createIndexBuffer();

		Device& device;
		std::vector<Vertex> vertices{};
		std::vector<uint32_t> indices{};
	};
}