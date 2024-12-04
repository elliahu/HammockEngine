#include "RaymarchingDemoApp.h"
#include "io/HmckWindow.h"

#include "io/HmckUserInterface.h"
#include "scene/HmckCamera.h"
#include "controllers/KeyboardMovementController.h"
#include "resources/HmckBuffer.h"
#include "resources/HmckDescriptors.h"
#include "scene/HmckGLTF.h"
#include "core/HmckRenderer.h"
#include "utils/HmckScopedMemory.h"

Hmck::RaymarchingDemoApp::RaymarchingDemoApp() {
    load();
}

void Hmck::RaymarchingDemoApp::run() {
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
            .byteCode = Hmck::Filesystem::readFile("../src/engine/shaders/compiled/raymarch_pretty.frag.spv"),
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
                .vertexBindingDescriptions =
                {
                    {
                        .binding = 0,
                        .stride = sizeof(Vertex),
                        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
                    }
                },
                .vertexAttributeDescriptions =
                {
                    {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)},
                    {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color)},
                    {2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)},
                    {3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)},
                    {4, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, tangent)}
                }
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

void Hmck::RaymarchingDemoApp::load() {
    Scene::SceneCreateInfo info = {
        .device = device,
        .memory = resources,
        .name = "Volumetric scene",
    };
    scene = std::make_unique<Scene>(info);

    GltfLoader gltfloader{device, resources, scene};
    gltfloader.load("../data/models/Sphere/Sphere.glb");

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


    {
        int w,h,c;
        ScopedMemory noiseData{Filesystem::readImage("../data/noise/noise2.png",w,h,c)};
        noiseTexture = resources.createTexture2D({
            .buffer = noiseData.get(),
            .instanceSize = sizeof(float),
            .width = static_cast<uint32_t>(w), .height = static_cast<uint32_t>(h), .channels = static_cast<uint32_t>(c),
            .format = VK_FORMAT_R32G32B32A32_SFLOAT,
        });
    }


    for (int i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        auto fbufferInfo = resources.getBuffer(buffers[i])->descriptorInfo();
        auto imageInfo = resources.getTexture2DDescriptorImageInfo(noiseTexture);
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

void Hmck::RaymarchingDemoApp::draw(int frameIndex, float elapsedTime, VkCommandBuffer commandBuffer) {
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

void Hmck::RaymarchingDemoApp::destroy() {
    resources.destroyTexture2D(noiseTexture);

    for (auto &uniformBuffer: buffers)
        resources.destroyBuffer(uniformBuffer);

    resources.destroyDescriptorSetLayout(descriptorSetLayout);

    resources.destroyBuffer(vertexBuffer);
    resources.destroyBuffer(indexBuffer);
}

void Hmck::RaymarchingDemoApp::ui() {
    const ImGuiWindowFlags window_flags = ImGuiWindowFlags_AlwaysAutoResize;
    ImGui::Begin("Cloud editor", (bool *) false, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("Edit cloud properties", window_flags);

    float sunPosition[4] = {
        bufferData.sunPosition.x, bufferData.sunPosition.y, bufferData.sunPosition.z, bufferData.sunPosition.w
    };
    ImGui::DragFloat4("Sun position", &sunPosition[0], 0.1f);
    bufferData.sunPosition = {sunPosition[0], sunPosition[1], sunPosition[2], sunPosition[3]};

    float sunColor[4] = {bufferData.sunColor.x, bufferData.sunColor.y, bufferData.sunColor.z, bufferData.sunColor.w};
    ImGui::ColorEdit4("Sun color", &sunColor[0]);
    bufferData.sunColor = {sunColor[0], sunColor[1], sunColor[2], sunColor[3]};

    float baseSkyColor[4] = {
        bufferData.baseSkyColor.x, bufferData.baseSkyColor.y, bufferData.baseSkyColor.z, bufferData.baseSkyColor.w
    };
    ImGui::ColorEdit4("Base sky color", &baseSkyColor[0]);
    bufferData.baseSkyColor = {baseSkyColor[0], baseSkyColor[1], baseSkyColor[2], baseSkyColor[3]};

    float gradientSkyColor[4] = {
        bufferData.gradientSkyColor.x, bufferData.gradientSkyColor.y, bufferData.gradientSkyColor.z,
        bufferData.gradientSkyColor.w
    };
    ImGui::ColorEdit4("Gradient sky color", &gradientSkyColor[0]);
    bufferData.gradientSkyColor = {gradientSkyColor[0], gradientSkyColor[1], gradientSkyColor[2], gradientSkyColor[3]};

    ImGui::DragFloat("Max steps", &pushData.maxSteps, 1.0f, 0.001f);
    ImGui::DragFloat("March size", &pushData.marchSize, 0.01f, 0.001f);

    ImGui::End();
}