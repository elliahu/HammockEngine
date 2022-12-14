#pragma once
#include "HmckDevice.h"
#include "HmckUtils.h"
#include "HmckBuffer.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPRIMENTAL
#include <glm/gtx/hash.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>


#include <vector>
#include <cassert>
#include <cstring>
#include <memory>
#include <unordered_map>

namespace Hmck
{
	class HmckModel
	{
	public:

		struct ModelInfo
		{
			struct AxisRange
			{
				float min;
				float max;
			};

			AxisRange x{};
			AxisRange y{};
			AxisRange z{};
		};

		// when making changes to Vertex struct,
		// dont forget to update Vertex::getAttributeDescriptions() to match
		struct Vertex 
		{
			glm::vec3 position{};
			glm::vec3 color{};
			glm::vec3 normal{};
			glm::vec2 uv{};
			glm::vec3 tangent{};

			static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
			static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();

			bool operator ==(const Vertex& other) const
			{
				return 
					position == other.position &&
					color == other.color &&
					normal == other.normal &&
					uv == other.uv;
			}
		};

		struct Builder 
		{
			std::vector<Vertex> vertices{};
			std::vector<uint32_t> indices{};

			ModelInfo loadModel(const std::string& filepath);
			ModelInfo loadModelAssimp(const std::string& filepath);
			void calculateTangent();
		};


		HmckModel(HmckDevice& device, const HmckModel::Builder& builder);
		~HmckModel();

		static std::unique_ptr<HmckModel> createModelFromFile(HmckDevice& device, const std::string& filepath, bool calculateTangents = true);

		// delete copy constructor and copy destructor
		HmckModel(const HmckModel&) = delete;
		HmckModel& operator=(const HmckModel&) = delete;

		void bind(VkCommandBuffer commandBuffer);
		void draw(VkCommandBuffer commandBuffer);

		ModelInfo modelInfo;

	private:
		void createVertexBuffers(const std::vector<Vertex>& vertices);
		void createIndexBuffers(const std::vector<uint32_t>& indices);

		HmckDevice& hmckDevice;

		std::unique_ptr<HmckBuffer> vertexBuffer;
		uint32_t vertexCount;

		bool hasIndexBuffer = false;

		std::unique_ptr<HmckBuffer> indexBuffer;
		uint32_t indexCount;
	};
}

