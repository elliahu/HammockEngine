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

		struct Vertex {
			glm::vec2 position;
			glm::vec3 color;

			static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
			static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();

		};

		HmckModel(HmckDevice& device, const std::vector<Vertex>& vertices);
		~HmckModel();

		// delete copy constructor and copy destructor
		HmckModel(const HmckModel&) = delete;
		HmckModel& operator=(const HmckModel&) = delete;

		void bind(VkCommandBuffer commandBuffer);
		void draw(VkCommandBuffer commandBuffer);


	private:
		void createVertexBuffers(const std::vector<Vertex>& vertices);

		HmckDevice& hmckDevice;
		VkBuffer vertexBuffer;
		VkDeviceMemory vertexBufferMemory;
		uint32_t vertexCount;
	
	};
}

