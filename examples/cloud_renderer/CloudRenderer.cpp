#include "CloudRenderer.h"

using namespace Hammock;

CloudRenderer::CloudRenderer(const int32_t width, const int32_t height)
    : window{instance, "Cloud Renderer", width, height}, device(instance, window.getSurface()) {
    load();
    prepareDescriptors();
    preparePipelines();
}

CloudRenderer::~CloudRenderer() {
}

void CloudRenderer::run() {
    auto currentTime = std::chrono::high_resolution_clock::now();
    while (!window.shouldClose()) {
        window.pollEvents();

        auto newTime = std::chrono::high_resolution_clock::now();
        float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();

        update(frameTime);
        draw();
    }

    device.waitIdle();
}

void CloudRenderer::load() {
    int32_t grid = 258;
    ScopedMemory sdfData(SignedDistanceField().loadFromFile(assetPath("sdf/dragon"), grid).data());
    ScopedMemory noiseData(PerlinNoise3D(69420).generateNoiseVolume(grid, grid, grid));

    noiseVolumeHandle = deviceStorage.createTexture3D({
        .buffer = noiseData.get(),
        .instanceSize = sizeof(float),
        .width = static_cast<uint32_t>(grid),
        .height = static_cast<uint32_t>(grid),
        .channels = 1,
        .depth = static_cast<uint32_t>(grid),
        .format = VK_FORMAT_R32_SFLOAT,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    });

    cloudSdfHandle = deviceStorage.createTexture3D({
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

void CloudRenderer::prepareDescriptors() {
    descriptorSetLayout = deviceStorage.createDescriptorSetLayout({
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
        cameraBuffers[i] = deviceStorage.createBuffer({
            .instanceSize = sizeof(CameraBuffer),
            .instanceCount = 1,
            .usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .memoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        });
        cloudBuffers[i] = deviceStorage.createBuffer({
            .instanceSize = sizeof(CloudBuffer),
            .instanceCount = 1,
            .usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .memoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        });
    }

    for (int i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        auto cameraBufferInfo = deviceStorage.getBuffer(cameraBuffers[i])->descriptorInfo();
        auto cloudBufferInfo = deviceStorage.getBuffer(cloudBuffers[i])->descriptorInfo();
        auto noiseInfo = deviceStorage.getTexture3DDescriptorImageInfo(noiseVolumeHandle);
        auto sdfInfo = deviceStorage.getTexture3DDescriptorImageInfo(cloudSdfHandle);
        descriptorSets[i] = deviceStorage.createDescriptorSet({
            .descriptorSetLayout = descriptorSetLayout,
            .bufferWrites = {{0, cameraBufferInfo},{1, cloudBufferInfo}},
            .imageWrites = {{2, noiseInfo}, {3, sdfInfo}}
        });
    }
}

void CloudRenderer::preparePipelines() {
    compositionPass.pipeline = GraphicsPipeline::createGraphicsPipelinePtr({
        .debugName = "Composition pass",
        .device = device,
        .VS
        {.byteCode = Filesystem::readFile(compiledShaderPath("fullscreen.vert")),},
        .FS
        {.byteCode = Filesystem::readFile(compiledShaderPath("volume.frag")),},
        .descriptorSetLayouts = {
            deviceStorage.getDescriptorSetLayout(descriptorSetLayout).getDescriptorSetLayout()
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

void CloudRenderer::update(float frameTime) {
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
    cameraBuffer.width = window.width;
    cameraBuffer.height = window.height;

    HmckMat4 translation = HmckTranslate(cloudTranslation);
    pushConstants.cloudTransform = translation;
}

void CloudRenderer::draw() {
    if (const auto commandBuffer = renderContext.beginFrame()) {
        const int frameIndex = renderContext.getFrameIndex();

        deviceStorage.getBuffer(cameraBuffers[frameIndex])->writeToBuffer(&cameraBuffer);
        deviceStorage.getBuffer(cloudBuffers[frameIndex])->writeToBuffer(&cloudBuffer);

        renderContext.beginSwapChainRenderPass(commandBuffer);

        deviceStorage.bindVertexBuffer(vertexBuffer, indexBuffer, commandBuffer);
        compositionPass.pipeline->bind(commandBuffer);

        deviceStorage.bindDescriptorSet(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            compositionPass.pipeline->graphicsPipelineLayout,
            0, 1,
            descriptorSets[frameIndex],
            0,
            nullptr);

        vkCmdPushConstants(commandBuffer, compositionPass.pipeline->graphicsPipelineLayout,
                           VK_SHADER_STAGE_ALL_GRAPHICS, 0,
                           sizeof(PushConstants), &pushConstants);

        // draw single triangle to trigger fullscreen vertex shader
        vkCmdDraw(commandBuffer, 3, 1, 0, 0);

        ui.beginUserInterface();
        drawUi();
        ui.showDebugStats(cameraBuffer.view, frameIndex);
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
    ImGui::DragFloat3("Position", &cloudTranslation.Elements[0], 0.01f);
    ImGui::End();

    ImGui::Begin("Camera", (bool *) false, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::DragFloat("Radius", &radius, 0.01f );
    ImGui::DragFloat("Azimuth", &azimuth, 0.01f );
    ImGui::DragFloat("Elevation", &elevation, 0.01f);
    ImGui::End();
}
