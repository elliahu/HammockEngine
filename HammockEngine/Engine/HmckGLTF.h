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

namespace gltf = tinygltf;
namespace Hmck
{
	// scalar float: N = 4 Bytes
	// vec2: 2N = 8 Bytes
	// vec3 (or vec4): 4N = 16 Bytes
	// taken from 15.6.4 Offset and Stride Assignment
	struct MatrixPushConstantData {
		glm::mat4 modelMatrix{ 1.f };
		glm::mat4 normalMatrix{ 1.f };
	};


	class HmckGLTF
	{
	public:

		HmckGLTF(HmckDevice& device);

		~HmckGLTF();

		// delete copy constructor and copy destructor
		HmckGLTF(const HmckGLTF&) = delete;
		HmckGLTF& operator=(const HmckGLTF&) = delete;

		struct Config
		{
			bool binary = false;
		};

		struct Vertex
		{
			glm::vec3 position{};
			glm::vec3 color{};
			glm::vec3 normal{};
			glm::vec2 uv{};
			glm::vec4 tangent{};
		};

		struct Primitive {
			uint32_t firstIndex;
			uint32_t indexCount;
			int32_t materialIndex;
		};

		struct Image
		{
			std::string uri;
			std::string name;
			HmckTexture2D texture{};
		};

		struct Texture
		{
			uint32_t imageIndex;
			uint32_t scale;
		};

		struct Material
		{
			std::string name;
			glm::vec4 baseColorFactor = glm::vec4(1.0f);
			uint32_t baseColorTextureIndex;
			uint32_t normalTextureIndex;
			uint32_t metallicRoughnessTextureIndex;
			uint32_t occlusionTexture;
			std::string alphaMode = "OPAQUE";
			float alphaCutOff;
			bool doubleSided = false;
			VkDescriptorSet descriptorSet;
			VkPipeline pipeline;
		};

		struct Mesh
		{
			std::vector<Primitive> primitives;
			std::string name;
		};

		struct Node {
			std::shared_ptr<Node> parent;
			std::vector<std::shared_ptr<Node>> children;
			Mesh mesh{};
			glm::mat4 matrix;
			std::string name{};
			bool visible = true;

			~Node() {
				// ensures that before parent node is deleted, all of its children are deleted as well
				for (auto& child : children) {
					child = nullptr;
				}
			}
		};

		void load(std::string filepath, Config& info);
		void draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, glm::mat4 transform);
		void drawNode(VkCommandBuffer commandBuffer, std::shared_ptr<Node>& node, VkPipelineLayout pipelineLayout, glm::mat4 objectTransform);

		static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
		static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();

		void prepareDescriptors();
		// TODO create per-material pipelines
		// void createMaterialPipelines();

		std::vector<Vertex> vertices{};
		std::vector<uint32_t> indices{};
		std::vector<Image> images;
		std::vector<Texture> textures;
		std::vector<Material> materials;
		std::vector<std::shared_ptr<Node>> nodes;
		std::string path;

		std::unique_ptr<HmckBuffer> vertexBuffer;
		uint32_t vertexCount;

		std::unique_ptr<HmckBuffer> indexBuffer;
		uint32_t indexCount;

	private:

		void loadImages(gltf::Model& input);
		void loadMaterials(gltf::Model& input);
		void loadTextures(gltf::Model& input);
		void loadNode(
			const tinygltf::Node& inputNode,
			const tinygltf::Model& input, std::shared_ptr<Node> parent);
		void createVertexBuffer();
		void createIndexBuffer();



		gltf::Model model;
		std::unique_ptr<HmckDescriptorPool> descriptorPool{};
		std::unique_ptr<HmckDescriptorSetLayout> descriptorSetLayout;
		HmckDevice& device;
	};
}


