#include "FirstApp.h"

Hmck::FirstApp::FirstApp()
{
	loadGameObjects();
}

Hmck::FirstApp::~FirstApp()
{
}

void Hmck::FirstApp::run()
{
	HmckSimpleRenderSystem simpleRenderSystem{ hmckDevice,hmckRenderer.getSwapChainRenderPass() };

	while (!hmckWindow.shouldClose())
	{
		glfwPollEvents();
		

		if (auto commandBuffer = hmckRenderer.beginFrame())
		{
			hmckRenderer.beginSwapChainRenderPass(commandBuffer);

			simpleRenderSystem.renderGameObjects(commandBuffer,gameObjects);

			hmckRenderer.endSwapChainRenderPass(commandBuffer);
			hmckRenderer.endFrame();
		}

	}

	vkDeviceWaitIdle(hmckDevice.device());
}

void Hmck::FirstApp::loadGameObjects()
{
	std::vector<HmckModel::Vertex> vertices{
		//position			//color
		{{0.0f, -0.5f},		{1.0f , 0.0f, 0.0f}},
		{{0.5f, 0.5f},		{0.0f , 1.0f, 0.0f}},
		{{-0.5f, 0.5f},		{1.0f , 0.0f, 1.0f}}
	};

	auto hmckModel = std::make_shared<HmckModel>(hmckDevice, vertices);

	auto triangle = HmckGameObject::createGameObject();
	triangle.model = hmckModel;
	triangle.color = { .1f, .8f, .1f };
	triangle.transform2d.translation.x = .2f;
	triangle.transform2d.scale = { 2.f, .5f };
	triangle.transform2d.rotation = .25f * glm::two_pi<float>();

	gameObjects.push_back(std::move(triangle));
}