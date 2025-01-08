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
    Logger::log(LOG_LEVEL_DEBUG, "Generating noise texture ... ");
    ScopedMemory noiseData(PerlinNoise3D(69420).generateNoiseVolume(128, 128, 128));

    noiseVolumeHandle = deviceStorage.createTexture3D({
        .buffer = noiseData.get(),
        .instanceSize = sizeof(float),
        .width = 128,
        .height = 128,
        .channels = 1,
        .depth = 128,
        .format = VK_FORMAT_R32_SFLOAT,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    });

    Logger::log(LOG_LEVEL_DEBUG, "DONE\n");

    Loader(geometry, device, deviceStorage).loadglTF(assetPath("models/Sphere/Sphere.glb"));

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
                .stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS
            },
            {
                .binding = 1, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS
            },
        }
    });

    for (int i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        buffers[i] = deviceStorage.createBuffer({
            .instanceSize = sizeof(BufferData),
            .instanceCount = 1,
            .usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .memoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        });
    }

    for (int i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        auto fbufferInfo = deviceStorage.getBuffer(buffers[i])->descriptorInfo();
        auto imageInfo = deviceStorage.getTexture3DDescriptorImageInfo(noiseVolumeHandle);
        descriptorSets[i] = deviceStorage.createDescriptorSet({
            .descriptorSetLayout = descriptorSetLayout,
            .bufferWrites = {{0, fbufferInfo}},
            .imageWrites = {{1, imageInfo}}
        });
    }
}

void CloudRenderer::preparePipelines() {
    compositionPass.pipeline = GraphicsPipeline::createGraphicsPipelinePtr({
        .debugName = "Composition pass",
        .device = device,
        .VS
        {.byteCode = Filesystem::readFile(compiledShaderPath("volume.vert")),},
        .FS
        {.byteCode = Filesystem::readFile(compiledShaderPath("volume.frag")),},
        .descriptorSetLayouts = {
            deviceStorage.getDescriptorSetLayout(descriptorSetLayout).getDescriptorSetLayout()
        },
        .pushConstantRanges{
            {
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                .offset = 0,
                .size = sizeof(PushData)
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

    // float speed = 0.001f;
    // if(window.getKeyState(KEY_A) == KeyState::DOWN) azimuth += speed * frameTime;
    // if(window.getKeyState(KEY_D) == KeyState::DOWN) azimuth -= speed * frameTime;
    // if(window.getKeyState(KEY_W) == KeyState::DOWN) elevation += speed * frameTime;
    // if(window.getKeyState(KEY_S) == KeyState::DOWN) elevation -= speed * frameTime;
    // if(window.getKeyState(KEY_UP) == KeyState::DOWN) radius -= speed * frameTime;
    // if(window.getKeyState(KEY_DOWN) == KeyState::DOWN) radius += speed * frameTime;
    // bufferData.cameraPosition.XYZ = Math::orbitalPosition(HmckVec3{.0f,.0f,.0f}, radius, azimuth, elevation);

    bufferData.projection = Projection().perspective(45.0f, renderContext.getAspectRatio(), 0.1f, 64.0f);
    bufferData.view = Projection().view(bufferData.cameraPosition.XYZ, {0.f, 0.f, 0.f}, Projection().upPosY());
}

void CloudRenderer::draw() {
    if (const auto commandBuffer = renderContext.beginFrame()) {
        const int frameIndex = renderContext.getFrameIndex();

        deviceStorage.getBuffer(buffers[frameIndex])->writeToBuffer(&bufferData);

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

        vkCmdPushConstants(commandBuffer, compositionPass.pipeline->graphicsPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                       sizeof(PushData), &pushData);

        // draw single triangle to trigger fullscreen vertex shader
        vkCmdDrawIndexed(commandBuffer, geometry.indices.size(), 1, 0, 0,0);

        ui.beginUserInterface();
        drawUi();
        ui.showDebugStats(bufferData.view, frameIndex);
        ui.endUserInterface(commandBuffer);

        renderContext.endRenderPass(commandBuffer);
        renderContext.endFrame();
    }
}

void CloudRenderer::drawUi() {
    ImGui::Begin("Property editor", (bool *) false, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::DragFloat3("Camera position", &bufferData.cameraPosition.Elements[0], 0.1f);
    ImGui::DragFloat3("Light direction", &bufferData.lightDirection.Elements[0], 0.1f);
    ImGui::ColorEdit4("Light color", &bufferData.lightColor.Elements[0]);
    ImGui::DragFloat("Step size", &pushData.stepSize, 0.001f, 0.f);
    ImGui::DragInt("Max steps", &pushData.maxSteps, 1, 0);
    ImGui::DragFloat("Minimal density threshold", &pushData.minDensityThreshold, 0.001f, 0.f);
    ImGui::End();
}
