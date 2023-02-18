#include "App.h"

Hmck::App::App()
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

void Hmck::App::run()
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

    // systems and layouts
    
    std::vector<VkDescriptorSetLayout> globalSetLayouts{
        globalSetLayout->getDescriptorSetLayout(),
    };

    HmckOffscreenRenderSystem offscreenRenderSystem{
        hmckDevice,
        hmckRenderer.getOffscreenRenderPass(),
        globalSetLayouts
    };

    std::vector<VkDescriptorSetLayout> sceneSetLayouts{
        globalSetLayout->getDescriptorSetLayout(),
        materialLayout->getDescriptorSetLayout(),
    };
	HmckRenderSystem renderSystem{ 
        hmckDevice,
        hmckRenderer.getSwapChainRenderPass(), 
        sceneSetLayouts
    };

    HmckLightSystem lightSystem{
        hmckDevice,
        hmckRenderer.getSwapChainRenderPass(),
        globalSetLayouts
    };

    HmckCollisionDetectionSystem collisionDetectionSystem{};

    HmckUISystem userInterfaceSystem{
        hmckDevice,
        hmckRenderer.getSwapChainRenderPass(),
        hmckWindow
    };

    // TODO this is ugly, thinking about restructure
    VkDescriptorImageInfo imageInfo = hmckRenderer.getOffscreenDescriptorImageInfo();
    renderSystem.writeToDescriptorSet(imageInfo);

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
        camera.setPerspectiveProjection(glm::radians(50.0f), aspect, 0.1f,  1000.f );

        // start a new frame
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

            // UPDATE 
            HmckGlobalUbo ubo{};
            ubo.projection = camera.getProjection();
            ubo.view = camera.getView();
            ubo.inverseView = camera.getInverseView();
            // TODO make this not ubo as it is not used in all systems
            // TODO make this get values from spectator object
            glm::mat4 depthProjectionMatrix = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 25.0f);
            glm::mat4 depthViewMatrix = glm::lookAt(glm::vec3(10.f, -10.f, -10.f), glm::vec3(0.0f), glm::vec3(0, 1, 0));
            glm::mat4 depthModelMatrix = glm::mat4(1.0f);
            ubo.depthBiasMVP = depthProjectionMatrix * depthViewMatrix * depthModelMatrix;
            lightSystem.update(frameInfo, ubo);

            uboBuffers[frameIndex]->writeToBuffer(&ubo);
            // RENDER
            // offscreen
            hmckRenderer.beginOffscreenRenderPass(commandBuffer);

            offscreenRenderSystem.render(frameInfo);

            hmckRenderer.endOffscreenRenderPass(commandBuffer);
            // on screen
			hmckRenderer.beginSwapChainRenderPass(commandBuffer);
            
            
			renderSystem.render(frameInfo);
            lightSystem.render(frameInfo);
        
            // ui
            userInterfaceSystem.beginUserInterface();
            userInterfaceSystem.showDebugStats(viewerObject);
            userInterfaceSystem.showGameObjectsInspector(gameObjects);
            userInterfaceSystem.endUserInterface(commandBuffer);
            
			hmckRenderer.endSwapChainRenderPass(commandBuffer);
			hmckRenderer.endFrame();
		}

	}

	vkDeviceWaitIdle(hmckDevice.device());
}

void Hmck::App::loadGameObjects()
{
    // models
    std::shared_ptr<HmckMesh> vaseModel = HmckMesh::createMeshFromFile(hmckDevice, std::string(MODELS_DIR) + "smooth_vase.obj");
    std::shared_ptr<HmckMesh> cubeModel = HmckMesh::createMeshFromFile(hmckDevice, std::string(MODELS_DIR) + "cube.obj");
    std::shared_ptr<HmckMesh> sphereModel = HmckMesh::createMeshFromFile(hmckDevice, std::string(MODELS_DIR) + "sphere.obj");

    // materials
    HmckCreateMaterialInfo floorMaterialIfno{};
    floorMaterialIfno.color = std::string(MATERIALS_DIR) + "Wood06/Wood060_1K_Color.jpg";
    floorMaterialIfno.normal = std::string(MATERIALS_DIR) + "Wood06/Wood060_1K_NormalDX.jpg";
    floorMaterialIfno.roughness = std::string(MATERIALS_DIR) + "Wood06/Wood060_1K_Roughness.jpg";
    floorMaterialIfno.ambientOcclusion = std::string(MATERIALS_DIR) + "Wood06/Wood060_1K_AmbientOcclusion.jpg";
    floorMaterialIfno.displacement = std::string(MATERIALS_DIR) + "Wood06/Wood060_1K_Displacement.jpg";
    std::shared_ptr<HmckMaterial> floorMaterial = HmckMaterial::createMaterial(hmckDevice, floorMaterialIfno);

    HmckCreateMaterialInfo spherematerialInfo{};
    spherematerialInfo.color = std::string(MATERIALS_DIR) + "Metal/Metal046B_1K_Color.jpg";
    spherematerialInfo.normal = std::string(MATERIALS_DIR) + "Metal/Metal046B_1K_NormalDX.jpg";
    spherematerialInfo.roughness = std::string(MATERIALS_DIR) + "Metal/Metal046B_1K_Roughness.jpg";
    spherematerialInfo.metalness = std::string(MATERIALS_DIR) + "Metal/Metal046B_1K_Metalness.jpg";
    spherematerialInfo.displacement = std::string(MATERIALS_DIR) + "Metal/Metal046B_1K_Displacement.jpg";
    std::shared_ptr<HmckMaterial> sphereMaterial = HmckMaterial::createMaterial(hmckDevice, spherematerialInfo);

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
    sphere.setName("Sphere");
    sphere.setModel(sphereModel);
    sphere.transformComponent.translation = { .0f, -0.5f, 0.f };
    sphere.transformComponent.scale = glm::vec3(.4f);
    sphere.fitBoundingBox(sphereModel->modelInfo);
    sphere.setMaterial(sphereMaterial);
    sphere.bindDescriptorSet(globalPool, materialLayout);
    gameObjects.emplace(sphere.getId(), std::move(sphere));

    // floor
    std::shared_ptr<HmckMesh> quadModel = HmckMesh::createMeshFromFile(hmckDevice, std::string(MODELS_DIR) + "quad.obj");
    auto floor = HmckGameObject::createGameObject();
    floor.setName("Floor");
    floor.setModel(quadModel);
    floor.transformComponent.translation = { .0f, 0.5f, 0.f };
    floor.transformComponent.scale = glm::vec3(3.f, 1.f, 3.f);
    floor.setMaterial(floorMaterial);
    floor.bindDescriptorSet(globalPool, materialLayout);
    gameObjects.emplace(floor.getId(), std::move(floor));


    std::shared_ptr<HmckMesh> logModel = HmckMesh::createMeshFromFile(hmckDevice, std::string(MODELS_DIR) + "log.obj");
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
         {1.f, 1.f, 1.f}, 
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
    gameObjects.emplace(directionalLight.getId(), std::move(directionalLight));

}