#include "HmckScene.h"

Hmck::Scene::Scene(SceneCreateInfo createInfo): device{createInfo.device}
{
	// create descriptor pool
	descriptorPool = DescriptorPool::Builder(device)
		.setMaxSets(100)
		.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2000)
		.addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2000)
		.build();

	descriptorSetLayout = DescriptorSetLayout::Builder(device)
		.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
		.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_ALL_GRAPHICS, 200, VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT)
		.build();

	// create root
	root = std::make_shared<Entity3D>(device, descriptorPool);

	// load all files
	for (auto& fileInfo : createInfo.loadFiles)
	{
		loadFile(fileInfo);
	}

	// prepare buffers
	for (int i = 0; i < sceneBuffers.size(); i++)
	{
		sceneBuffers[i] = std::make_unique<Buffer>(
			device,
			sizeof(SceneUbo),
			1,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
			);
		sceneBuffers[i]->map();
	}

	// write to buffers as it is usualy done once at the start
	std::vector<VkDescriptorImageInfo> imageInfos{ images.size() };
	for (int im = 0; im < images.size(); im++)
	{
		imageInfos[im] = images[im].texture.descriptor;
	}

	// write to descriptors
	for (int i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; i++)
	{
		auto sceneBufferInfo = sceneBuffers[i]->descriptorInfo();

		DescriptorWriter(*descriptorSetLayout, *descriptorPool)
			.writeBuffer(0, &sceneBufferInfo)
			.writeImages(1, imageInfos)
			.build(descriptorSets[i]);
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

	for (unsigned int i = 0; i < materials.size(); i++)
	{
		materials[i].buffer = nullptr;
		materials[i].descriptorSetLayout = nullptr;
	}

	// TODO entities are not being destroyed properly :/

	root->buffer = nullptr;
	root->descriptorSetLayout = nullptr;
	root = nullptr;
}


void Hmck::Scene::loadFile(SceneLoadFileInfo loadInfo)
{
	// load the entity tree
	std::vector<std::shared_ptr<Entity>> roots = Gltf::load(
		loadInfo.filename, 
		device, 
		descriptorPool,
		images,
		static_cast<uint32_t>(images.size()),
		materials, 
		static_cast<uint32_t>(materials.size()),
		textures, 
		static_cast<uint32_t>(textures.size()),
		vertices, 
		indices, 
		root,
		loadInfo.binary);
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
