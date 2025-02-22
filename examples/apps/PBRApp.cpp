#include "PBRApp.h"

Hammock::PBRApp::PBRApp() {
    load();
}

Hammock::PBRApp::~PBRApp() {
    // No need for cleanup as all allocated resources are cleaned automatically
}

void Hammock::PBRApp::run() {
    RenderContext renderContext{window, device};
    init();
    createPipelines(renderContext);
    UserInterface ui{device, renderContext.getSwapChainRenderPass(), deviceStorage.getDescriptorPool(), window};


    int r = 0;
    for (auto &mesh: geometry.renderMeshes) {
        globalBuffer.baseColorFactors[r] = HmckVec4{mesh.baseColorFactor, 1.0f};
        globalBuffer.metallicRoughnessAlphaCutOffFactors[r] =  HmckVec4{mesh.metallicRoughnessAlphaCutOffFactor};
        globalBuffer.baseColorTextureIndexes[r].value = mesh.baseColorTextureIndex;
        globalBuffer.normalTextureIndexes[r].value = mesh.normalTextureIndex;
        globalBuffer.metallicRoughnessTextureIndexes[r].value = mesh.metallicRoughnessTextureIndex;
        globalBuffer.occlusionTextureIndexes[r].value = mesh.occlusionTextureIndex;
        globalBuffer.visibilityFlags[r].value = mesh.visibilityFlags;
        r++;
    }
    for (int i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        deviceStorage.getBuffer(globalDescriptors.buffers[i])->writeToBuffer(&globalBuffer);
    }



    auto currentTime = std::chrono::high_resolution_clock::now();
    while (!window.shouldClose()) {
        window.pollEvents();


        // gameloop timing
        auto newTime = std::chrono::high_resolution_clock::now();
        float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
        currentTime = newTime;


        // start a new frame
        if (const auto commandBuffer = renderContext.beginFrame()) {
            const int frameIndex = renderContext.getFrameIndex();


            // do per frame calculations

            if(window.getKeyState(KEY_A) == KeyState::DOWN) azimuth += 1.f * frameTime;
            if(window.getKeyState(KEY_D) == KeyState::DOWN) azimuth -= 1.f * frameTime;
            if(window.getKeyState(KEY_W) == KeyState::DOWN) elevation += 1.f * frameTime;
            if(window.getKeyState(KEY_S) == KeyState::DOWN) elevation -= 1.f * frameTime;
            if(window.getKeyState(KEY_UP) == KeyState::DOWN) radius -= 1.f * frameTime;
            if(window.getKeyState(KEY_DOWN) == KeyState::DOWN) radius += 1.f * frameTime;
            HmckVec3 pos = Math::orbitalPosition(HmckVec3{.0f,.0f,.0f}, radius, azimuth, elevation);

            // write to projection buffer
            projectionBuffer.projectionMat = Projection().perspective(45.0f, renderContext.getAspectRatio(), 0.1f, 64.0f);
            projectionBuffer.viewMat = Projection().view(pos, HmckVec3{0.0f, 0.0f, 0.0f},
                                                         Projection().upPosY());
            projectionBuffer.inverseViewMat = Projection().inverseView(HmckVec3{0.0f, 0.0f, -2.0f},
                                                                       HmckVec3{0.0f, 0.0f, 0.0f},
                                                                       Projection().upPosY());
            deviceStorage.getBuffer(projectionDescriptors.buffers[frameIndex])->writeToBuffer(&projectionBuffer);


            deviceStorage.bindVertexBuffer(geomBuffers.vertexBuffer, geomBuffers.indexBuffer, commandBuffer);
            // Opaque geometry pass
            renderContext.beginRenderPass(opaqueGeometryPass.framebuffer, commandBuffer, {
                                         {.color = {0.0f, 0.0f, 0.0f, 0.0f}},
                                         {.color = {0.0f, 0.0f, 0.0f, 0.0f}},
                                         {.color = {0.0f, 0.0f, 0.0f, 0.0f}},
                                         {.color = {0.0f, 0.0f, 0.0f, 0.0f}},
                                         {.depthStencil = {1.0f, 0}}
                                     });


            // Environment pipeline
            environmentPass.pipeline->bind(commandBuffer);

            // Bind global buffer
            deviceStorage.bindDescriptorSet(
                commandBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                environmentPass.pipeline->graphicsPipelineLayout,
                globalDescriptors.binding, 1,
                globalDescriptors.descriptorSets[frameIndex],
                0, nullptr);

            // Bind projection buffer
            deviceStorage.bindDescriptorSet(
                commandBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                environmentPass.pipeline->graphicsPipelineLayout,
                projectionDescriptors.binding, 1,
                projectionDescriptors.descriptorSets[frameIndex],
                0, nullptr);

            // draw environment
            vkCmdDraw(commandBuffer, 3, 1, 0, 0);

            // opaque pass
            opaqueGeometryPass.pipeline->bind(commandBuffer);

            // Draw opaque, bind push block for each mesh
            for (int i = 0; i < geometry.renderMeshes.size(); i++) {
                Geometry::MeshInstance &mesh = geometry.renderMeshes[i];
                if ((mesh.visibilityFlags & Geometry::VisibilityFlags::VISIBILITY_VISIBLE) ==  Geometry::VisibilityFlags::VISIBILITY_NONE) { continue;}
                if (mesh.visibilityFlags & Geometry::VisibilityFlags::VISIBILITY_OPAQUE) {
                    meshPushBlock.modelMat = mesh.transform;
                    meshPushBlock.meshIndex = i;

                    vkCmdPushConstants(commandBuffer, opaqueGeometryPass.pipeline->graphicsPipelineLayout,
                                       VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT, 0,
                                       sizeof(PushBlockDataBuffer), &meshPushBlock);

                    vkCmdDrawIndexed(commandBuffer, mesh.indexCount, 1, mesh.firstIndex, 0,
                                     0);
                }
            }

            renderContext.endRenderPass(commandBuffer);

            // transparent pass
            renderContext.beginRenderPass(transparentGeometryPass.framebuffer, commandBuffer, {
                                         {.color = {0.0f, 0.0f, 0.0f, 0.0f}},
                                         {.color = {0.0f, 0.0f, 0.0f, 0.0f}},
                                         {.depthStencil = {1.0f, 0}}
                                     });
            transparentGeometryPass.pipeline->bind(commandBuffer);

            // Draw transparent, bind push block for each mesh
            for (int i = 0; i < geometry.renderMeshes.size(); i++) {
                Geometry::MeshInstance &mesh = geometry.renderMeshes[i];
                if ((mesh.visibilityFlags & Geometry::VisibilityFlags::VISIBILITY_VISIBLE) ==  Geometry::VisibilityFlags::VISIBILITY_NONE) { continue;}
                if (mesh.visibilityFlags & Geometry::VisibilityFlags::VISIBILITY_BLEND) {
                    meshPushBlock.modelMat = mesh.transform;
                    meshPushBlock.meshIndex = i;

                    vkCmdPushConstants(commandBuffer, opaqueGeometryPass.pipeline->graphicsPipelineLayout,
                                       VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT, 0,
                                       sizeof(PushBlockDataBuffer), &meshPushBlock);

                    vkCmdDrawIndexed(commandBuffer, mesh.indexCount, 1, mesh.firstIndex, 0,
                                     0);
                }
            }


            renderContext.endRenderPass(commandBuffer);

            // composition pass
            renderContext.beginSwapChainRenderPass(commandBuffer);

            compositionPass.pipeline->bind(commandBuffer);
            // bind composition buffer
            deviceStorage.bindDescriptorSet(
                commandBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                compositionPass.pipeline->graphicsPipelineLayout,
                compositionDescriptors.binding, 1,
                compositionDescriptors.descriptorSets[frameIndex],
                0, nullptr);

            // draw composed image on screen
            vkCmdDraw(commandBuffer, 3, 1, 0, 0);

            // draw ui
            ui.beginUserInterface();
            ui.showDebugStats(projectionBuffer.inverseViewMat, frameTime);
            ui.endUserInterface(commandBuffer);

            // end frame
            renderContext.endRenderPass(commandBuffer);
            renderContext.endFrame();
        }
    }
    device.waitIdle();
}

void Hammock::PBRApp::load() {
    // Load the meshes
    Loader(geometry, device, deviceStorage)
    .loadglTF(assetPath("models/helmet/helmet.glb"));
    //.loadglTF(dataRepository("models/helmet/DamagedHelmet.glb");

    int32_t w, h, c;
    ScopedMemory environmentData = ScopedMemory(Filesystem::readImage(
        assetPath("env/ibl/precomp/sunset/env.hdr"), w, h, c, Filesystem::ImageFormat::R32G32B32A32_SFLOAT));
    const uint32_t mipLevels = getNumberOfMipLevels(w, h);
    environment.environmentMap = deviceStorage.createTexture2D({
        .buffer = environmentData.get(),
        .instanceSize = sizeof(float),
        .width = static_cast<uint32_t>(w), .height = static_cast<uint32_t>(h), .channels = static_cast<uint32_t>(c),
        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
        .samplerInfo = {
            .filter = VK_FILTER_LINEAR,
            .addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .maxLod = static_cast<float>(mipLevels)
        }
    });

    Generator generator{};
    environment.prefilteredEnvMap = generator.generatePrefilteredMap(device, environment.environmentMap, deviceStorage);

    ScopedMemory irradianceData = ScopedMemory(Filesystem::readImage(
        assetPath("env/ibl/precomp/sunset/irradiance.hdr"), w, h, c, Filesystem::ImageFormat::R32G32B32A32_SFLOAT));
    environment.irradianceMap = deviceStorage.createTexture2D({
        .buffer = irradianceData.get(),
        .instanceSize = sizeof(float),
        .width = static_cast<uint32_t>(w), .height = static_cast<uint32_t>(h), .channels = static_cast<uint32_t>(c),
        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
    });

    ScopedMemory brdfLutData = ScopedMemory(Filesystem::readImage(assetPath("env/ibl/precomp/sunset/brdf_lut.hdr"), w,
                                                                  h, c, Filesystem::ImageFormat::R32G32_SFLOAT));
    environment.brdfLut = deviceStorage.createTexture2D({
        .buffer = brdfLutData.get(),
        .instanceSize = sizeof(float),
        .width = static_cast<uint32_t>(w), .height = static_cast<uint32_t>(h), .channels = static_cast<uint32_t>(c),
        .format = VK_FORMAT_R32G32_SFLOAT,
    });


    geomBuffers.vertexBuffer = deviceStorage.createVertexBuffer({
        .vertexSize = sizeof(geometry.vertices[0]),
        .vertexCount = static_cast<uint32_t>(geometry.vertices.size()),
        .data = static_cast<void *>(geometry.vertices.data())
    });

    geomBuffers.indexBuffer = deviceStorage.createIndexBuffer({
        .indexSize = sizeof(geometry.indices[0]),
        .indexCount = static_cast<uint32_t>(geometry.indices.size()),
        .data = static_cast<void *>(geometry.indices.data())
    });

    geomBuffers.numTriangles = static_cast<uint32_t>(geometry.vertices.size()) / 3;
    Logger::log(LOG_LEVEL_DEBUG, "Vertex buffer created. Number of vertices: %d, Number of triangles: %d\n",
                geometry.vertices.size(), geomBuffers.numTriangles);
    Logger::log(LOG_LEVEL_DEBUG, "Index buffer created. Number of indices: %d\n", geometry.indices.size());

    // free the host memory
    geometry.vertices.clear();
    geometry.indices.clear();
}

void Hammock::PBRApp::init() {
    // Setup global descriptor set
    // Descriptor image infos for all textures in the bindless texture array
    std::vector<VkDescriptorImageInfo> imageInfos{geometry.textures.size()};
    for (int im = 0; im < geometry.textures.size(); im++) {
        imageInfos[im] = deviceStorage.getTexture2D(geometry.textures[im])->descriptor;
    }
    globalDescriptors.descriptorSetLayout = deviceStorage.createDescriptorSetLayout({
        .bindings = {
            {
                // Uniform buffer with the state data of all meshes
                .binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS
            },
            {
                // Bindless texture array
                .binding = 1, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS, .count = 2000,
                .bindingFlags = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT
            },
            {
                // env map
                .binding = 2, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS
            },
            {
                // prefiltered env map
                .binding = 3, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS
            },
            {
                //  brdfLUT
                .binding = 4, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS
            },
            {
                //  irradiance map
                .binding = 5, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS
            },
        }
    });


    globalDescriptors.buffers.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
    globalDescriptors.descriptorSets.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        globalDescriptors.buffers[i] = deviceStorage.createBuffer({
            .instanceSize = sizeof(GlobalDataBuffer),
            .instanceCount = 1,
            .usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .memoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        });

        globalDescriptors.descriptorSets[i] = deviceStorage.createDescriptorSet({
            .descriptorSetLayout = globalDescriptors.descriptorSetLayout,
            .bufferWrites = {{0, deviceStorage.getBuffer(globalDescriptors.buffers[i])->descriptorInfo()}},
            .imageWrites = {
                {2, deviceStorage.getTexture2DDescriptorImageInfo(environment.environmentMap)},
                {3, deviceStorage.getTexture2DDescriptorImageInfo(environment.prefilteredEnvMap)},
                {4, deviceStorage.getTexture2DDescriptorImageInfo(environment.brdfLut)},
                {5, deviceStorage.getTexture2DDescriptorImageInfo(environment.irradianceMap)},
            },
            .imageArrayWrites = {{1, imageInfos}}
        });
    }


    // setup projection descriptors
    projectionDescriptors.descriptorSetLayout = deviceStorage.createDescriptorSetLayout({
        .bindings = {
            {
                // Uniform buffer with the state data of all meshes
                .binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS
            }
        }
    });
    projectionDescriptors.buffers.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
    projectionDescriptors.descriptorSets.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        projectionDescriptors.buffers[i] = deviceStorage.createBuffer({
            .instanceSize = sizeof(FrameDataBuffer),
            .instanceCount = 1,
            .usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .memoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        });
        projectionDescriptors.descriptorSets[i] = deviceStorage.createDescriptorSet({
            .descriptorSetLayout = projectionDescriptors.descriptorSetLayout,
            .bufferWrites = {{0, deviceStorage.getBuffer(projectionDescriptors.buffers[i])->descriptorInfo()}},
        });
    }

    // setup composition descriptors
    compositionDescriptors.descriptorSets.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
    compositionDescriptors.descriptorSetLayout = deviceStorage.createDescriptorSetLayout({
        .bindings = {
            {
                // position buffer
                .binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
            },
            {
                // albedo buffer
                .binding = 1, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
            },
            {
                // normal buffer
                .binding = 2, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
            },
            {
                // material (roughness, metalness, ao) buffer
                .binding = 3, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
            },
            {
                // accumulation buffer
                .binding = 4, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
            },
            {
                // transparency weight buffer
                .binding = 5, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
            },
        }
    });
}


void Hammock::PBRApp::createPipelines(const RenderContext &renderer) {
    // create piplines and framebuffers
    auto depthFormat = device.findSupportedFormat(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);


    opaqueGeometryPass.framebuffer = Framebuffer::createFramebufferPtr({
        .device = device,
        .width = IApp::WINDOW_WIDTH, .height = IApp::WINDOW_HEIGHT,
        .attachments{
            // 0 position
            {
                .width = IApp::WINDOW_WIDTH, .height = IApp::WINDOW_HEIGHT,
                .layerCount = 1,
                .format = VK_FORMAT_R16G16B16A16_SFLOAT,
                .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            },
            // 1 albedo
            {
                .width = IApp::WINDOW_WIDTH, .height = IApp::WINDOW_HEIGHT,
                .layerCount = 1,
                .format = VK_FORMAT_R8G8B8A8_UNORM,
                .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            },
            // 2 normal
            {
                .width = IApp::WINDOW_WIDTH, .height = IApp::WINDOW_HEIGHT,
                .layerCount = 1,
                .format = VK_FORMAT_R16G16B16A16_SFLOAT,
                .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            },
            // 3 rough, metal, ao
            {
                .width = IApp::WINDOW_WIDTH, .height = IApp::WINDOW_HEIGHT,
                .layerCount = 1,
                .format = VK_FORMAT_R16G16B16A16_SFLOAT,
                .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            },
            // 4 depth
            {
                .width = IApp::WINDOW_WIDTH, .height = IApp::WINDOW_HEIGHT,
                .layerCount = 1,
                .format = depthFormat,
                .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            },
        }
    });

    transparentGeometryPass.framebuffer = Framebuffer::createFrameBufferPtrWithSharedAttachments({
        .device = device,
        .width = IApp::WINDOW_WIDTH, .height = IApp::WINDOW_HEIGHT,
        .attachments{
            // 0 accumulator
            {
                .width = IApp::WINDOW_WIDTH, .height = IApp::WINDOW_HEIGHT,
                .layerCount = 1,
                .format = VK_FORMAT_R16G16B16A16_SFLOAT,
                .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            },
            // 1 transparency weight
            {
                .width = IApp::WINDOW_WIDTH, .height = IApp::WINDOW_HEIGHT,
                .layerCount = 1,
                .format = VK_FORMAT_R16_SFLOAT,
                .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            }
        }
    }, {std::ref(opaqueGeometryPass.framebuffer->attachments[4])});

    environmentPass.pipeline = GraphicsPipeline::createGraphicsPipelinePtr({
        .debugName = "skybox_pass",
        .device = device,
        .VS{
            .byteCode = Hammock::Filesystem::readFile(compiledShaderPath("fullscreen.vert")),
            .entryFunc = "main"
        },
        .FS{
            .byteCode = Hammock::Filesystem::readFile(compiledShaderPath("environment.frag")),
            .entryFunc = "main"
        },
        .descriptorSetLayouts = {
            deviceStorage.getDescriptorSetLayout(globalDescriptors.descriptorSetLayout).getDescriptorSetLayout(),
            deviceStorage.getDescriptorSetLayout(projectionDescriptors.descriptorSetLayout).getDescriptorSetLayout()
        },
        .pushConstantRanges{
            {
                .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                .offset = 0,
                .size = sizeof(PushBlockDataBuffer)
            }
        },
        .graphicsState{
            .depthTest = VK_FALSE,
            .cullMode = VK_CULL_MODE_NONE,
            .blendAtaAttachmentStates{
                Hammock::Init::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
                Hammock::Init::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
                Hammock::Init::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
                Hammock::Init::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
            },
            .vertexBufferBindings
            {
                .vertexBindingDescriptions = Vertex::vertexInputBindingDescriptions(),
                .vertexAttributeDescriptions = Vertex::vertexInputAttributeDescriptions(),
            }
        },
        .renderPass = opaqueGeometryPass.framebuffer->renderPass
    });

    opaqueGeometryPass.pipeline = GraphicsPipeline::createGraphicsPipelinePtr({
        .debugName = "gbuffer_pass",
        .device = device,
        .VS{
            .byteCode = Hammock::Filesystem::readFile(compiledShaderPath("gbuffer.vert")),
            .entryFunc = "main"
        },
        .FS{
            .byteCode = Hammock::Filesystem::readFile(compiledShaderPath("gbuffer.frag")),
            .entryFunc = "main"
        },
        .descriptorSetLayouts = {
            deviceStorage.getDescriptorSetLayout(globalDescriptors.descriptorSetLayout).getDescriptorSetLayout(),
            deviceStorage.getDescriptorSetLayout(projectionDescriptors.descriptorSetLayout).getDescriptorSetLayout()
        },
        .pushConstantRanges{
            {
                .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                .offset = 0,
                .size = sizeof(PushBlockDataBuffer)
            }
        },
        .graphicsState{
            .depthTest = VK_TRUE,
            .depthTestCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
            .blendAtaAttachmentStates{
                Hammock::Init::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
                Hammock::Init::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
                Hammock::Init::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
                Hammock::Init::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
            },
            .vertexBufferBindings
            {
                .vertexBindingDescriptions = Vertex::vertexInputBindingDescriptions(),
                .vertexAttributeDescriptions = Vertex::vertexInputAttributeDescriptions(),
            }
        },
        .renderPass = opaqueGeometryPass.framebuffer->renderPass
    });

    transparentGeometryPass.pipeline = GraphicsPipeline::createGraphicsPipelinePtr({
        .debugName = "transparency_pass",
        .device = device,
        .VS{
            .byteCode = Hammock::Filesystem::readFile(compiledShaderPath("gbuffer.vert")),
            .entryFunc = "main"
        },
        .FS{
            .byteCode = Hammock::Filesystem::readFile(compiledShaderPath("wboitbuffer.frag")),
            .entryFunc = "main"
        },
        .descriptorSetLayouts = {
            deviceStorage.getDescriptorSetLayout(globalDescriptors.descriptorSetLayout).getDescriptorSetLayout(),
            deviceStorage.getDescriptorSetLayout(projectionDescriptors.descriptorSetLayout).getDescriptorSetLayout()
        },
        .pushConstantRanges{
            {
                .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                .offset = 0,
                .size = sizeof(PushBlockDataBuffer)
            }
        },
        .graphicsState{
            .depthTest = VK_TRUE,
            .depthTestCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
            .blendAtaAttachmentStates{
                Hammock::Init::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
                Hammock::Init::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
            },
            .vertexBufferBindings
            {
                .vertexBindingDescriptions = Vertex::vertexInputBindingDescriptions(),
                .vertexAttributeDescriptions = Vertex::vertexInputAttributeDescriptions(),
            }
        },
        .renderPass = transparentGeometryPass.framebuffer->renderPass
    });

    // setup composition descriptor set here as it samples from gbuffer and transparency accumulator
    for (int i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        std::vector<VkDescriptorImageInfo> compositionImageInfos = {
            {
                .sampler = opaqueGeometryPass.framebuffer->sampler,
                .imageView = opaqueGeometryPass.framebuffer->attachments[0].view,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            },
            {
                .sampler = opaqueGeometryPass.framebuffer->sampler,
                .imageView = opaqueGeometryPass.framebuffer->attachments[1].view,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            },
            {
                .sampler = opaqueGeometryPass.framebuffer->sampler,
                .imageView = opaqueGeometryPass.framebuffer->attachments[2].view,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            },
            {
                .sampler = opaqueGeometryPass.framebuffer->sampler,
                .imageView = opaqueGeometryPass.framebuffer->attachments[3].view,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            },
            {
                .sampler = transparentGeometryPass.framebuffer->sampler,
                .imageView = transparentGeometryPass.framebuffer->attachments[0].view,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            },
            {
                .sampler = transparentGeometryPass.framebuffer->sampler,
                .imageView = transparentGeometryPass.framebuffer->attachments[1].view,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            },
        };

        compositionDescriptors.descriptorSets[i] = deviceStorage.createDescriptorSet({
            .descriptorSetLayout = compositionDescriptors.descriptorSetLayout,
            .imageArrayWrites = {{0, compositionImageInfos}},
        });
    }

    compositionPass.pipeline = GraphicsPipeline::createGraphicsPipelinePtr({
        .debugName = "deferred_pass",
        .device = device,
        .VS{
            .byteCode = Hammock::Filesystem::readFile(compiledShaderPath("fullscreen.vert")),
            .entryFunc = "main"
        },
        .FS{
            .byteCode = Hammock::Filesystem::readFile(compiledShaderPath("pbr_ibl_deferred.frag")),
            .entryFunc = "main"
        },
        .descriptorSetLayouts = {
            deviceStorage.getDescriptorSetLayout(globalDescriptors.descriptorSetLayout).getDescriptorSetLayout(),
            deviceStorage.getDescriptorSetLayout(projectionDescriptors.descriptorSetLayout).getDescriptorSetLayout(),
            deviceStorage.getDescriptorSetLayout(compositionDescriptors.descriptorSetLayout).getDescriptorSetLayout()
        },
        .pushConstantRanges{
            {
                .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                .offset = 0,
                .size = sizeof(PushBlockDataBuffer)
            }
        },
        .graphicsState{
            .depthTest = VK_FALSE,
            .cullMode = VK_CULL_MODE_NONE,
            .blendAtaAttachmentStates{},
            .vertexBufferBindings
            {
                .vertexBindingDescriptions = Vertex::vertexInputBindingDescriptions(),
                .vertexAttributeDescriptions = Vertex::vertexInputAttributeDescriptions(),
            }
        },
        .renderPass = renderer.getSwapChainRenderPass()
    });
}
