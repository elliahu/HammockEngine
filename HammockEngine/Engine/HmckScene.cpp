#include "HmckScene.h"

Hmck::Scene::Scene(SceneCreateInfo createInfo): device{createInfo.device}
{
	// create root
	auto root = std::make_shared<Entity3D>();
	entities.push_back(root);

	// load all files
	for (auto& fileInfo : createInfo.loadFiles)
	{
		loadFile(fileInfo);
	}

	// create buffers
	createVertexBuffer();
	createIndexBuffer();
	vertices.clear();
	vertices.clear();

}

Hmck::Scene::~Scene()
{
	for (unsigned int i = 0; i < images.size(); i++)
	{
		images[i].texture.destroy(device);
	}
}

void Hmck::Scene::draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout)
{
	VkDeviceSize offsets[] = { 0 };
	VkBuffer buffers[] = { vertexBuffer->getBuffer() };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
	vkCmdBindIndexBuffer(commandBuffer, indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);

	// Render all nodes at top-level
	for (auto& entity : entities) {
		drawEntity(commandBuffer, entity, pipelineLayout);
	}
}

void Hmck::Scene::loadFile(SceneLoadFileInfo loadInfo)
{
	// load the entity tree
	std::vector<std::shared_ptr<Entity>> roots = Gltf::load(
		loadInfo.filename, 
		device, 
		images,
		materials, 
		textures, 
		vertices, 
		indices, 
		entities,
		loadInfo.binary);
	// set the entity tree as a child of a scene root
	for(auto& entity : roots)
		addChildOfRoot(entity);
}

void Hmck::Scene::drawEntity(VkCommandBuffer commandBuffer, std::shared_ptr<Entity>& entity, VkPipelineLayout pipelineLayout)
{
	// don't render invisible nodes
	if (!entity->visible) { return; }

	if (std::dynamic_pointer_cast<Entity3D>(entity)->mesh.primitives.size() > 0) {
		// Pass the node's matrix via push constants
		// Traverse the node hierarchy to the top-most parent to get the final matrix of the current node
		glm::mat4 nodeMatrix = entity->transform;
		std::shared_ptr<Entity> currentParent = entity->parent;
		while (currentParent) {
			nodeMatrix = currentParent->transform * nodeMatrix;
			currentParent = currentParent->parent;
		}

		for (Primitive& primitive : std::dynamic_pointer_cast<Entity3D>(entity)->mesh.primitives) {
			if (primitive.indexCount > 0) {
				// Get the material index for this primitive
				Material& material = materials[primitive.materialIndex];

				// Pass the final matrix to the vertex shader using push constants
				Entity::TransformPushConstantData pushData{
					.modelMatrix = nodeMatrix,
					.normalMatrix = glm::transpose(glm::inverse(nodeMatrix)),
					//.albedo_index = textures[material.baseColorTextureIndex].imageIndex
				};

				vkCmdPushConstants(
					commandBuffer,
					pipelineLayout,
					VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
					0, sizeof(Entity::TransformPushConstantData),
					&pushData);

				// draw
				vkCmdDrawIndexed(commandBuffer, primitive.indexCount, 1, primitive.firstIndex, 0, 0);
			}
		}
	}
	for (auto& child : entity->children) {
		drawEntity(commandBuffer, child, pipelineLayout);
	}
}

void Hmck::Scene::createVertexBuffer()
{
	// copy data to staging memory on device, then copy from staging to v/i memory
	vertexCount = static_cast<uint32_t>(vertices.size());
	assert(vertexCount >= 3 && "Vertex count must be at least 3");
	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertexCount;
	uint32_t vertexSize = sizeof(vertices[0]);

	Buffer stagingBuffer{
		device,
		vertexSize,
		vertexCount,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	};


	stagingBuffer.map();
	stagingBuffer.writeToBuffer((void*)vertices.data());

	vertexBuffer = std::make_unique<Buffer>(
		device,
		vertexSize,
		vertexCount,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		);

	device.copyBuffer(stagingBuffer.getBuffer(), vertexBuffer->getBuffer(), bufferSize);
}

void Hmck::Scene::createIndexBuffer()
{
	indexCount = static_cast<uint32_t>(indices.size());

	VkDeviceSize bufferSize = sizeof(indices[0]) * indexCount;
	uint32_t indexSize = sizeof(indices[0]);

	Buffer stagingBuffer{
		device,
		indexSize,
		indexCount,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	};

	stagingBuffer.map();
	stagingBuffer.writeToBuffer((void*)indices.data());

	indexBuffer = std::make_unique<Buffer>(
		device,
		indexSize,
		indexCount,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		);


	device.copyBuffer(stagingBuffer.getBuffer(), indexBuffer->getBuffer(), bufferSize);
}
