#include "VolumetricRenderingApp.h"

Hmck::VolumetricRenderingApp::VolumetricRenderingApp()
{
	descriptorPool = DescriptorPool::Builder(hmckDevice)
		.setPoolFlags(VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT)
		.setMaxSets(100)
		.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2000)
		.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 2000)
		.addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2000)
		.addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 2000)
		.addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2000)
		.build();

	load();

	descriptorSetLayout = DescriptorSetLayout::Builder(hmckDevice)
		.addBinding(cameraBinding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
		.addBinding(textureBinding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 
			VK_SHADER_STAGE_ALL_GRAPHICS, 2000, 
			VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT)
		.build();
}

void Hmck::VolumetricRenderingApp::run()
{
	std::vector<std::unique_ptr<Buffer>> uboBuffers{ SwapChain::MAX_FRAMES_IN_FLIGHT };
	for (int i = 0; i < uboBuffers.size(); i++)
	{
		uboBuffers[i] = std::make_unique<Buffer>(
			hmckDevice,
			sizeof(GlobalUbo),
			1,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
			);
		uboBuffers[i]->map();
	}

	std::vector<VkDescriptorSet> globalDescriptorSets(SwapChain::MAX_FRAMES_IN_FLIGHT);
	for (int i = 0; i < globalDescriptorSets.size(); i++)
	{
		std::vector<VkDescriptorImageInfo> imageInfos{};
		for (auto& material : scene->materials)
		{
			imageInfos.push_back(scene->images[scene->textures[material.baseColorTextureIndex].imageIndex].texture.descriptor);
			imageInfos.push_back(scene->images[scene->textures[material.normalTextureIndex].imageIndex].texture.descriptor);
			imageInfos.push_back(scene->images[scene->textures[material.metallicRoughnessTextureIndex].imageIndex].texture.descriptor);
			imageInfos.push_back(scene->images[scene->textures[material.occlusionTexture].imageIndex].texture.descriptor);
		}

		auto bufferInfo = uboBuffers[i]->descriptorInfo();
		
		DescriptorWriter(*descriptorSetLayout, *descriptorPool)
			.writeBuffer(cameraBinding, &bufferInfo)
			.writeImages(textureBinding, imageInfos)
			.build(globalDescriptorSets[i]);
	}

	// camera and movement
	Camera camera{};
	camera.setViewTarget({ 1.f, 1.f, -1.f }, { 0.f, 0.f, 0.f });
	auto viewerObject = Entity::createEntity();
	viewerObject->translate({ 1.f, 1.f, -5.f });
	scene->addChildOfRoot(viewerObject);
	KeyboardMovementController cameraController{};

	GraphicsPipeline standardPipeline = GraphicsPipeline::createGraphicsPipeline({
		.debugName = "standard_forward_pass",
		.device = hmckDevice,
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
.renderPass = hmckRenderer.getSwapChainRenderPass()
		});


	auto currentTime = std::chrono::high_resolution_clock::now();
	while (!hmckWindow.shouldClose())
	{
		hmckWindow.pollEvents();


		// gameloop timing
		auto newTime = std::chrono::high_resolution_clock::now();
		float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
		currentTime = newTime;

		// camera
		cameraController.moveInPlaneXZ(hmckWindow, frameTime, viewerObject);
		camera.setViewYXZ(viewerObject->translation(), viewerObject->rotation());
		float aspect = hmckRenderer.getAspectRatio();
		camera.setPerspectiveProjection(glm::radians(50.0f), aspect, 0.1f, 1000.f);

		// start a new frame
		if (auto commandBuffer = hmckRenderer.beginFrame())
		{
			int frameIndex = hmckRenderer.getFrameIndex();
			PerFrameData data{
				.projection = camera.getProjection(),
				.view = camera.getView(),
				.inverseView = camera.getInverseView()
			};
			uboBuffers[frameIndex]->writeToBuffer(&data);

			// on screen rendering
			hmckRenderer.beginSwapChainRenderPass(commandBuffer);

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
				nullptr
			);

			scene->draw(commandBuffer, standardPipeline.graphicsPipelineLayout);

			hmckRenderer.endRenderPass(commandBuffer);
			hmckRenderer.endFrame();
		}

		vkDeviceWaitIdle(hmckDevice.device());

		// destroy allocated stuff here
	}
}

void Hmck::VolumetricRenderingApp::load()
{
	Scene::SceneCreateInfo info = {
		.device = hmckDevice,
		.name = "Volumetric scene",
		.loadFiles = {
			{
				.filename = std::string(MODELS_DIR) + "helmet/helmet.glb"
			}
		}
	};
	scene = std::make_unique<Scene>(info);
}
