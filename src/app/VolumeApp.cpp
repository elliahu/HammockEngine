#include "VolumeApp.h"

#include "core/HmckRenderContext.h"
#include "scene/HmckAssetDelivery.h"
#include "scene/HmckCamera.h"
#include "utils/HmckScopedMemory.h"

Hmck::VolumeApp::VolumeApp() {
    load();
}

void Hmck::VolumeApp::run() {
    pipeline = GraphicsPipeline::createGraphicsPipelinePtr({
        .debugName = "standard_forward_pass",
        .device = device,
        .VS
        {
            .byteCode = Hmck::Filesystem::readFile("../src/engine/shaders/compiled/fullscreen.vert.spv"),
            .entryFunc = "main"
        },
        .FS
        {
            .byteCode = Hmck::Filesystem::readFile("../src/engine/shaders/compiled/raymarch_3d_texture.frag.spv"),
            .entryFunc = "main"
        },
        .descriptorSetLayouts =
        {
            deviceStorage.getDescriptorSetLayout(descriptorSetLayout).getDescriptorSetLayout()
        },
        .pushConstantRanges{
            {
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                .offset = 0,
                .size = sizeof(PushData)
            }
        },
        .graphicsState
        {
            .depthTest = VK_TRUE,
            .depthTestCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
            .frontFace = VK_FRONT_FACE_CLOCKWISE,
            .blendAtaAttachmentStates{},
            .vertexBufferBindings
            {
                .vertexBindingDescriptions = Vertex::vertexInputBindingDescriptions(),
                .vertexAttributeDescriptions = Vertex::vertexInputAttributeDescriptions()
            }
        },
        .renderPass = renderContext.getSwapChainRenderPass()
    });


    const UserInterface ui{device, renderContext.getSwapChainRenderPass(), window};

    float elapsedTime = 0.f;
    auto currentTime = std::chrono::high_resolution_clock::now();

    while (!window.shouldClose()) {
        window.pollEvents();

        // gameloop timing
        auto newTime = std::chrono::high_resolution_clock::now();
        float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
        currentTime = newTime;
        elapsedTime += frameTime;


        // start a new frame
        if (const auto commandBuffer = renderContext.beginFrame()) {
            const int frameIndex = renderContext.getFrameIndex();

            vkCmdSetDepthBias(
                commandBuffer,
                1.25f,
                0.0f,
                1.75f);

            renderContext.beginSwapChainRenderPass(commandBuffer);

            draw(frameIndex, elapsedTime, commandBuffer); {
                ui.beginUserInterface();
                this->ui();
                ui.showWindowControls();
                ui.endUserInterface(commandBuffer);
            }

            renderContext.endRenderPass(commandBuffer);
            renderContext.endFrame();
        }

        vkDeviceWaitIdle(device.device());
    }

    destroy();
}

void Hmck::VolumeApp::load() {
    Loader(state, device, deviceStorage).loadglTF("../data/models/Sphere/Sphere.glb");


    // Resources
    descriptorSets.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
    buffers.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);

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

    for (int i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; i++)
        buffers[i] = deviceStorage.createBuffer({
            .instanceSize = sizeof(BufferData),
            .instanceCount = 1,
            .usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .memoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        }); {
        // Load the volume texture
        int w, h, c, d;
        const auto volumeImages = Filesystem::ls("../data/textures/volumes/female_ankle");
        ScopedMemory volumeData{
            Filesystem::readVolume(volumeImages, w, h, c, d, Filesystem::ImageFormat::R32_SFLOAT,
                                   Filesystem::ReadImageLoadingFlags::FLIP_Y)
        };
        bufferData.textureDim = {
            static_cast<float>(w), static_cast<float>(h), static_cast<float>(d), static_cast<float>(c)
        };
        texture = deviceStorage.createTexture3D({
            .buffer = volumeData.get(),
            .instanceSize = sizeof(float),
            .width = static_cast<uint32_t>(w), .height = static_cast<uint32_t>(h),
            .channels = static_cast<uint32_t>(c), .depth = static_cast<uint32_t>(d),
            .format = VK_FORMAT_R32_SFLOAT,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        });
    }

    for (int i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        auto fbufferInfo = deviceStorage.getBuffer(buffers[i])->descriptorInfo();
        auto imageInfo = deviceStorage.getTexture3DDescriptorImageInfo(texture);
        descriptorSets[i] = deviceStorage.createDescriptorSet({
            .descriptorSetLayout = descriptorSetLayout,
            .bufferWrites = {{0, fbufferInfo}},
            .imageWrites = {{1, imageInfo}}
        });
    }

    vertexBuffer = deviceStorage.createVertexBuffer({
        .vertexSize = sizeof(state.vertices[0]),
        .vertexCount = static_cast<uint32_t>(state.vertices.size()),
        .data = static_cast<void *>(state.vertices.data())
    });

    indexBuffer = deviceStorage.createIndexBuffer({
        .indexSize = sizeof(state.indices[0]),
        .indexCount = static_cast<uint32_t>(state.indices.size()),
        .data = static_cast<void *>(state.indices.data())
    });
}

void Hmck::VolumeApp::draw(int frameIndex, float elapsedTime, VkCommandBuffer commandBuffer) {
    deviceStorage.bindVertexBuffer(vertexBuffer, indexBuffer, commandBuffer);
    pipeline->bind(commandBuffer);

    bufferData.projection = Projection().perspective(45.0f, renderContext.getAspectRatio(), 0.1f, 64.0f);
    bufferData.view = Projection().view(HmckVec3{0.0f, 0.0f, -2.0f}, HmckVec3{0.0f, 0.0f, 0.0f},
                                                         HmckVec3{0.0f, -1.0f, 0.0f});
    bufferData.inverseView = Projection().inverseView(HmckVec3{0.0f, 0.0f, -2.0f},
                                                                       HmckVec3{0.0f, 0.0f, 0.0f},
                                                                       HmckVec3{0.0f, -1.0f, 0.0f});
    deviceStorage.getBuffer(buffers[frameIndex])->writeToBuffer(&bufferData);

    deviceStorage.bindDescriptorSet(
        commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline->graphicsPipelineLayout,
        0, 1,
        descriptorSets[frameIndex],
        0,
        nullptr);

    pushData.elapsedTime = elapsedTime;

    vkCmdPushConstants(commandBuffer, pipeline->graphicsPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                       sizeof(PushData), &pushData);

    vkCmdDraw(commandBuffer, 3, 1, 0, 0);
}

void Hmck::VolumeApp::destroy() {
    deviceStorage.destroyTexture3D(texture);

    for (auto &uniformBuffer: buffers)
        deviceStorage.destroyBuffer(uniformBuffer);

    deviceStorage.destroyDescriptorSetLayout(descriptorSetLayout);

    deviceStorage.destroyBuffer(vertexBuffer);
    deviceStorage.destroyBuffer(indexBuffer);
}

void Hmck::VolumeApp::ui() {
    const ImGuiWindowFlags window_flags = ImGuiWindowFlags_AlwaysAutoResize;
    ImGui::Begin("Volume editor", (bool *) false, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("Edit rendering properties", window_flags);

    float baseSkyColor[4] = {
        bufferData.baseSkyColor.X, bufferData.baseSkyColor.Y, bufferData.baseSkyColor.Z, bufferData.baseSkyColor.W
    };
    ImGui::ColorEdit4("Base sky color", &baseSkyColor[0]);
    bufferData.baseSkyColor = {baseSkyColor[0], baseSkyColor[1], baseSkyColor[2], baseSkyColor[3]};

    ImGui::DragFloat("Max steps", &pushData.maxSteps, 1.0f, 0.001f);
    ImGui::DragFloat("March size", &pushData.marchSize, 0.0001f, 0.0001f, 100.f, "%.5f");
    ImGui::DragFloat("Air threshold", &pushData.airTrheshold, 0.01f, 0.001f, 1.0f);
    ImGui::DragFloat("Tissue threshold", &pushData.tissueThreshold, 0.01f, 0.001f, 1.0f);
    ImGui::DragFloat("Fat threshold", &pushData.fatThreshold, 0.01f, 0.001f, 1.0f);

    float tissueColor[4] = {
        bufferData.tissueColor.X, bufferData.tissueColor.Y, bufferData.tissueColor.Z, bufferData.tissueColor.W
    };
    ImGui::ColorEdit4("Tissue color", &tissueColor[0]);
    bufferData.tissueColor = {tissueColor[0], tissueColor[1], tissueColor[2], tissueColor[3]};

    float fatColor[4] = {
        bufferData.fatColor.X, bufferData.fatColor.Y, bufferData.fatColor.Z, bufferData.fatColor.W
    };
    ImGui::ColorEdit4("Fat color", &fatColor[0]);
    bufferData.fatColor = {fatColor[0], fatColor[1], fatColor[2], fatColor[3]};

    float boneColor[4] = {
        bufferData.boneColor.X, bufferData.boneColor.Y, bufferData.boneColor.Z, bufferData.boneColor.W
    };
    ImGui::ColorEdit4("Bone color", &boneColor[0]);
    bufferData.boneColor = {boneColor[0], boneColor[1], boneColor[2], boneColor[3]};

    bool nDotL = pushData.nDotL == 1;
    ImGui::Checkbox("Blinn-phong", &nDotL);
    pushData.nDotL = (nDotL) ? 1 : 0;

    ImGui::End();
}
