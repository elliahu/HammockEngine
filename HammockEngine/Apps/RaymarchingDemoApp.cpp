#include "RaymarchingDemoApp.h"

Hmck::RaymarchingDemoApp::RaymarchingDemoApp()
{
	load();
}

void Hmck::RaymarchingDemoApp::run()
{
	// camera and movement
	Renderer renderer{ window, device, scene };

	scene->camera.setViewTarget({ 1.f, 1.f, -1.f }, { 0.f, 0.f, 0.f });
	auto viewerObject = std::make_shared<Entity>();
	viewerObject->transform.translation = { 0.f, 0.f, -5.f };
	viewerObject->name = "Viewer object";
	scene->addChildOfRoot(viewerObject);

	std::shared_ptr<OmniLight> light = std::make_shared<OmniLight>();
	light->transform.translation = { 0.f, 10.f, 0.f };
	light->name = "Point light";
	scene->addChildOfRoot(light);


	KeyboardMovementController cameraController{};

	UserInterface ui{ device, renderer.getSwapChainRenderPass(), window };


	renderer.writeEnvironmentData(scene->images, {
		.omniLights = {{
			.position = glm::vec4(light->transform.translation, 1.0f),
			.color = glm::vec4(light->color, 1.0f)
		}},
		.numOmniLights = 1
		},
		scene->skyboxTexture
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


			renderer.updateEnvironmentBuffer({
				.omniLights = {{
					.position = glm::vec4(light->transform.translation, 1.0f),
					.color = glm::vec4(light->color, 1.0f)
				}},
				.numOmniLights = 1
				});

			renderer.renderForward(frameIndex, commandBuffer);


			{
				ui.beginUserInterface();
				ui.showDebugStats(viewerObject);
				ui.showWindowControls();
				ui.showEntityInspector(scene->getRoot());
				ui.showDemoWindow();
				ui.endUserInterface(commandBuffer);
			}

			renderer.endRenderPass(commandBuffer);
			renderer.endFrame();
		}

		vkDeviceWaitIdle(device.device());
	}
}

void Hmck::RaymarchingDemoApp::load()
{
	Scene::SceneCreateInfo info = {
		.device = device,
		.name = "Volumetric scene",
		.loadFiles = {
			{
				.filename = std::string(MODELS_DIR) + "Sphere/Sphere.glb",
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
