#include "VolumetricRenderingApp.h"

Hmck::VolumetricRenderingApp::VolumetricRenderingApp()
{
	descriptorPool = DescriptorPool::Builder(device)
		.setPoolFlags(VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT)
		.setMaxSets(100)
		.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2000)
		.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 2000)
		.addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2000)
		.addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 2000)
		.addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2000)
		.build();

	load();

	descriptorSetLayout = DescriptorSetLayout::Builder(device)
		.addBinding(sceneBinding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
		.addBinding(textureBinding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 
			VK_SHADER_STAGE_ALL_GRAPHICS, 2000, 
			VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT)
		.addBinding(transformBinding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
		.addBinding(materialPropertyBinding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,VK_SHADER_STAGE_ALL_GRAPHICS)
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
	std::vector<VkDescriptorSet> globalDescriptorSets{ SwapChain::MAX_FRAMES_IN_FLIGHT };
	for (int i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; i++)
	{
		std::vector<VkDescriptorImageInfo> imageInfos{ scene->images.size() };
		for (int im = 0; im < scene->images.size(); im ++)
		{
			imageInfos[im] = scene->images[im].texture.descriptor;
		}

		auto sceneBufferInfo = sceneBuffers[i]->descriptorInfo();
		auto transformBufferInfo = transformBuffers[i]->descriptorInfo();
		auto materialPropertyBufferInfo = materialPropertyBuffers[i]->descriptorInfo();
		
		DescriptorWriter(*descriptorSetLayout, *descriptorPool)
			.writeBuffer(sceneBinding, &sceneBufferInfo)
			.writeImages(textureBinding, imageInfos)
			.writeBuffer(transformBinding, &transformBufferInfo)
			.writeBuffer(materialPropertyBinding, &materialPropertyBufferInfo)
			.build(globalDescriptorSets[i]);
	}

	// camera and movement
	Camera camera{};
	camera.setViewTarget({ 1.f, 1.f, -1.f }, { 0.f, 0.f, 0.f });
	auto viewerObject = Entity::createEntity();
	viewerObject->translate({ 1.f, 1.f, -5.f });
	scene->addChildOfRoot(viewerObject);
	auto root = scene->root();
	KeyboardMovementController cameraController{};

	UserInterface ui{device, renderer.getSwapChainRenderPass(), window};

	GraphicsPipeline standardPipeline = GraphicsPipeline::createGraphicsPipeline({
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
			descriptorSetLayout->getDescriptorSetLayout()
		},
		.pushConstantRanges {
			{
				.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				.offset = 0,
				.size = sizeof(Entity::TransformPushConstantData)
			},
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
			// order is location, binding, format, offset
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
		camera.setViewYXZ(viewerObject->translation(), viewerObject->rotation());
		float aspect = renderer.getAspectRatio();
		camera.setPerspectiveProjection(glm::radians(50.0f), aspect, 0.1f, 1000.f);

		// start a new frame
		if (auto commandBuffer = renderer.beginFrame())
		{
			int frameIndex = renderer.getFrameIndex();

			// scene data
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
				transformBuffers[frameIndex],
				materialPropertyBuffers[frameIndex]);

			{
				ui.beginUserInterface();
				ui.showDebugStats(viewerObject);
				ui.showWindowControls();
				ui.showEntityInspector(scene->root()); // TODO crashes on rotation
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
	Scene::SceneCreateInfo info = {
		.device = device,
		.name = "Volumetric scene",
		.loadFiles = {
			{
				.filename = std::string(MODELS_DIR) + "helmet/helmet.glb"
			}
		}
	};
	scene = std::make_unique<Scene>(info);
}
