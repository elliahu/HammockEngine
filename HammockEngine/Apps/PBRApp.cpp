#include "PBRApp.h"

Hmck::PBRApp::PBRApp()
{
	load();
}

Hmck::PBRApp::~PBRApp()
{
	for (int i = 0; i < sceneDescriptors.sceneBuffers.size(); i++)
		memoryManager.destroyBuffer(sceneDescriptors.sceneBuffers[i]);

	for (const auto& pair : entityDescriptors.entityBuffers)
		memoryManager.destroyBuffer(entityDescriptors.entityBuffers[pair.first]);

	for (int i = 0; i < primitiveDescriptors.primitiveBuffers.size(); i++)
		memoryManager.destroyBuffer(primitiveDescriptors.primitiveBuffers[i]);

	memoryManager.destroyDescriptorSetLayout(sceneDescriptors.descriptorSetLayout);
	memoryManager.destroyDescriptorSetLayout(entityDescriptors.descriptorSetLayout);
	memoryManager.destroyDescriptorSetLayout(primitiveDescriptors.descriptorSetLayout);
	memoryManager.destroyDescriptorSetLayout(gBufferDescriptors.descriptorSetLayout);

	memoryManager.destroyBuffer(geometry.vertexBuffer);
	memoryManager.destroyBuffer(geometry.indexBuffer);
}

void Hmck::PBRApp::run()
{
	Renderer renderer{ window, device };
	KeyboardMovementController cameraController{};
	UserInterface ui{ device, renderer.getSwapChainRenderPass(), window };

	auto camera = std::make_shared<Camera>();
	scene->add(camera, scene->getRoot());
	scene->setActiveCamera(camera->id);
	scene->getActiveCamera()->flipY = true;
	scene->getActiveCamera()->setPerspectiveProjection(glm::radians(50.f), renderer.getAspectRatio(), 0.1f, 512.0f);

	std::shared_ptr<OmniLight> light = std::make_shared<OmniLight>();
	light->transform.translation = { 0.0f, 2.0f, -8.0f };
	scene->add(light, scene->getRoot());

	init();
	createPipelines(renderer);

	SceneBufferData sceneData{
		.projection = scene->getActiveCamera()->getProjection(),
		.view = scene->getActiveCamera()->getView(),
		.inverseView = scene->getActiveCamera()->getInverseView()
	};

	auto currentTime = std::chrono::high_resolution_clock::now();
	while (!window.shouldClose())
	{
		window.pollEvents();


		// gameloop timing
		auto newTime = std::chrono::high_resolution_clock::now();
		float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
		currentTime = newTime;

		// camera
		cameraController.moveInPlaneXZ(window, frameTime, scene->getActiveCamera());
		scene->getActiveCamera()->update();


		// start a new frame
		if (auto commandBuffer = renderer.beginFrame())
		{
			int frameIndex = renderer.getFrameIndex();

			memoryManager.bindVertexBuffer(geometry.vertexBuffer, geometry.indexBuffer, commandBuffer);
			renderer.beginRenderPass(gbufferFramebuffer, commandBuffer, {
					{.color = { 0.0f, 0.0f, 0.0f, 0.0f } },
					{.color = { 0.0f, 0.0f, 0.0f, 0.0f } },
					{.color = { 0.0f, 0.0f, 0.0f, 0.0f } },
					{.color = { 0.0f, 0.0f, 0.0f, 0.0f } },
					{.depthStencil = { 1.0f, 0 }} });

			

			environmentSpherePipeline->bind(commandBuffer);


			/// Write scene data
			sceneData.projection = scene->getActiveCamera()->getProjection();
			sceneData.view = scene->getActiveCamera()->getView();
			sceneData.inverseView = scene->getActiveCamera()->getInverseView();
			uint32_t ldx = 0;
			for (int i = 0; i < scene->lights.size(); i++)
			{
				uint32_t lightId = scene->lights[i];
				auto l = cast<Entity, OmniLight>(scene->entities[lightId]);
				sceneData.omniLights[ldx] = { .position = scene->getActiveCamera()->getView() * glm::vec4(l->transform.translation,1.0f), .color = glm::vec4(l->color,1.0f) };
				ldx++;
			}
			sceneData.numOmniLights = ldx;
			memoryManager.getBuffer(sceneDescriptors.sceneBuffers[frameIndex])->writeToBuffer(&sceneData);
			memoryManager.bindDescriptorSet(
				commandBuffer,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				environmentSpherePipeline->graphicsPipelineLayout,
				sceneDescriptors.binding, 1,
				sceneDescriptors.descriptorSets[frameIndex],
				0, nullptr);


			
			vkCmdDraw(commandBuffer, 3, 1, 0, 0);

			gbufferPipeline->bind(commandBuffer);

			// Render all nodes at top-level off-screen
			renderEntity(frameIndex, commandBuffer, gbufferPipeline, scene->getRoot());

			renderer.endRenderPass(commandBuffer);
			renderer.beginSwapChainRenderPass(commandBuffer);

			compositionPipeline->bind(commandBuffer);

			memoryManager.bindDescriptorSet(
				commandBuffer,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				compositionPipeline->graphicsPipelineLayout,
				gBufferDescriptors.binding, 1,
				gBufferDescriptors.descriptorSets[frameIndex],
				0, nullptr);

			// draw deffered
			vkCmdDraw(commandBuffer, 3, 1, 0, 0);


			{
				ui.beginUserInterface();
				ui.showDebugStats(scene->getActiveCamera());
				ui.showWindowControls();
				ui.showEntityInspector(scene);
				ui.showColorSettings(&sceneData.exposure, &sceneData.gamma, &sceneData.whitePoint);
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
	scene = std::make_unique<Scene>(Scene::SceneCreateInfo{
		.device = device,
		.memory = memoryManager,
		.name = "Physically based rendering demo", });

	EnvironmentLoader loader{ device, memoryManager };
	//loader.loadHDR("../../Resources/env/ibl/precomp/lebombo/lebombo_prefiltered_map.hdr", scene->environment->environmentSphere, VK_FORMAT_R32G32B32A32_SFLOAT);
	//loader.loadHDR("../../Resources/env/ibl/room.hdr", scene->environment->environmentSphere, VK_FORMAT_R32G32B32A32_SFLOAT);
	loader.loadHDR("../../Resources/env/ibl/sunset.hdr", scene->environment->environmentSphere, VK_FORMAT_R32G32B32A32_SFLOAT);
	scene->environment->generatePrefilteredSphere(device, memoryManager);
	scene->environment->generateIrradianceSphere(device, memoryManager);
	scene->environment->generateBRDFLUT(device, memoryManager);

	GltfLoader gltfloader{ device, memoryManager, scene };
	//gltfloader.load(std::string(MODELS_DIR) + "sponza/sponza.glb");
	//gltfloader.load(std::string(MODELS_DIR) + "helmet/DamagedHelmet.glb");
	gltfloader.load(std::string(MODELS_DIR) + "helmet/helmet.glb");
	//gltfloader.load(std::string(MODELS_DIR) + "6887_allied_avenger/ship.glb");
	//gltfloader.load(std::string(MODELS_DIR) + "Bistro/BistroInterior.glb");
	//gltfloader.load(std::string(MODELS_DIR) + "Bistro/BistroExterior.glb");
	//gltfloader.load("../../Resources/sceneData/models/reflection_scene.gltf");


	geometry.vertexBuffer = memoryManager.createVertexBuffer({
		.vertexSize = sizeof(scene->vertices[0]),
		.vertexCount = static_cast<uint32_t>(scene->vertices.size()),
		.data = (void*)scene->vertices.data()});

	geometry.indexBuffer = memoryManager.createIndexBuffer({
		.indexSize = sizeof(scene->indices[0]),
		.indexCount = static_cast<uint32_t>(scene->indices.size()),
		.data = (void*)scene->indices.data()});

	geometry.numTriangles = static_cast<uint32_t>(scene->vertices.size()) / 3;

	Logger::log(HMCK_LOG_LEVEL_DEBUG, "Number of triangles: %d\n", geometry.numTriangles);

	scene->vertices.clear();
	scene->indices.clear();
}

void Hmck::PBRApp::init()
{
	std::vector<VkDescriptorImageInfo> imageInfos{ scene->images.size() };
	for (int im = 0; im < scene->images.size(); im++)
	{
		imageInfos[im] = memoryManager.getTexture2D(scene->images[im].texture)->descriptor;
	}

	sceneDescriptors.descriptorSets.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
	sceneDescriptors.sceneBuffers.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
	sceneDescriptors.descriptorSetLayout = memoryManager.createDescriptorSetLayout({
		.bindings = {
			{.binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS},
			{.binding = 1, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS, .count = 2000, .bindingFlags = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT}, // bindless textures
			{.binding = 2, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS}, // prefiltered env map
			{.binding = 3, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS}, //  brdfLUT
			{.binding = 4, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS}, //  irradiance map
		}
		});

	for (int i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; i++)
	{
		sceneDescriptors.sceneBuffers[i] = memoryManager.createBuffer({
			.instanceSize = sizeof(SceneBufferData),
			.instanceCount = 1,
			.usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			.memoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
			});

		auto fbufferInfo = memoryManager.getBuffer(sceneDescriptors.sceneBuffers[i])->descriptorInfo();
		sceneDescriptors.descriptorSets[i] = memoryManager.createDescriptorSet({
			.descriptorSetLayout = sceneDescriptors.descriptorSetLayout,
			.bufferWrites = {{0,fbufferInfo}},
			.imageWrites = {
				{2, memoryManager.getTexture2DDescriptorImageInfo(scene->environment->prefilteredSphere)},
				{3, memoryManager.getTexture2DDescriptorImageInfo(scene->environment->brdfLUT)},
				{4, memoryManager.getTexture2DDescriptorImageInfo(scene->environment->irradianceSphere)},
			},
			.imageArrayWrites = {{1,imageInfos}}
		});
	}

	entityDescriptors.descriptorSetLayout = memoryManager.createDescriptorSetLayout({
		.bindings = {
			{.binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS},
		}
		});

	for (const auto& ep : scene->entities)
	{
		entityDescriptors.entityBuffers[ep.first] = memoryManager.createBuffer({
			.instanceSize = sizeof(EntityBufferData),
			.instanceCount = 1,
			.usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			.memoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
			});
		auto ebufferInfo = memoryManager.getBuffer(entityDescriptors.entityBuffers[ep.first])->descriptorInfo();
		entityDescriptors.descriptorSets[ep.first] = memoryManager.createDescriptorSet({
			.descriptorSetLayout = entityDescriptors.descriptorSetLayout,
			.bufferWrites = {{0,ebufferInfo}},
			});
	}

	primitiveDescriptors.descriptorSets.resize(scene->materials.size());
	primitiveDescriptors.primitiveBuffers.resize(scene->materials.size());
	primitiveDescriptors.descriptorSetLayout = memoryManager.createDescriptorSetLayout({
		.bindings = {
			{.binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS},
		}
		});

	for (size_t i = 0; i < primitiveDescriptors.descriptorSets.size(); i++)
	{
		primitiveDescriptors.primitiveBuffers[i] = memoryManager.createBuffer({
			.instanceSize = sizeof(PrimitiveBufferData),
			.instanceCount = 1,
			.usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			.memoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
			});

		auto pbufferInfo = memoryManager.getBuffer(primitiveDescriptors.primitiveBuffers[i])->descriptorInfo();
		primitiveDescriptors.descriptorSets[i] = memoryManager.createDescriptorSet({
			.descriptorSetLayout = primitiveDescriptors.descriptorSetLayout,
			.bufferWrites = {{0,pbufferInfo}},
			});
	}

	gBufferDescriptors.descriptorSets.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
	gBufferDescriptors.descriptorSetLayout = memoryManager.createDescriptorSetLayout({
		.bindings = {
			{.binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT}, // position
			{.binding = 1, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT}, // albedo
			{.binding = 2, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT}, // normal
			{.binding = 3, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT}, // material (roughness, metalness, ao)
		}
		});
}

void Hmck::PBRApp::renderEntity(uint32_t frameIndex, VkCommandBuffer commandBuffer, std::unique_ptr<GraphicsPipeline>& pipeline, std::shared_ptr<Entity> entity)
{
	// don't render invisible nodes
	if (isInstanceOf<Entity, Entity3D>(entity) && entity->visible)
	{
		auto _entity = cast<Entity, Entity3D>(entity);

		if (_entity->mesh.primitives.size() > 0)
		{
			if (_entity->dataChanged)
			{
				EntityBufferData entityData{ .model = _entity->mat4(), .normal = _entity->mat4N() };
				memoryManager.getBuffer(entityDescriptors.entityBuffers[entity->id])->writeToBuffer(&entityData);
				_entity->dataChanged = false;
			}

			memoryManager.bindDescriptorSet(
				commandBuffer,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipeline->graphicsPipelineLayout,
				entityDescriptors.binding, 1,
				entityDescriptors.descriptorSets[entity->id],
				0, nullptr);

			for (Primitive& primitive : _entity->mesh.primitives)
			{
				if (primitive.indexCount > 0)
				{
					if (primitive.materialIndex >= 0)
					{
						Material& material = scene->materials[primitive.materialIndex];

						if (material.alphaMode == "BLEND") continue; // skip blend in this pass

						PrimitiveBufferData primitiveData{
							.baseColorFactor = material.baseColorFactor,
							.baseColorTextureIndex = (material.baseColorTextureIndex != TextureIndex::Invalid) ? scene->textures[material.baseColorTextureIndex].imageIndex : TextureIndex::Invalid,
							.normalTextureIndex = (material.normalTextureIndex != TextureIndex::Invalid) ? scene->textures[material.normalTextureIndex].imageIndex : TextureIndex::Invalid,
							.metallicRoughnessTextureIndex = (material.metallicRoughnessTextureIndex != TextureIndex::Invalid) ? scene->textures[material.metallicRoughnessTextureIndex].imageIndex : TextureIndex::Invalid,
							.occlusionTextureIndex = (material.occlusionTextureIndex != TextureIndex::Invalid) ? scene->textures[material.occlusionTextureIndex].imageIndex : TextureIndex::Invalid,
							.alphaCutoff = material.alphaCutOff,
							.metallicFactor = material.metallicFactor,
							.roughnessFactor = material.roughnessFactor
						};

						if (material.dataChanged)
						{
							memoryManager.getBuffer(primitiveDescriptors.primitiveBuffers[primitive.materialIndex])->writeToBuffer(&primitiveData);
							material.dataChanged = false;
						}

					}

					memoryManager.bindDescriptorSet(
						commandBuffer,
						VK_PIPELINE_BIND_POINT_GRAPHICS,
						pipeline->graphicsPipelineLayout,
						primitiveDescriptors.binding, 1,
						primitiveDescriptors.descriptorSets[(primitive.materialIndex >= 0 ? primitive.materialIndex : 0)],
						0, nullptr);

					// draw
					vkCmdDrawIndexed(commandBuffer, primitive.indexCount, 1, primitive.firstIndex, 0, 0);
				}
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

	environmentSpherePipeline = GraphicsPipeline::createGraphicsPipelinePtr({
		.debugName = "skybox_pass",
		.device = device,
		.VS {
			.byteCode = Hmck::Filesystem::readFile("../../HammockEngine/Engine/Shaders/Compiled/fullscreen.vert.spv"),
			.entryFunc = "main"
		},
		.FS {
			.byteCode = Hmck::Filesystem::readFile("../../HammockEngine/Engine/Shaders/Compiled/environment.frag.spv"),
			.entryFunc = "main"
		},
		.descriptorSetLayouts = {
			memoryManager.getDescriptorSetLayout(sceneDescriptors.descriptorSetLayout).getDescriptorSetLayout()
		},
		.pushConstantRanges {},
		.graphicsState {
			.depthTest = VK_FALSE,
			.depthTestCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
			.cullMode = VK_CULL_MODE_NONE,
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
			memoryManager.getDescriptorSetLayout(sceneDescriptors.descriptorSetLayout).getDescriptorSetLayout(),
			memoryManager.getDescriptorSetLayout(entityDescriptors.descriptorSetLayout).getDescriptorSetLayout(),
			memoryManager.getDescriptorSetLayout(primitiveDescriptors.descriptorSetLayout).getDescriptorSetLayout()
		},
		.pushConstantRanges {},
		.graphicsState {
			.depthTest = VK_TRUE,
			.depthTestCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
			.cullMode = VK_CULL_MODE_BACK_BIT,
			.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
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

	for (int i = 0; i < gBufferDescriptors.descriptorSets.size(); i++)
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

		gBufferDescriptors.descriptorSets[i] = memoryManager.createDescriptorSet({
			.descriptorSetLayout = gBufferDescriptors.descriptorSetLayout,
			.imageArrayWrites = {{0,gbufferImageInfos}},
			});
	}

	compositionPipeline = GraphicsPipeline::createGraphicsPipelinePtr({
		.debugName = "deferred_pass",
		.device = device,
		.VS {
			.byteCode = Hmck::Filesystem::readFile("../../HammockEngine/Engine/Shaders/Compiled/fullscreen.vert.spv"),
			.entryFunc = "main"
		},
		.FS {
			.byteCode = Hmck::Filesystem::readFile("../../HammockEngine/Engine/Shaders/Compiled/pbr_ibl_deferred.frag.spv"),
			.entryFunc = "main"
		},
		.descriptorSetLayouts = {
			memoryManager.getDescriptorSetLayout(sceneDescriptors.descriptorSetLayout).getDescriptorSetLayout(),
			memoryManager.getDescriptorSetLayout(entityDescriptors.descriptorSetLayout).getDescriptorSetLayout(),
			memoryManager.getDescriptorSetLayout(primitiveDescriptors.descriptorSetLayout).getDescriptorSetLayout(),
			memoryManager.getDescriptorSetLayout(gBufferDescriptors.descriptorSetLayout).getDescriptorSetLayout()
		},
		.pushConstantRanges {},
		.graphicsState {
			.depthTest = VK_TRUE,
			.depthTestCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
			.cullMode = VK_CULL_MODE_NONE,
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