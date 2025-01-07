#include "Renderer.h"
#include "MarchingCubes.h"

Renderer::Renderer(): window{instance, "Marching cubes", 1920, 1080},
                      device(instance, window.getSurface()) {
    init();
    loadSph();
}

void Renderer::draw() {
    float elapsedTime = 0.f;
    auto currentTime = std::chrono::high_resolution_clock::now();

    int frameCount = 0;

    while (!window.shouldClose()) {
        window.pollEvents();

        auto newTime = std::chrono::high_resolution_clock::now();
        float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
        currentTime = newTime;
        elapsedTime += frameTime;


        if (window.getKeyState(KEY_A) == Hammock::KeyState::DOWN) azimuth += 1.f * frameTime;
        if (window.getKeyState(KEY_D) == Hammock::KeyState::DOWN) azimuth -= 1.f * frameTime;
        if (window.getKeyState(KEY_W) == Hammock::KeyState::DOWN) elevation += 1.f * frameTime;
        if (window.getKeyState(KEY_S) == Hammock::KeyState::DOWN) elevation -= 1.f * frameTime;
        if (window.getKeyState(KEY_UP) == Hammock::KeyState::DOWN) radius -= 1.f * frameTime;
        if (window.getKeyState(KEY_DOWN) == Hammock::KeyState::DOWN) radius += 1.f * frameTime;
        if (orbit) {
            cameraPosition = Hammock::Math::orbitalPosition(cameraTarget, HmckClamp(0.f, radius, 1000.0f), azimuth,
                                                            elevation);
        }


        if (const auto commandBuffer = renderContext.beginFrame()) {
            const int frameIndex = renderContext.getFrameIndex();


            renderContext.beginSwapChainRenderPass(commandBuffer);

            if (loop) {
                if (vertexBufferId >= vertexBuffers.size()) {
                    vertexBufferId = 0;
                }
            }

            deviceStorage.bindVertexBuffer(vertexBuffers[vertexBufferId], commandBuffer);

            pipeline->bind(commandBuffer);

            HmckMat4 m0 = HmckMat4{
                1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f
            };
            HmckMat4 view = Hammock::Projection().view(cameraPosition, cameraTarget,
                                                       Hammock::Projection().upPosY());
            HmckMat4 perspective = Hammock::Projection().
                    perspective(45.f, renderContext.getAspectRatio(), 0.1f, 1000.f);

            HmckMat4 rotation = HmckMat4{
                1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 0.0f, -1.0f, 0.0f,
                0.0f, 1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f
            };
            HmckMat4 translation = HmckTranslate({-20.f, -20.f, 20.f});
            HmckMat4 scale = HmckScale({1 / cubeSize, 1 / cubeSize, 1 / cubeSize});
            HmckMat4 model = HmckMul(translation, HmckMul(rotation, HmckMul(scale, m0)));

            bufferData.mvp = HmckMul(perspective, HmckMul(view, model));
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
            vkCmdPushConstants(commandBuffer, pipeline->graphicsPipelineLayout,
                               VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT, 0,
                               sizeof(PushData), &pushData);


            vkCmdDraw(commandBuffer, vertices[vertexBufferId].size(), 1, 0, 0);


            if (loop) {
                frameCount++;
                if (frameCount >= framing) {
                    vertexBufferId++;
                    frameCount=0;
                }
            }


            ui.beginUserInterface();
            drawUi();
            ui.endUserInterface(commandBuffer);

            renderContext.endRenderPass(commandBuffer);
            renderContext.endFrame();
        }
    }
    device.waitIdle();
}

void Renderer::loadSph() {
    auto files = Hammock::Filesystem::ls(assetPath("sph/"));

    int f = 0;
    for (const auto &file: files) {
        if (!file.contains(".bin")) { continue; }

        // Load particles
        std::vector<Particle> particles;
        Hammock::Logger::log(Hammock::LOG_LEVEL_DEBUG, "Loading particles...\n");
        if (loadParticles(file, particles)) {
            Hammock::Logger::log(Hammock::LOG_LEVEL_DEBUG, "Loaded %d particles\n", particles.size());
        } else {
            Hammock::Logger::log(Hammock::LOG_LEVEL_ERROR, "Failed to load particles\n");
            throw std::runtime_error("Failed to load particles");
        }


        // Create a scalar field
        float fieldSize = 1.0f; // Total domain size from -0.5 to 0.5
        float gridSize = fieldSize / 40.0f; // To get 40x40x40 grid
        Hammock::Logger::log(Hammock::LOG_LEVEL_DEBUG, "Creating scalar field...\n");
        auto scalarField = createScalarField(particles, gridSize, fieldSize);
        Hammock::Logger::log(Hammock::LOG_LEVEL_DEBUG, "Created scalar field of %d x %d x %d\n", scalarField.size(),
                             scalarField[0].size(), scalarField[0][0].size());

        // marching cubes
        Hammock::Logger::log(Hammock::LOG_LEVEL_DEBUG, "Marching cubes...\n");
        std::vector<Hammock::Triangle> triangles = marchingCubes(scalarField, isovalue, cubeSize);
        Hammock::Logger::log(Hammock::LOG_LEVEL_DEBUG, "Marched surface of %d triangles\n", triangles.size());

        // create buffers
        std::vector<Hammock::Vertex> v;
        for (auto &triangle: triangles) {
            HmckVec3 v1 = HmckSub(triangle.v1.position, triangle.v0.position);
            HmckVec3 v2 = HmckSub(triangle.v2.position, triangle.v0.position);

            HmckVec3 normal = HmckNorm(HmckMul(HmckCross(v1, v2), {1.0f, -1.0f, 1.f}));
            triangle.v0.normal = normal;
            triangle.v1.normal = normal;
            triangle.v2.normal = normal;

            v.push_back(triangle.v0);
            v.push_back(triangle.v1);
            v.push_back(triangle.v2);
        }

        vertices.push_back(v);


        vertexBuffers.push_back( deviceStorage.createVertexBuffer({
            .vertexSize = sizeof(vertices[f][0]),
            .vertexCount = static_cast<uint32_t>(vertices[f].size()),
            .data = static_cast<void *>(vertices[f].data())
        }));


        Hammock::Logger::log(Hammock::LOG_LEVEL_DEBUG, "Vertex buffer created with %d vertices \n",
                             vertices[f].size());

        f++;
    }

    Hammock::Logger::log(Hammock::LOG_LEVEL_DEBUG, "Created %d vertex buffers \n",
                             vertexBuffers.size());
}

void Renderer::init() {
    // Resources
    descriptorSets.resize(Hammock::SwapChain::MAX_FRAMES_IN_FLIGHT);
    buffers.resize(Hammock::SwapChain::MAX_FRAMES_IN_FLIGHT);

    descriptorSetLayout = deviceStorage.createDescriptorSetLayout({
        .bindings = {
            {
                .binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS
            },
        }
    });

    for (int i = 0; i < Hammock::SwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        buffers[i] = deviceStorage.createBuffer({
            .instanceSize = sizeof(BufferData),
            .instanceCount = 1,
            .usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .memoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        });
    }

    for (int i = 0; i < Hammock::SwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        auto fbufferInfo = deviceStorage.getBuffer(buffers[i])->descriptorInfo();
        descriptorSets[i] = deviceStorage.createDescriptorSet({
            .descriptorSetLayout = descriptorSetLayout,
            .bufferWrites = {{0, fbufferInfo}}
        });
    }
    // pipeline
    pipeline = Hammock::GraphicsPipeline::createGraphicsPipelinePtr({
        .debugName = "marching_cubes",
        .device = device,
        .VS
        {
            .byteCode = Hammock::Filesystem::readFile(compiledShaderPath("marching_cubes.vert")),
            .entryFunc = "main"
        },
        .FS
        {
            .byteCode = Hammock::Filesystem::readFile(compiledShaderPath("marching_cubes.frag")),
            .entryFunc = "main"
        },
        .descriptorSetLayouts =
        {
            deviceStorage.getDescriptorSetLayout(descriptorSetLayout).getDescriptorSetLayout()
        },
        .pushConstantRanges{
            {
                .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                .offset = 0,
                .size = sizeof(PushData)
            }
        },
        .graphicsState
        {
            .depthTest = VK_TRUE,
            .cullMode = VK_CULL_MODE_NONE,
            .blendAtaAttachmentStates{},
            .vertexBufferBindings
            {
                .vertexBindingDescriptions = Hammock::Vertex::vertexInputBindingDescriptions(),
                .vertexAttributeDescriptions = Hammock::Vertex::vertexInputAttributeDescriptions()
            }
        },
        .renderPass = renderContext.getSwapChainRenderPass()
    });
}

void Renderer::drawUi() {
    const ImGuiWindowFlags window_flags = ImGuiWindowFlags_AlwaysAutoResize;
    ImGui::Begin("Marching cubes editor", (bool *) false, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("Edit rendering properties", window_flags);

    ImGui::DragFloat3("Camera position", &cameraPosition.Elements[0], 0.1f);
    ImGui::DragFloat3("Camera target", &cameraTarget.Elements[0], 0.1f);
    ImGui::DragFloat3("Light position", &bufferData.lightPos.Elements[0], 0.1f);
    ImGui::Checkbox("Orbital camera", &orbit);
    ImGui::Checkbox("Loop", &loop);
    ImGui::DragFloat("Azimuth", &azimuth, 0.01f);
    ImGui::DragFloat("Elevation", &elevation, 0.01f);
    ImGui::DragFloat("Radius", &radius);
    ImGui::DragInt("Animation speed", &framing);
    ImGui::End();
}
