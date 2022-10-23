#include "FirstApp.h"

Hmck::FirstApp::FirstApp()
{
    globalPool = HmckDescriptorPool::Builder(hmckDevice)
        .setMaxSets(HmckSwapChain::MAX_FRAMES_IN_FLIGHT)
        .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, HmckSwapChain::MAX_FRAMES_IN_FLIGHT)
        .build();
	loadGameObjects();
}

Hmck::FirstApp::~FirstApp(){}

void Hmck::FirstApp::run()
{
    std::vector<std::unique_ptr<HmckBuffer>> uboBuffers{ HmckSwapChain::MAX_FRAMES_IN_FLIGHT };

    for (int i = 0; i < uboBuffers.size(); i++)
    {
        uboBuffers[i] = std::make_unique<HmckBuffer>(
            hmckDevice,
            sizeof(HmckGlobalUbo),
            1,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );
        uboBuffers[i]->map();
    }

    auto globalSetLayout = HmckDescriptorSetLayout::Builder(hmckDevice)
        .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
        .build();

    std::vector<VkDescriptorSet> globalDescriptorSets(HmckSwapChain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < globalDescriptorSets.size(); i++)
    {
        auto bufferInfo = uboBuffers[i]->descriptorInfo();
        HmckDescriptorWriter(*globalSetLayout, *globalPool)
            .writeBuffer(0, &bufferInfo)
            .build(globalDescriptorSets[i]);
    }

	HmckSimpleRenderSystem simpleRenderSystem{ hmckDevice,hmckRenderer.getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout()};
    HmckCamera camera{};
    camera.setViewTarget(glm::vec3(-1.f, -2.f, 2.f), glm::vec3(0.f, 0.f, 2.5f));

    auto viewerObject = HmckGameObject::createGameObject();
    viewerObject.transform.translation.z = -2.5f;
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
            int frameIndex = hmckRenderer.getFrameIndex();
            HmckFrameInfo frameInfo{
                frameIndex,
                frameTime,
                commandBuffer,
                camera,
                globalDescriptorSets[frameIndex]
            };

            // update
            HmckGlobalUbo ubo{};
            ubo.projectionView = camera.getProjection() * camera.getView();
            uboBuffers[frameIndex]->writeToBuffer(&ubo);
            //uboBuffers[frameIndex]->flush(); // no need to flush -> VK_MEMORY_PROPERTY_HOST_COHERENT_BIT is set

            // render
			hmckRenderer.beginSwapChainRenderPass(commandBuffer);
			simpleRenderSystem.renderGameObjects(frameInfo,gameObjects);
			hmckRenderer.endSwapChainRenderPass(commandBuffer);
			hmckRenderer.endFrame();
		}

	}

	vkDeviceWaitIdle(hmckDevice.device());
}

void Hmck::FirstApp::loadGameObjects()
{

    // vase
    std::shared_ptr<HmckModel> vaseModel = HmckModel::createModelFromFile(hmckDevice, "Models/smooth_vase.obj");
    auto vase = HmckGameObject::createGameObject();
    vase.model = vaseModel;
    vase.transform.translation = { .0f, 0.5f, 0.f };
    vase.transform.scale = glm::vec3(3.f);
    gameObjects.push_back(std::move(vase));

    // floor
    std::shared_ptr<HmckModel> quadModel = HmckModel::createModelFromFile(hmckDevice, "Models/quad.obj");
    auto floor = HmckGameObject::createGameObject();
    floor.model = quadModel;
    floor.transform.translation = { .0f, 0.5f, 0.f };
    floor.transform.scale = glm::vec3(3.f, 1.f, 3.f);
    gameObjects.push_back(std::move(floor));
}