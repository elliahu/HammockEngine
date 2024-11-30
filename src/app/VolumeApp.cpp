#include "VolumeApp.h"

#include "controllers/KeyboardMovementController.h"
#include "core/HmckRenderer.h"
#include "scene/HmckGLTF.h"
#include "utils/HmckScopedMemory.h"

Hmck::VolumeApp::VolumeApp() {
    load();
}

void Hmck::VolumeApp::run() {
    Renderer renderer{window, device};

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
            resources.getDescriptorSetLayout(descriptorSetLayout).getDescriptorSetLayout()
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
        .renderPass = renderer.getSwapChainRenderPass()
    });

    // camera and movement
    auto camera = std::make_shared<Camera>();
    camera->transform.translation = {0.f, 0.f, -6.f};
    scene->add(camera, scene->getRoot());
    scene->setActiveCamera(camera->id);
    scene->getActiveCamera()->setPerspectiveProjection(glm::radians(50.0f), renderer.getAspectRatio(), 0.1f, 1000.f);

    KeyboardMovementController cameraController{};
    const UserInterface ui{device, renderer.getSwapChainRenderPass(), window};

    float elapsedTime = 0.f;
    auto currentTime = std::chrono::high_resolution_clock::now();

    while (!window.shouldClose()) {
        window.pollEvents();

        // gameloop timing
        auto newTime = std::chrono::high_resolution_clock::now();
        float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
        currentTime = newTime;
        elapsedTime += frameTime;

        // camera
        cameraController.moveInPlaneXZ(window, frameTime, scene->getActiveCamera());
        scene->getActiveCamera()->update();


        // start a new frame
        if (const auto commandBuffer = renderer.beginFrame()) {
            const int frameIndex = renderer.getFrameIndex();

            vkCmdSetDepthBias(
                commandBuffer,
                1.25f,
                0.0f,
                1.75f);

            renderer.beginSwapChainRenderPass(commandBuffer);

            draw(frameIndex, elapsedTime, commandBuffer); {
                ui.beginUserInterface();
                this->ui();
                ui.showDebugStats(scene->getActiveCamera());
                ui.showWindowControls();
                ui.endUserInterface(commandBuffer);
            }

            renderer.endRenderPass(commandBuffer);
            renderer.endFrame();
        }

        vkDeviceWaitIdle(device.device());
    }

    destroy();
}

void Hmck::VolumeApp::load() {
    // Scene
    Scene::SceneCreateInfo info = {
        .device = device,
        .memory = resources,
        .name = "3D texture rendering",
    };
    scene = std::make_unique<Scene>(info);

    GltfLoader gltfloader{device, resources, scene};
    gltfloader.load("../data/models/Sphere/Sphere.glb");

    // Resources
    descriptorSets.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
    buffers.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);

    descriptorSetLayout = resources.createDescriptorSetLayout({
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
        buffers[i] = resources.createBuffer({
            .instanceSize = sizeof(BufferData),
            .instanceCount = 1,
            .usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .memoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        });


    { // Load the volume texture
        int w, h, c, d;
        const auto volumeImages = Filesystem::ls("../data/textures/volumes/female_ankle");
        ScopedMemory volumeData{Filesystem::readVolume(volumeImages, w, h, c, d, Filesystem::ImageFormat::R32_SFLOAT, Filesystem::ReadImageLoadingFlags::FLIP_Y)};
        bufferData.textureDim = {
            static_cast<float>(w), static_cast<float>(h), static_cast<float>(d), static_cast<float>(c)
        };
        texture = resources.createTexture3DFromBuffer({
            .buffer = volumeData.get(),
            .instanceSize = sizeof(float),
            .width = static_cast<uint32_t>(w), .height = static_cast<uint32_t>(h),
            .channels = static_cast<uint32_t>(c), .depth = static_cast<uint32_t>(d),
            .format = VK_FORMAT_R32_SFLOAT,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        });
    }

    for (int i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        auto fbufferInfo = resources.getBuffer(buffers[i])->descriptorInfo();
        auto imageInfo = resources.getTexture3DDescriptorImageInfo(texture);
        descriptorSets[i] = resources.createDescriptorSet({
            .descriptorSetLayout = descriptorSetLayout,
            .bufferWrites = {{0, fbufferInfo}},
            .imageWrites = {{1, imageInfo}}
        });
    }

    vertexBuffer = resources.createVertexBuffer({
        .vertexSize = sizeof(scene->vertices[0]),
        .vertexCount = static_cast<uint32_t>(scene->vertices.size()),
        .data = static_cast<void *>(scene->vertices.data())
    });

    indexBuffer = resources.createIndexBuffer({
        .indexSize = sizeof(scene->indices[0]),
        .indexCount = static_cast<uint32_t>(scene->indices.size()),
        .data = static_cast<void *>(scene->indices.data())
    });
}

void Hmck::VolumeApp::draw(int frameIndex, float elapsedTime, VkCommandBuffer commandBuffer) {
    resources.bindVertexBuffer(vertexBuffer, indexBuffer, commandBuffer);
    pipeline->bind(commandBuffer);

    bufferData.projection = scene->getActiveCamera()->getProjection();
    bufferData.view = scene->getActiveCamera()->getView();
    bufferData.inverseView = scene->getActiveCamera()->getInverseView();
    resources.getBuffer(buffers[frameIndex])->writeToBuffer(&bufferData);

    resources.bindDescriptorSet(
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
    resources.destroyTexture3D(texture);

    for (auto &uniformBuffer: buffers)
        resources.destroyBuffer(uniformBuffer);

    resources.destroyDescriptorSetLayout(descriptorSetLayout);

    resources.destroyBuffer(vertexBuffer);
    resources.destroyBuffer(indexBuffer);
}

void Hmck::VolumeApp::ui() {
    const ImGuiWindowFlags window_flags = ImGuiWindowFlags_AlwaysAutoResize;
    ImGui::Begin("Volume editor", (bool *) false, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("Edit rendering properties", window_flags);

   float baseSkyColor[4] = {
        bufferData.baseSkyColor.x, bufferData.baseSkyColor.y, bufferData.baseSkyColor.z, bufferData.baseSkyColor.w
    };
    ImGui::ColorEdit4("Base sky color", &baseSkyColor[0]);
    bufferData.baseSkyColor = {baseSkyColor[0], baseSkyColor[1], baseSkyColor[2], baseSkyColor[3]};

    ImGui::DragFloat("Max steps", &pushData.maxSteps, 1.0f, 0.001f);
    ImGui::DragFloat("March size", &pushData.marchSize, 0.0001f, 0.0001f,100.f, "%.5f");
    ImGui::DragFloat("Air threshold", &pushData.airTrheshold, 0.01f, 0.001f, 1.0f);
    ImGui::DragFloat("Tissue threshold", &pushData.tissueThreshold, 0.01f, 0.001f, 1.0f);
    ImGui::DragFloat("Fat threshold", &pushData.fatThreshold, 0.01f, 0.001f, 1.0f);

    float tissueColor[4] = {
        bufferData.tissueColor.x, bufferData.tissueColor.y, bufferData.tissueColor.z, bufferData.tissueColor.w
    };
    ImGui::ColorEdit4("Tissue color", &tissueColor[0]);
    bufferData.tissueColor = {tissueColor[0], tissueColor[1], tissueColor[2], tissueColor[3]};

    float fatColor[4] = {
        bufferData.fatColor.x, bufferData.fatColor.y, bufferData.fatColor.z, bufferData.fatColor.w
    };
    ImGui::ColorEdit4("Fat color", &fatColor[0]);
    bufferData.fatColor = {fatColor[0], fatColor[1], fatColor[2], fatColor[3]};

    float boneColor[4] = {
        bufferData.boneColor.x, bufferData.boneColor.y, bufferData.boneColor.z, bufferData.boneColor.w
    };
    ImGui::ColorEdit4("Bone color", &boneColor[0]);
    bufferData.boneColor = {boneColor[0], boneColor[1], boneColor[2], boneColor[3]};

    bool nDotL = pushData.nDotL == 1;
    ImGui::Checkbox("Blinn-phong", &nDotL);
    pushData.nDotL = (nDotL) ? 1 : 0;

    ImGui::End();
}
