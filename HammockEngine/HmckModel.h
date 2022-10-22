#pragma once
#include "HmckDevice.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>


#include <vector>
#include <cassert>
#include <cstring>

namespace Hmck
{
	class HmckModel
	{
	public:

		// when making changes to Vertex struct,
		// dont forget to update Vertex::getAttributeDescriptions() to match
		struct Vertex 
		{
			glm::vec3 position;
			glm::vec3 color;

			static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
			static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
		};

		struct Builder 
		{
			std::vector<Vertex> vertices{};
			std::vector<uint32_t> indices{};
		};

		HmckModel(HmckDevice& device, const HmckModel::Builder& builder);
		~HmckModel();

		// delete copy constructor and copy destructor
		HmckModel(const HmckModel&) = delete;
		HmckModel& operator=(const HmckModel&) = delete;

		void bind(VkCommandBuffer commandBuffer);
		void draw(VkCommandBuffer commandBuffer);


	private:
		void createVertexBuffers(const std::vector<Vertex>& vertices);
		void createIndexBuffers(const std::vector<uint32_t>& indices);

		HmckDevice& hmckDevice;

		VkBuffer vertexBuffer;
		VkDeviceMemory vertexBufferMemory;
		uint32_t vertexCount;

		bool hasIndexBuffer = false;
		
		VkBuffer indexBuffer;
		VkDeviceMemory indexBufferMemory;
		uint32_t indexCount;
	
	};
}

