#include "VolumetricRenderingApp.h"

Hmck::VolumetricRenderingApp::VolumetricRenderingApp()
{
	load();
}

void Hmck::VolumetricRenderingApp::run()
{
	// camera and movement
	Renderer renderer{ window, device, scene };
	scene->camera.setViewTarget({ 1.f, 1.f, -1.f }, { 0.f, 0.f, 0.f });
	auto viewerObject = std::make_shared<Entity>();
	viewerObject->transform.translation = { 1.f, 1.f, -5.f };
	scene->addChildOfRoot(viewerObject);
	auto root = scene->getRoot();
	KeyboardMovementController cameraController{};

	UserInterface ui{ device, renderer.getSwapChainRenderPass(), window };


	renderer.writeSceneData(scene->images, {});


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

			// on screen rendering
			renderer.beginSwapChainRenderPass(commandBuffer);

			vkCmdSetDepthBias(
				commandBuffer,
				1.25f,
				0.0f,
				1.75f);


			// TODO make this bound once
			renderer.bindSceneData(commandBuffer);

		
			renderer.render(frameIndex, commandBuffer);


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
