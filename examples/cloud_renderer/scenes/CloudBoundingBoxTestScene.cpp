#include "CloudBoundingBoxTestScene.h"

using namespace Hammock;

CloudBoundingBoxTestScene::CloudBoundingBoxTestScene(const int32_t width, const int32_t height)
    : window{instance, "Cloud Renderer", width, height}, device(instance, window.getSurface()) {
    load();
    prepareRenderPasses();
}

CloudBoundingBoxTestScene::~CloudBoundingBoxTestScene() {
}

void CloudBoundingBoxTestScene::run() {
    auto currentTime = std::chrono::high_resolution_clock::now();
    while (!window.shouldClose()) {
        window.pollEvents();

        auto newTime = std::chrono::high_resolution_clock::now();
        frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
        currentTime = newTime;

        update();
        draw();
    }

    device.waitIdle();
}

void CloudBoundingBoxTestScene::load() {
    // Data for scene pass
    Loader(geometry, device, deviceStorage)
            .loadglTF(assetPath("models/SampleScene/SampleScene.glb"));

    // Data for cloud pass

    // Low frequency noise
    int width, height, depth, channels;
    const auto lowFreqNoiseSlices = Filesystem::ls(assetPath("noise/perlin-worley-128x128x128"));
    ScopedMemory lowFreqNoiseBuffer{
        readVolume(lowFreqNoiseSlices, width, height, channels, depth, Filesystem::ImageFormat::R32G32B32A32_SFLOAT)
    };

    cloudPass.lowFreqNoise = deviceStorage.createTexture3D({
        .buffer = lowFreqNoiseBuffer.get(),
        .instanceSize = sizeof(float),
        .width = static_cast<uint32_t>(width),
        .height = static_cast<uint32_t>(height),
        .channels = static_cast<uint32_t>(channels),
        .depth = static_cast<uint32_t>(depth),
        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .samplerInfo = {
            .addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        }
    });

    // High frequency noise
    const auto highFreqNoiseSlices = Filesystem::ls(assetPath("noise/worley-32x32x32"));
    ScopedMemory highFreqNoiseBuffer{
        readVolume(lowFreqNoiseSlices, width, height, channels, depth, Filesystem::ImageFormat::R32G32B32A32_SFLOAT)
    };

    cloudPass.highFreqNoise = deviceStorage.createTexture3D({
        .buffer = highFreqNoiseBuffer.get(),
        .instanceSize = sizeof(float),
        .width = static_cast<uint32_t>(width),
        .height = static_cast<uint32_t>(height),
        .channels = static_cast<uint32_t>(channels),
        .depth = static_cast<uint32_t>(depth),
        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .samplerInfo = {
            .addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        }
    });

   // ScopedMemory densityGradientBuffer{Filesystem::readImage(assetPath("noise/denistygradient.png"),width, height, channels, Filesystem::ImageFormat::R32_SFLOAT)};




    // Create vertex and index buffers
    vertexBuffer = deviceStorage.createVertexBuffer({
        .vertexSize = sizeof(geometry.vertices[0]),
        .vertexCount = static_cast<uint32_t>(geometry.vertices.size()),
        .data = static_cast<void *>(geometry.vertices.data())
    });
    indexBuffer = deviceStorage.createIndexBuffer({
        .indexSize = sizeof(geometry.indices[0]),
        .indexCount = static_cast<uint32_t>(geometry.indices.size()),
        .data = static_cast<void *>(geometry.indices.data())
    });
}

void CloudBoundingBoxTestScene::prepareRenderPasses() {
    // Scene uses only a buffer
    scenePass.descriptorSetLayout = deviceStorage.createDescriptorSetLayout({
        .bindings = {
            {
                // Camera buffer
                .binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS,
            },
        }
    });

    // Cloud pass need one additional buffer and a noise
    cloudPass.descriptorSetLayout = deviceStorage.createDescriptorSetLayout({
        .bindings = {
            {
                // Cloud buffer
                .binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            },
            {
                // Cloud shape noise
                .binding = 1, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
            },
            {
                // Cloud detail noise
                .binding = 2, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
            },
        }
    });

    // Create all buffers
    for (int i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        scenePass.cameraBuffers[i] = deviceStorage.createBuffer({
            .instanceSize = sizeof(CameraBuffer),
            .instanceCount = 1,
            .usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .memoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        });
        cloudPass.cloudBuffers[i] = deviceStorage.createBuffer({
            .instanceSize = sizeof(CloudBuffer),
            .instanceCount = 1,
            .usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .memoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        });
    }


    // Write camera buffer to scene pass descriptors
    // write camera buffer, cloud buffer and noise to cloudpass descriptors
    for (int i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        auto cameraBufferInfo = deviceStorage.getBuffer(scenePass.cameraBuffers[i])->descriptorInfo();
        scenePass.descriptorSets[i] = deviceStorage.createDescriptorSet({
            .descriptorSetLayout = scenePass.descriptorSetLayout,
            .bufferWrites = {{0, cameraBufferInfo},},
        });

        auto cloudBufferInfo = deviceStorage.getBuffer(cloudPass.cloudBuffers[i])->descriptorInfo();
        auto shapeInfo = deviceStorage.getTexture3DDescriptorImageInfo(cloudPass.lowFreqNoise);
        auto detailInfo = deviceStorage.getTexture3DDescriptorImageInfo(cloudPass.highFreqNoise);
        cloudPass.descriptorSets[i] = deviceStorage.createDescriptorSet({
            .descriptorSetLayout = cloudPass.descriptorSetLayout,
            .bufferWrites = {{0, cloudBufferInfo}},
            .imageWrites = {{1, shapeInfo}, {2, detailInfo}}
        });
    }


    // Find supported depth attachment format (not all depth formats may be supported by the GPU)
    auto depthFormat = device.findSupportedFormat(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

    // create scene pass framebuffer
    scenePass.framebuffer = Framebuffer::createFramebufferPtr({
        .device = device,
        .width = static_cast<uint32_t>(scenePass.resolution.X * window.width),
        .height = static_cast<uint32_t>(scenePass.resolution.Y * window.height),
        .attachments{
            // 0 color
            {
                .width = static_cast<uint32_t>(scenePass.resolution.X * window.width),
                .height = static_cast<uint32_t>(scenePass.resolution.Y * window.height),
                .format = VK_FORMAT_R8G8B8A8_UNORM,
                .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            },
            // 1 depth
            {
                .width = static_cast<uint32_t>(scenePass.resolution.X * window.width),
                .height = static_cast<uint32_t>(scenePass.resolution.Y * window.height),
                .format = depthFormat,
                .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            },
        }
    });

    // create scene pass pipeline
    scenePass.pipeline = GraphicsPipeline::createGraphicsPipelinePtr({
        .debugName = "Scene Pass",
        .device = device,
        .VS
        {.byteCode = Filesystem::readFile(compiledShaderPath("lambert_forward.vert")),},
        .FS
        {.byteCode = Filesystem::readFile(compiledShaderPath("lambert_forward.frag")),},
        .descriptorSetLayouts = {
            deviceStorage.getDescriptorSetLayout(scenePass.descriptorSetLayout).getDescriptorSetLayout()
        },
        .pushConstantRanges = {
            {
                .stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS,
                .offset = 0,
                .size = sizeof(PushConstants)
            }
        },
        .graphicsState{
            .blendAtaAttachmentStates = {
                Init::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
            },
            .vertexBufferBindings{
                .vertexBindingDescriptions = Vertex::vertexInputBindingDescriptions(),
                .vertexAttributeDescriptions = Vertex::vertexInputAttributeDescriptions()
            }
        },
        .renderPass = scenePass.framebuffer->renderPass
    });


    // Create cloud pass framebuffer
    cloudPass.framebuffer = Framebuffer::createFramebufferPtr({
        .device = device,
        .width = static_cast<uint32_t>(cloudPass.resolution.X * window.width),
        .height = static_cast<uint32_t>(cloudPass.resolution.Y * window.height),
        .attachments{
            // 0 color
            {
                .width = static_cast<uint32_t>(cloudPass.resolution.X * window.width),
                .height = static_cast<uint32_t>(cloudPass.resolution.Y * window.height),
                .format = VK_FORMAT_R8G8B8A8_UNORM,
                .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            },
            // 1 view-space depth of the bounding box
            {
                .width = static_cast<uint32_t>(cloudPass.resolution.X * window.width),
                .height = static_cast<uint32_t>(cloudPass.resolution.Y * window.height),
                .format = VK_FORMAT_R16_SFLOAT,
                .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            }
        }
    });

    // create cloud pass pipeline
    cloudPass.pipeline = GraphicsPipeline::createGraphicsPipelinePtr({
        .debugName = "Cloud pass",
        .device = device,
        .VS
        {.byteCode = Filesystem::readFile(compiledShaderPath("fullscreen.vert")),},
        .FS
        {.byteCode = Filesystem::readFile(compiledShaderPath("sky.frag")),},
        .descriptorSetLayouts = {
            deviceStorage.getDescriptorSetLayout(scenePass.descriptorSetLayout).getDescriptorSetLayout(),
            deviceStorage.getDescriptorSetLayout(cloudPass.descriptorSetLayout).getDescriptorSetLayout()
        },
        .pushConstantRanges = {
            {
                .stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS,
                .offset = 0,
                .size = sizeof(PushConstants)
            }
        },
        .graphicsState{
            .cullMode = VK_CULL_MODE_NONE,
            .blendAtaAttachmentStates = {
                Init::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
                Init::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
            },
            .vertexBufferBindings{
                .vertexBindingDescriptions = Vertex::vertexInputBindingDescriptions(),
                .vertexAttributeDescriptions = Vertex::vertexInputAttributeDescriptions()
            }
        },
        .renderPass = cloudPass.framebuffer->renderPass
    });

    // Create composition pass descriptor set layout, we sample from two color attachments and one depth attachment
    compositionPass.descriptorSetLayout = deviceStorage.createDescriptorSetLayout({
        .bindings = {
            // scene color
            {
                .binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
            },
            // scene depth
            {
                .binding = 1, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
            },
            // cloud color
            {
                .binding = 2, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
            },
            // cloud bb view-space position
            {
                .binding = 3, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
            },
        }
    });

    // Create composition descriptors
    for (int i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorImageInfo sceneColor{
            .sampler = scenePass.framebuffer->sampler,
            .imageView = scenePass.framebuffer->attachments[0].view,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        };
        VkDescriptorImageInfo sceneDepth{
            .sampler = scenePass.framebuffer->sampler,
            .imageView = scenePass.framebuffer->attachments[1].view,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        };
        VkDescriptorImageInfo cloudColor{
            .sampler = cloudPass.framebuffer->sampler,
            .imageView = cloudPass.framebuffer->attachments[0].view,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        };
        VkDescriptorImageInfo cloudPos{
            .sampler = cloudPass.framebuffer->sampler,
            .imageView = cloudPass.framebuffer->attachments[1].view,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        };
        compositionPass.descriptorSets[i] = deviceStorage.createDescriptorSet({
            .descriptorSetLayout = compositionPass.descriptorSetLayout,
            .imageWrites = {{0, sceneColor}, {1, sceneDepth}, {2, cloudColor}, {3, cloudPos}}
        });
    }

    compositionPass.pipeline = GraphicsPipeline::createGraphicsPipelinePtr({
        .debugName = "Composition pass",
        .device = device,
        .VS
        {.byteCode = Filesystem::readFile(compiledShaderPath("fullscreen.vert")),},
        .FS
        {.byteCode = Filesystem::readFile(compiledShaderPath("cloud_composition.frag")),},
        .descriptorSetLayouts = {
            deviceStorage.getDescriptorSetLayout(compositionPass.descriptorSetLayout).getDescriptorSetLayout()
        },
        .pushConstantRanges{
            {
                .stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS,
                .offset = 0,
                .size = sizeof(PushConstants)
            }
        },
        .graphicsState{
            .depthTest = VK_FALSE,
            .cullMode = VK_CULL_MODE_NONE,
            .vertexBufferBindings{
                .vertexBindingDescriptions = Vertex::vertexInputBindingDescriptions(),
                .vertexAttributeDescriptions = Vertex::vertexInputAttributeDescriptions()
            }
        },
        .renderPass = renderContext.getSwapChainRenderPass()
    });
}


void CloudBoundingBoxTestScene::update() {
    float speed = 0.001f;
    if (window.getKeyState(KEY_A) == KeyState::DOWN) azimuth += speed * frameTime;
    if (window.getKeyState(KEY_D) == KeyState::DOWN) azimuth -= speed * frameTime;
    if (window.getKeyState(KEY_W) == KeyState::DOWN) elevation += speed * frameTime;
    if (window.getKeyState(KEY_S) == KeyState::DOWN) elevation -= speed * frameTime;
    if (window.getKeyState(KEY_UP) == KeyState::DOWN) radius -= speed * frameTime;
    if (window.getKeyState(KEY_DOWN) == KeyState::DOWN) radius += speed * frameTime;
    cameraPosition.XYZ = Math::orbitalPosition(HmckVec3{.0f, .0f, .0f}, radius, azimuth, elevation);

    cameraBuffer.proj = Projection().perspective(45.0f, renderContext.getAspectRatio(), 0.1f, 64.0f);
    cameraBuffer.view = Projection().view(cameraPosition.XYZ, {0.f, 0.f, 0.f}, Projection().upPosY());
    cameraBuffer.cameraPosition = cameraPosition;
    cameraBuffer.width = window.width * cloudPass.resolution.X;
    cameraBuffer.height = window.height * cloudPass.resolution.X;
}

void CloudBoundingBoxTestScene::draw() {
    if (const auto commandBuffer = renderContext.beginFrame()) {
        const int frameIndex = renderContext.getFrameIndex();

        deviceStorage.getBuffer(scenePass.cameraBuffers[frameIndex])->writeToBuffer(&cameraBuffer);
        deviceStorage.getBuffer(cloudPass.cloudBuffers[frameIndex])->writeToBuffer(&cloudBuffer);
        deviceStorage.bindVertexBuffer(vertexBuffer, indexBuffer, commandBuffer);

        // Scene pass
        renderContext.beginRenderPass(scenePass.framebuffer, commandBuffer, {
                                          {.color = {0.0f, 0.0f, 0.0f, 0.0f}},
                                          {.depthStencil = {1.0f, 0}}
                                      });
        scenePass.pipeline->bind(commandBuffer);
        deviceStorage.bindDescriptorSet(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            scenePass.pipeline->graphicsPipelineLayout,
            0, 1,
            scenePass.descriptorSets[frameIndex],
            0,
            nullptr);
        vkCmdDrawIndexed(commandBuffer, geometry.indices.size(), 1, 0, 0,
                         0);
        renderContext.endRenderPass(commandBuffer);


        // Cloud pass
        renderContext.beginRenderPass(cloudPass.framebuffer, commandBuffer, {
                                          {.color = {0.0f, 0.0f, 0.0f, 0.0f}},
                                          {.color = {0.0f, 0.0f, 0.0f, 0.0f}},
                                      });
        cloudPass.pipeline->bind(commandBuffer);
        deviceStorage.bindDescriptorSet(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            cloudPass.pipeline->graphicsPipelineLayout,
            1, 1,
            cloudPass.descriptorSets[frameIndex],
            0,
            nullptr);
        vkCmdPushConstants(commandBuffer, cloudPass.pipeline->graphicsPipelineLayout,
                           VK_SHADER_STAGE_ALL_GRAPHICS, 0,
                           sizeof(PushConstants), &pushConstants);
        vkCmdDraw(commandBuffer, 3, 1, 0, 0);
        renderContext.endRenderPass(commandBuffer);

        // Composition pass
        renderContext.beginSwapChainRenderPass(commandBuffer);
        compositionPass.pipeline->bind(commandBuffer);
        deviceStorage.bindDescriptorSet(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            compositionPass.pipeline->graphicsPipelineLayout,
            0, 1,
            compositionPass.descriptorSets[frameIndex],
            0,
            nullptr);
        vkCmdDraw(commandBuffer, 3, 1, 0, 0);

        // UI
        ui.beginUserInterface();
        drawUi();
        ui.endUserInterface(commandBuffer);

        renderContext.endRenderPass(commandBuffer);
        renderContext.endFrame();
    }
}

void CloudBoundingBoxTestScene::drawUi() {
    ImGui::Begin("Cloud Property editor", (bool *) false, ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::Text("Frequency options:");
    ImGui::SliderFloat4("Low frequency channels weights (Base shape)", &cloudBuffer.lowFreqWeights.Elements[0], 0.0f, 1.0f);
    ImGui::SliderFloat("Low frequency scale", &cloudBuffer.lowFreqScale, 0.0f, 1.0f);
    ImGui::SliderFloat3("High channels weights (Erosion)", &cloudBuffer.highFreqWeights.Elements[0], 0.0f, 1.0f);
    ImGui::SliderFloat("High frequency scale", &cloudBuffer.highFreqScale, 0.0f, 1.0f);
    ImGui::DragFloat3("Frequency sampling offset", &cloudBuffer.freqSamplOffset.Elements[0], 0.1f);
    ImGui::SliderFloat("Frequency sampling scale", &cloudBuffer.freqSampleScale, 0.0f, 1.0f);

    ImGui::Text("Density options:");
    ImGui::SliderFloat("Freq offset", &cloudBuffer.freqOffset, -5.0f, 0.0f);
    ImGui::SliderFloat("Density multiplier", &cloudBuffer.densityMultiplier, 0.f, 20.f);

    ImGui::Text("Cloud properties:");
    ImGui::Text("Cloud placement (bounding box):");
    ImGui::DragFloat3("BB1", &pushConstants.bb1.Elements[0], 0.1f);
    ImGui::DragFloat3("BB2", &pushConstants.bb2.Elements[0], 0.1f);


    ImGui::Text("Rendering options:");
    ImGui::DragInt("Num steps", &cloudBuffer.numSteps, 1, 1);
    ImGui::DragInt("Light march num steps", &cloudBuffer.numLightSteps, 1, 1);
    ImGui::End();


    ImGui::Begin("Camera", (bool *) false, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("%.1f FPS ", 1.0f / frameTime);
    ImGui::Text("Frametime: %.2f ms", frameTime * 1000.0f);
    ImGui::DragFloat("Radius", &radius, 0.01f);
    ImGui::DragFloat("Azimuth", &azimuth, 0.01f);
    ImGui::DragFloat("Elevation", &elevation, 0.01f);
    ImGui::DragFloat3("LightDir", &cameraBuffer.lightDir.Elements[0], 0.1f);
    ImGui::ColorEdit4("LightColor", &cameraBuffer.lightColor.Elements[0]);
    ImGui::End();
}
