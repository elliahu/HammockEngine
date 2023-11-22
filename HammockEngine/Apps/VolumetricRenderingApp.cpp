#include "VolumetricRenderingApp.h"

Hmck::VolumetricRenderingApp::VolumetricRenderingApp()
{
	load();
	//VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT
}

void Hmck::VolumetricRenderingApp::run()
{
	// camera and movement
	Camera camera{};
	camera.setViewTarget({ 1.f, 1.f, -1.f }, { 0.f, 0.f, 0.f });
	auto viewerObject = std::make_shared<Entity>(device, scene->descriptorPool);
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
				scene->descriptorSetLayout->getDescriptorSetLayout(),
				viewerObject->descriptorSetLayout->getDescriptorSetLayout(),
				scene->materials[0].descriptorSetLayout->getDescriptorSetLayout() // all materials have same layout for now
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

			

			// on screen rendering
			renderer.beginSwapChainRenderPass(commandBuffer);

			standardPipeline.bind(commandBuffer);

			vkCmdSetDepthBias(
				commandBuffer,
				1.25f,
				0.0f,
				1.75f);

		
			renderer.render(
				scene,
				commandBuffer,
				standardPipeline.graphicsPipelineLayout,
				// per frame
				[&camera, &frameIndex](std::unique_ptr<Scene>& scene, VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout)
				{
					Scene::SceneUbo sceneData{
						.projection = camera.getProjection(),
						.view = camera.getView(),
						.inverseView = camera.getInverseView()
						};
					scene->sceneBuffers[frameIndex]->writeToBuffer(&sceneData);

					vkCmdBindDescriptorSets(
						commandBuffer,
						VK_PIPELINE_BIND_POINT_GRAPHICS,
						pipelineLayout,
						0, 1,
						&scene->descriptorSets[frameIndex],
						0,
						nullptr);
				},
				// per entity
				[](std::shared_ptr<Entity3D> entity, VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout)
				{
					entity->updateBuffer();

					vkCmdBindDescriptorSets(
						commandBuffer,
						VK_PIPELINE_BIND_POINT_GRAPHICS,
						pipelineLayout,
						1, 1,
						&entity->descriptorSet,
						0,
						nullptr);
				},
				// per material
				[&](uint32_t materialIndex, VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout)
				{
					// TODO per material pipeline in the future
					scene->materials[materialIndex].updateBuffer();

					vkCmdBindDescriptorSets(
						commandBuffer,
						VK_PIPELINE_BIND_POINT_GRAPHICS,
						pipelineLayout,
						2, 1,
						&scene->materials[materialIndex].descriptorSet,
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
