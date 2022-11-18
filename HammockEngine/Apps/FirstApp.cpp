#include "FirstApp.h"

Hmck::FirstApp::FirstApp()
{
    globalPool = HmckDescriptorPool::Builder(hmckDevice)
        .setMaxSets(HmckSwapChain::MAX_FRAMES_IN_FLIGHT)
        .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, HmckSwapChain::MAX_FRAMES_IN_FLIGHT)
        .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, HmckSwapChain::MAX_FRAMES_IN_FLIGHT) // texture sampler
        .build();
	loadGameObjects();
}

Hmck::FirstApp::~FirstApp()
{
    for (auto& kv : gameObjects)
    {
        if (kv.second.material == nullptr)
            continue;
        kv.second.material->hmckMaterial->destroy();
    }
}

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
        .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
        .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .build();

    std::vector<VkDescriptorSet> globalDescriptorSets(HmckSwapChain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < globalDescriptorSets.size(); i++)
    {
        auto bufferInfo = uboBuffers[i]->descriptorInfo();

        // TODO somehow bind the actual image and sampler resources to the descriptors
        // i mean better than like this
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = gameObjects.at(0).material->hmckMaterial->texture->image.imageView;
        imageInfo.sampler = gameObjects.at(0).material->hmckMaterial->texture->sampler;

        // TODO make it so that more texture can be used
        // currently only one can be used

        HmckDescriptorWriter(*globalSetLayout, *globalPool)
            .writeBuffer(0, &bufferInfo)
            .writeImage(1, &imageInfo) // TODO this is very much ugly
            .build(globalDescriptorSets[i]);
    }

    // systems
	HmckSimpleRenderSystem simpleRenderSystem{ 
        hmckDevice,
        hmckRenderer.getSwapChainRenderPass(), 
        globalSetLayout->getDescriptorSetLayout()
    };
    HmckLightSystem lightSystem{
        hmckDevice,
        hmckRenderer.getSwapChainRenderPass(),
        globalSetLayout->getDescriptorSetLayout()
    };
    HmckCollisionDetectionSystem collisionDetectionSystem{};
    HmckUISystem userInterfaceSystem{
        hmckDevice,
        hmckRenderer.getSwapChainRenderPass(),
        hmckWindow
    };

    // camera and movement
    HmckCamera camera{};
    camera.setViewTarget(glm::vec3(-1.f, -2.f, 2.f), glm::vec3(0.f, 0.f, 2.5f));
    auto viewerObject = HmckGameObject::createGameObject();
    viewerObject.transform.translation.z = -2.5f;
    KeyboardMovementController cameraController{ hmckWindow.getGLFWwindow() };

    auto currentTime = std::chrono::high_resolution_clock::now();
	while (!hmckWindow.shouldClose())
	{
        hmckWindow.pollEvents();
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
                globalDescriptorSets[frameIndex],
                gameObjects
            };

            // update
            HmckGlobalUbo ubo{};
            ubo.projection = camera.getProjection();
            ubo.view = camera.getView();
            ubo.inverseView = camera.getInverseView();
            lightSystem.update(frameInfo, ubo);
            uboBuffers[frameIndex]->writeToBuffer(&ubo);

            // render
			hmckRenderer.beginSwapChainRenderPass(commandBuffer);


            // start rendering
            
			simpleRenderSystem.renderGameObjects(frameInfo);
            lightSystem.render(frameInfo);

            // UI
            userInterfaceSystem.beginUserInterface();
            userInterfaceSystem.showDebugStats(viewerObject);
            userInterfaceSystem.showGameObjectsInspector(gameObjects);
            userInterfaceSystem.endUserInterface(commandBuffer);
            
            // check if vases colide
            if (collisionDetectionSystem.intersect(gameObjects.at(0), gameObjects.at(1)))
            {
                HmckLogger::debug("Vases intersect");
            }

            // end rendering
			hmckRenderer.endSwapChainRenderPass(commandBuffer);
			hmckRenderer.endFrame();
		}

	}

	vkDeviceWaitIdle(hmckDevice.device());
}

void Hmck::FirstApp::loadGameObjects()
{
    // models
    std::shared_ptr<HmckModel> vaseModel = HmckModel::createModelFromFile(hmckDevice, std::string(MODELS_DIR) + "smooth_vase.obj");

    // materials
    HmckCreateMaterialInfo bricksMaterialInfo{};
    bricksMaterialInfo.texture = std::string(MATERIALS_DIR) + "Bricks/PavingStones122_1K_Color.jpg";
    std::shared_ptr<HmckMaterial> bricksMaterial = HmckMaterial::createMaterial(hmckDevice, bricksMaterialInfo);

    HmckCreateMaterialInfo stoneMaterialInfo{};
    stoneMaterialInfo.texture = std::string(MATERIALS_DIR) + "PavingStone/PavingStones110_1K_Color.jpg";
    std::shared_ptr<HmckMaterial> stoneMaterial = HmckMaterial::createMaterial(hmckDevice, stoneMaterialInfo);

    // vase
    auto vase = HmckGameObject::createGameObject();
    vase.setName("Vase");
    vase.model = vaseModel;
    vase.transform.translation = { .0f, 0.5f, 0.f };
    vase.transform.scale = glm::vec3(3.f);
    vase.fitBoundingBox(vaseModel->modelInfo);
    vase.applyMaterial(bricksMaterial);
    gameObjects.emplace(vase.getId(), std::move(vase));

    // vase 2
    auto smallVase = HmckGameObject::createGameObject();
    smallVase.setName("Small vase");
    smallVase.model = vaseModel;
    smallVase.transform.translation = { 1.0f, 0.5f, 0.f };
    smallVase.transform.scale = glm::vec3(1.f);
    smallVase.fitBoundingBox(vaseModel->modelInfo);
    gameObjects.emplace(smallVase.getId(), std::move(smallVase));

    // floor
    std::shared_ptr<HmckModel> quadModel = HmckModel::createModelFromFile(hmckDevice, std::string(MODELS_DIR) + "quad.obj");
    auto floor = HmckGameObject::createGameObject();
    floor.setName("Floor");
    floor.model = quadModel;
    floor.transform.translation = { .0f, 0.5f, 0.f };
    floor.transform.scale = glm::vec3(3.f, 1.f, 3.f);
    floor.applyMaterial(stoneMaterial);
    gameObjects.emplace(floor.getId(), std::move(floor));

    // Point light
    std::vector<glm::vec3> lightColors{
     {1.f, .1f, .1f},
     {.1f, .1f, 1.f},
     {.1f, 1.f, .1f},
     {1.f, 1.f, .1f},
     {.1f, 1.f, 1.f},
     {1.f, 1.f, 1.f}  
    };
    for (int i = 0; i < lightColors.size(); i++)
    {
        auto pointLight = HmckGameObject::createPointLight(0.45f);
        pointLight.setName("Point light");
        pointLight.color = lightColors[i];
        auto rotateLight = glm::rotate(
            glm::mat4(1.f), 
            (i * glm::two_pi<float>()) / lightColors.size(),
            { 0.f, -1.f, 0.f }
        );
        pointLight.transform.translation = glm::vec3(rotateLight * glm::vec4(-1.f, -1.f, -1.f, 1.f));
        gameObjects.emplace(pointLight.getId(), std::move(pointLight));
    }
    
    // Directional light
    auto directionalLight = HmckGameObject::createDirectionalLight();
    directionalLight.setName("Directional light");
    gameObjects.emplace(directionalLight.getId(), std::move(directionalLight));

}