#include "PBRApp.h"

Hmck::PBRApp::PBRApp()
{
	load();
}

Hmck::PBRApp::~PBRApp()
{
	clean();
}

void Hmck::PBRApp::run()
{
	Renderer renderer{ window, device };
	KeyboardMovementController cameraController{};
	UserInterface ui{ device, renderer.getSwapChainRenderPass(), window };

	init();
	createPipelines(renderer);

	auto camera = std::make_shared<Camera>();
	scene->add(camera);
	scene->setActiveCamera(camera->id);
	scene->getActiveCamera()->setPerspectiveProjection(glm::radians(50.f), renderer.getAspectRatio(), 0.1f, 1000.f);

	std::shared_ptr<OmniLight> light = std::make_shared<OmniLight>();
	light->transform.translation = { 0.0f, 2.0f, -2.0f };
	//scene->add(light);

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

			vkCmdSetDepthBias(commandBuffer, 1.25f, 0.0f, 1.75f);

			renderer.beginRenderPass(gbufferFramebuffer, commandBuffer, {
					{.color = { 0.0f, 0.0f, 0.0f, 0.0f } },
					{.color = { 0.0f, 0.0f, 0.0f, 0.0f } },
					{.color = { 0.0f, 0.0f, 0.0f, 0.0f } },
					{.color = { 0.0f, 0.0f, 0.0f, 0.0f } },
					{.depthStencil = { 1.0f, 0 }} });

			// write env data
			EnvironmentBufferData envData{};
			uint32_t ldx = 0;
			for (int i = 0; i < scene->entities.size(); i++)
			{
				if (isInstanceOf<Entity, OmniLight>(scene->entities[i]))
				{
					auto l = cast<Entity, OmniLight>(scene->entities[i]);
					envData.omniLights[ldx] = { .position = scene->getActiveCamera()->getView() * glm::vec4(l->transform.translation,1.0f), .color = glm::vec4(l->color,1.0f) };
					ldx++;
				}
			}
			envData.numOmniLights = ldx;
			memoryManager.getBuffer(environmentBuffer)->writeToBuffer(&envData);

			memoryManager.bindDescriptorSet(
				commandBuffer,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				skyboxPipeline->graphicsPipelineLayout,
				0, 1,
				environmentDescriptorSet,
				0, nullptr);


			FrameBufferData data{
				.projection = scene->getActiveCamera()->getProjection(),
				.view = scene->getActiveCamera()->getView(),
				.inverseView = scene->getActiveCamera()->getInverseView()
			};
			memoryManager.getBuffer(frameBuffers[frameIndex])->writeToBuffer(&data);

			memoryManager.bindDescriptorSet(
				commandBuffer,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				skyboxPipeline->graphicsPipelineLayout,
				1, 1,
				frameDescriptorSets[frameIndex],
				0, nullptr);

			memoryManager.bindVertexBuffer(vertexBuffer, indexBuffer, commandBuffer);

			skyboxPipeline->bind(commandBuffer);
			vkCmdDraw(commandBuffer, 3, 1, 0, 0);

			gbufferPipeline->bind(commandBuffer);

			// Render all nodes at top-level off-screen
			auto root = scene->getRoot();
			renderEntity(frameIndex, commandBuffer, gbufferPipeline, root);

			renderer.endRenderPass(commandBuffer);
			renderer.beginSwapChainRenderPass(commandBuffer);

			defferedPipeline->bind(commandBuffer);

			memoryManager.bindDescriptorSet(
				commandBuffer,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				defferedPipeline->graphicsPipelineLayout,
				4, 1,
				gbufferDescriptorSets[frameIndex],
				0, nullptr);

			// draw deffered
			vkCmdDraw(commandBuffer, 3, 1, 0, 0);


			{
				ui.beginUserInterface();
				ui.showDebugStats(scene->getActiveCamera());
				ui.showWindowControls();
				ui.showEntityInspector(scene);
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
		.name = "Physically based rendering demo",});

	EnvironmentLoader loader{ device, memoryManager};
	loader.loadHDR("../../Resources/env/ibl/precomp/lebombo_prefiltered_map.hdr", scene->environment->prefilteredMap, VK_FORMAT_R32G32B32A32_SFLOAT);
	loader.loadHDR("../../Resources/env/ibl/precomp/lebombo_irradiance_map.hdr", scene->environment->irradianceMap, VK_FORMAT_R32G32B32A32_SFLOAT);
	loader.loadHDR("../../Resources/env/ibl/precomp/brdf_integration_map_ct_ggx.hdr", scene->environment->brdfLUT, VK_FORMAT_R32G32B32A32_SFLOAT);

	GltfLoader gltfloader{ device, memoryManager, scene };
	//gltfloader.load(std::string(MODELS_DIR) + "sponza/sponza_lights.glb");
	gltfloader.load(std::string(MODELS_DIR) + "helmet/DamagedHelmet.glb");
	//gltfloader.load(std::string(MODELS_DIR) + "helmet/helmet.glb");
	

	vertexBuffer = memoryManager.createVertexBuffer({
		.vertexSize = sizeof(scene->vertices[0]),
		.vertexCount = static_cast<uint32_t>(scene->vertices.size()),
		.data = (void*)scene->vertices.data() });

	indexBuffer = memoryManager.createIndexBuffer({
		.indexSize = sizeof(scene->indices[0]),
		.indexCount = static_cast<uint32_t>(scene->indices.size()),
		.data = (void*)scene->indices.data() });
}

void Hmck::PBRApp::init()
{
	environmentDescriptorSetLayout = memoryManager.createDescriptorSetLayout({
		.bindings = {
			{.binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS},
			{.binding = 1, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS, .count = 200, .bindingFlags = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT}, // bindless textures
			{.binding = 2, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS}, // prefiltered env map
			{.binding = 3, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS}, //  brdfLUT
			{.binding = 4, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS}, //  irradiance map
		}
		});

	environmentBuffer = memoryManager.createBuffer({
			.instanceSize = sizeof(EnvironmentBufferData),
			.instanceCount = 1,
			.usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			.memoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		});

	std::vector<VkDescriptorImageInfo> imageInfos{ scene->images.size() };
	for (int im = 0; im < scene->images.size(); im++)
	{
		imageInfos[im] = memoryManager.getTexture2D(scene->images[im].texture)->descriptor;
	}
	auto sceneBufferInfo = memoryManager.getBuffer(environmentBuffer)->descriptorInfo();
	environmentDescriptorSet = memoryManager.createDescriptorSet({
			.descriptorSetLayout = environmentDescriptorSetLayout,
			.bufferWrites = {{0,sceneBufferInfo}},
			.imageWrites = {
				{2, memoryManager.getTexture2DDescriptorImageInfo(scene->environment->prefilteredMap)},
				{3, memoryManager.getTexture2DDescriptorImageInfo(scene->environment->brdfLUT)},
				{4, memoryManager.getTexture2DDescriptorImageInfo(scene->environment->irradianceMap)},
			},
			.imageArrayWrites = {{1,imageInfos}}
		});

	frameDescriptorSets.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
	frameBuffers.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
	frameDescriptorSetLayout = memoryManager.createDescriptorSetLayout({
		.bindings = {
			{.binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS},
		}
		});

	for (int i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; i++)
	{
		frameBuffers[i] = memoryManager.createBuffer({
			.instanceSize = sizeof(FrameBufferData),
			.instanceCount = 1,
			.usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			.memoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		});

		auto fbufferInfo = memoryManager.getBuffer(frameBuffers[i])->descriptorInfo();
		frameDescriptorSets[i] = memoryManager.createDescriptorSet({
			.descriptorSetLayout = frameDescriptorSetLayout,
			.bufferWrites = {{0,fbufferInfo}},
			});
	}

	entityDescriptorSetLayout = memoryManager.createDescriptorSetLayout({
		.bindings = {
			{.binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS},
		}
		});

	for (const auto& ep : scene->entities)
	{
		entityBuffers[ep.first] = memoryManager.createBuffer({
			.instanceSize = sizeof(EntityBufferData),
			.instanceCount = 1,
			.usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			.memoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
			});
		auto ebufferInfo = memoryManager.getBuffer(entityBuffers[ep.first])->descriptorInfo();
		entityDescriptorSets[ep.first] = memoryManager.createDescriptorSet({
			.descriptorSetLayout = entityDescriptorSetLayout,
			.bufferWrites = {{0,ebufferInfo}},
			});
	}

	materialDescriptorSets.resize(scene->materials.size());
	materialBuffers.resize(scene->materials.size());
	materialDescriptorSetLayout = memoryManager.createDescriptorSetLayout({
		.bindings = {
			{.binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS},
		}
		});

	for (size_t i = 0; i < materialDescriptorSets.size(); i++)
	{
		materialBuffers[i] = memoryManager.createBuffer({
			.instanceSize = sizeof(PrimitiveBufferData),
			.instanceCount = 1,
			.usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			.memoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
			});

		auto pbufferInfo = memoryManager.getBuffer(materialBuffers[i])->descriptorInfo();
		materialDescriptorSets[i] = memoryManager.createDescriptorSet({
			.descriptorSetLayout = materialDescriptorSetLayout,
			.bufferWrites = {{0,pbufferInfo}},
			});
	}

	gbufferDescriptorSets.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
	gbufferDescriptorSetLayout = memoryManager.createDescriptorSetLayout({
		.bindings = {
			{.binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT}, // position
			{.binding = 1, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT}, // albedo
			{.binding = 2, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT}, // normal
			{.binding = 3, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT}, // material (roughness, metalness, ao)
		}
		});
	
}

void Hmck::PBRApp::renderEntity(uint32_t frameIndex, VkCommandBuffer commandBuffer, std::unique_ptr<GraphicsPipeline>& pipeline, std::shared_ptr<Entity>& entity)
{
	// don't render invisible nodes
	if (isInstanceOf<Entity, Entity3D>(entity) && entity->visible)
	{
		auto _entity = cast<Entity, Entity3D>(entity);

		if (_entity->mesh.primitives.size() > 0) 
		{
			glm::mat4 model = entity->transform.mat4();
			std::shared_ptr<Entity> currentParent = scene->getEntity(entity->parent);
			while (currentParent) 
			{
				model = currentParent->transform.mat4() * model;
				currentParent = scene->getEntity(currentParent->parent);
			}

			EntityBufferData entityData{
				.model = model,
				.normal = glm::transpose(glm::inverse(model))
			};
			memoryManager.getBuffer(entityBuffers[entity->id])->writeToBuffer(&entityData);

			memoryManager.bindDescriptorSet(
				commandBuffer,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipeline->graphicsPipelineLayout,
				2, 1,
				entityDescriptorSets[entity->id],
				0, nullptr);
			
			for (Primitive& primitive : _entity->mesh.primitives) 
			{
				if (primitive.indexCount > 0) 
				{

					if (primitive.materialIndex >= 0)
					{
						Material& material = scene->materials[primitive.materialIndex];

						PrimitiveBufferData pData{
						.baseColorFactor = material.baseColorFactor,
						.baseColorTextureIndex = (material.baseColorTextureIndex != TextureIndex::Invalid) ? scene->textures[material.baseColorTextureIndex].imageIndex : TextureIndex::Invalid,
						.normalTextureIndex = (material.normalTextureIndex != TextureIndex::Invalid) ? scene->textures[material.normalTextureIndex].imageIndex : TextureIndex::Invalid,
						.metallicRoughnessTextureIndex = (material.metallicRoughnessTextureIndex != TextureIndex::Invalid) ? scene->textures[material.metallicRoughnessTextureIndex].imageIndex : TextureIndex::Invalid,
						.occlusionTextureIndex = (material.occlusionTextureIndex != TextureIndex::Invalid) ? scene->textures[material.occlusionTextureIndex].imageIndex : TextureIndex::Invalid,
						.alphaCutoff = material.alphaCutOff
						};

						memoryManager.getBuffer(materialBuffers[primitive.materialIndex])->writeToBuffer(&pData);
					}

					memoryManager.bindDescriptorSet(
						commandBuffer,
						VK_PIPELINE_BIND_POINT_GRAPHICS,
						pipeline->graphicsPipelineLayout,
						3, 1,
						materialDescriptorSets[(primitive.materialIndex >= 0 ? primitive.materialIndex : 0)],
						0, nullptr);

					// draw
					vkCmdDrawIndexed(commandBuffer, primitive.indexCount, 1, primitive.firstIndex, 0, 0);
				}
			}
		}	
	}

	for (auto& child : entity->children) {
		auto c = scene->getEntity(child);
		renderEntity(frameIndex, commandBuffer, pipeline, c);
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
			.byteCode = Hmck::Filesystem::readFile("../../HammockEngine/Engine/Shaders/Compiled/fullscreen.vert.spv"),
			.entryFunc = "main"
		},
		.FS {
			.byteCode = Hmck::Filesystem::readFile("../../HammockEngine/Engine/Shaders/Compiled/environment.frag.spv"),
			.entryFunc = "main"
		},
		.descriptorSetLayouts = {
			memoryManager.getDescriptorSetLayout(environmentDescriptorSetLayout).getDescriptorSetLayout(),
			memoryManager.getDescriptorSetLayout(frameDescriptorSetLayout).getDescriptorSetLayout()
		},
		.pushConstantRanges {},
		.graphicsState {
			.depthTest = VK_FALSE,
			.depthTestCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
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
			memoryManager.getDescriptorSetLayout(environmentDescriptorSetLayout).getDescriptorSetLayout(),
			memoryManager.getDescriptorSetLayout(frameDescriptorSetLayout).getDescriptorSetLayout(),
			memoryManager.getDescriptorSetLayout(entityDescriptorSetLayout).getDescriptorSetLayout(),
			memoryManager.getDescriptorSetLayout(materialDescriptorSetLayout).getDescriptorSetLayout()
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

		gbufferDescriptorSets[i] = memoryManager.createDescriptorSet({
			.descriptorSetLayout = gbufferDescriptorSetLayout,
			.imageArrayWrites = {{0,gbufferImageInfos}},
			});
	}

	defferedPipeline = GraphicsPipeline::createGraphicsPipelinePtr({
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
			memoryManager.getDescriptorSetLayout(environmentDescriptorSetLayout).getDescriptorSetLayout(),
			memoryManager.getDescriptorSetLayout(frameDescriptorSetLayout).getDescriptorSetLayout(),
			memoryManager.getDescriptorSetLayout(entityDescriptorSetLayout).getDescriptorSetLayout(),
			memoryManager.getDescriptorSetLayout(materialDescriptorSetLayout).getDescriptorSetLayout(),
			memoryManager.getDescriptorSetLayout(gbufferDescriptorSetLayout).getDescriptorSetLayout()
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

void Hmck::PBRApp::clean()
{
	memoryManager.destroyBuffer(environmentBuffer);

	for (int i = 0; i < frameBuffers.size(); i++)
		memoryManager.destroyBuffer(frameBuffers[i]);

	for (const auto& pair : entityBuffers) 
		memoryManager.destroyBuffer(entityBuffers[pair.first]);

	for (int i = 0; i < materialBuffers.size(); i++)
		memoryManager.destroyBuffer(materialBuffers[i]);

	memoryManager.destroyDescriptorSetLayout(environmentDescriptorSetLayout);
	memoryManager.destroyDescriptorSetLayout(frameDescriptorSetLayout);
	memoryManager.destroyDescriptorSetLayout(entityDescriptorSetLayout);
	memoryManager.destroyDescriptorSetLayout(materialDescriptorSetLayout);
	memoryManager.destroyDescriptorSetLayout(gbufferDescriptorSetLayout);

	memoryManager.destroyBuffer(vertexBuffer);
	memoryManager.destroyBuffer(indexBuffer);
}
