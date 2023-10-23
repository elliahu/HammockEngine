#include "VolumetricRenderingApp.h"

Hmck::VolumetricRenderingApp::VolumetricRenderingApp()
{
    descriptorPool = HmckDescriptorPool::Builder(hmckDevice)
        .setPoolFlags(VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT)
        .setMaxSets(100)
        .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2000)
        .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2000)
        .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2000)
        .build();

    load();

    descriptorSetLayout = HmckDescriptorSetLayout::Builder(hmckDevice)
        .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
        .build();

    materialLayout = HmckDescriptorSetLayout::Builder(hmckDevice)
        .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // albedo
        .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // normal
        .addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // roughnessMetalic
        .addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // occlusion
        .build();
}

void Hmck::VolumetricRenderingApp::run()
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
        HmckDescriptorWriter(*descriptorSetLayout, *descriptorPool)
            .writeBuffer(0, &bufferInfo)
            .build(globalDescriptorSets[i]);
    }

    // camera and movement
    HmckCamera camera{};
    camera.setViewTarget(glm::vec3(-1.f, -2.f, 2.f), glm::vec3(0.f, 0.f, 2.5f));
    auto viewerObject = HmckGameObject::createGameObject();
    viewerObject.transformComponent.translation.z = -2.5f;
    KeyboardMovementController cameraController{};


    auto depthFormat = hmckDevice.findSupportedFormat(
        { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

    HmckGraphicsPipeline standardPipeline = HmckGraphicsPipeline::createGraphicsPipeline({
        .debugName = "standard_forward_pass",
        .device = hmckDevice,
        .VS {
            .byteCode = Hmck::Filesystem::readFile("../../HammockEngine/Engine/Shaders/Compiled/volumetric.vert.spv"),
            .entryFunc = "main"
        },
        .FS {
            .byteCode = Hmck::Filesystem::readFile("../../HammockEngine/Engine/Shaders/Compiled/volumetric.frag.spv"),
            .entryFunc = "main"
        },
        .descriptorSetLayouts = {
            descriptorSetLayout->getDescriptorSetLayout(),
            materialLayout->getDescriptorSetLayout()
        },
        .pushConstantRanges {
            {
                .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                .offset = 0,
                .size = sizeof(PushConstantData)
            },
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
        .renderPass = hmckRenderer.getSwapChainRenderPass()
        });


    auto currentTime = std::chrono::high_resolution_clock::now();
    while (!hmckWindow.shouldClose())
    {
        hmckWindow.pollEvents();


        // gameloop timing
        auto newTime = std::chrono::high_resolution_clock::now();
        float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
        currentTime = newTime;

        // camera
        cameraController.moveInPlaneXZ(hmckWindow, frameTime, viewerObject);
        camera.setViewYXZ(viewerObject.transformComponent.translation, viewerObject.transformComponent.rotation);
        float aspect = hmckRenderer.getAspectRatio();
        camera.setPerspectiveProjection(glm::radians(50.0f), aspect, 0.1f, 1000.f);

        // start a new frame
        if (auto commandBuffer = hmckRenderer.beginFrame())
        {
            int frameIndex = hmckRenderer.getFrameIndex();
            PerFrameData data{
                .projection = camera.getProjection(),
                .view = camera.getView(),
                .inverseView = camera.getInverseView()
            };
            uboBuffers[frameIndex]->writeToBuffer(&data);

            // on screen rendering
            hmckRenderer.beginSwapChainRenderPass(commandBuffer);

            standardPipeline.bind(commandBuffer);

            vkCmdSetDepthBias(
                commandBuffer,
                1.25f,
                0.0f,
                1.75f);

            vkCmdBindDescriptorSets(
                commandBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                standardPipeline.graphicsPipelineLayout,
                0, 1,
                &globalDescriptorSets[frameIndex],
                0,
                nullptr
            );

            for (auto& kv : gameObjects)
            {
                auto& obj = kv.second;
                // dont render object that would not be visible
                if (obj.glTFComponent == nullptr) continue;

                obj.glTFComponent->glTF->draw(commandBuffer, standardPipeline.graphicsPipelineLayout, obj.transformComponent.mat4());
            }

            hmckRenderer.endRenderPass(commandBuffer);
            hmckRenderer.endFrame();
        }

        vkDeviceWaitIdle(hmckDevice.device());

        // destroy allocated stuff here
    }
}

void Hmck::VolumetricRenderingApp::load()
{
    auto helmet = HmckGameObject::createFromGLTF(std::string(MODELS_DIR) + "helmet/helmet.glb", hmckDevice,{ .binary = true });
    helmet.setName("Flight Helmet");
    gameObjects.emplace(helmet.getId(), std::move(helmet));
}
