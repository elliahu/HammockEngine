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

	HmckDeferredRenderSystem deferredRenderSystem{ 
        hmckDevice,
        hmckRenderer.getSwapChainRenderPass(), 
        globalSetLayouts
    };

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
    KeyboardMovementController cameraController{};

    HmckGlobalUbo ubo{};

    { // set lights
        //TODO make better system for this
        int dirLightIdx = 0;
        int lightIndex = 0;
        for (auto& kv : gameObjects)
        {
            auto& obj = kv.second;

            if (obj.pointLightComponent != nullptr)
            {
                assert(lightIndex < MAX_LIGHTS && "Point light limit exeeded");

                // update lights
                ubo.pointLights[lightIndex].position = ubo.view * glm::vec4(obj.transformComponent.translation, 1.f);
                ubo.pointLights[lightIndex].color = glm::vec4(obj.colorComponent, obj.pointLightComponent->lightIntensity);
                ubo.pointLights[lightIndex].lightTerms.x = obj.pointLightComponent->quadraticTerm;
                ubo.pointLights[lightIndex].lightTerms.y = obj.pointLightComponent->linearTerm;
                ubo.pointLights[lightIndex].lightTerms.z = obj.pointLightComponent->constantTerm;

                lightIndex += 1;
            }

            if (obj.directionalLightComponent != nullptr)
            {
                // directional light
                assert(dirLightIdx < 1 && "Directional light limit exeeded. There can only be one directional light");


                ubo.directionalLight.color = glm::vec4(obj.colorComponent, obj.directionalLightComponent->lightIntensity);
                ubo.directionalLight.direction = glm::vec4(
                    glm::normalize((glm::mat3(ubo.view) * obj.directionalLightComponent->target) - (glm::mat3(ubo.view) * obj.transformComponent.translation)),
                    obj.directionalLightComponent->fov);

                // TODO make this not ubo as it is not used in all systems
                glm::mat4 depthPerpectiveProjectionMatrix = glm::perspective(
                    obj.directionalLightComponent->fov, 1.0f,
                    obj.directionalLightComponent->_near,
                    obj.directionalLightComponent->_far);
                glm::mat4 depthOrthogonalProjectionMatrix = glm::ortho(
                    -20.f, +20.0f,
                    -20.f, +20.f,
                    obj.directionalLightComponent->_near, obj.directionalLightComponent->_far);
                glm::mat4 depthViewMatrix = glm::lookAt(obj.transformComponent.translation, obj.directionalLightComponent->target, glm::vec3(0, 1, 0));
                glm::mat4 depthModelMatrix = glm::mat4(1.0f);
                ubo.depthBiasMVP = depthOrthogonalProjectionMatrix * depthViewMatrix * depthModelMatrix;

                dirLightIdx += 1;
            }
        }
        ubo.numLights = lightIndex;
    }

    auto depthFormat = hmckDevice.findSupportedFormat(
        { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

    HmckFramebuffer shadowmapFramebuffer = HmckFramebuffer::createFramebuffer({
        .device = hmckDevice,
        .width = SHADOW_RES_WIDTH, .height = SHADOW_RES_WIDTH,
        .attachments {
            {
                .width = SHADOW_RES_WIDTH, .height = SHADOW_RES_HEIGHT,
                .layerCount = 1,
                .format = depthFormat,
                .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            }
        }
    });

    HmckGraphicsPipeline shadowmapPipeline = HmckGraphicsPipeline::createGraphicsPipeline({
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
        .pushConstantRanges {
            {
                .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                .offset = 0,
                .size = sizeof(MatrixPushConstantData)
            }
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
        .renderPass = shadowmapFramebuffer.renderPass
    });

    HmckFramebuffer gbuffer = HmckFramebuffer::createFramebuffer({
        .device = hmckDevice,
        .width = WINDOW_WIDTH, .height = WINDOW_HEIGHT,
        .attachments {
            // 0 position
            {
                .width = WINDOW_WIDTH, .height = WINDOW_HEIGHT,
                .layerCount = 1,
                .format = VK_FORMAT_R16G16B16A16_SFLOAT,
                .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            },
            // 1 albedo
            {
                .width = WINDOW_WIDTH, .height = WINDOW_HEIGHT,
                .layerCount = 1,
                .format = VK_FORMAT_R8G8B8A8_UNORM,
                .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            },
            // 2 normal
            {
                .width = WINDOW_WIDTH, .height = WINDOW_HEIGHT,
                .layerCount = 1,
                .format = VK_FORMAT_R16G16B16A16_SFLOAT,
                .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            },
            // 3 rough, metal, ao
            {
                .width = WINDOW_WIDTH, .height = WINDOW_HEIGHT,
                .layerCount = 1,
                .format = VK_FORMAT_R16G16B16A16_SFLOAT,
                .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            },
            // 4 depth
            {
                .width = WINDOW_WIDTH, .height = WINDOW_HEIGHT,
                .layerCount = 1,
                .format = depthFormat,
                .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            },
        }
    });

    HmckGraphicsPipeline gbufferPipeline = HmckGraphicsPipeline::createGraphicsPipeline({
        .debugName = "gbuffer_pass",
        .device = hmckDevice,
        .VS {
            .byteCode = Hmck::Filesystem::readFile("../../HammockEngine/Engine/Shaders/Compiled/gbuffer.vert.spv"),
            .entryFunc = "main"
        },
        .FS {
            .byteCode = Hmck::Filesystem::readFile("../../HammockEngine/Engine/Shaders/Compiled/gbuffer.frag.spv"),
            .entryFunc = "main"
        },
        .descriptorSetLayouts = gbufferSetLayouts,
        .pushConstantRanges {
            {
                .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                .offset = 0,
                .size = sizeof(MatrixPushConstantData)
            },
        },
        .graphicsState {
            .depthTest = VK_TRUE,
            .depthTestCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
            .blendAtaAttachmentStates {
                Hmck::Init::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
                Hmck::Init::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
                Hmck::Init::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
                Hmck::Init::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
            },
            .vertexBufferBindings {
                .vertexBindingDescriptions = HmckGLTF::getBindingDescriptions(),
                .vertexAttributeDescriptions = HmckGLTF::getAttributeDescriptions()
            }
        },
        .renderPass = gbuffer.renderPass
    });

    HmckFramebuffer ssaoFramebuffer = HmckFramebuffer::createFramebuffer({
        .device = hmckDevice,
        .width = WINDOW_WIDTH, .height = WINDOW_HEIGHT,
        .attachments {
            {
                .width = WINDOW_WIDTH, .height = WINDOW_HEIGHT,
                .layerCount = 1,
                .format = VK_FORMAT_R8_UNORM,
                .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            }
        }
    });

    std::unique_ptr<HmckDescriptorSetLayout> ssaoLayout = HmckDescriptorSetLayout::Builder(hmckDevice)
        .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .addBinding(3, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .addBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .build();

    HmckGraphicsPipeline ssaoPipeline = HmckGraphicsPipeline::createGraphicsPipeline({
        .debugName = "ssao_pass",
        .device = hmckDevice,
        .VS {
            .byteCode = Hmck::Filesystem::readFile("../../HammockEngine/Engine/Shaders/Compiled/fullscreen.vert.spv"),
            .entryFunc = "main"
        },
        .FS {
            .byteCode = Hmck::Filesystem::readFile("../../HammockEngine/Engine/Shaders/Compiled/ssao.frag.spv"),
            .entryFunc = "main"
        },
        .descriptorSetLayouts {
            globalSetLayout->getDescriptorSetLayout(),
            ssaoLayout->getDescriptorSetLayout()
        },
        .pushConstantRanges {},
        .graphicsState {
            .depthTest = VK_TRUE,
            .depthTestCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
            .blendAtaAttachmentStates {},
            .vertexBufferBindings {
                .vertexBindingDescriptions = HmckGLTF::getBindingDescriptions(),
                .vertexAttributeDescriptions = HmckGLTF::getAttributeDescriptions()
            }
        },
        .renderPass = ssaoFramebuffer.renderPass
    });
    // TODO make method for this in Framebuffer class
    VkDescriptorImageInfo imgInf1 = {
        .sampler = gbuffer.sampler,
        .imageView = gbuffer.attachments[0].view,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };
    VkDescriptorImageInfo imgInf2 = {
         .sampler = gbuffer.sampler,
         .imageView = gbuffer.attachments[2].view,
         .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };
    VkDescriptorImageInfo imgInf3 = {
         .sampler = gbuffer.sampler,
         .imageView = gbuffer.attachments[4].view,
         .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

    VkDescriptorSet ssaoDescriptorSet;
    std::unique_ptr<HmckBuffer> ssaoKernelBuffer = Rnd::createSSAOKernel(hmckDevice, 64);
    HmckTexture2D ssaoNoiseTexture = Rnd::createNoiseTexture(hmckDevice, 4);
    auto bufferInfo = ssaoKernelBuffer->descriptorInfo();
    auto writer = HmckDescriptorWriter(*ssaoLayout, *globalPool)
        .writeImage(0, &imgInf1)
        .writeImage(1, &imgInf2)
        .writeImage(2, &ssaoNoiseTexture.descriptor)
        .writeBuffer(3, &bufferInfo)
        .writeImage(4, &imgInf3)
        .build(ssaoDescriptorSet);



    HmckFramebuffer blurFramebuffer = HmckFramebuffer::createFramebuffer({
        .device = hmckDevice,
        .width = WINDOW_WIDTH, .height = WINDOW_HEIGHT,
        .attachments {
            {
                .width = WINDOW_WIDTH, .height = WINDOW_HEIGHT,
                .layerCount = 1,
                .format = VK_FORMAT_R8_UNORM,
                .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            }
        }
    });

    std::unique_ptr<HmckDescriptorSetLayout> ssaoBlurLayout = HmckDescriptorSetLayout::Builder(hmckDevice)
        .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .build();

    HmckGraphicsPipeline blurPipeline = HmckGraphicsPipeline::createGraphicsPipeline({
        .debugName = "blur_pass",
        .device = hmckDevice,
        .VS {
            .byteCode = Hmck::Filesystem::readFile("../../HammockEngine/Engine/Shaders/Compiled/fullscreen.vert.spv"),
            .entryFunc = "main"
        },
        .FS {
            .byteCode = Hmck::Filesystem::readFile("../../HammockEngine/Engine/Shaders/Compiled/blur.frag.spv"),
            .entryFunc = "main"
        },
        .descriptorSetLayouts = {
            globalSetLayout->getDescriptorSetLayout(),
            ssaoBlurLayout->getDescriptorSetLayout()
        },
        .pushConstantRanges {},
        .graphicsState {
            .depthTest = VK_TRUE,
            .depthTestCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
            .blendAtaAttachmentStates {},
            .vertexBufferBindings {
                .vertexBindingDescriptions = HmckGLTF::getBindingDescriptions(),
                .vertexAttributeDescriptions = HmckGLTF::getAttributeDescriptions()
            }
        },
        .renderPass = blurFramebuffer.renderPass
    });

    VkDescriptorSet ssaoBlurDescriptorSet;
    VkDescriptorImageInfo ssaoImageInfo{
        ssaoFramebuffer.sampler,
        ssaoFramebuffer.attachments[0].view,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };
    HmckDescriptorWriter(*ssaoBlurLayout, *globalPool)
        .writeImage(0, &ssaoImageInfo)
        .build(ssaoBlurDescriptorSet);


    deferredRenderSystem.updateShadowmapDescriptorSet({
        .sampler = shadowmapFramebuffer.sampler,
        .imageView = shadowmapFramebuffer.attachments[0].view,
        .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
    });

    deferredRenderSystem.updateGbufferDescriptorSet({
        {
            .sampler = gbuffer.sampler,
            .imageView = gbuffer.attachments[0].view,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        },
        {
            .sampler = gbuffer.sampler,
            .imageView = gbuffer.attachments[1].view,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        },
        {
           .sampler = gbuffer.sampler,
            .imageView = gbuffer.attachments[2].view,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        },
        {
            .sampler = gbuffer.sampler,
            .imageView = gbuffer.attachments[3].view,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        }
    });

    deferredRenderSystem.updateSSAODescriptorSet({
        .sampler = ssaoFramebuffer.sampler,
        .imageView = ssaoFramebuffer.attachments[0].view,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        }, {
        .sampler = blurFramebuffer.sampler,
        .imageView = blurFramebuffer.attachments[0].view,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
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


            uboBuffers[frameIndex]->writeToBuffer(&ubo);
            // RENDER
            // OFFSCREEN RENDER
            // shadowmap
            {
                hmckRenderer.beginRenderPass(
                    shadowmapFramebuffer,
                    commandBuffer,
                    {
                        {.depthStencil = { 1.0f, 0 }},
                    });
                shadowmapPipeline.bind(commandBuffer);
                vkCmdSetDepthBias(
                    frameInfo.commandBuffer,
                    1.25f,
                    0.0f,
                    1.75f);

                vkCmdBindDescriptorSets(
                    frameInfo.commandBuffer,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    shadowmapPipeline.graphicsPipelineLayout,
                    0, 1,
                    &frameInfo.globalDescriptorSet,
                    0,
                    nullptr
                );
                for (auto& kv : frameInfo.gameObjects)
                {
                    auto& obj = kv.second;
                    if (obj.glTFComponent == nullptr) continue;

                    obj.glTFComponent->glTF->draw(frameInfo.commandBuffer, shadowmapPipeline.graphicsPipelineLayout, obj.transformComponent.mat4());
                }
                hmckRenderer.endRenderPass(commandBuffer);
            }

            //gbuffer
            {
                hmckRenderer.beginRenderPass(gbuffer, commandBuffer, {
                    {.color = { 0.0f, 0.0f, 0.0f, 0.0f } },
                    {.color = { 0.0f, 0.0f, 0.0f, 0.0f } },
                    {.color = { 0.0f, 0.0f, 0.0f, 0.0f } },
                    {.color = { 0.0f, 0.0f, 0.0f, 0.0f } },
                    {.depthStencil = { 1.0f, 0 }}
                    });
                gbufferPipeline.bind(commandBuffer);
                vkCmdBindDescriptorSets(
                    frameInfo.commandBuffer,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    gbufferPipeline.graphicsPipelineLayout,
                    0, 1,
                    &frameInfo.globalDescriptorSet,
                    0,
                    nullptr
                    );
                for (auto& kv : frameInfo.gameObjects)
                {
                    auto& obj = kv.second;
                    // dont render object that would not be visible
                    if (obj.glTFComponent == nullptr) continue;

                    obj.glTFComponent->glTF->draw(frameInfo.commandBuffer, gbufferPipeline.graphicsPipelineLayout, obj.transformComponent.mat4());
                }
                hmckRenderer.endRenderPass(commandBuffer);
            }

            
            // ssao
            {
                hmckRenderer.beginRenderPass(ssaoFramebuffer, commandBuffer, {
                    {.color = { 0.0f, 0.0f, 0.0f, 1.0f } },
                    {.depthStencil = { 1.0f, 0 }}
                    });
                ssaoPipeline.bind(commandBuffer);
                vkCmdBindDescriptorSets(
                    frameInfo.commandBuffer,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    ssaoPipeline.graphicsPipelineLayout,
                    0, 1,
                    &frameInfo.globalDescriptorSet,
                    0,
                    nullptr
                );

                vkCmdBindDescriptorSets(
                    frameInfo.commandBuffer,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    ssaoPipeline.graphicsPipelineLayout,
                    1, 1,
                    &ssaoDescriptorSet,
                    0,
                    nullptr
                );

                vkCmdDraw(frameInfo.commandBuffer, 3, 1, 0, 0);
                hmckRenderer.endRenderPass(commandBuffer);

                // blur
                hmckRenderer.beginRenderPass(blurFramebuffer, commandBuffer, {
                    {.color = { 0.0f, 0.0f, 0.0f, 1.0f } },
                    {.depthStencil = { 1.0f, 0 }}
                    });
                blurPipeline.bind(commandBuffer);
                vkCmdBindDescriptorSets(
                    frameInfo.commandBuffer,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    blurPipeline.graphicsPipelineLayout,
                    0, 1,
                    &frameInfo.globalDescriptorSet,
                    0,
                    nullptr
                );
                vkCmdBindDescriptorSets(
                    frameInfo.commandBuffer,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    blurPipeline.graphicsPipelineLayout,
                    1, 1,
                    &ssaoBlurDescriptorSet,
                    0,
                    nullptr
                );

                vkCmdDraw(frameInfo.commandBuffer, 3, 1, 0, 0);
                hmckRenderer.endRenderPass(commandBuffer);
            }

            // on screen
            hmckRenderer.beginSwapChainRenderPass(commandBuffer);
            deferredRenderSystem.render(frameInfo);
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