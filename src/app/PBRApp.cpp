#include "PBRApp.h"

#include "controllers/KeyboardMovementController.h"
#include "scene/HmckGLTF.h"
#include "scene/HmckLights.h"
#include "utils/HmckLogger.h"


Hmck::PBRApp::PBRApp() {
    load();
}

Hmck::PBRApp::~PBRApp() {
    for (const unsigned int sceneBuffer: sceneDescriptors.sceneBuffers)
        resources.destroyBuffer(sceneBuffer);

    for (const auto &[fst, snd]: entityDescriptors.entityBuffers)
        resources.destroyBuffer(entityDescriptors.entityBuffers[fst]);

    for (int i = 0; i < primitiveDescriptors.primitiveBuffers.size(); i++)
        resources.destroyBuffer(primitiveDescriptors.primitiveBuffers[i]);

    resources.destroyDescriptorSetLayout(sceneDescriptors.descriptorSetLayout);
    resources.destroyDescriptorSetLayout(entityDescriptors.descriptorSetLayout);
    resources.destroyDescriptorSetLayout(primitiveDescriptors.descriptorSetLayout);
    resources.destroyDescriptorSetLayout(gBufferDescriptors.descriptorSetLayout);

    resources.destroyBuffer(geometry.vertexBuffer);
    resources.destroyBuffer(geometry.indexBuffer);
}

void Hmck::PBRApp::run() {
    Renderer renderer{window, device};
    KeyboardMovementController cameraController{};
    UserInterface ui{device, renderer.getSwapChainRenderPass(), window};

    auto camera = std::make_shared<Camera>();
    scene->add(camera, scene->getRoot());
    scene->setActiveCamera(camera->id);
    scene->getActiveCamera()->flipY = true;
    scene->getActiveCamera()->setPerspectiveProjection(glm::radians(50.f), renderer.getAspectRatio(), 0.1f, 512.0f);

    std::shared_ptr<OmniLight> light = std::make_shared<OmniLight>();
    light->transform.translation = {0.0f, 2.0f, -8.0f};
    scene->add(light, scene->getRoot());

    init();
    createPipelines(renderer);

    SceneBufferData sceneData{
        .projection = scene->getActiveCamera()->getProjection(),
        .view = scene->getActiveCamera()->getView(),
        .inverseView = scene->getActiveCamera()->getInverseView()
    };


    auto currentTime = std::chrono::high_resolution_clock::now();
    while (!window.shouldClose()) {
        window.pollEvents();


        // gameloop timing
        auto newTime = std::chrono::high_resolution_clock::now();
        float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
        currentTime = newTime;

        // camera
        cameraController.moveInPlaneXZ(window, frameTime, scene->getActiveCamera());
        scene->getActiveCamera()->update();


        // start a new frame
        if (const auto commandBuffer = renderer.beginFrame()) {
            const int frameIndex = renderer.getFrameIndex();

            resources.bindVertexBuffer(geometry.vertexBuffer, geometry.indexBuffer, commandBuffer);
            renderer.beginRenderPass(gbufferPass.framebuffer, commandBuffer, {
                                         {.color = {0.0f, 0.0f, 0.0f, 0.0f}},
                                         {.color = {0.0f, 0.0f, 0.0f, 0.0f}},
                                         {.color = {0.0f, 0.0f, 0.0f, 0.0f}},
                                         {.color = {0.0f, 0.0f, 0.0f, 0.0f}},
                                         {.depthStencil = {1.0f, 0}}
                                     });


            environmentPass.pipeline->bind(commandBuffer);

            sceneData.projection = scene->getActiveCamera()->getProjection();
            sceneData.view = scene->getActiveCamera()->getView();
            sceneData.inverseView = scene->getActiveCamera()->getInverseView();
            uint32_t ldx = 0;
            for (int i = 0; i < scene->lights.size(); i++) {
                uint32_t lightId = scene->lights[i];
                auto l = cast<Entity, OmniLight>(scene->entities[lightId]);
                sceneData.omniLights[ldx] = {
                    .position = scene->getActiveCamera()->getView() * glm::vec4(l->transform.translation, 1.0f),
                    .color = glm::vec4(l->color, 1.0f)
                };
                ldx++;
            }
            sceneData.numOmniLights = ldx;
            resources.getBuffer(sceneDescriptors.sceneBuffers[frameIndex])->writeToBuffer(&sceneData);
            resources.bindDescriptorSet(
                commandBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                environmentPass.pipeline->graphicsPipelineLayout,
                sceneDescriptors.binding, 1,
                sceneDescriptors.descriptorSets[frameIndex],
                0, nullptr);


            vkCmdDraw(commandBuffer, 3, 1, 0, 0);

            gbufferPass.pipeline->bind(commandBuffer);
            blend = false;
            renderEntity(frameIndex, commandBuffer, gbufferPass.pipeline, scene->getRoot());

            renderer.endRenderPass(commandBuffer);
            renderer.beginSwapChainRenderPass(commandBuffer);

            compositionPass.pipeline->bind(commandBuffer);

            resources.bindDescriptorSet(
                commandBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                compositionPass.pipeline->graphicsPipelineLayout,
                gBufferDescriptors.binding, 1,
                gBufferDescriptors.descriptorSets[frameIndex],
                0, nullptr);

            // draw deffered
            vkCmdDraw(commandBuffer, 3, 1, 0, 0);


            // Transparency pass
            transparencyPass.pipeline->bind(commandBuffer);
            blend = true;
            renderEntity(frameIndex, commandBuffer, gbufferPass.pipeline, scene->getRoot()); {
                ui.beginUserInterface();
                ui.showDebugStats(scene->getActiveCamera());
                ui.showWindowControls();
                ui.showEntityInspector(scene);
                ui.showColorSettings(&sceneData.exposure, &sceneData.gamma, &sceneData.whitePoint);
                ui.endUserInterface(commandBuffer);
            }

            renderer.endRenderPass(commandBuffer);
            renderer.endFrame();
        }

        vkDeviceWaitIdle(device.device());
    }
}

void Hmck::PBRApp::load() {
    scene = std::make_unique<Scene>(Scene::SceneCreateInfo{
        .device = device,
        .memory = resources,
        .name = "Physically based rendering demo",
    });

    scene->environment->load(device, resources, "../data/env/ibl/sunset.hdr", VK_FORMAT_R32G32B32A32_SFLOAT);
    scene->environment->generatePrefilteredSphere(device, resources);
    scene->environment->generateIrradianceSphere(device, resources);
    scene->environment->generateBRDFLUT(device, resources);

    GltfLoader gltfloader{device, resources, scene};
    gltfloader.load("../data/models/helmet/helmet.glb");
    //gltfloader.load("../data/models/helmet/DamagedHelmet.glb");
    //gltfloader.load("../data/models/sponza/sponza.glb");
    //gltfloader.load("../data/models/MetalRoughSpheres.glb");

    geometry.vertexBuffer = resources.createVertexBuffer({
        .vertexSize = sizeof(scene->vertices[0]),
        .vertexCount = static_cast<uint32_t>(scene->vertices.size()),
        .data = static_cast<void *>(scene->vertices.data())
    });

    geometry.indexBuffer = resources.createIndexBuffer({
        .indexSize = sizeof(scene->indices[0]),
        .indexCount = static_cast<uint32_t>(scene->indices.size()),
        .data = static_cast<void *>(scene->indices.data())
    });

    geometry.numTriangles = static_cast<uint32_t>(scene->vertices.size()) / 3;

    Logger::log(HMCK_LOG_LEVEL_DEBUG, "Number of triangles: %d\n", geometry.numTriangles);

    scene->vertices.clear();
    scene->indices.clear();
}

void Hmck::PBRApp::init() {
    std::vector<VkDescriptorImageInfo> imageInfos{scene->images.size()};
    for (int im = 0; im < scene->images.size(); im++) {
        imageInfos[im] = resources.getTexture2D(scene->images[im].texture)->descriptor;
    }

    sceneDescriptors.descriptorSets.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
    sceneDescriptors.sceneBuffers.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
    sceneDescriptors.descriptorSetLayout = resources.createDescriptorSetLayout({
        .bindings = {
            {
                .binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS
            },
            {
                .binding = 1, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS, .count = 2000,
                .bindingFlags = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT
            }, // bindless textures
            {
                .binding = 2, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS
            }, // env map
            {
                .binding = 3, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS
            }, // prefiltered env map
            {
                .binding = 4, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS
            }, //  brdfLUT
            {
                .binding = 5, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS
            }, //  irradiance map
        }
    });

    for (int i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        sceneDescriptors.sceneBuffers[i] = resources.createBuffer({
            .instanceSize = sizeof(SceneBufferData),
            .instanceCount = 1,
            .usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .memoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        });

        auto fbufferInfo = resources.getBuffer(sceneDescriptors.sceneBuffers[i])->descriptorInfo();
        sceneDescriptors.descriptorSets[i] = resources.createDescriptorSet({
            .descriptorSetLayout = sceneDescriptors.descriptorSetLayout,
            .bufferWrites = {{0, fbufferInfo}},
            .imageWrites = {
                {2, resources.getTexture2DDescriptorImageInfo(scene->environment->environmentSphere)},
                {3, resources.getTexture2DDescriptorImageInfo(scene->environment->prefilteredSphere)},
                {4, resources.getTexture2DDescriptorImageInfo(scene->environment->brdfLUT)},
                {5, resources.getTexture2DDescriptorImageInfo(scene->environment->irradianceSphere)},
            },
            .imageArrayWrites = {{1, imageInfos}}
        });
    }

    entityDescriptors.descriptorSetLayout = resources.createDescriptorSetLayout({
        .bindings = {
            {
                .binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS
            },
        }
    });

    for (const auto &ep: scene->entities) {
        entityDescriptors.entityBuffers[ep.first] = resources.createBuffer({
            .instanceSize = sizeof(EntityBufferData),
            .instanceCount = 1,
            .usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .memoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        });
        auto ebufferInfo = resources.getBuffer(entityDescriptors.entityBuffers[ep.first])->descriptorInfo();
        entityDescriptors.descriptorSets[ep.first] = resources.createDescriptorSet({
            .descriptorSetLayout = entityDescriptors.descriptorSetLayout,
            .bufferWrites = {{0, ebufferInfo}},
        });
    }

    primitiveDescriptors.descriptorSets.resize(scene->materials.size());
    primitiveDescriptors.primitiveBuffers.resize(scene->materials.size());
    primitiveDescriptors.descriptorSetLayout = resources.createDescriptorSetLayout({
        .bindings = {
            {
                .binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS
            },
        }
    });

    for (size_t i = 0; i < primitiveDescriptors.descriptorSets.size(); i++) {
        primitiveDescriptors.primitiveBuffers[i] = resources.createBuffer({
            .instanceSize = sizeof(PrimitiveBufferData),
            .instanceCount = 1,
            .usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .memoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        });

        auto pbufferInfo = resources.getBuffer(primitiveDescriptors.primitiveBuffers[i])->descriptorInfo();
        primitiveDescriptors.descriptorSets[i] = resources.createDescriptorSet({
            .descriptorSetLayout = primitiveDescriptors.descriptorSetLayout,
            .bufferWrites = {{0, pbufferInfo}},
        });
    }

    gBufferDescriptors.descriptorSets.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
    gBufferDescriptors.descriptorSetLayout = resources.createDescriptorSetLayout({
        .bindings = {
            {
                .binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
            }, // position
            {
                .binding = 1, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
            }, // albedo
            {
                .binding = 2, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
            }, // normal
            {
                .binding = 3, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
            }, // material (roughness, metalness, ao)
        }
    });
}

void Hmck::PBRApp::renderEntity(const uint32_t frameIndex, const VkCommandBuffer commandBuffer,
                                std::unique_ptr<GraphicsPipeline> &pipeline, const std::shared_ptr<Entity> &entity) {
    // don't render invisible nodes
    if (isInstanceOf<Entity, Entity3D>(entity) && entity->visible) {
        if (const auto _entity = cast<Entity, Entity3D>(entity); !_entity->mesh.primitives.empty()) {
            if (_entity->dataChanged) {
                const EntityBufferData entityData{.model = _entity->mat4(), .normal = _entity->mat4N()};
                resources.getBuffer(entityDescriptors.entityBuffers[entity->id])->writeToBuffer(&entityData);
                _entity->dataChanged = false;
            }

            resources.bindDescriptorSet(
                commandBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                pipeline->graphicsPipelineLayout,
                entityDescriptors.binding, 1,
                entityDescriptors.descriptorSets[entity->id],
                0, nullptr);

            for (const auto &[firstIndex, indexCount, materialIndex]: _entity->mesh.primitives) {
                if (indexCount > 0) {
                    if (materialIndex >= 0) {
                        auto &[name, baseColorFactor, baseColorTextureIndex, normalTextureIndex,
                            metallicRoughnessTextureIndex, occlusionTextureIndex, alphaMode, alphaCutOff, metallicFactor
                            , roughnessFactor, doubleSided, dataChanged] = scene->materials[materialIndex];

                        if (alphaMode == "BLEND" && !blend) continue; // skip blend in this pass

                        if (alphaMode == "OPAQUE" && blend) continue; // skip opaque in this pass

                        PrimitiveBufferData primitiveData{
                            .baseColorFactor = baseColorFactor,
                            .baseColorTextureIndex = (baseColorTextureIndex != TextureIndex::Invalid)
                                                         ? scene->textures[baseColorTextureIndex].imageIndex
                                                         : TextureIndex::Invalid,
                            .normalTextureIndex = (normalTextureIndex != TextureIndex::Invalid)
                                                      ? scene->textures[normalTextureIndex].imageIndex
                                                      : TextureIndex::Invalid,
                            .metallicRoughnessTextureIndex =
                            (metallicRoughnessTextureIndex != TextureIndex::Invalid)
                                ? scene->textures[metallicRoughnessTextureIndex].imageIndex
                                : TextureIndex::Invalid,
                            .occlusionTextureIndex = (occlusionTextureIndex != TextureIndex::Invalid)
                                                         ? scene->textures[occlusionTextureIndex].imageIndex
                                                         : TextureIndex::Invalid,
                            .alphaCutoff = alphaCutOff,
                            .metallicFactor = metallicFactor,
                            .roughnessFactor = roughnessFactor
                        };

                        if (dataChanged) {
                            resources.getBuffer(primitiveDescriptors.primitiveBuffers[materialIndex])->
                                    writeToBuffer(&primitiveData);
                            dataChanged = false;
                        }
                    }

                    resources.bindDescriptorSet(
                        commandBuffer,
                        VK_PIPELINE_BIND_POINT_GRAPHICS,
                        pipeline->graphicsPipelineLayout,
                        primitiveDescriptors.binding, 1,
                        primitiveDescriptors.descriptorSets[(
                            materialIndex >= 0 ? materialIndex : 0)],
                        0, nullptr);

                    // draw
                    vkCmdDrawIndexed(commandBuffer, indexCount, 1, firstIndex, 0, 0);
                }
            }
        }
    }

    for (auto &child: entity->children) {
        renderEntity(frameIndex, commandBuffer, pipeline, child);
    }
}

void Hmck::PBRApp::createPipelines(const Renderer &renderer) {
    // create piplines and framebuffers
    auto depthFormat = device.findSupportedFormat(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);


    gbufferPass.framebuffer = Framebuffer::createFramebufferPtr({
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

    environmentPass.pipeline = GraphicsPipeline::createGraphicsPipelinePtr({
        .debugName = "skybox_pass",
        .device = device,
        .VS{
            .byteCode = Hmck::Filesystem::readFile("../src/engine/shaders/compiled/fullscreen.vert.spv"),
            .entryFunc = "main"
        },
        .FS{
            .byteCode = Hmck::Filesystem::readFile("../src/engine/shaders/compiled/environment.frag.spv"),
            .entryFunc = "main"
        },
        .descriptorSetLayouts = {
            resources.getDescriptorSetLayout(sceneDescriptors.descriptorSetLayout).getDescriptorSetLayout()
        },
        .pushConstantRanges{},
        .graphicsState{
            .depthTest = VK_FALSE,
            .depthTestCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
            .cullMode = VK_CULL_MODE_NONE,
            .blendAtaAttachmentStates{
                Hmck::Init::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
                Hmck::Init::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
                Hmck::Init::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
                Hmck::Init::pipelineColorBlendAttachmentState(0xf, VK_FALSE)
            },
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
        .renderPass = gbufferPass.framebuffer->renderPass
    });

    gbufferPass.pipeline = GraphicsPipeline::createGraphicsPipelinePtr({
        .debugName = "gbuffer_pass",
        .device = device,
        .VS{
            .byteCode = Hmck::Filesystem::readFile("../src/engine/shaders/compiled/gbuffer.vert.spv"),
            .entryFunc = "main"
        },
        .FS{
            .byteCode = Hmck::Filesystem::readFile("../src/engine/shaders/compiled/gbuffer.frag.spv"),
            .entryFunc = "main"
        },
        .descriptorSetLayouts = {
            resources.getDescriptorSetLayout(sceneDescriptors.descriptorSetLayout).getDescriptorSetLayout(),
            resources.getDescriptorSetLayout(entityDescriptors.descriptorSetLayout).getDescriptorSetLayout(),
            resources.getDescriptorSetLayout(primitiveDescriptors.descriptorSetLayout).getDescriptorSetLayout()
        },
        .pushConstantRanges{},
        .graphicsState{
            .depthTest = VK_TRUE,
            .depthTestCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
            .cullMode = VK_CULL_MODE_BACK_BIT,
            .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
            .blendAtaAttachmentStates{
                Hmck::Init::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
                Hmck::Init::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
                Hmck::Init::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
                Hmck::Init::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
            },
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
        .renderPass = gbufferPass.framebuffer->renderPass
    });

    for (unsigned int &descriptorSet: gBufferDescriptors.descriptorSets) {
        std::vector<VkDescriptorImageInfo> gbufferImageInfos = {
            {
                .sampler = gbufferPass.framebuffer->sampler,
                .imageView = gbufferPass.framebuffer->attachments[0].view,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            },
            {
                .sampler = gbufferPass.framebuffer->sampler,
                .imageView = gbufferPass.framebuffer->attachments[1].view,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            },
            {
                .sampler = gbufferPass.framebuffer->sampler,
                .imageView = gbufferPass.framebuffer->attachments[2].view,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            },
            {
                .sampler = gbufferPass.framebuffer->sampler,
                .imageView = gbufferPass.framebuffer->attachments[3].view,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            },
        };

        descriptorSet = resources.createDescriptorSet({
            .descriptorSetLayout = gBufferDescriptors.descriptorSetLayout,
            .imageArrayWrites = {{0, gbufferImageInfos}},
        });
    }


    compositionPass.pipeline = GraphicsPipeline::createGraphicsPipelinePtr({
        .debugName = "deferred_pass",
        .device = device,
        .VS{
            .byteCode = Hmck::Filesystem::readFile("../src/engine/shaders/compiled/fullscreen.vert.spv"),
            .entryFunc = "main"
        },
        .FS{
            .byteCode = Hmck::Filesystem::readFile("../src/engine/shaders/compiled/pbr_ibl_deferred.frag.spv"),
            .entryFunc = "main"
        },
        .descriptorSetLayouts = {
            resources.getDescriptorSetLayout(sceneDescriptors.descriptorSetLayout).getDescriptorSetLayout(),
            resources.getDescriptorSetLayout(entityDescriptors.descriptorSetLayout).getDescriptorSetLayout(),
            resources.getDescriptorSetLayout(primitiveDescriptors.descriptorSetLayout).getDescriptorSetLayout(),
            resources.getDescriptorSetLayout(gBufferDescriptors.descriptorSetLayout).getDescriptorSetLayout()
        },
        .pushConstantRanges{},
        .graphicsState{
            .depthTest = VK_FALSE,
            .cullMode = VK_CULL_MODE_NONE,
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

    transparencyPass.pipeline = GraphicsPipeline::createGraphicsPipelinePtr({
        .debugName = "deferred_pass",
        .device = device,
        .VS{
            .byteCode = Hmck::Filesystem::readFile("../src/engine/shaders/compiled/gbuffer.vert.spv"),
            .entryFunc = "main"
        },
        .FS{
            .byteCode = Hmck::Filesystem::readFile("../src/engine/shaders/compiled/transparency_pass.frag.spv"),
            .entryFunc = "main"
        },
        .descriptorSetLayouts = {
            resources.getDescriptorSetLayout(sceneDescriptors.descriptorSetLayout).getDescriptorSetLayout(),
            resources.getDescriptorSetLayout(entityDescriptors.descriptorSetLayout).getDescriptorSetLayout(),
            resources.getDescriptorSetLayout(primitiveDescriptors.descriptorSetLayout).getDescriptorSetLayout(),
            resources.getDescriptorSetLayout(gBufferDescriptors.descriptorSetLayout).getDescriptorSetLayout(),
        },
        .pushConstantRanges{},
        .graphicsState{
            .depthTest = VK_FALSE,
            .cullMode = VK_CULL_MODE_BACK_BIT,
            .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
            .blendAtaAttachmentStates{Hmck::Init::pipelineColorBlendAttachmentState(0xf, VK_TRUE)},
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
}
