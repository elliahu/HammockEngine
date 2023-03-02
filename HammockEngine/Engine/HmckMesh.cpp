#include "HmckMesh.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>


namespace std
{
	template <>
	struct hash<Hmck::HmckMesh::Vertex>
	{
		size_t operator()(Hmck::HmckMesh::Vertex const& vertex) const
		{
			size_t seed = 0;
			Hmck::hashCombine(seed, vertex.position, vertex.color, vertex.normal, vertex.uv);
			return seed;
		}
	};
}

Hmck::HmckMesh::HmckMesh(HmckDevice& device, const HmckMesh::Builder& builder) : hmckDevice{ device }
{
	createVertexBuffers(builder.vertices);
	createIndexBuffers(builder.indices);
}

Hmck::HmckMesh::~HmckMesh() {}


std::unique_ptr<Hmck::HmckMesh> Hmck::HmckMesh::createMeshFromFile(HmckDevice& device, const std::string& filepath, bool calculateTangents)
{
	Builder builder{};
	MeshInfo mInfo = builder.loadObjMesh(filepath, calculateTangents);

	std::unique_ptr<HmckMesh> model = std::make_unique<HmckMesh>(device, builder);
	model->modelInfo = mInfo;

	return model;
}

void Hmck::HmckMesh::draw(VkCommandBuffer commandBuffer)
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

void Hmck::HmckMesh::bind(VkCommandBuffer commandBuffer)
{
	VkBuffer buffers[] = { vertexBuffer->getBuffer() };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);

	if (hasIndexBuffer)
	{
		vkCmdBindIndexBuffer(commandBuffer, indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
	}
}

void Hmck::HmckMesh::createVertexBuffers(const std::vector<Vertex>& vertices)
{
	// copy data to staging memory on device, then copy from staging to v/i memory
	vertexCount = static_cast<uint32_t>(vertices.size());
	assert(vertexCount >= 3 && "Vertex count must be at least 3");
	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertexCount;
	uint32_t vertexSize = sizeof(vertices[0]);

	HmckBuffer stagingBuffer{
		hmckDevice,
		vertexSize,
		vertexCount,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	};


	stagingBuffer.map();
	stagingBuffer.writeToBuffer((void*)vertices.data());

	vertexBuffer = std::make_unique<HmckBuffer>(
		hmckDevice,
		vertexSize,
		vertexCount,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		);

	hmckDevice.copyBuffer(stagingBuffer.getBuffer(), vertexBuffer->getBuffer(), bufferSize);
}

void Hmck::HmckMesh::createIndexBuffers(const std::vector<uint32_t>& indices)
{
	indexCount = static_cast<uint32_t>(indices.size());

	hasIndexBuffer = indexCount > 0;

	if (!hasIndexBuffer)
	{
		return;
	}

	VkDeviceSize bufferSize = sizeof(indices[0]) * indexCount;
	uint32_t indexSize = sizeof(indices[0]);

	HmckBuffer stagingBuffer{
		hmckDevice,
		indexSize,
		indexCount,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	};

	stagingBuffer.map();
	stagingBuffer.writeToBuffer((void*)indices.data());

	indexBuffer = std::make_unique<HmckBuffer>(
		hmckDevice,
		indexSize,
		indexCount,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		);


	hmckDevice.copyBuffer(stagingBuffer.getBuffer(), indexBuffer->getBuffer(), bufferSize);
}

std::vector<VkVertexInputBindingDescription> Hmck::HmckMesh::Vertex::getBindingDescriptions()
{
	std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
	bindingDescriptions[0].binding = 0;
	bindingDescriptions[0].stride = sizeof(Vertex);
	bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	return bindingDescriptions;
}

std::vector<VkVertexInputAttributeDescription> Hmck::HmckMesh::Vertex::getAttributeDescriptions()
{
	return
	{
		// order is location, binding, format, offset
		{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)},
		{1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color)},
		{2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)},
		{3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)},
		{4, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, tangent)}
	};
}

Hmck::HmckMesh::MeshInfo Hmck::HmckMesh::Builder::loadObjMesh(const std::string& filepath, bool calcTangent)
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

	MeshInfo mInfo{};

	std::unordered_map<Vertex, uint32_t> uniqueVertices{};

	for (const auto& shape : shapes)
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

				// fill Info struct
				updateMeshInfo(mInfo, vertex);
			}

			indices.push_back(uniqueVertices[vertex]);
		}
	}

	// calculate tangent
	calculateTangent();

	return mInfo;
}


void Hmck::HmckMesh::Builder::updateMeshInfo(MeshInfo& mInfo, Vertex& vertex)
{
	if (vertex.position.x <= mInfo.x.min)
	{
		mInfo.x.min = vertex.position.x;
	}

	if (vertex.position.x >= mInfo.x.max)
	{
		mInfo.x.max = vertex.position.x;
	}

	if (vertex.position.y <= mInfo.y.min)
	{
		mInfo.y.min = vertex.position.y;
	}

	if (vertex.position.y >= mInfo.y.max)
	{
		mInfo.y.max = vertex.position.y;
	}

	if (vertex.position.z <= mInfo.z.min)
	{
		mInfo.z.min = vertex.position.z;
	}

	if (vertex.position.z >= mInfo.z.max)
	{
		mInfo.z.max = vertex.position.z;
	}
}


void Hmck::HmckMesh::Builder::calculateTangent()
{
	for (uint32_t i = 0; i < static_cast<uint32_t>(indices.size()); i += 3)
	{
		uint32_t i0 = indices[i + 0];
		uint32_t i1 = indices[i + 1];
		uint32_t i2 = indices[i + 2];

		// Shortcuts for vertices
		glm::vec3& v0 = vertices[i0].position;
		glm::vec3& v1 = vertices[i1].position;
		glm::vec3& v2 = vertices[i2].position;

		// Shortcuts for UVs
		glm::vec2& uv0 = vertices[i0].uv;
		glm::vec2& uv1 = vertices[i1].uv;
		glm::vec2& uv2 = vertices[i2].uv;

		// Edges of the triangle : position delta
		glm::vec3 deltaPos1 = v1 - v0;
		glm::vec3 deltaPos2 = v2 - v0;

		// UV delta
		glm::vec2 deltaUV1 = uv1 - uv0;
		glm::vec2 deltaUV2 = uv2 - uv0;

		float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
		glm::vec3 tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r;
		glm::vec3 bitangent = (deltaPos2 * deltaUV1.x - deltaPos1 * deltaUV2.x) * r;

		vertices[i0].tangent = tangent;
		vertices[i1].tangent = tangent;
		vertices[i2].tangent = tangent;
	}
}
