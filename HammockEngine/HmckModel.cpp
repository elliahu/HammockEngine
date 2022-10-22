#include "HmckModel.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

namespace std
{
	template <>
	struct hash<Hmck::HmckModel::Vertex>
	{
		size_t operator()(Hmck::HmckModel::Vertex const& vertex) const
		{
			size_t seed = 0;
			Hmck::hashCombine(seed, vertex.position, vertex.color, vertex.normal, vertex.uv);
			return seed;
		}
	};
}

Hmck::HmckModel::HmckModel(HmckDevice& device, const HmckModel::Builder& builder) : hmckDevice{ device }
{
	createVertexBuffers(builder.vertices);
	createIndexBuffers(builder.indices);
}

Hmck::HmckModel::~HmckModel()
{
	vkDestroyBuffer(hmckDevice.device(), vertexBuffer, nullptr);
	vkFreeMemory(hmckDevice.device(), vertexBufferMemory, nullptr);

	if (hasIndexBuffer)
	{
		vkDestroyBuffer(hmckDevice.device(), indexBuffer, nullptr);
		vkFreeMemory(hmckDevice.device(), indexBufferMemory, nullptr);
	}
}


std::unique_ptr<Hmck::HmckModel> Hmck::HmckModel::createModelFromFile(HmckDevice& device, const std::string& filepath)
{
	Builder builder{};
	builder.loadModel(filepath);

	return std::make_unique<HmckModel>(device, builder);
}

void Hmck::HmckModel::draw(VkCommandBuffer commandBuffer)
{
	if (hasIndexBuffer)
	{
		vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);
	}
	else 
	{
		vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
	}
}

void Hmck::HmckModel::bind(VkCommandBuffer commandBuffer)
{
	VkBuffer buffers[] = { vertexBuffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);

	if (hasIndexBuffer)
	{
		vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
	}
}

void Hmck::HmckModel::createVertexBuffers(const std::vector<Vertex>& vertices)
{
	// copy data to staging memory on device, then copy from staging to v/i memory

	vertexCount = static_cast<uint32_t>(vertices.size());
	assert(vertexCount >= 3 && "Vertex count must be at least 3");
	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertexCount;

	VkBuffer stagingBufer;
	VkDeviceMemory stagingBufferMemory;

	hmckDevice.createBuffer(
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBufer,
		stagingBufferMemory
	);

	void* data;
	vkMapMemory(hmckDevice.device(), stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
	vkUnmapMemory(hmckDevice.device(), stagingBufferMemory);

	hmckDevice.createBuffer(
		bufferSize,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		vertexBuffer,
		vertexBufferMemory
	);

	hmckDevice.copyBuffer(stagingBufer, vertexBuffer, bufferSize);

	vkDestroyBuffer(hmckDevice.device(), stagingBufer, nullptr);
	vkFreeMemory(hmckDevice.device(), stagingBufferMemory, nullptr);
}

void Hmck::HmckModel::createIndexBuffers(const std::vector<uint32_t>& indices)
{
	indexCount = static_cast<uint32_t>(indices.size());

	hasIndexBuffer = indexCount > 0;

	if (!hasIndexBuffer)
	{
		return;
	}

	VkDeviceSize bufferSize = sizeof(indices[0]) * indexCount;

	VkBuffer stagingBufer;
	VkDeviceMemory stagingBufferMemory;

	hmckDevice.createBuffer(
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBufer,
		stagingBufferMemory
	);

	void* data;
	vkMapMemory(hmckDevice.device(), stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, indices.data(), static_cast<size_t>(bufferSize));
	vkUnmapMemory(hmckDevice.device(), stagingBufferMemory);

	hmckDevice.createBuffer(
		bufferSize,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		indexBuffer,
		indexBufferMemory
	);

	hmckDevice.copyBuffer(stagingBufer, indexBuffer, bufferSize);

	vkDestroyBuffer(hmckDevice.device(), stagingBufer, nullptr);
	vkFreeMemory(hmckDevice.device(), stagingBufferMemory, nullptr);
}

std::vector<VkVertexInputBindingDescription> Hmck::HmckModel::Vertex::getBindingDescriptions()
{
	std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
	bindingDescriptions[0].binding = 0;
	bindingDescriptions[0].stride = sizeof(Vertex);
	bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	return bindingDescriptions;
}
std::vector<VkVertexInputAttributeDescription> Hmck::HmckModel::Vertex::getAttributeDescriptions()
{
	return
	{	
		// order is location, binding, format, offset
		{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)},
		{1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color)},
		{2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)},
		{3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)},
	};
}

void Hmck::HmckModel::Builder::loadModel(const std::string& filepath)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.c_str()))
	{
		throw std::runtime_error(warn + err);
	}

	vertices.clear();
	indices.clear();

	std::unordered_map<Vertex, uint32_t> uniqueVertices{};

	for (const auto &shape : shapes)
	{
		for (const auto& index : shape.mesh.indices)
		{
			Vertex vertex{};

			if (index.vertex_index >= 0)
			{
				vertex.position = {
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2]
				};

				vertex.color = {
					attrib.colors[3 * index.vertex_index + 0],
					attrib.colors[3 * index.vertex_index + 1],
					attrib.colors[3 * index.vertex_index + 2]
				};
			}

			if (index.vertex_index >= 0)
			{
				vertex.normal = {
					attrib.normals[3 * index.normal_index + 0],
					attrib.normals[3 * index.normal_index + 1],
					attrib.normals[3 * index.normal_index + 2]
				};
			}

			if (index.texcoord_index >= 0) 
			{
				vertex.uv = {
					attrib.texcoords[2 * index.texcoord_index + 0],
					attrib.texcoords[2 * index.texcoord_index + 1],
				};
			}

			if (uniqueVertices.count(vertex) == 0)
			{
				uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
				vertices.push_back(vertex);
			}

			indices.push_back(uniqueVertices[vertex]);
		}
	}
}