#include "FirstApp.h"

Hmck::FirstApp::FirstApp()
{
    // TODO change this so that material sets are allocated dynamicly or from different pool object
    globalPool = HmckDescriptorPool::Builder(hmckDevice)
        .setMaxSets(100)
        .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, HmckSwapChain::MAX_FRAMES_IN_FLIGHT)
        .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100)
        .build();

	loadGameObjects();

    globalSetLayout = HmckDescriptorSetLayout::Builder(hmckDevice)
        .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
        .build();
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

    std::vector<VkDescriptorSet> globalDescriptorSets(HmckSwapChain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < globalDescriptorSets.size(); i++)
    {
        auto bufferInfo = uboBuffers[i]->descriptorInfo();
        HmckDescriptorWriter(*globalSetLayout, *globalPool)
            .writeBuffer(0, &bufferInfo)
            .build(globalDescriptorSets[i]);
    }

    std::vector<VkDescriptorSetLayout> setLayouts{
        globalSetLayout->getDescriptorSetLayout(),
        materialLayout->getDescriptorSetLayout()
    };

    // systems
	HmckRenderSystem renderSystem{ 
        hmckDevice,
        hmckRenderer.getSwapChainRenderPass(), 
        setLayouts
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
    viewerObject.transformComponent.translation.z = -2.5f;
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
        camera.setViewYXZ(viewerObject.transformComponent.translation, viewerObject.transformComponent.rotation);

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
			renderSystem.renderGameObjects(frameInfo);
            lightSystem.render(frameInfo);

            // UI
            userInterfaceSystem.beginUserInterface();
            userInterfaceSystem.showDebugStats(viewerObject);
            userInterfaceSystem.showGameObjectsInspector(gameObjects);
            userInterfaceSystem.endUserInterface(commandBuffer);
            
            // check if vases colide
            //if (collisionDetectionSystem.intersect(gameObjects.at(0), gameObjects.at(1)))
            //{
            //    HmckLogger::debug("Vases intersect");
            //}

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
    std::shared_ptr<HmckModel> cubeModel = HmckModel::createModelFromFile(hmckDevice, std::string(MODELS_DIR) + "cube.obj");
    std::shared_ptr<HmckModel> sphereModel = HmckModel::createModelFromFile(hmckDevice, std::string(MODELS_DIR) + "sphere.obj");

    // materials
    HmckCreateMaterialInfo floorMaterial{};
    floorMaterial.color = std::string(MATERIALS_DIR) + "Bricks/PavingStones122_1K_Color.jpg";
    floorMaterial.normal = std::string(MATERIALS_DIR) + "Bricks/PavingStones122_1K_NormalDX.jpg";
    floorMaterial.roughness = std::string(MATERIALS_DIR) + "Bricks/PavingStones122_1K_Roughness.jpg";
    floorMaterial.ambientOcclusion = std::string(MATERIALS_DIR) + "Bricks/PavingStones122_1K_AmbientOcclusion.jpg";
    floorMaterial.displacement = std::string(MATERIALS_DIR) + "Bricks/PavingStones122_1K_Displacement.jpg";
    std::shared_ptr<HmckMaterial> bricksMaterial = HmckMaterial::createMaterial(hmckDevice, floorMaterial);

    HmckCreateMaterialInfo metalMaterialInfo{};
    metalMaterialInfo.color = std::string(MATERIALS_DIR) + "Metal/Metal046B_1K_Color.jpg";
    metalMaterialInfo.normal = std::string(MATERIALS_DIR) + "Metal/Metal046B_1K_NormalDX.jpg";
    metalMaterialInfo.roughness = std::string(MATERIALS_DIR) + "Metal/Metal046B_1K_Roughness.jpg";
    metalMaterialInfo.metalness = std::string(MATERIALS_DIR) + "Metal/Metal046B_1K_Metalness.jpg";
    metalMaterialInfo.displacement = std::string(MATERIALS_DIR) + "Metal/Metal046B_1K_Displacement.jpg";
    std::shared_ptr<HmckMaterial> metalMaterial = HmckMaterial::createMaterial(hmckDevice, metalMaterialInfo);

    HmckCreateMaterialInfo logMaterialInfo{};
    logMaterialInfo.color = std::string(MATERIALS_DIR) + "Log/3DStick001_SQ-1K_Color.jpg";
    logMaterialInfo.normal = std::string(MATERIALS_DIR) + "Log/3DStick001_SQ-1K_NormalDX.jpg";
    logMaterialInfo.ambientOcclusion = std::string(MATERIALS_DIR) + "Log/3DStick001_SQ-1K_AmbientOcclusion.jpg";
    std::shared_ptr<HmckMaterial> logMaterial = HmckMaterial::createMaterial(hmckDevice, logMaterialInfo);

    // layouts
    // TODO think about using array of combined image samplers
    materialLayout = HmckDescriptorSetLayout::Builder(hmckDevice)
        .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .addBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .addBinding(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .build();

    // vase
    auto sphere = HmckGameObject::createGameObject();
    sphere.setName("Vase");
    sphere.setModel(sphereModel);
    sphere.transformComponent.translation = { .0f, -0.5f, 0.f };
    sphere.transformComponent.scale = glm::vec3(1.f);
    sphere.fitBoundingBox(sphereModel->modelInfo);
    sphere.setMaterial(metalMaterial);
    sphere.bindDescriptorSet(globalPool, materialLayout);
    gameObjects.emplace(sphere.getId(), std::move(sphere));

    // floor
    std::shared_ptr<HmckModel> quadModel = HmckModel::createModelFromFile(hmckDevice, std::string(MODELS_DIR) + "quad.obj");
    auto floor = HmckGameObject::createGameObject();
    floor.setName("Floor");
    floor.setModel(quadModel);
    floor.transformComponent.translation = { .0f, 0.5f, 0.f };
    floor.transformComponent.scale = glm::vec3(3.f, 1.f, 3.f);
    floor.setMaterial(bricksMaterial);
    floor.bindDescriptorSet(globalPool, materialLayout);
    gameObjects.emplace(floor.getId(), std::move(floor));


    std::shared_ptr<HmckModel> logModel = HmckModel::createModelFromFile(hmckDevice, std::string(MODELS_DIR) + "log.obj");
    auto log = HmckGameObject::createGameObject();
    log.setModel(logModel);
    log.transformComponent.translation = { 1.f, -0.5f, 0.f };
    log.transformComponent.scale = glm::vec3(10);
    log.transformComponent.rotation = glm::vec3(0,0,1);
    log.setMaterial(logMaterial);
    log.bindDescriptorSet(globalPool, materialLayout);
    //gameObjects.emplace(log.getId(), std::move(log));


    // Point light
    std::vector<glm::vec3> lightColors{
     //{1.f, .1f, .1f},
     //{.1f, .1f, 1.f},
     //{.1f, 1.f, .1f},
     //{1.f, 1.f, .1f},
     //{.1f, 1.f, 1.f},
     {1.f, 1.f, 1.f}  
    };
    for (int i = 0; i < lightColors.size(); i++)
    {
        auto pointLight = HmckGameObject::createPointLight(1.0f);
        pointLight.setName("Point light");
        pointLight.colorComponent = lightColors[i];
        auto rotateLight = glm::rotate(
            glm::mat4(1.f), 
            (i * glm::two_pi<float>()) / lightColors.size(),
            { 0.f, -1.f, 0.f }
        );
        pointLight.transformComponent.translation = glm::vec3(rotateLight * glm::vec4(-1.f, -1.f, -1.f, 1.f));
        gameObjects.emplace(pointLight.getId(), std::move(pointLight));
    }
    
    // Directional light
    auto directionalLight = HmckGameObject::createDirectionalLight();
    directionalLight.setName("Directional light");
    //gameObjects.emplace(directionalLight.getId(), std::move(directionalLight));

}