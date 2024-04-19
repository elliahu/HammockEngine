#include "PBRApp.h"

Hmck::PBRApp::PBRApp()
{
	load();
}

void Hmck::PBRApp::run()
{
	Renderer renderer{ window, device, scene};
	KeyboardMovementController cameraController{};
	UserInterface ui{ device, renderer.getSwapChainRenderPass(), window };

	init();
	createPipelines(renderer);


	scene->camera.setViewTarget({ 1.f, 1.f, -1.f }, { 0.f, 0.f, 0.f });
	auto viewerObject = std::make_shared<Entity>();
	viewerObject->transform.translation = { 0.f, 0.f, -2.f };
	viewerObject->name = "Viewer object";
	scene->addChildOfRoot(viewerObject);

	std::shared_ptr<OmniLight> light = std::make_shared<OmniLight>();
	light->transform.translation = { 1.f, 1.f, -1.f };
	light->name = "Point light";
	scene->addChildOfRoot(light);



	
	std::vector<VkDescriptorImageInfo> imageInfos{ scene->images.size() };
	for (int im = 0; im < scene->images.size(); im++)
	{
		imageInfos[im] = scene->images[im].texture.descriptor;
	}
	auto sceneBufferInfo = environmentBuffer->descriptorInfo();
	auto result = DescriptorWriter(*environmentDescriptorSetLayout, *descriptorPool)
		.writeBuffer(0, &sceneBufferInfo)
		.writeImageArray(1, imageInfos)
		.writeImage(2, &scene->skyboxTexture.descriptor)
		.build(environmentDescriptorSet);


	auto currentTime = std::chrono::high_resolution_clock::now();
	while (!window.shouldClose())
	{
		window.pollEvents();


		// gameloop timing
		auto newTime = std::chrono::high_resolution_clock::now();
		float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
		currentTime = newTime;

		// camera
		cameraController.moveInPlaneXZ(window, frameTime, viewerObject);
		scene->camera.setViewYXZ(viewerObject->transform.translation, viewerObject->transform.rotation);
		float aspect = renderer.getAspectRatio();
		scene->camera.setPerspectiveProjection(glm::radians(50.0f), aspect, 0.1f, 1000.f);

		// start a new frame
		if (auto commandBuffer = renderer.beginFrame())
		{
			int frameIndex = renderer.getFrameIndex();

			vkCmdSetDepthBias(
				commandBuffer,
				1.25f,
				0.0f,
				1.75f);


			renderer.beginRenderPass(gbufferFramebuffer, commandBuffer, {
					{.color = { 0.0f, 0.0f, 0.0f, 0.0f } },
					{.color = { 0.0f, 0.0f, 0.0f, 0.0f } },
					{.color = { 0.0f, 0.0f, 0.0f, 0.0f } },
					{.color = { 0.0f, 0.0f, 0.0f, 0.0f } },
					{.depthStencil = { 1.0f, 0 }} });

			// write env data
			EnvironmentBufferData envData{
				.omniLights = {{
					.position = scene->camera.getView() * glm::vec4(light->transform.translation, 1.0f),
					.color = glm::vec4(light->color, 1.0f)
				}},
				.numOmniLights = 1
			};
			environmentBuffer->writeToBuffer(&envData);
			// bind env
			vkCmdBindDescriptorSets(
				commandBuffer,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				skyboxPipeline->graphicsPipelineLayout,
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
				skyboxPipeline->graphicsPipelineLayout,
				1, 1,
				&frameDescriptorSets[frameIndex],
				0,
				nullptr);

			renderer.bindSkyboxVertexBuffer(commandBuffer);
			skyboxPipeline->bind(commandBuffer);
			vkCmdDrawIndexed(commandBuffer, 36, 1, 0, 0, 0);

			renderer.bindVertexBuffer(commandBuffer);

			gbufferPipeline->bind(commandBuffer);

			// Render all nodes at top-level off-screen
			renderEntity(frameIndex, commandBuffer, gbufferPipeline, scene->root);

			renderer.endRenderPass(commandBuffer);
			renderer.beginSwapChainRenderPass(commandBuffer);

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


			{
				ui.beginUserInterface();
				ui.showDebugStats(viewerObject);
				ui.showWindowControls();
				ui.showEntityInspector(scene->getRoot());
				ui.endUserInterface(commandBuffer);
			}

			renderer.endRenderPass(commandBuffer);
			renderer.endFrame();
		}

		vkDeviceWaitIdle(device.device());
	}
}

void Hmck::PBRApp::load()
{
	Scene::SceneCreateInfo info = {
		.device = device,
		.name = "Volumetric scene",
		.loadFiles = {
			{
				//.filename = std::string(MODELS_DIR) + "helmet/helmet.glb",
				.filename = std::string(MODELS_DIR) + "sponza/sponza.glb",
				//.filename = std::string(MODELS_DIR) + "SunTemple/SunTemple.glb",
				//.filename = std::string(MODELS_DIR) + "Bistro/BistroExterior.glb",
			},
			
		},
		.loadSkybox = {
			.textures = {
				"../../Resources/env/skybox/right.jpg",
				"../../Resources/env/skybox/left.jpg",
				"../../Resources/env/skybox/bottom.jpg",
				"../../Resources/env/skybox/top.jpg",
				"../../Resources/env/skybox/front.jpg",
				"../../Resources/env/skybox/back.jpg"
			}
		}
	};
	scene = std::make_unique<Scene>(info);
}

void Hmck::PBRApp::init()
{
	descriptorPool = DescriptorPool::Builder(device)
		.setMaxSets(10000)
		.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 5000)
		.addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 5000)
		.build();

	// prepare descriptor set layouts
	environmentDescriptorSetLayout = DescriptorSetLayout::Builder(device)
		.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
		.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_ALL_GRAPHICS, 2000, VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT)
		.addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_ALL_GRAPHICS)
		.build();

	frameDescriptorSetLayout = DescriptorSetLayout::Builder(device)
		.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
		.build();

	entityDescriptorSetLayout = DescriptorSetLayout::Builder(device)
		.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
		.build();

	materialDescriptorSetLayout = DescriptorSetLayout::Builder(device)
		.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
		.build();

	gbufferDescriptorSetLayout = DescriptorSetLayout::Builder(device)
		.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // position
		.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // albedo
		.addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // normal
		.addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // material (roughness, metalness, ao)
		.build();

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

	materialDescriptorSets.resize(scene->materials.size());
	materialBuffers.resize(scene->materials.size());
	for (size_t i = 0; i < materialDescriptorSets.size(); i++)
	{
		materialBuffers[i] = std::make_unique<Buffer>(
			device,
			sizeof(PrimitiveBufferData),
			1,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
			);
		materialBuffers[i]->map();

		auto pbufferInfo = materialBuffers[i]->descriptorInfo();
		DescriptorWriter(*materialDescriptorSetLayout, *descriptorPool)
			.writeBuffer(0, &pbufferInfo)
			.build(materialDescriptorSets[i]);
	}
}

void Hmck::PBRApp::renderEntity(uint32_t frameIndex, VkCommandBuffer commandBuffer, std::unique_ptr<GraphicsPipeline>& pipeline, std::shared_ptr<Entity>& entity)
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

					materialBuffers[primitive.materialIndex]->writeToBuffer(&pData);
				}

				// bind per primitive
				vkCmdBindDescriptorSets(
					commandBuffer,
					VK_PIPELINE_BIND_POINT_GRAPHICS,
					pipeline->graphicsPipelineLayout,
					3, 1,
					&materialDescriptorSets[(primitive.materialIndex >= 0 ? primitive.materialIndex : 0)],
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

void Hmck::PBRApp::createPipelines(Renderer& renderer)
{
	// create piplines and framebuffers
	auto depthFormat = device.findSupportedFormat(
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

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
			materialDescriptorSetLayout->getDescriptorSetLayout()
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
			.writeImageArray(0, gbufferImageInfos)
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
			.byteCode = Hmck::Filesystem::readFile("../../HammockEngine/Engine/Shaders/Compiled/phong.frag.spv"),
			.entryFunc = "main"
		},
		.descriptorSetLayouts = {
			environmentDescriptorSetLayout->getDescriptorSetLayout(),
			frameDescriptorSetLayout->getDescriptorSetLayout(),
			entityDescriptorSetLayout->getDescriptorSetLayout(),
			materialDescriptorSetLayout->getDescriptorSetLayout(),
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
		.renderPass = renderer.getSwapChainRenderPass()
	});
}
