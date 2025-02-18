#include "NoiseEditor.h"

ResourceHandle NoiseEditor::generateWorleyPointsBuffer() {
    std::mt19937 gen{static_cast<uint32_t>(seed)};
    std::uniform_real_distribution<float> uniformRealDist{0.0f, 1.0f};

    const size_t bufferSize = static_cast<size_t>(gridSizeX * gridSizeY * gridSizeZ);
    std::vector<HmckVec4> points{};
    points.resize(bufferSize);
    HmckVec3 cellSize = HmckVec3{1.0f, 1.0f, 1.0f} / HmckVec3{static_cast<float>(gridSizeX), static_cast<float>(gridSizeY), static_cast<float>(gridSizeZ)};

    for (int x = 0; x < gridSizeX; x++) {
        for (int y = 0; y < gridSizeY; y++) {
            for (int z = 0; z < gridSizeZ; z++) {
                HmckVec3 randOffset{uniformRealDist(gen), uniformRealDist(gen), uniformRealDist(gen)};
                HmckVec3 position = (HmckVec3{static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)} + randOffset) * cellSize;
                int index = x * (gridSizeY * gridSizeZ) + y * gridSizeZ + z;
                points[index] = HmckVec4{position, 0.0f};
            }
        }
    }

    ResourceHandle buffer = rm.createResource<Buffer>(
        "worley-points-staging-buffer",
        BufferDesc{
            .instanceSize = sizeof(HmckVec4),
            .instanceCount = static_cast<uint32_t>(bufferSize),
            .usageFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .allocationFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        }
    );

    rm.getResource<Buffer>(buffer)->map();
    rm.getResource<Buffer>(buffer)->writeToBuffer(points.data());
    rm.getResource<Buffer>(buffer)->unmap();


    releaseQueue.push(buffer);
    return buffer;
}

void NoiseEditor::init() {
    pointsBuffer = rm.createResource<Buffer>(
        "worley-points-buffer",
        BufferDesc{
            .instanceSize = sizeof(HmckVec4),
            .instanceCount = static_cast<uint32_t>(maxGridSize.X * maxGridSize.Y * maxGridSize.Z),
            .usageFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            .allocationFlags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT, // GPU only
            .sharingMode = VK_SHARING_MODE_CONCURRENT
        }
    );

    auto stagingBuffer = generateWorleyPointsBuffer();
    rm.getResource<Buffer>(pointsBuffer)->copy(rm.getResource<Buffer>(stagingBuffer)->getBuffer(),
                                               sizeof(HmckVec4) * gridSizeX * gridSizeY * gridSizeZ);

    buildRenderGraph();
    buildPipelines();
}

void NoiseEditor::buildRenderGraph() {
    // Points buffer
    renderGraph->addStaticResource<ResourceNode::Type::StorageBuffer>("worley-points-buffer", pointsBuffer);

    // 3D noise texture
    renderGraph->addResource<ResourceNode::Type::StorageImage, Image, ImageDesc>(
        "noise-texture",
        ImageDesc{
            .width = texWidth,
            .height = texHeight,
            .channels = 4,
            .depth = texDepth,
            .format = VK_FORMAT_R32G32B32A32_SFLOAT,
            .usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            .imageType = VK_IMAGE_TYPE_3D,
            .imageViewType = VK_IMAGE_VIEW_TYPE_3D,
            .sharingMode = VK_SHARING_MODE_CONCURRENT
        }
    );

    // Create a default sampler that will be used to sample output images
    renderGraph->createSampler("default-sampler");

    // And tell the graph that we will be outputting to swap chain
    renderGraph->addSwapChainImageResource("swap-color-image");

    // Next, declare the render passes and what resources each pass uses
    renderGraph->addPass<CommandQueueFamily::Compute>("compute-pass")
            .read(ResourceAccess{.resourceName = "worley-points-buffer"})
            .descriptor(0, {
                            {0, {"noise-texture"}, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT},
                            {1, {"worley-points-buffer"}, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT}
                        })
            .write(ResourceAccess{
                .resourceName = "noise-texture", .requiredLayout = VK_IMAGE_LAYOUT_GENERAL, .finalLayout = VK_IMAGE_LAYOUT_GENERAL
            })
            .execute([this](RenderPassContext context) {
                computePipeline->bind(context.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE);
                context.bindDescriptorSet(0, 0, computePipeline->pipelineLayout,
                                          VK_PIPELINE_BIND_POINT_COMPUTE);

                HmckVec3 gridSize{static_cast<float>(gridSizeX), static_cast<float>(gridSizeY), static_cast<float>(gridSizeZ)};
                computePushData.cellSizeR = HmckVec4{HmckVec3{1.0f, 1.0f, 1.0f} / gridSize, 0.0f};
                computePushData.gridSizeR = HmckVec4{gridSize, 0.f};

                vkCmdPushConstants(context.commandBuffer, computePipeline->pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0,
                                   sizeof(ComputePushData), &computePushData);

                int groupsX = texWidth / 8;
                int groupsY = texHeight / 8;
                int groupsZ = texDepth / 8;


                vkCmdDispatch(context.commandBuffer, groupsX, groupsY, groupsZ);
            });

    // Lastly, present pass for noise preview and ui
    renderGraph->addPass<CommandQueueFamily::Graphics>("graphics-pass")
            .read(ResourceAccess{
                .resourceName = "noise-texture",
                .requiredLayout = VK_IMAGE_LAYOUT_GENERAL,
                .finalLayout = VK_IMAGE_LAYOUT_GENERAL
            })
            .descriptor(0, {
                            {0, {"noise-texture"}, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT},
                        })
            .write(ResourceAccess{
                .resourceName = "swap-color-image",
                .requiredLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
            })
            .execute([this](RenderPassContext context) {
                graphicsPipeline->bind(context.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS);
                context.bindDescriptorSet(0, 0, graphicsPipeline->pipelineLayout,
                                          VK_PIPELINE_BIND_POINT_GRAPHICS);

                vkCmdPushConstants(context.commandBuffer, graphicsPipeline->pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                                   sizeof(GraphicsPushData), &graphicsPushData);

                vkCmdDraw(context.commandBuffer, 3, 1, 0, 0);
            });

    renderGraph->addPass<CommandQueueFamily::Graphics, RelativeViewPortSize::SwapChainRelative>("user-interface-pass")
            .autoBeginRenderingDisabled()
            .write(ResourceAccess{
                .resourceName = "swap-color-image",
                .requiredLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            })
            .execute([this](RenderPassContext context) {
                VkRenderPassBeginInfo renderPassInfo{};
                renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                renderPassInfo.renderPass = fm.getSwapChain()->getRenderPass();
                renderPassInfo.framebuffer = fm.getSwapChain()->getFramebuffer(fm.getSwapChainImageIndex());
                renderPassInfo.renderArea.offset = {0, 0};
                renderPassInfo.renderArea.extent = fm.getSwapChain()->getSwapChainExtent();
                // Define a clear color (e.g., black with full alpha)
                VkClearValue clearColor = {.color = {{0.0f, 0.0f, 0.0f, 1.0f}}};
                renderPassInfo.clearValueCount = 1;
                renderPassInfo.pClearValues = &clearColor;

                // Begin the render pass
                vkCmdBeginRenderPass(context.commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

                ui->beginUserInterface();
                ImGui::Begin("Noise editor", (bool *) false, ImGuiWindowFlags_AlwaysAutoResize);

                ImGui::SetNextItemOpen(true);
                if (ImGui::CollapsingHeader("Display settings")) {
                    ImGui::Checkbox("Animate", &animate);
                    if (animate) {
                        ImGui::SliderFloat("Animation speed", &animateStepsize, 0.01f, 1.0f);
                    }
                    ImGui::SliderFloat("Z-slice", &graphicsPushData.slice, 0.0f, 1.0f);
                    ImGui::SliderFloat("scale", &graphicsPushData.scale, 1.0f, 10.f);
                    ImGui::SliderFloat3("Display channels", &graphicsPushData.displayChannels.Elements[0], 0.0f, 1.0f, "%.0f");
                }

                ImGui::SetNextItemOpen(true);
                if (ImGui::CollapsingHeader("Noise settings")) {
                    ImGui::SetNextItemOpen(true);

                    int32_t prevSeed = seed;
                    ImGui::SliderInt("Seed", &seed, 0, 1000);
                    valuesChanged = (prevSeed != seed) | valuesChanged ;

                    HmckVec3 prevGridSize = HmckVec3{static_cast<float>(gridSizeX), static_cast<float>(gridSizeY), static_cast<float>(gridSizeZ)};
                    ImGui::SliderInt("Number of cells X", &gridSizeX, 1.0f, maxGridSize.X);
                    ImGui::SliderInt("Number of cells Y", &gridSizeY, 1.0f, maxGridSize.Y);
                    ImGui::SliderInt("Number of cells Z", &gridSizeZ, 1.0f, maxGridSize.Z);
                    valuesChanged = (prevGridSize != HmckVec3{static_cast<float>(gridSizeX), static_cast<float>(gridSizeY), static_cast<float>(gridSizeZ)}) | valuesChanged ;
                }


                ImGui::End();
                ui->endUserInterface(context.commandBuffer);

                // End the render pass
                vkCmdEndRenderPass(context.commandBuffer);
            });


    renderGraph->build();
}

void NoiseEditor::buildPipelines() {
    // Compute pipeline is not very complicated
    computePipeline = ComputePipeline::create({
        .debugName = "compute-pipeline",
        .device = device,
        .computeShader{.byteCode = Filesystem::readFile(compiledShaderPath("noise.comp")),},
        .descriptorSetLayouts = {renderGraph->getDescriptorSetLayouts("compute-pass")},
        .pushConstantRanges{
            {VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputePushData)},
        }
    });

    graphicsPipeline = GraphicsPipeline::create({
        .debugName = "graphics-pipeline",
        .device = device,
        .vertexShader
        // Fullscreen vertex shader
        {.byteCode = Filesystem::readFile(compiledShaderPath("fullscreen_headless.vert")),},
        .fragmentShader
        // Fragment shader samples storage texture and writes it to swapchain image
        {.byteCode = Filesystem::readFile(compiledShaderPath("texture3d.frag")),},
        .descriptorSetLayouts = {renderGraph->getDescriptorSetLayouts("graphics-pass")},
        .pushConstantRanges{
            {VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(GraphicsPushData)},
        },
        .graphicsState{
            // We disable cull so that the vkCmdDraw command is not skipped
            .cullMode = VK_CULL_MODE_NONE,
            // No vertex buffer means no buffer bindings
            .vertexBufferBindings{}
        },
        .dynamicRendering = {
            // Render graph requires dynamic rendering
            .enabled = true,
            .colorAttachmentCount = 1,
            // We draw to the swapchain image
            .colorAttachmentFormats = {fm.getSwapChain()->getSwapChainImageFormat()},
        }
    });
}

void NoiseEditor::update() {
    if (animate) {
        graphicsPushData.slice += animateStepsize * frametime;
        if (graphicsPushData.slice >= 1.0f) {
            graphicsPushData.slice = 0.f;
        }
    }

    if (valuesChanged) {
        auto stagingBuffer = generateWorleyPointsBuffer();
        rm.getResource<Buffer>(pointsBuffer)->copy(rm.getResource<Buffer>(stagingBuffer)->getBuffer(),
                                                   sizeof(HmckVec4) * gridSizeX * gridSizeY * gridSizeZ);

        valuesChanged = false;
    }

    // Release resources
    while (!releaseQueue.empty()) {
        // Get the front element
        auto frontElement = releaseQueue.front();

        rm.releaseResource(frontElement.getUid());

        // Remove the processed element from the queue
        releaseQueue.pop();
    }
}

void NoiseEditor::render() {
    auto currentTime = std::chrono::high_resolution_clock::now();
    while (!window.shouldClose()) {
        // Poll for events
        window.pollEvents();

        // Timing
        auto newTime = std::chrono::high_resolution_clock::now();
        frametime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
        currentTime = newTime;


        update();

        // Execute the render graph
        renderGraph->execute();
    }
    // Wait for queues to finish before deallocating resources
    device.waitIdle();
}
