#include "FirstApp.h"

Hmck::FirstApp::FirstApp()
{
	loadGameObjects();
}

Hmck::FirstApp::~FirstApp(){}

void Hmck::FirstApp::run()
{
	HmckSimpleRenderSystem simpleRenderSystem{ hmckDevice,hmckRenderer.getSwapChainRenderPass() };
    HmckCamera camera{};
    camera.setViewTarget(glm::vec3(-1.f, -2.f, 2.f), glm::vec3(0.f, 0.f, 2.5f));

    auto viewerObject = HmckGameObject::createGameObject();
    KeyboardMovementController cameraController{};

    auto currentTime = std::chrono::high_resolution_clock::now();

	while (!hmckWindow.shouldClose())
	{
		glfwPollEvents();

        // gameloop timing
        auto newTime = std::chrono::high_resolution_clock::now();
        float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
        currentTime = newTime;

        cameraController.moveInPlaneXZ(hmckWindow.getGLFWwindow(), frameTime, viewerObject);
        camera.setViewYXZ(viewerObject.transform.translation, viewerObject.transform.rotation);

        float aspect = hmckRenderer.getAspectRatio();
        camera.setPerspectiveProjection(glm::radians(50.f), aspect, 0.1f, 1000.f);

		if (auto commandBuffer = hmckRenderer.beginFrame())
		{
			hmckRenderer.beginSwapChainRenderPass(commandBuffer);

			simpleRenderSystem.renderGameObjects(commandBuffer,gameObjects, camera);

			hmckRenderer.endSwapChainRenderPass(commandBuffer);
			hmckRenderer.endFrame();
		}

	}

	vkDeviceWaitIdle(hmckDevice.device());
}

void Hmck::FirstApp::loadGameObjects()
{
    std::shared_ptr<HmckModel> hmckModel = HmckModel::createModelFromFile(hmckDevice, "Models/smooth_vase.obj");

    auto gameObj = HmckGameObject::createGameObject();
    gameObj.model = hmckModel;
    gameObj.transform.translation = { .0f, 0.f, 2.5f };
    gameObj.transform.scale = glm::vec3(3.f);
    gameObjects.push_back(std::move(gameObj));
}