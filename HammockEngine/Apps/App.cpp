#include "App.h"

Hmck::App::App()
{
    // TODO change this so that material sets are allocated dynamicly or from different pool object
    globalPool = HmckDescriptorPool::Builder(hmckDevice)
        .setPoolFlags(VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT)
        .setMaxSets(100)
        .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, HmckSwapChain::MAX_FRAMES_IN_FLIGHT)
        .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000)
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

    // TODO make HmckGbufferRenderSystem own material set layout 
    std::vector<VkDescriptorSetLayout> gbufferSetLayouts{
        globalSetLayout->getDescriptorSetLayout(),
        materialLayout->getDescriptorSetLayout()
    };

    HmckGbufferRenderSystem gbufferRenderSystem{
        hmckDevice,
        hmckRenderer.getGbufferRenderPass(),
        gbufferSetLayouts
    };

    HmckSSAOSystem ssaoSystem{
        hmckDevice,
        hmckRenderer.getSSAORenderPass(),
        hmckRenderer.getSSAOBlurRenderPass(),
        globalSetLayouts
    };

	HmckDeferredRenderSystem deferredRenderSystem{ 
        hmckDevice,
        hmckRenderer.getSwapChainRenderPass(), 
        globalSetLayouts
    };

    HmckLightSystem lightSystem{
        hmckDevice,
        hmckRenderer.getSwapChainRenderPass(),
        globalSetLayouts
    };

    HmckUISystem userInterfaceSystem{
        hmckDevice,
        hmckRenderer.getSwapChainRenderPass(),
        hmckWindow
    };

    std::array<VkDescriptorImageInfo, 4> imageInfos{
        hmckRenderer.getGbufferDescriptorImageInfo(0),
        hmckRenderer.getGbufferDescriptorImageInfo(1),
        hmckRenderer.getGbufferDescriptorImageInfo(2),
        hmckRenderer.getGbufferDescriptorImageInfo(3),
    };
    deferredRenderSystem.updateGbufferDescriptorSet(imageInfos);

    std::array<VkDescriptorImageInfo, 3> ssaoImageInfos{
        hmckRenderer.getGbufferDescriptorImageInfo(0),
        hmckRenderer.getGbufferDescriptorImageInfo(2),
        hmckRenderer.getGbufferDepthDescriptorImageInfo()
    };
    ssaoSystem.updateSSAODescriptorSet(ssaoImageInfos);

    auto ssao = hmckRenderer.getSSAODescriptorImageInfo();
    auto ssaBlur = hmckRenderer.getSSAOBlurDescriptorImageInfo();
    deferredRenderSystem.updateSSAODescriptorSet(ssao, ssaBlur);

    // TODO layout error
    auto ssaoImageInfo = hmckRenderer.getSSAODescriptorImageInfo();
    ssaoSystem.updateSSAOBlurDescriptorSet(ssaoImageInfo);

    // camera and movement
    HmckCamera camera{};
    camera.setViewTarget(glm::vec3(-1.f, -2.f, 2.f), glm::vec3(0.f, 0.f, 2.5f));
    auto viewerObject = HmckGameObject::createGameObject();
    viewerObject.transformComponent.translation.z = -2.5f;
    KeyboardMovementController cameraController{};

    HmckGlobalUbo ubo{};


    HmckFramebuffer sf = HmckFramebuffer::createFramebuffer({
        .device = hmckDevice,
        .width = SHADOW_RES_WIDTH, .height = SHADOW_RES_WIDTH,
        .attachments {
            {
                .width = SHADOW_RES_WIDTH,
                .height = SHADOW_RES_HEIGHT,
                .layerCount = 1,
                .format = hmckDevice.findSupportedFormat(
                    { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
                    VK_IMAGE_TILING_OPTIMAL,
                    VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT),
                .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            }
        }
    });

    HmckGraphicsPipeline sp = HmckGraphicsPipeline::createGraphicsPipeline({
        .debugName = "shadow_pass",
        .device = hmckDevice,
        .VS {
            .byteCode = Hmck::Filesystem::readFile("../../HammockEngine/Engine/Shaders/Compiled/shadowmap.vert.spv"),
            .entryFunc = "main"
        },
        .FS {
            .byteCode = Hmck::Filesystem::readFile("../../HammockEngine/Engine/Shaders/Compiled/shadowmap.frag.spv"),
            .entryFunc = "main"
        },
        .descriptorSetLayouts = gbufferSetLayouts,
        .pushConstantRange {
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            .offset = 0,
            .size = sizeof(MatrixPushConstantData)
        },
        .graphicsState {
            .depthTest = VK_TRUE,
            .depthTestCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
            .blendAtaAttachmentStates {},
            .vertexBufferBindings {
                .vertexBindingDescriptions = HmckGLTF::getBindingDescriptions(),
                .vertexAttributeDescriptions = HmckGLTF::getAttributeDescriptions()
            }
        },
        .renderPass = sf.renderPass
    });

    deferredRenderSystem.updateShadowmapDescriptorSet({
        .sampler = sf.sampler,
        .imageView = sf.attachments[0].view,
        .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
    });


    auto currentTime = std::chrono::high_resolution_clock::now();
	while (!hmckWindow.shouldClose())
	{
        hmckWindow.pollEvents();

        // gameloop timing
        auto newTime = std::chrono::high_resolution_clock::now();
        float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
        currentTime = newTime;

        cameraController.moveInPlaneXZ(hmckWindow, frameTime, viewerObject);
        camera.setViewYXZ(viewerObject.transformComponent.translation, viewerObject.transformComponent.rotation);

        float aspect = hmckRenderer.getAspectRatio();
        camera.setPerspectiveProjection(glm::radians(50.0f), aspect, 0.1f,  64.f );

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
            ubo.projection = camera.getProjection();
            ubo.view = camera.getView();
            ubo.inverseView = camera.getInverseView();
            lightSystem.update(frameInfo, ubo);

            uboBuffers[frameIndex]->writeToBuffer(&ubo);
            // RENDER
            // offscreen

            hmckRenderer.beginRenderPass(
                sf,
                commandBuffer,
                {
                    {.depthStencil = { 1.0f, 0 }},
                });
            sp.bind(commandBuffer);
            vkCmdSetDepthBias(
                frameInfo.commandBuffer,
                1.25f,
                0.0f,
                1.75f);

            vkCmdBindDescriptorSets(
                frameInfo.commandBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                sp.graphicsPipelineLayout,
                0, 1,
                &frameInfo.globalDescriptorSet,
                0,
                nullptr
            );
            for (auto& kv : frameInfo.gameObjects)
            {
                auto& obj = kv.second;
                if (obj.glTFComponent == nullptr) continue;

                obj.glTFComponent->glTF->draw(frameInfo.commandBuffer, sp.graphicsPipelineLayout, obj.transformComponent.mat4());
            }
            hmckRenderer.endRenderPass(commandBuffer);


            hmckRenderer.beginGbufferRenderPass(commandBuffer);
            gbufferRenderSystem.pipeline->bind(commandBuffer);
            gbufferRenderSystem.render(frameInfo);
            hmckRenderer.endRenderPass(commandBuffer);

            hmckRenderer.beginSSAORenderPass(commandBuffer);
            ssaoSystem.ssao(frameInfo);
            hmckRenderer.endRenderPass(commandBuffer);

            hmckRenderer.beginSSAOBlurRenderPass(commandBuffer);
            ssaoSystem.ssaoBlur(frameInfo);
            hmckRenderer.endRenderPass(commandBuffer);

            // on screen
			hmckRenderer.beginSwapChainRenderPass(commandBuffer);
			deferredRenderSystem.render(frameInfo);

            // TODO doesn't work because of the depth test failing 
            //lightSystem.render(frameInfo);
        
            // ui
            userInterfaceSystem.beginUserInterface();
            //userInterfaceSystem.showDemoWindow();
            userInterfaceSystem.showDebugStats(viewerObject);
            userInterfaceSystem.showWindowControls();
            userInterfaceSystem.showGameObjectsInspector(gameObjects);
            userInterfaceSystem.showGlobalSettings(ubo);
            userInterfaceSystem.endUserInterface(commandBuffer);
            
			hmckRenderer.endRenderPass(commandBuffer);
			hmckRenderer.endFrame();
		}

	}

	vkDeviceWaitIdle(hmckDevice.device());
}

void Hmck::App::loadGameObjects()
{
    // layouts
    // TODO think about using array of combined image samplers
    // TODO move this to Gbuffer system as it is the only system that uses this, no need for this to be in App
    materialLayout = HmckDescriptorSetLayout::Builder(hmckDevice)
        .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // albedo
        .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // normal
        .addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // roughnessMetalic
        .addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // occlusion
        .build();


    HmckGLTF::Config config{
        .binary = true
    };

    //auto helmet = HmckGameObject::createFromGLTF(std::string(MODELS_DIR) + "helmet/helmet.glb", hmckDevice, config);
    //helmet.setName("Flight Helmet");
    //gameObjects.emplace(helmet.getId(), std::move(helmet));


    auto sponza = HmckGameObject::createFromGLTF(std::string(MODELS_DIR) + "sponza/sponza.glb", hmckDevice, config);
    sponza.transformComponent.translation.y = - .25f;
    sponza.setName("Sponza");
    gameObjects.emplace(sponza.getId(), std::move(sponza));

    // Point lights
    std::vector<glm::vec3> lightColors{
         {1.f, .1f, .1f},
         {.1f, .1f, 1.f},
         {.1f, 1.f, .1f},
         {1.f, 1.f, .1f},
         {.1f, 1.f, 1.f},
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
        pointLight.transformComponent.translation = glm::vec3(rotateLight * glm::vec4(-1.f, 1.f, -1.f, 1.f));
        gameObjects.emplace(pointLight.getId(), std::move(pointLight));
    }
    
    // Directional light
    auto directionalLight = HmckGameObject::createDirectionalLight();
    directionalLight.transformComponent.translation = { 15.6f, 32.71f, -20.15f };
    directionalLight.setName("Directional light");
    gameObjects.emplace(directionalLight.getId(), std::move(directionalLight));

}