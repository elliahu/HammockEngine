#include "VolumetricRenderingApp.h"

Hmck::VolumetricRenderingApp::VolumetricRenderingApp()
{
	descriptorPool = DescriptorPool::Builder(device)
		//.setPoolFlags(VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT)
		.setMaxSets(100)
		.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2000)
		.addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2000)
		.build();

	load();
	//VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT
	globalDescriptorSetLayout = DescriptorSetLayout::Builder(device)
		.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
		.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_ALL_GRAPHICS, 200 )
		.build();
	
	transformDescriptorSetLayout = DescriptorSetLayout::Builder(device)
		.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
		.build();

	materialDescriptorSetLayout = DescriptorSetLayout::Builder(device)
		.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
		.build();
}

void Hmck::VolumetricRenderingApp::run()
{
	// Prepare buffers
	std::vector<std::unique_ptr<Buffer>> sceneBuffers{ SwapChain::MAX_FRAMES_IN_FLIGHT };
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

	std::vector<std::unique_ptr<Buffer>> transformBuffers{ SwapChain::MAX_FRAMES_IN_FLIGHT };
	for (int i = 0; i < transformBuffers.size(); i++)
	{
		transformBuffers[i] = std::make_unique<Buffer>(
			device,
			sizeof(TransformUbo),
			1,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
			);
		transformBuffers[i]->map();
	}

	std::vector<std::unique_ptr<Buffer>> materialPropertyBuffers{ SwapChain::MAX_FRAMES_IN_FLIGHT };
	for (int i = 0; i < materialPropertyBuffers.size(); i++)
	{
		materialPropertyBuffers[i] = std::make_unique<Buffer>(
			device,
			sizeof(MaterialPropertyUbo),
			1,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
			);
		materialPropertyBuffers[i]->map();
	}
	

	// write to buffers
	std::vector<VkDescriptorImageInfo> imageInfos{ scene->images.size() };
	for (int im = 0; im < scene->images.size(); im++)
	{
		imageInfos[im] = scene->images[im].texture.descriptor;
	}
	std::vector<VkDescriptorSet> globalDescriptorSets{ SwapChain::MAX_FRAMES_IN_FLIGHT };
	for (int i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; i++)
	{
		auto sceneBufferInfo = sceneBuffers[i]->descriptorInfo();

		DescriptorWriter(*globalDescriptorSetLayout, *descriptorPool)
			.writeBuffer(sceneBinding, &sceneBufferInfo)
			.writeImages(textureBinding, imageInfos)
			.build(globalDescriptorSets[i]);
	}

	std::vector<VkDescriptorSet> transformDescriptorSets{ SwapChain::MAX_FRAMES_IN_FLIGHT };
	for (int i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; i++)
	{
		auto transformBufferInfo = transformBuffers[i]->descriptorInfo();

		DescriptorWriter(*transformDescriptorSetLayout, *descriptorPool)
			.writeBuffer(0, &transformBufferInfo)
			.build(transformDescriptorSets[i]);
	}

	std::vector<VkDescriptorSet> materialDescriptorSets{ SwapChain::MAX_FRAMES_IN_FLIGHT };
	for (int i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; i++)
	{
		auto materialPropertyBufferInfo = materialPropertyBuffers[i]->descriptorInfo();

		DescriptorWriter(*materialDescriptorSetLayout, *descriptorPool)
			.writeBuffer(0, &materialPropertyBufferInfo)
			.build(materialDescriptorSets[i]);
	}


	// camera and movement
	Camera camera{};
	camera.setViewTarget({ 1.f, 1.f, -1.f }, { 0.f, 0.f, 0.f });
	auto viewerObject = Entity::createEntity();
	viewerObject->transform.translation = { 1.f, 1.f, -5.f };
	scene->addChildOfRoot(viewerObject);
	auto root = scene->root();
	KeyboardMovementController cameraController{};

	UserInterface ui{ device, renderer.getSwapChainRenderPass(), window };

	GraphicsPipeline standardPipeline = GraphicsPipeline::createGraphicsPipeline(
		{
			.debugName = "standard_forward_pass",
			.device = device,
			.VS {
				.byteCode = Hmck::Filesystem::readFile("../../HammockEngine/Engine/Shaders/Compiled/volumetric.vert.spv"),
				.entryFunc = "main"
			},
			.FS {
				.byteCode = Hmck::Filesystem::readFile("../../HammockEngine/Engine/Shaders/Compiled/volumetric.frag.spv"),
				.entryFunc = "main"
			},
			.descriptorSetLayouts = {
				globalDescriptorSetLayout->getDescriptorSetLayout(),
				transformDescriptorSetLayout->getDescriptorSetLayout(),
				materialDescriptorSetLayout->getDescriptorSetLayout()
			},
			.pushConstantRanges {
			},
		.graphicsState {
			.depthTest = VK_TRUE,
			.depthTestCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
			.blendAtaAttachmentStates {},
			.vertexBufferBindings {
				.vertexBindingDescriptions = {
					{
						.binding = 0,
						.stride = sizeof(Vertex),
						.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
					}
				},
				.vertexAttributeDescriptions = {
		{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)},
		{1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color)},
		{2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)},
		{3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)},
		{4, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, tangent)}
	}
}
},
.renderPass = renderer.getSwapChainRenderPass()
		}
	);



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
		camera.setViewYXZ(viewerObject->transform.translation, viewerObject->transform.rotation);
		float aspect = renderer.getAspectRatio();
		camera.setPerspectiveProjection(glm::radians(50.0f), aspect, 0.1f, 1000.f);

		// start a new frame
		if (auto commandBuffer = renderer.beginFrame())
		{
			int frameIndex = renderer.getFrameIndex();

			// scene data is written to once per frame
			SceneUbo sceneData{
				.projection = camera.getProjection(),
				.view = camera.getView(),
				.inverseView = camera.getInverseView()
			};
			sceneBuffers[frameIndex]->writeToBuffer(&sceneData);

			// on screen rendering
			renderer.beginSwapChainRenderPass(commandBuffer);

			standardPipeline.bind(commandBuffer);

			vkCmdSetDepthBias(
				commandBuffer,
				1.25f,
				0.0f,
				1.75f);

			vkCmdBindDescriptorSets(
				commandBuffer,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				standardPipeline.graphicsPipelineLayout,
				0, 1,
				&globalDescriptorSets[frameIndex],
				0,
				nullptr);

			// TODO FIXME wrong texture being used
			renderer.render(
				scene,
				commandBuffer,
				standardPipeline.graphicsPipelineLayout,
				[&](std::shared_ptr<Entity3D> entity)
				{
					TransformUbo transformData{
						.model = entity->transform.mat4(),
						.normal = entity->transform.normalMatrix()
					};
					transformBuffers[frameIndex]->writeToBuffer(&transformData);

					vkCmdBindDescriptorSets(
						commandBuffer,
						VK_PIPELINE_BIND_POINT_GRAPHICS,
						standardPipeline.graphicsPipelineLayout,
						1, 1,
						&transformDescriptorSets[frameIndex],
						0,
						nullptr);
				},
				[&](uint32_t materialIndex)
				{
					Material& material = this->scene->materials[materialIndex];

					MaterialPropertyUbo materialData{
						.baseColorFactor = material.baseColorFactor,
						.baseColorTextureIndex =  (material.baseColorTextureIndex != TextureHandle::Invalid) ? this->scene->textures[material.baseColorTextureIndex].imageIndex: TextureHandle::Invalid,
						.normalTextureIndex = (material.normalTextureIndex != TextureHandle::Invalid) ? this->scene->textures[material.normalTextureIndex].imageIndex : TextureHandle::Invalid,
						.metallicRoughnessTextureIndex = (material.metallicRoughnessTextureIndex != TextureHandle::Invalid) ? this->scene->textures[material.metallicRoughnessTextureIndex].imageIndex : TextureHandle::Invalid,
						.occlusionTextureIndex = (material.occlusionTextureIndex != TextureHandle::Invalid) ? this->scene->textures[material.occlusionTextureIndex].imageIndex : TextureHandle::Invalid,
						.alphaCutoff = material.alphaCutOff
					};

					materialPropertyBuffers[frameIndex]->writeToBuffer(&materialData);

					vkCmdBindDescriptorSets(
						commandBuffer,
						VK_PIPELINE_BIND_POINT_GRAPHICS,
						standardPipeline.graphicsPipelineLayout,
						2, 1,
						&materialDescriptorSets[frameIndex],
						0,
						nullptr);
				});


			{
				ui.beginUserInterface();
				ui.showDebugStats(viewerObject);
				ui.showWindowControls();
				ui.showEntityInspector(scene->root()); 
				ui.endUserInterface(commandBuffer);
			}

			renderer.endRenderPass(commandBuffer);
			renderer.endFrame();
		}

		vkDeviceWaitIdle(device.device());

		// destroy allocated stuff here
	}
}

void Hmck::VolumetricRenderingApp::load()
{
	// TODO FIXME anything else than helmet is not visible
	Scene::SceneCreateInfo info = {
		.device = device,
		.name = "Volumetric scene",
		.loadFiles = {
			{
				.filename = std::string(MODELS_DIR) + "helmet/helmet.glb",
			}
		}
	};
	scene = std::make_unique<Scene>(info);
}
