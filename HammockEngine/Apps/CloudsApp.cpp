#include "CloudsApp.h"


Hmck::CloudsApp::CloudsApp()
{
	init();
	load();
}

void Hmck::CloudsApp::run()
{
	Renderer renderer{ window, device };

	pipeline = GraphicsPipeline::createGraphicsPipelinePtr({
			.debugName = "standard_forward_pass",
			.device = device,
			.VS
			{
				.byteCode = Hmck::Filesystem::readFile("../../HammockEngine/Engine/Shaders/Compiled/fullscreen.vert.spv"),
				.entryFunc = "main"
			},
			.FS
			{
				.byteCode = Hmck::Filesystem::readFile("../../HammockEngine/Engine/Shaders/Compiled/raymarch_pb.frag.spv"),
				.entryFunc = "main"
			},
			.descriptorSetLayouts =
			{
				memoryManager.getDescriptorSetLayout(descriptorSetLayout).getDescriptorSetLayout()
			},
			.pushConstantRanges {
				{
					.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
					.offset = 0,
					.size = sizeof(PushData)
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
			.renderPass = renderer.getSwapChainRenderPass()
		});

	// camera and movement
	scene->camera.setViewTarget({ 1.f, 1.f, -1.f }, { 0.f, 0.f, 0.f });
	auto viewerObject = std::make_shared<Entity>();
	viewerObject->transform.translation = { 0.f, 0.f, -5.f };
	viewerObject->name = "Viewer object";
	scene->add(viewerObject);


	KeyboardMovementController cameraController{};
	UserInterface ui{ device, renderer.getSwapChainRenderPass(), window };


	float elapsedTime = 0.f;
	auto currentTime = std::chrono::high_resolution_clock::now();
	while (!window.shouldClose())
	{
		window.pollEvents();


		// gameloop timing
		auto newTime = std::chrono::high_resolution_clock::now();
		float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
		currentTime = newTime;
		elapsedTime += frameTime;

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

			renderer.beginSwapChainRenderPass(commandBuffer);

			draw(frameIndex, elapsedTime, commandBuffer);


			{
				ui.beginUserInterface();
				this->ui();
				ui.showDebugStats(viewerObject);
				ui.showWindowControls();
				ui.showEntityInspector(scene);
				ui.endUserInterface(commandBuffer);
			}

			renderer.endRenderPass(commandBuffer);
			renderer.endFrame();
		}

		vkDeviceWaitIdle(device.device());
	}

	destroy();
}

void Hmck::CloudsApp::load()
{
	Scene::SceneCreateInfo info = {
		.device = device,
		.memory = memoryManager,
		.name = "Volumetric scene",
	};
	scene = std::make_unique<Scene>(info);

	GltfLoader::GltfLoaderCreateInfo gltfinfo{
		.device = device,
		.memory = memoryManager,
		.scene = scene
	};
	GltfLoader gltfloader{ gltfinfo };
	gltfloader.load(std::string(MODELS_DIR) + "Sphere/Sphere.glb");

	vertexBuffer = memoryManager.createVertexBuffer({
		.vertexSize = sizeof(scene->vertices[0]),
		.vertexCount = static_cast<uint32_t>(scene->vertices.size()),
		.data = (void*)scene->vertices.data() });

	indexBuffer = memoryManager.createIndexBuffer({
		.indexSize = sizeof(scene->indices[0]),
		.indexCount = static_cast<uint32_t>(scene->indices.size()),
		.data = (void*)scene->indices.data() });


	noiseTexture = memoryManager.createTexture2DFromFile({
		.filepath = "../../Resources/Noise/noise2.png",
		.format = VK_FORMAT_R8G8B8A8_UNORM
		});

	blueNoiseTexture = memoryManager.createTexture2DFromFile({
		.filepath = "../../Resources/Noise/blue_noise.jpg",
		.format = VK_FORMAT_R8G8B8A8_UNORM
		});

	for (int i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; i++)
	{
		auto fbufferInfo = memoryManager.getBuffer(uniformBuffers[i])->descriptorInfo();
		auto nimageInfo = memoryManager.getTexture2DDescriptorImageInfo(noiseTexture);
		auto bnimageInfo = memoryManager.getTexture2DDescriptorImageInfo(blueNoiseTexture);
		descriptorSets[i] = memoryManager.createDescriptorSet({
			.descriptorSetLayout = descriptorSetLayout,
			.bufferWrites = {{0,fbufferInfo}},
			.imageWrites = {{1,nimageInfo},{2,bnimageInfo}}
			});
	}
}

void Hmck::CloudsApp::init()
{
	descriptorSets.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
	uniformBuffers.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);

	descriptorSetLayout = memoryManager.createDescriptorSetLayout({
		.bindings = {
			{.binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS},
			{.binding = 1, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS},
			{.binding = 2, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS},
		}
		});

	for (int i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; i++)
		uniformBuffers[i] = memoryManager.createBuffer({
			.instanceSize = sizeof(BufferData),
			.instanceCount = 1,
			.usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			.memoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
			});
}

void Hmck::CloudsApp::draw(int frameIndex, float elapsedTime, VkCommandBuffer commandBuffer)
{
	memoryManager.bindVertexBuffer(vertexBuffer, indexBuffer, commandBuffer);

	pipeline->bind(commandBuffer);

	bufferData.projection = scene->camera.getProjection();
	bufferData.view = scene->camera.getView();
	bufferData.inverseView = scene->camera.getInverseView();
	
	memoryManager.getBuffer(uniformBuffers[frameIndex])->writeToBuffer(&bufferData);

	memoryManager.bindDescriptorSet(
		commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		pipeline->graphicsPipelineLayout,
		0, 1,
		descriptorSets[frameIndex],
		0,
		nullptr);


	pushData.elapsedTime = elapsedTime;

	vkCmdPushConstants(commandBuffer, pipeline->graphicsPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushData), &pushData);

	vkCmdDraw(commandBuffer, 3, 1, 0, 0);
}

void Hmck::CloudsApp::destroy()
{
	memoryManager.destroyTexture2D(noiseTexture);
	memoryManager.destroyTexture2D(blueNoiseTexture);

	for (auto& uniformBuffer : uniformBuffers)
		memoryManager.destroyBuffer(uniformBuffer);

	memoryManager.destroyDescriptorSetLayout(descriptorSetLayout);

	memoryManager.destroyBuffer(vertexBuffer);
	memoryManager.destroyBuffer(indexBuffer);
}

void Hmck::CloudsApp::ui()
{
	const ImGuiWindowFlags window_flags = ImGuiWindowFlags_AlwaysAutoResize;
	ImGui::Begin("Cloud editor", (bool*)false, ImGuiWindowFlags_AlwaysAutoResize);
	ImGui::Text("Edit cloud properties", window_flags);

	float sunPosition[4] = { bufferData.sunPosition.x,bufferData.sunPosition.y,bufferData.sunPosition.z,bufferData.sunPosition.w };
	ImGui::DragFloat4("Sun position", &sunPosition[0], 0.1f);
	bufferData.sunPosition = { sunPosition[0],sunPosition[1] ,sunPosition[2] ,sunPosition[3] };

	float sunColor[4] = { bufferData.sunColor.x,bufferData.sunColor.y,bufferData.sunColor.z,bufferData.sunColor.w };
	ImGui::ColorEdit4("Sun color", &sunColor[0]);
	bufferData.sunColor = { sunColor[0],sunColor[1] ,sunColor[2] ,sunColor[3] };

	float baseSkyColor[4] = { bufferData.baseSkyColor.x,bufferData.baseSkyColor.y,bufferData.baseSkyColor.z,bufferData.baseSkyColor.w };
	ImGui::ColorEdit4("Base sky color", &baseSkyColor[0]);
	bufferData.baseSkyColor = { baseSkyColor[0],baseSkyColor[1] ,baseSkyColor[2] ,baseSkyColor[3] };

	float gradientSkyColor[4] = { bufferData.gradientSkyColor.x,bufferData.gradientSkyColor.y,bufferData.gradientSkyColor.z,bufferData.gradientSkyColor.w };
	ImGui::ColorEdit4("Gradient sky color", &gradientSkyColor[0]);
	bufferData.gradientSkyColor = { gradientSkyColor[0],gradientSkyColor[1] ,gradientSkyColor[2] ,gradientSkyColor[3] };

	ImGui::DragFloat("Max steps", &pushData.maxSteps,1.0f, 0.001f);
	ImGui::DragFloat("Max steps light", &pushData.maxStepsLight, 1.0f, 0.001f);
	ImGui::DragFloat("March size", &pushData.marchSize,0.01f,0.001f);
	ImGui::DragFloat("Absorbtion", &pushData.absorbtionCoef,0.01f, 0.001f, 1.0f);
	ImGui::DragFloat("Scattering Anisotropy (g)", &pushData.scatteringAniso, 0.01f, 0.001f, 1.0f);

	ImGui::End();
}
