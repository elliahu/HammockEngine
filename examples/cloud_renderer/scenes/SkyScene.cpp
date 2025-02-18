#include "SkyScene.h"

void SkyScene::init() {
    // For fast reads in the compute stage, storage buffer has GPU only memory
    // Data needs to be staged -> moved to GPU/CPU visible memory and then transferred to GPU only memory
    ResourceHandle stagingStorageBufferHandle = rm.createResource<Buffer>(
        "staging-storage-buffer",
        BufferDesc{
            .instanceSize = sizeof(StorageBufferData),
            .instanceCount = 1,
            .usageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .allocationFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        });

    // Map the buffer and write the data
    rm.getResource<Buffer>(stagingStorageBufferHandle)->map();
    rm.getResource<Buffer>(stagingStorageBufferHandle)->writeToBuffer(&storageBufferData);

    // Create the actual GPU only storage buffer
    compute.storageBuffer = rm.createResource<Buffer>(
        "compute-storage-buffer",
        BufferDesc{
            .instanceSize = sizeof(StorageBufferData),
            .instanceCount = 1,
            .usageFlags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            .allocationFlags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT, // GPU only
            // concurrent so we can access the buffer from both compute and graphics queues
            // without having to release and acquire the buffer on the queues
            // although this comes at a cost, the difference in almost immeasurable
            .sharingMode = VK_SHARING_MODE_CONCURRENT
        });

    // Transfer the data between buffer and release the staging buffer
    device.copyBuffer(rm.getResource<Buffer>(stagingStorageBufferHandle)->getBuffer(),
                      rm.getResource<Buffer>(compute.storageBuffer)->getBuffer(), sizeof(StorageBufferData));
    rm.releaseResource(stagingStorageBufferHandle.getUid());

    //Load the cloud map into host RAM
    int w, h, c;
    ScopedMemory cloudMapHostMemory(Filesystem::readImage(assetPath("noise/cloudmap.png"), w, h, c));

    // Create host visible staging buffer on device
    ResourceHandle cloudMapStagingBuffer = rm.createResource<Buffer>(
        "cloudmap-staging-buffer",
        BufferDesc{
            .instanceSize = sizeof(float),
            .instanceCount = static_cast<uint32_t>(w * h * c),
            .usageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .allocationFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        }
    );

    // Write the data into the staging buffer
    rm.getResource<Buffer>(cloudMapStagingBuffer)->map();
    rm.getResource<Buffer>(cloudMapStagingBuffer)->writeToBuffer(cloudMapHostMemory.get());

    // Create image resource
    compute.cloudMap = rm.createResource<Image>(
        "cloudmap-image",
        ImageDesc{
            .width = static_cast<uint32_t>(w),
            .height = static_cast<uint32_t>(h),
            .channels = static_cast<uint32_t>(c),
            .format = VK_FORMAT_R32G32B32A32_SFLOAT,
            .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            .imageType = VK_IMAGE_TYPE_2D,
            .imageViewType = VK_IMAGE_VIEW_TYPE_2D,
        }
    );

    // Copy the data from buffer into the image
    rm.getResource<Image>(compute.cloudMap)->queueImageLayoutTransition(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    rm.getResource<Image>(compute.cloudMap)->queueCopyFromBuffer(rm.getResource<Buffer>(cloudMapStagingBuffer)->getBuffer());
    // Image will be transitioned into SHADER_READ_ONLY_OPTIMAL by the render graph automatically

    // Release the staging buffer
    rm.releaseResource(cloudMapStagingBuffer.getUid());


    // Other resource are managed by the render graph
    buildRenderGraph();

    // Finally build the pipelines
    buildPipelines();
}

void SkyScene::buildRenderGraph() {
    // First, declare the resources
    // Add the uniform buffer that holds mutable data
    renderGraph->addResource<ResourceNode::Type::UniformBuffer, Buffer, BufferDesc>(
        "compute-uniform-buffer", BufferDesc{
            .instanceSize = sizeof(UniformBufferData),
            .instanceCount = 1,
            .usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .allocationFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
            .sharingMode = VK_SHARING_MODE_CONCURRENT,
        });

    // The storage buffer created earlier
    renderGraph->addStaticResource<ResourceNode::Type::StorageBuffer>("compute-storage-buffer", compute.storageBuffer);

    // Cloud map
    renderGraph->addStaticResource<ResourceNode::Type::SampledImage>("compute-cloudmap", compute.cloudMap);

    // Storage image that the compute pass outputs to and that is then read in the composition pass
    renderGraph->addResource<ResourceNode::Type::StorageImage, Image, ImageDesc>(
        "compute-storage-image", ImageDesc{
            .width = window.getExtent().width,
            .height = window.getExtent().height,
            .channels = 4,
            .format = VK_FORMAT_R8G8B8A8_UNORM,
            .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
            .imageType = VK_IMAGE_TYPE_2D,
            .imageViewType = VK_IMAGE_VIEW_TYPE_2D,
            // Concurrent for the same reasons as storage buffer
            .sharingMode = VK_SHARING_MODE_CONCURRENT
        });

    // Create a default sampler that will be used to sample output images (in this case storage image)
    renderGraph->createSampler("default-sampler");

    // And tell the graph that we will be outputting to swap chain
    renderGraph->addSwapChainImageResource("swap-color-image");

    // Next, declare the render passes and what resources each pass uses
    // Compute pass reads from storage and uniform buffers and writes into storage image
    renderGraph->addPass<CommandQueueFamily::Compute>("compute-pass")
            .read(ResourceAccess{
                .resourceName = "compute-uniform-buffer",
            })
            .read(ResourceAccess{
                .resourceName = "compute-storage-buffer",
            })
            .read(ResourceAccess{
                .resourceName = "compute-cloudmap",
                .requiredLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            })
            .descriptor(0, {
                            {0, {"compute-storage-image"}, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT},
                            {1, {"compute-uniform-buffer"}, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT},
                            {2, {"compute-storage-buffer"}, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT},
                            {3, {"compute-cloudmap"}, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT},
                        })
            .write(ResourceAccess{
                .resourceName = "compute-storage-image",
                .requiredLayout = VK_IMAGE_LAYOUT_GENERAL,
            })
            .execute([&](RenderPassContext context)-> void {
                // This is the execution code of the compute pass
                // First we bind the pipeline and descriptor
                compute.pipeline->bind(context.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE);
                context.get<Buffer>("compute-uniform-buffer")->writeToBuffer(&uniformBufferData);
                context.bindDescriptorSet(0, 0, compute.pipeline->pipelineLayout,
                                          VK_PIPELINE_BIND_POINT_COMPUTE);

                // Then we dispatch the compute
                vkCmdDispatch(context.commandBuffer, groupsX, groupsY, 1);
            });

    // Composition pass reads from the storage image and writes to the swap chain image
    renderGraph->addPass<CommandQueueFamily::Graphics, RelativeViewPortSize::SwapChainRelative>("composition-pass")
            .read(ResourceAccess{
                .resourceName = "compute-storage-image",
                .requiredLayout = VK_IMAGE_LAYOUT_GENERAL,
                .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD
            })
            .descriptor(0, {
                            {0, {"compute-storage-image"}, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT},
                        })
            .write(ResourceAccess{
                .resourceName = "swap-color-image",
                .requiredLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, // TODO remove this
            })
            .execute([&](RenderPassContext context)-> void {
                // The composition pass is straight forward
                composition.pipeline->bind(context.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS);
                context.bindDescriptorSet(0, 0, composition.pipeline->pipelineLayout,
                                          VK_PIPELINE_BIND_POINT_GRAPHICS);

                // Even though there is no vertex buffer, this call is safe as it does not actually read the vertices in the shader
                // This only triggers fullscreen effect in ver shader that runs fragment shader for each pixel of the screen
                vkCmdDraw(context.commandBuffer, 3, 1, 0, 0);
            });

    // Last is the UI pass
    // Because it uses SwapChain's VkRenderPass and VkFrameBuffer, we mark it as such -> autoBeginRenderingDisabled()
    renderGraph->addPass<CommandQueueFamily::Graphics, RelativeViewPortSize::SwapChainRelative>("user-interface-pass")
            .autoBeginRenderingDisabled() // This will tell the render graph to not call begin rendering
            .write(ResourceAccess{
                .resourceName = "swap-color-image",
                .requiredLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            })
            .execute([this](RenderPassContext context)-> void {
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

                // Draw the UI
                ui.get()->beginUserInterface();
                ui.get()->showDemoWindow();

                ImGui::Begin("Cloud Property editor", (bool *) false, ImGuiWindowFlags_AlwaysAutoResize);

                ImGui::End();

                ImGui::Begin("Camera property editor", (bool *) false, ImGuiWindowFlags_AlwaysAutoResize);
                ImGui::Text("%.1f FPS ", 1.0f / frameTime);
                ImGui::Text("Frametime: %.2f ms", frameTime * 1000.0f);
                ImGui::DragFloat("Radius (distance from the planet center)", &radius, 0.1f, storageBufferData.surfaceRadius);
                ImGui::DragFloat("Elevation (horizontal inclination)", &elevation);
                ImGui::DragFloat("Azimuth (vertical inclination)", &azimuth);
                ImGui::DragFloat("Yaw", &yaw, 0.01f);
                ImGui::DragFloat("Pitch", &pitch, 0.01f);
                ImGui::End();

                ui.get()->endUserInterface(context.commandBuffer);

                // End the render pass
                vkCmdEndRenderPass(context.commandBuffer);
            });

    // Finally, build the render graph
    // Render graph will prepare resources, optimize the execution and will handle the synchronization
    // This render graph is not re-build every frame like some other implementations
    renderGraph->build();
}

void SkyScene::buildPipelines() {
    // Compute pipeline is not very complicated
    compute.pipeline = ComputePipeline::create({
        .debugName = "compute-pipeline",
        .device = device,
        .computeShader{.byteCode = Filesystem::readFile(compiledShaderPath("clouds.comp")),},
        .descriptorSetLayouts = {renderGraph->getDescriptorSetLayouts("compute-pass")},
        .pushConstantRanges{} // We do not use push constants
    });

    // Composition pass
    composition.pipeline = GraphicsPipeline::create({
        .debugName = "composition-pipeline",
        .device = device,
        .vertexShader
        // Fullscreen vertex shader
        {.byteCode = Filesystem::readFile(compiledShaderPath("fullscreen_headless.vert")),},
        .fragmentShader
        // Fragment shader samples storage texture and writes it to swapchain image
        {.byteCode = Filesystem::readFile(compiledShaderPath("texture.frag")),},
        .descriptorSetLayouts = {renderGraph->getDescriptorSetLayouts("composition-pass")},
        .pushConstantRanges{},
        .graphicsState{
            // We disable cull so that the vkCmdDraw command is not skipped
            .cullMode = VK_CULL_MODE_NONE,
            // No vertex buffer means no buffer bindings
            .vertexBufferBindings{}
        },
        .dynamicRendering = {
            // Render graph requires by default dynamic rendering
            .enabled = true,
            .colorAttachmentCount = 1,
            // We draw to the swapchain image
            .colorAttachmentFormats = {fm.getSwapChain()->getSwapChainImageFormat()},
        }
    });
}

void SkyScene::update() {
    // Spherical coords
    HmckVec3 cameraPosition = {
        radius * cos(elevation) * cos(azimuth), // X
        radius * sin(elevation), // Y
        radius * cos(elevation) * sin(azimuth) // Z
    };

    // Compute the up vector as the normalized vector from the origin to the camera.
    const HmckVec3 up = HmckNorm(cameraPosition);

    HmckVec3 worldNorth = {0.0f, 0.0f, -1.0f};
    HmckVec3 baseForward = worldNorth - HmckDot(worldNorth, up) * up;
    if (HmckLen(baseForward) < 1e-5f) {
        // Fallback if camera is near the north/south pole.
        baseForward = {1.0f, 0.0f, 0.0f};
    }
    baseForward = HmckNorm(baseForward);

    // Compute the right vector (tangent to the sphere surface).
    HmckVec3 right = HmckNorm(HmckCross(up, baseForward));

    // Compute the final view direction.
    // When pitch == 0, the direction lies entirely in the tangent plane:
    //       cos(yaw)*baseForward + sin(yaw)*right.
    // A non-zero pitch tilts the view by adding a component along up.
    HmckVec3 finalDirection = HmckNorm(
        cos(pitch) * (cos(yaw) * baseForward + sin(yaw) * right)
        + sin(pitch) * up
    );

    // The target is where the camera is looking: its position plus the view direction.
    HmckVec3 target = cameraPosition + finalDirection;


    // Create inverse view matrix
    const HmckMat4 view = Projection().view(cameraPosition, target, up);

    uniformBufferData.camera.position = HmckVec4{cameraPosition, 0.0f};
    uniformBufferData.camera.inverseView = HmckInvGeneral(view);
}


void SkyScene::render() {
    auto currentTime = std::chrono::high_resolution_clock::now();
    while (!window.shouldClose()) {
        // Timing
        auto newTime = std::chrono::high_resolution_clock::now();
        frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
        currentTime = newTime;

        // Poll for events
        window.pollEvents();

        // Update the data for the frame
        update();

        // Execute the render graph
        renderGraph->execute();
    }
    // Wait for queues to finish before deallocating resources
    device.waitIdle();
}
