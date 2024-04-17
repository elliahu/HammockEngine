#include "HmckRenderer.h"

Hmck::Renderer::Renderer(Window& window, Device& device, std::unique_ptr<Scene>& scene) : window{ window }, device{ device }, scene{ scene }
{
	recreateSwapChain();
	createCommandBuffer();

	// create descriptor pool
	descriptorPool = DescriptorPool::Builder(device)
		.setMaxSets(10000)
		.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 5000)
		.addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 5000)
		.build();

	// prepare descriptor set layouts
	environmentDescriptorSetLayout = DescriptorSetLayout::Builder(device)
		.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
		.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_ALL_GRAPHICS, 200, VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT)
		.addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_ALL_GRAPHICS)
		.build();

	frameDescriptorSetLayout = DescriptorSetLayout::Builder(device)
		.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
		.build();

	entityDescriptorSetLayout = DescriptorSetLayout::Builder(device)
		.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
		.build();

	primitiveDescriptorSetLayout = DescriptorSetLayout::Builder(device)
		.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
		.build();

	noiseDescriptorSetLayout = DescriptorSetLayout::Builder(device)
		.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_ALL_GRAPHICS)
		.build();

	std::string noiseFile = "../../Resources/Noise/noise2.png";
	noiseTexture.loadFromFile(noiseFile, device, VK_FORMAT_R8G8B8A8_UNORM);
	noiseTexture.createSampler(device);
	noiseTexture.updateDescriptor();

	DescriptorWriter(*noiseDescriptorSetLayout, *descriptorPool)
		.writeImage(0, &noiseTexture.descriptor)
		.build(noiseDescriptorSet);

	// prepare buffers
	environmentBuffer = std::make_unique<Buffer>(
		device,
		sizeof(EnvironmentBufferData),
		1,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		);
	environmentBuffer->map();


	for (int i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; i++)
	{
		frameBuffers[i] = std::make_unique<Buffer>(
			device,
			sizeof(FrameBufferData),
			1,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
			);
		frameBuffers[i]->map();

		auto fbufferInfo = frameBuffers[i]->descriptorInfo();
		DescriptorWriter(*frameDescriptorSetLayout, *descriptorPool)
			.writeBuffer(0, &fbufferInfo)
			.build(frameDescriptorSets[i]);
	}

	entityDescriptorSets.resize(scene->root->numberOfEntities());
	entityBuffers.resize(scene->root->numberOfEntities());
	for (size_t i = 0; i < entityDescriptorSets.size(); i++)
	{
		entityBuffers[i] = std::make_unique<Buffer>(
			device,
			sizeof(EntityBufferData),
			1,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
			);
		entityBuffers[i]->map();

		auto ebufferInfo = entityBuffers[i]->descriptorInfo();
		DescriptorWriter(*entityDescriptorSetLayout, *descriptorPool)
			.writeBuffer(0, &ebufferInfo)
			.build(entityDescriptorSets[i]);
	}

	primitiveDescriptorSets.resize(scene->materials.size());
	primitiveBuffers.resize(scene->materials.size());
	for (size_t i = 0; i < primitiveDescriptorSets.size(); i++)
	{
		primitiveBuffers[i] = std::make_unique<Buffer>(
			device,
			sizeof(PrimitiveBufferData),
			1,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
			);
		primitiveBuffers[i]->map();

		auto pbufferInfo = primitiveBuffers[i]->descriptorInfo();
		DescriptorWriter(*primitiveDescriptorSetLayout, *descriptorPool)
			.writeBuffer(0, &pbufferInfo)
			.build(primitiveDescriptorSets[i]);
	}

	// create pipline
	// TODO change this to per material pipeline
	forwardPipeline = GraphicsPipeline::createGraphicsPipelinePtr({
			.debugName = "standard_forward_pass",
			.device = device,
			.VS
			{
				.byteCode = Hmck::Filesystem::readFile("../../HammockEngine/Engine/Shaders/Compiled/fullscreen.vert.spv"),
				.entryFunc = "main"
			},
			.FS 
			{
				.byteCode = Hmck::Filesystem::readFile("../../HammockEngine/Engine/Shaders/Compiled/raymarch.frag.spv"),
				.entryFunc = "main"
			},
			.descriptorSetLayouts = 
			{
				environmentDescriptorSetLayout->getDescriptorSetLayout(),
				frameDescriptorSetLayout->getDescriptorSetLayout(),
				entityDescriptorSetLayout->getDescriptorSetLayout(),
				primitiveDescriptorSetLayout->getDescriptorSetLayout(),
				noiseDescriptorSetLayout->getDescriptorSetLayout()
			},
			.pushConstantRanges {
				{
					.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
					.offset = 0,
					.size = sizeof(Renderer::PushConstantData)
				}
			},
			.graphicsState 
			{
				.depthTest = VK_TRUE,
				.depthTestCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
				.blendAtaAttachmentStates {},
				.vertexBufferBindings 
				{
					.vertexBindingDescriptions = 
					{
						{
							.binding = 0,
							.stride = sizeof(Vertex),
							.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
						}
					},
					.vertexAttributeDescriptions = 
					{
						{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)},
						{1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color)},
						{2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)},
						{3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)},
						{4, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, tangent)}
					}
				}
			},
			.renderPass = getSwapChainRenderPass()
	});

	

	auto depthFormat = device.findSupportedFormat(
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

	gbufferDescriptorSetLayout = DescriptorSetLayout::Builder(device)
		.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // position
		.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // albedo
		.addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // normal
		.addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // material (roughness, metalness, ao)
		.build();

	gbufferFramebuffer = Framebuffer::createFramebufferPtr({
		.device = device,
		.width = IApp::WINDOW_WIDTH, .height = IApp::WINDOW_HEIGHT,
		.attachments {
			// 0 position
			{
				.width = IApp::WINDOW_WIDTH, .height = IApp::WINDOW_HEIGHT,
				.layerCount = 1,
				.format = VK_FORMAT_R16G16B16A16_SFLOAT,
				.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			},
			// 1 albedo
			{
				.width = IApp::WINDOW_WIDTH, .height = IApp::WINDOW_HEIGHT,
				.layerCount = 1,
				.format = VK_FORMAT_R8G8B8A8_UNORM,
				.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			},
			// 2 normal
			{
				.width = IApp::WINDOW_WIDTH, .height = IApp::WINDOW_HEIGHT,
				.layerCount = 1,
				.format = VK_FORMAT_R16G16B16A16_SFLOAT,
				.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			},
			// 3 rough, metal, ao
			{
				.width = IApp::WINDOW_WIDTH, .height = IApp::WINDOW_HEIGHT,
				.layerCount = 1,
				.format = VK_FORMAT_R16G16B16A16_SFLOAT,
				.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			},
			// 4 depth
			{
				.width = IApp::WINDOW_WIDTH, .height = IApp::WINDOW_HEIGHT,
				.layerCount = 1,
				.format = depthFormat,
				.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			},
		}
	});

	skyboxPipeline = GraphicsPipeline::createGraphicsPipelinePtr({
		.debugName = "skybox_pass",
		.device = device,
		.VS {
			.byteCode = Hmck::Filesystem::readFile("../../HammockEngine/Engine/Shaders/Compiled/skybox.vert.spv"),
			.entryFunc = "main"
		},
		.FS {
			.byteCode = Hmck::Filesystem::readFile("../../HammockEngine/Engine/Shaders/Compiled/skybox.frag.spv"),
			.entryFunc = "main"
		},
		.descriptorSetLayouts = {
			environmentDescriptorSetLayout->getDescriptorSetLayout(),
			frameDescriptorSetLayout->getDescriptorSetLayout()
		},
		.pushConstantRanges {},
		.graphicsState {
			.depthTest = VK_FALSE,
			.depthTestCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
			.cullMode = VK_CULL_MODE_FRONT_BIT,
			.blendAtaAttachmentStates {
				Hmck::Init::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
				Hmck::Init::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
				Hmck::Init::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
				Hmck::Init::pipelineColorBlendAttachmentState(0xf, VK_FALSE)
			},
			.vertexBufferBindings
			{
					.vertexBindingDescriptions =
					{
						{
							.binding = 0,
							.stride = sizeof(Vertex),
							.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
						}
					},
					.vertexAttributeDescriptions =
					{
						{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)},
						{1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color)},
						{2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)},
						{3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)},
						{4, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, tangent)}
					}
				}
		},
		.renderPass = gbufferFramebuffer->renderPass
	});

	gbufferPipeline = GraphicsPipeline::createGraphicsPipelinePtr({
		.debugName = "gbuffer_pass",
		.device = device,
		.VS {
			.byteCode = Hmck::Filesystem::readFile("../../HammockEngine/Engine/Shaders/Compiled/gbuffer.vert.spv"),
			.entryFunc = "main"
		},
		.FS {
			.byteCode = Hmck::Filesystem::readFile("../../HammockEngine/Engine/Shaders/Compiled/gbuffer.frag.spv"),
			.entryFunc = "main"
		},
		.descriptorSetLayouts = {
			environmentDescriptorSetLayout->getDescriptorSetLayout(),
			frameDescriptorSetLayout->getDescriptorSetLayout(),
			entityDescriptorSetLayout->getDescriptorSetLayout(),
			primitiveDescriptorSetLayout->getDescriptorSetLayout()
		},
		.pushConstantRanges {},
		.graphicsState {
			.depthTest = VK_TRUE,
			.depthTestCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
			.blendAtaAttachmentStates {
				Hmck::Init::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
				Hmck::Init::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
				Hmck::Init::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
				Hmck::Init::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
			},
			.vertexBufferBindings 
			{
					.vertexBindingDescriptions =
					{
						{
							.binding = 0,
							.stride = sizeof(Vertex),
							.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
						}
					},
					.vertexAttributeDescriptions =
					{
						{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)},
						{1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color)},
						{2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)},
						{3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)},
						{4, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, tangent)}
					}
				}
		},
		.renderPass = gbufferFramebuffer->renderPass
	});

	for (int i = 0; i < gbufferDescriptorSets.size(); i++)
	{
		std::vector<VkDescriptorImageInfo> gbufferImageInfos = {
			{
				.sampler = gbufferFramebuffer->sampler,
				.imageView = gbufferFramebuffer->attachments[0].view,
				.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			},
			{
				.sampler = gbufferFramebuffer->sampler,
				.imageView = gbufferFramebuffer->attachments[1].view,
				.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			},
			{
				.sampler = gbufferFramebuffer->sampler,
				.imageView = gbufferFramebuffer->attachments[2].view,
				.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			},
			{
				.sampler = gbufferFramebuffer->sampler,
				.imageView = gbufferFramebuffer->attachments[3].view,
				.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			}
		};

		DescriptorWriter(*gbufferDescriptorSetLayout, *descriptorPool)
			.writeImages(0, gbufferImageInfos)
			.build(gbufferDescriptorSets[i]);
	}

	defferedPipeline = GraphicsPipeline::createGraphicsPipelinePtr({
		.debugName = "deferred_pass",
		.device = device,
		.VS {
			.byteCode = Hmck::Filesystem::readFile("../../HammockEngine/Engine/Shaders/Compiled/fullscreen.vert.spv"),
			.entryFunc = "main"
		},
		.FS {
			.byteCode = Hmck::Filesystem::readFile("../../HammockEngine/Engine/Shaders/Compiled/deferred.frag.spv"),
			.entryFunc = "main"
		},
		.descriptorSetLayouts = {
			environmentDescriptorSetLayout->getDescriptorSetLayout(),
			frameDescriptorSetLayout->getDescriptorSetLayout(),
			entityDescriptorSetLayout->getDescriptorSetLayout(),
			primitiveDescriptorSetLayout->getDescriptorSetLayout(),
			gbufferDescriptorSetLayout->getDescriptorSetLayout()
		},
		.pushConstantRanges {},
		.graphicsState {
			.depthTest = VK_TRUE,
			.depthTestCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
			.blendAtaAttachmentStates {},
			.vertexBufferBindings 
			{
					.vertexBindingDescriptions =
					{
						{
							.binding = 0,
							.stride = sizeof(Vertex),
							.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
						}
					},
					.vertexAttributeDescriptions =
					{
						{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)},
						{1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color)},
						{2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)},
						{3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)},
						{4, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, tangent)}
					}
				}
		},
		.renderPass = getSwapChainRenderPass()
	});

	// create vertex & index buffer
	createVertexBuffer();
	createIndexBuffer();
}

Hmck::Renderer::~Renderer()
{
	freeCommandBuffers();
	noiseTexture.destroy(device);
}


void Hmck::Renderer::createCommandBuffer()
{
	commandBuffers.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = device.getCommandPool();
	allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

	if (vkAllocateCommandBuffers(device.device(), &allocInfo, commandBuffers.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate command buffer");
	}
}


void Hmck::Renderer::freeCommandBuffers()
{
	vkFreeCommandBuffers(
		device.device(),
		device.getCommandPool(),
		static_cast<uint32_t>(commandBuffers.size()),
		commandBuffers.data());
	commandBuffers.clear();
}

void Hmck::Renderer::createVertexBuffer()
{
	// copy data to staging memory on device, then copy from staging to v/i memory
	uint32_t vertexCount = static_cast<uint32_t>(scene->vertices.size());
	assert(vertexCount >= 3 && "Vertex count must be at least 3");
	VkDeviceSize bufferSize = sizeof(scene->vertices[0]) * vertexCount;
	uint32_t vertexSize = sizeof(scene->vertices[0]);

	Buffer stagingBuffer{
		device,
		vertexSize,
		vertexCount,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	};


	stagingBuffer.map();
	stagingBuffer.writeToBuffer((void*)scene->vertices.data());

	sceneVertexBuffer = std::make_unique<Buffer>(
		device,
		vertexSize,
		vertexCount,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		);

	device.copyBuffer(stagingBuffer.getBuffer(), sceneVertexBuffer->getBuffer(), bufferSize);

	if (scene->hasSkybox)
	{
		uint32_t vertexCount = static_cast<uint32_t>(scene->skyboxVertices.size());
		assert(vertexCount >= 3 && "Vertex count must be at least 3");
		VkDeviceSize bufferSize = sizeof(scene->skyboxVertices[0]) * vertexCount;
		uint32_t vertexSize = sizeof(scene->skyboxVertices[0]);

		Buffer stagingBuffer{
			device,
			vertexSize,
			vertexCount,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		};


		stagingBuffer.map();
		stagingBuffer.writeToBuffer((void*)scene->skyboxVertices.data());

		skyboxVertexBuffer = std::make_unique<Buffer>(
			device,
			vertexSize,
			vertexCount,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
			);

		device.copyBuffer(stagingBuffer.getBuffer(), skyboxVertexBuffer->getBuffer(), bufferSize);
	}
}

void Hmck::Renderer::createIndexBuffer()
{
	uint32_t indexCount = static_cast<uint32_t>(scene->indices.size());

	VkDeviceSize bufferSize = sizeof(scene->indices[0]) * indexCount;
	uint32_t indexSize = sizeof(scene->indices[0]);

	Buffer stagingBuffer{
		device,
		indexSize,
		indexCount,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	};

	stagingBuffer.map();
	stagingBuffer.writeToBuffer((void*)scene->indices.data());

	sceneIndexBuffer = std::make_unique<Buffer>(
		device,
		indexSize,
		indexCount,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		);


	device.copyBuffer(stagingBuffer.getBuffer(), sceneIndexBuffer->getBuffer(), bufferSize);

	if (scene->hasSkybox)
	{
		uint32_t indexCount = static_cast<uint32_t>(scene->skyboxIndices.size());

		VkDeviceSize bufferSize = sizeof(scene->skyboxIndices[0]) * indexCount;
		uint32_t indexSize = sizeof(scene->skyboxIndices[0]);

		Buffer stagingBuffer{
			device,
			indexSize,
			indexCount,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		};

		stagingBuffer.map();
		stagingBuffer.writeToBuffer((void*)scene->skyboxIndices.data());

		skyboxIndexBuffer = std::make_unique<Buffer>(
			device,
			indexSize,
			indexCount,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
			);


		device.copyBuffer(stagingBuffer.getBuffer(), skyboxIndexBuffer->getBuffer(), bufferSize);
	}
}



void Hmck::Renderer::recreateSwapChain()
{
	auto extent = window.getExtent();
	while (extent.width == 0 || extent.height == 0)
	{
		extent = window.getExtent();
		glfwWaitEvents();
	}
	vkDeviceWaitIdle(device.device());

	if (hmckSwapChain == nullptr)
	{
		hmckSwapChain = std::make_unique<SwapChain>(device, extent);
	}
	else {
		std::shared_ptr<SwapChain> oldSwapChain = std::move(hmckSwapChain);
		hmckSwapChain = std::make_unique<SwapChain>(device, extent, oldSwapChain);

		if (!oldSwapChain->compareSwapFormats(*hmckSwapChain.get()))
		{
			throw std::runtime_error("Swapchain image (or detph) format has changed");
		}
	}

	//
}

void Hmck::Renderer::renderEntity(
	uint32_t frameIndex,
	VkCommandBuffer commandBuffer,
	std::unique_ptr<GraphicsPipeline>& pipeline,
	std::shared_ptr<Entity>& entity)
{
	// don't render invisible nodes
	if (!entity->visible) { return; }

	if (std::dynamic_pointer_cast<Entity3D>(entity)->mesh.primitives.size() > 0) {
		// Pass the node's matrix via push constants
		// Traverse the node hierarchy to the top-most parent to get the final matrix of the current node
		glm::mat4 model = entity->transform.mat4();
		std::shared_ptr<Entity> currentParent = entity->parent;
		while (currentParent) {
			model = currentParent->transform.mat4() * model;
			currentParent = currentParent->parent;
		}

		EntityBufferData entityData{
			.model = model,
			.normal = glm::transpose(glm::inverse(model))
		};
		entityBuffers[entity->id]->writeToBuffer(&entityData);

		// bind per entity
		vkCmdBindDescriptorSets(
			commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipeline->graphicsPipelineLayout,
			2, 1,
			&entityDescriptorSets[entity->id],
			0,
			nullptr);

		// TODO Reduce writes by checking if material changed
		auto _entity = std::dynamic_pointer_cast<Entity3D>(entity);
		for (Primitive& primitive : _entity->mesh.primitives) {
			if (primitive.indexCount > 0) {

				if (primitive.materialIndex >= 0)
				{
					Material& material = scene->materials[primitive.materialIndex];

					PrimitiveBufferData pData{
					.baseColorFactor = material.baseColorFactor,
					.baseColorTextureIndex = (material.baseColorTextureIndex != TextureHandle::Invalid) ? scene->textures[material.baseColorTextureIndex].imageIndex : TextureHandle::Invalid,
					.normalTextureIndex = (material.normalTextureIndex != TextureHandle::Invalid) ? scene->textures[material.normalTextureIndex].imageIndex : TextureHandle::Invalid,
					.metallicRoughnessTextureIndex = (material.metallicRoughnessTextureIndex != TextureHandle::Invalid) ? scene->textures[material.metallicRoughnessTextureIndex].imageIndex : TextureHandle::Invalid,
					.occlusionTextureIndex = (material.occlusionTextureIndex != TextureHandle::Invalid) ? scene->textures[material.occlusionTextureIndex].imageIndex : TextureHandle::Invalid,
					.alphaCutoff = material.alphaCutOff
					};

					primitiveBuffers[primitive.materialIndex]->writeToBuffer(&pData);
				}

				// bind per primitive
				vkCmdBindDescriptorSets(
					commandBuffer,
					VK_PIPELINE_BIND_POINT_GRAPHICS,
					pipeline->graphicsPipelineLayout,
					3, 1,
					&primitiveDescriptorSets[(primitive.materialIndex >= 0 ? primitive.materialIndex : 0)],
					0,
					nullptr);

				// draw
				vkCmdDrawIndexed(commandBuffer, primitive.indexCount, 1, primitive.firstIndex, 0, 0);
			}
		}
	}
	for (auto& child : entity->children) {
		renderEntity(frameIndex, commandBuffer, pipeline, child);
	}
}

VkCommandBuffer Hmck::Renderer::beginFrame()
{
	assert(!isFrameStarted && "Cannot call beginFrame while already in progress");

	auto result = hmckSwapChain->acquireNextImage(&currentImageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		recreateSwapChain();
		return nullptr;
	}

	if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		throw std::runtime_error("failed to aquire swap chain image!");
	}

	isFrameStarted = true;

	auto commandBuffer = getCurrentCommandBuffer();
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to begin recording commadn buffer");
	}
	return commandBuffer;
}

void Hmck::Renderer::endFrame()
{
	assert(isFrameStarted && "Cannot call endFrame while frame is not in progress");
	auto commandBuffer = getCurrentCommandBuffer();

	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to record command buffer");
	}

	auto result = hmckSwapChain->submitCommandBuffers(&commandBuffer, &currentImageIndex);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || window.wasWindowResized())
	{
		// Window was resized (resolution was changed)
		window.resetWindowResizedFlag();
		recreateSwapChain();
	}

	else if (result != VK_SUCCESS)
	{
		throw std::runtime_error("failed to present swap chain image");
	}

	isFrameStarted = false;

	currentFrameIndex = (currentFrameIndex + 1) % SwapChain::MAX_FRAMES_IN_FLIGHT;
}


void Hmck::Renderer::beginRenderPass(
	std::unique_ptr<Framebuffer>& framebuffer,
	VkCommandBuffer commandBuffer,
	std::vector<VkClearValue> clearValues)
{
	// TODO delete this method when no longer used
	assert(isFrameInProgress() && "Cannot call beginRenderPass if frame is not in progress");
	assert(commandBuffer == getCurrentCommandBuffer() && "Cannot begin render pass on command buffer from a different frame");
	assert(framebuffer->renderPass && "Provided framebuffer doesn't have a render pass");

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = framebuffer->renderPass;
	renderPassInfo.framebuffer = framebuffer->framebuffer;
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = { framebuffer->width, framebuffer->height };
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport = Hmck::Init::viewport(
		static_cast<float>(framebuffer->width),
		static_cast<float>(framebuffer->height),
		0.0f, 1.0f);
	VkRect2D scissor{ {0,0}, { framebuffer->width, framebuffer->height} };
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void Hmck::Renderer::beginRenderPass(Framebuffer& framebuffer, VkCommandBuffer commandBuffer, std::vector<VkClearValue> clearValues)
{
	assert(isFrameInProgress() && "Cannot call beginRenderPass if frame is not in progress");
	assert(commandBuffer == getCurrentCommandBuffer() && "Cannot begin render pass on command buffer from a different frame");
	assert(framebuffer.renderPass && "Provided framebuffer doesn't have a render pass");

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = framebuffer.renderPass;
	renderPassInfo.framebuffer = framebuffer.framebuffer;
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = { framebuffer.width, framebuffer.height };
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport = Hmck::Init::viewport(
		static_cast<float>(framebuffer.width),
		static_cast<float>(framebuffer.height),
		0.0f, 1.0f);
	VkRect2D scissor{ {0,0}, { framebuffer.width, framebuffer.height} };
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void Hmck::Renderer::beginSwapChainRenderPass(VkCommandBuffer commandBuffer)
{
	assert(isFrameInProgress() && "Cannot call beginSwapChainRenderPass if frame is not in progress");
	assert(commandBuffer == getCurrentCommandBuffer() && "Cannot begin render pass on command buffer from a different frame");

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = hmckSwapChain->getRenderPass();
	renderPassInfo.framebuffer = hmckSwapChain->getFrameBuffer(currentImageIndex);

	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = hmckSwapChain->getSwapChainExtent();

	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = HMCK_CLEAR_COLOR; // clear color
	clearValues[1].depthStencil = { 1.0f, 0 };

	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport = Hmck::Init::viewport(
		static_cast<float>(hmckSwapChain->getSwapChainExtent().width),
		static_cast<float>(hmckSwapChain->getSwapChainExtent().height),
		0.0f, 1.0f);
	VkRect2D scissor{ {0,0}, hmckSwapChain->getSwapChainExtent() };
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void Hmck::Renderer::endRenderPass(VkCommandBuffer commandBuffer)
{
	assert(isFrameInProgress() && "Cannot call endActiveRenderPass if frame is not in progress");
	assert(commandBuffer == getCurrentCommandBuffer() && "Cannot end render pass on command buffer from a different frame");

	vkCmdEndRenderPass(commandBuffer);
}

void Hmck::Renderer::renderForward(
	uint32_t frameIndex,
	float time,
	VkCommandBuffer commandBuffer)
{
	beginSwapChainRenderPass(commandBuffer);
	

	VkDeviceSize offsets[] = { 0 };
	VkBuffer buffers[] = { sceneVertexBuffer->getBuffer() };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
	vkCmdBindIndexBuffer(commandBuffer, sceneIndexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);

	forwardPipeline->bind(commandBuffer);

	// bind env
	vkCmdBindDescriptorSets(
		commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		forwardPipeline->graphicsPipelineLayout,
		0, 1,
		&environmentDescriptorSet,
		0,
		nullptr);

	FrameBufferData data{
		.projection = scene->camera.getProjection(),
		.view = scene->camera.getView(),
		.inverseView = scene->camera.getInverseView()
	};
	frameBuffers[frameIndex]->writeToBuffer(&data);

	// bind per frame
	vkCmdBindDescriptorSets(
		commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		forwardPipeline->graphicsPipelineLayout,
		1, 1,
		&frameDescriptorSets[frameIndex],
		0,
		nullptr);
	
	
	Renderer::PushConstantData pushdata 
	{ 
		.resolution = { IApp::WINDOW_WIDTH, IApp::WINDOW_HEIGHT },
		.elapsedTime = time 
	};
	vkCmdPushConstants(commandBuffer, forwardPipeline->graphicsPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(Renderer::PushConstantData), &pushdata);

	vkCmdBindDescriptorSets(
		commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		forwardPipeline->graphicsPipelineLayout,
		4, 1,
		&noiseDescriptorSet,
		0,
		nullptr);

	vkCmdDraw(commandBuffer, 3, 1, 0, 0);
}

void Hmck::Renderer::renderDeffered(uint32_t frameIndex, VkCommandBuffer commandBuffer)
{
	beginRenderPass(gbufferFramebuffer, commandBuffer, {
					{.color = { 0.0f, 0.0f, 0.0f, 0.0f } },
					{.color = { 0.0f, 0.0f, 0.0f, 0.0f } },
					{.color = { 0.0f, 0.0f, 0.0f, 0.0f } },
					{.color = { 0.0f, 0.0f, 0.0f, 0.0f } },
					{.depthStencil = { 1.0f, 0 }}});

	FrameBufferData data{
		.projection = scene->camera.getProjection(),
		.view = scene->camera.getView(),
		.inverseView = scene->camera.getInverseView()
	};
	frameBuffers[frameIndex]->writeToBuffer(&data);

	// bind env
	vkCmdBindDescriptorSets(
		commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		skyboxPipeline->graphicsPipelineLayout,
		0, 1,
		&environmentDescriptorSet,
		0,
		nullptr);

	// bind per frame
	vkCmdBindDescriptorSets(
		commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		skyboxPipeline->graphicsPipelineLayout,
		1, 1,
		&frameDescriptorSets[frameIndex],
		0,
		nullptr);

	if (scene->hasSkybox) 
	{
		// skybox
		VkDeviceSize offsets[] = { 0 };
		VkBuffer buffers[] = { skyboxVertexBuffer->getBuffer() };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
		vkCmdBindIndexBuffer(commandBuffer, skyboxIndexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);

		skyboxPipeline->bind(commandBuffer);

		vkCmdDrawIndexed(commandBuffer, 36, 1, 0, 0, 0);
	}
	
	// gbuffer
	VkDeviceSize sceneoffsets[] = { 0 };
	VkBuffer scenebuffers[] = { sceneVertexBuffer->getBuffer() };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, scenebuffers, sceneoffsets);
	vkCmdBindIndexBuffer(commandBuffer, sceneIndexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);

	gbufferPipeline->bind(commandBuffer);

	// Render all nodes at top-level off-screen
	renderEntity(frameIndex, commandBuffer, gbufferPipeline, scene->root);

	endRenderPass(commandBuffer);
	beginSwapChainRenderPass(commandBuffer);

	defferedPipeline->bind(commandBuffer);

	//bind gbuffer descriptors
	vkCmdBindDescriptorSets(
		commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		defferedPipeline->graphicsPipelineLayout,
		4, 1,
		&gbufferDescriptorSets[frameIndex],
		0,
		nullptr);

	// draw deffered
	vkCmdDraw(commandBuffer, 3, 1, 0, 0);

}


void Hmck::Renderer::writeEnvironmentData(std::vector<Image>& images, EnvironmentBufferData data, TextureCubeMap skybox)
{
	std::vector<VkDescriptorImageInfo> imageInfos{ images.size() };
	for (int im = 0; im < images.size(); im++)
	{
		imageInfos[im] = images[im].texture.descriptor;
	}

	environmentBuffer->writeToBuffer(&data);

	auto sceneBufferInfo = environmentBuffer->descriptorInfo();
	auto result = DescriptorWriter(*environmentDescriptorSetLayout, *descriptorPool)
		.writeBuffer(0, &sceneBufferInfo)
		.writeImages(1, imageInfos)
		.writeImage(2, &skybox.descriptor)
		.build(environmentDescriptorSet);
}

void Hmck::Renderer::updateEnvironmentBuffer(EnvironmentBufferData data)
{
	environmentBuffer->writeToBuffer(&data);
}


void Hmck::Renderer::updateFrameBuffer(uint32_t index, FrameBufferData data)
{
	frameBuffers[index]->writeToBuffer(&data);
}

void Hmck::Renderer::updateEntityBuffer(uint32_t index, EntityBufferData data)
{
	entityBuffers[index]->writeToBuffer(&data);
}

void Hmck::Renderer::updatePrimitiveBuffer(uint32_t index, PrimitiveBufferData data)
{
	primitiveBuffers[index]->writeToBuffer(&data);
}
