#include "CloudRenderer.h"

using namespace Hammock;

CloudRenderer::CloudRenderer(const int32_t width, const int32_t height)
    : window{instance, "Cloud Renderer", width, height}, device(instance, window.getSurface()) {
    load();
    prepareRenderPasses();
}

CloudRenderer::~CloudRenderer() {
}

void CloudRenderer::run() {
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

void CloudRenderer::load() {
    int32_t grid = 258;
    ScopedMemory sdfData(SignedDistanceField().loadFromFile(assetPath("sdf/dragon"), grid).data());
    ScopedMemory noiseData(PerlinNoise3D(69420).generateNoiseVolume(grid, grid, grid, 25.f));

    cloudPass.noiseVolumeHandle = deviceStorage.createTexture3D({
        .buffer = noiseData.get(),
        .instanceSize = sizeof(float),
        .width = static_cast<uint32_t>(grid),
        .height = static_cast<uint32_t>(grid),
        .channels = 1,
        .depth = static_cast<uint32_t>(grid),
        .format = VK_FORMAT_R32_SFLOAT,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .samplerInfo = {
            .addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        }
    });

    cloudPass.cloudSdfHandle = deviceStorage.createTexture3D({
        .buffer = sdfData.get(),
        .instanceSize = sizeof(float),
        .width = static_cast<uint32_t>(grid),
        .height = static_cast<uint32_t>(grid),
        .channels = 1,
        .depth = static_cast<uint32_t>(grid),
        .format = VK_FORMAT_R32_SFLOAT,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    });


    BoundingBox cloudBoundingBox = BoundingBox({0.f, 0.f, 0.f}, 2.f);
    geometry.vertices.insert(geometry.vertices.end(), cloudBoundingBox.getVertices().begin(), cloudBoundingBox.getVertices().end());
    geometry.indices.insert(geometry.indices.end(), cloudBoundingBox.getIndices().begin(), cloudBoundingBox.getIndices().end());

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

void CloudRenderer::prepareRenderPasses() {
    cloudPass.descriptorSetLayout = deviceStorage.createDescriptorSetLayout({
        .bindings = {
            {
                .binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS,
            },
            {
                .binding = 1, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            },
            {
                .binding = 2, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
            },
            {
                .binding = 3, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
            },
        }
    });

    for (int i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        cloudPass.cameraBuffers[i] = deviceStorage.createBuffer({
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

    for (int i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        auto cameraBufferInfo = deviceStorage.getBuffer(cloudPass.cameraBuffers[i])->descriptorInfo();
        auto cloudBufferInfo = deviceStorage.getBuffer(cloudPass.cloudBuffers[i])->descriptorInfo();
        auto noiseInfo = deviceStorage.getTexture3DDescriptorImageInfo(cloudPass.noiseVolumeHandle);
        auto sdfInfo = deviceStorage.getTexture3DDescriptorImageInfo(cloudPass.cloudSdfHandle);
        cloudPass.descriptorSets[i] = deviceStorage.createDescriptorSet({
            .descriptorSetLayout = cloudPass.descriptorSetLayout,
            .bufferWrites = {{0, cameraBufferInfo},{1, cloudBufferInfo}},
            .imageWrites = {{2, noiseInfo}, {3, sdfInfo}}
        });
    }

    cloudPass.framebuffer = Framebuffer::createFramebufferPtr({
       .device = device,
       .width = static_cast<uint32_t>(cloudPass.resolution.X * window.width),
       .height = static_cast<uint32_t>(cloudPass.resolution.Y * window.height),
       .attachments{
           // 1 albedo
           {
               .width = static_cast<uint32_t>(cloudPass.resolution.X * window.width),
               .height = static_cast<uint32_t>(cloudPass.resolution.Y * window.height),
               .layerCount = 1,
               .format = VK_FORMAT_R8G8B8A8_UNORM,
               .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
           },
       }
   });

    cloudPass.pipeline = GraphicsPipeline::createGraphicsPipelinePtr({
        .debugName = "Cloud pass",
        .device = device,
        .VS
        {.byteCode = Filesystem::readFile(compiledShaderPath("fullscreen.vert")),},
        .FS
        {.byteCode = Filesystem::readFile(compiledShaderPath("volume.frag")),},
        .descriptorSetLayouts = {
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
            },
            .vertexBufferBindings{
                .vertexBindingDescriptions = Vertex::vertexInputBindingDescriptions(),
                .vertexAttributeDescriptions = Vertex::vertexInputAttributeDescriptions()
            }
        },
        .renderPass = cloudPass.framebuffer->renderPass
    });

    compositionPass.descriptorSetLayout = deviceStorage.createDescriptorSetLayout({
        .bindings = {
            {
                .binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
            },
        }
    });

    for (int i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorImageInfo imageInfo{
            .sampler = cloudPass.framebuffer->sampler,
            .imageView = cloudPass.framebuffer->attachments[0].view,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        };
        compositionPass.descriptorSets[i] = deviceStorage.createDescriptorSet({
           .descriptorSetLayout = compositionPass.descriptorSetLayout,
           .imageWrites = {{0, imageInfo}, }
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
            deviceStorage.getDescriptorSetLayout(cloudPass.descriptorSetLayout).getDescriptorSetLayout(),
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
            .cullMode = VK_CULL_MODE_NONE,
            .vertexBufferBindings{
                .vertexBindingDescriptions = Vertex::vertexInputBindingDescriptions(),
                .vertexAttributeDescriptions = Vertex::vertexInputAttributeDescriptions()
            }
        },
        .renderPass = renderContext.getSwapChainRenderPass()
    });
}


void CloudRenderer::update() {
    float speed = 0.001f;
    if(window.getKeyState(KEY_A) == KeyState::DOWN) azimuth += speed * frameTime;
    if(window.getKeyState(KEY_D) == KeyState::DOWN) azimuth -= speed * frameTime;
    if(window.getKeyState(KEY_W) == KeyState::DOWN) elevation += speed * frameTime;
    if(window.getKeyState(KEY_S) == KeyState::DOWN) elevation -= speed * frameTime;
    if(window.getKeyState(KEY_UP) == KeyState::DOWN) radius -= speed * frameTime;
    if(window.getKeyState(KEY_DOWN) == KeyState::DOWN) radius += speed * frameTime;
    cameraPosition.XYZ = Math::orbitalPosition(HmckVec3{.0f,.0f,.0f}, radius, azimuth, elevation);

    cameraBuffer.proj = Projection().perspective(45.0f, renderContext.getAspectRatio(), 0.1f, 64.0f);
    cameraBuffer.view = Projection().view(cameraPosition.XYZ, {0.f, 0.f, 0.f}, Projection().upPosY());
    cameraBuffer.cameraPosition = cameraPosition;
    cameraBuffer.width = window.width * cloudPass.resolution.X;
    cameraBuffer.height = window.height * cloudPass.resolution.X;

    HmckMat4 translation = HmckTranslate(cloudTranslation);
    pushConstants.cloudTransform = translation;
}

void CloudRenderer::draw() {
    if (const auto commandBuffer = renderContext.beginFrame()) {
        const int frameIndex = renderContext.getFrameIndex();

        deviceStorage.getBuffer(cloudPass.cameraBuffers[frameIndex])->writeToBuffer(&cameraBuffer);
        deviceStorage.getBuffer(cloudPass.cloudBuffers[frameIndex])->writeToBuffer(&cloudBuffer);
        deviceStorage.bindVertexBuffer(vertexBuffer, indexBuffer, commandBuffer);

        // Cloud pass
        renderContext.beginRenderPass(cloudPass.framebuffer, commandBuffer, {
                {.color = {0.0f, 0.0f, 0.0f, 0.0f}}
        });
        cloudPass.pipeline->bind(commandBuffer);
        deviceStorage.bindDescriptorSet(
             commandBuffer,
             VK_PIPELINE_BIND_POINT_GRAPHICS,
             cloudPass.pipeline->graphicsPipelineLayout,
             0, 1,
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
             1, 1,
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

void CloudRenderer::drawUi() {
    ImGui::Begin("Cloud Property editor", (bool *) false, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::DragFloat3("LightDir", &cloudBuffer.lightDir.Elements[0]);
    ImGui::ColorEdit4("LightColor", &cloudBuffer.lightColor.Elements[0]);
    ImGui::DragFloat("Density", &cloudBuffer.density, 0.01f, 0.5f, 2.f);
    ImGui::DragFloat("Absorption", &cloudBuffer.absorption, 0.01f, 0.01f, 1.f);
    ImGui::DragFloat("Phase", &cloudBuffer.phase, 0.01f, -1.0f, 1.0f);
    ImGui::DragFloat("Step size", &cloudBuffer.stepSize, 0.0001f, 0.0f, 100.f);
    ImGui::DragInt("Max steps", &cloudBuffer.maxSteps, 1.f, 0.0f, 10000.0f);
    ImGui::DragFloat("Light march step size multiplier", &cloudBuffer.lsMul, 0.01f, 0.0f, 100.f);
    ImGui::DragInt("Light march max steps", &cloudBuffer.maxLs, 1.f, 0.0f, 10000.0f);
    ImGui::DragFloat3("Position", &cloudTranslation.Elements[0], 0.01f);
    ImGui::End();

    ImGui::Begin("Camera", (bool *) false, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("%.1f FPS ", 1.0f / frameTime);
    ImGui::Text("Frametime: %.2f ms", frameTime * 1000.0f);
    ImGui::DragFloat("Radius", &radius, 0.01f );
    ImGui::DragFloat("Azimuth", &azimuth, 0.01f );
    ImGui::DragFloat("Elevation", &elevation, 0.01f);
    ImGui::End();
}
