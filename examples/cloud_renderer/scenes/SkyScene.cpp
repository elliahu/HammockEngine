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

    // Load the base noise
    int w, h, c, d;
    // Read the data from disk
    ScopedMemory baseNoiseData(Filesystem::readVolume(Filesystem::ls(assetPath("noise/base")), w, h, c, d, Filesystem::ImageFormat::R16G16B16A16_SFLOAT));
    // Create staging buffer
    ResourceHandle baseNoiseStagingBuffer = rm.createResource<Buffer>(
        "base-noise-staging-buffer",
        BufferDesc{
            .instanceSize = sizeof(float16),
            .instanceCount = static_cast<uint32_t>(w * h * d * c),
            .usageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .allocationFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        }
    );
    // Write data to the buffer
    rm.getResource<Buffer>(baseNoiseStagingBuffer)->map();
    rm.getResource<Buffer>(baseNoiseStagingBuffer)->writeToBuffer(baseNoiseData.get());

    // Create the actual image resource
    compute.baseNoise = rm.createResource<Image>(
        "base-noise",
        ImageDesc{
            .width = static_cast<uint32_t>(w),
            .height = static_cast<uint32_t>(h),
            .channels = static_cast<uint32_t>(c),
            .depth = static_cast<uint32_t>(d),
            .format = VK_FORMAT_R16G16B16A16_SFLOAT,
            .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            .imageType = VK_IMAGE_TYPE_3D,
            .imageViewType = VK_IMAGE_VIEW_TYPE_3D,
        }
    );

    // Copy data from buffer to image
    rm.getResource<Image>(compute.baseNoise)->queueImageLayoutTransition(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    rm.getResource<Image>(compute.baseNoise)->queueCopyFromBuffer(rm.getResource<Buffer>(baseNoiseStagingBuffer)->getBuffer());
    // Image will be transitioned into SHADER_READ_ONLY_OPTIMAL by the render graph automatically

    // Release the staging buffer
    rm.releaseResource(baseNoiseStagingBuffer.getUid());


    // Load the detail noise
    // Read the data from disk
    ScopedMemory detailNoiseData(Filesystem::readVolume(Filesystem::ls(assetPath("noise/detail")), w, h, c, d));
    // Create staging buffer
    ResourceHandle detailNoiseStagingBuffer = rm.createResource<Buffer>(
        "detail-noise-staging-buffer",
        BufferDesc{
            .instanceSize = sizeof(float32),
            .instanceCount = static_cast<uint32_t>(w * h * d * c),
            .usageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .allocationFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        }
    );

    // Write data to the buffer
    rm.getResource<Buffer>(detailNoiseStagingBuffer)->map();
    rm.getResource<Buffer>(detailNoiseStagingBuffer)->writeToBuffer(detailNoiseData.get());

    // Create the actual image resource
    compute.detailNoise = rm.createResource<Image>(
        "detail-noise",
        ImageDesc{
            .width = static_cast<uint32_t>(w),
            .height = static_cast<uint32_t>(h),
            .channels = static_cast<uint32_t>(c),
            .depth = static_cast<uint32_t>(d),
            .format = VK_FORMAT_R32G32B32A32_SFLOAT,
            .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            .imageType = VK_IMAGE_TYPE_3D,
            .imageViewType = VK_IMAGE_VIEW_TYPE_3D,
        }
    );

    // Copy data from buffer to image
    rm.getResource<Image>(compute.detailNoise)->queueImageLayoutTransition(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    rm.getResource<Image>(compute.detailNoise)->queueCopyFromBuffer(rm.getResource<Buffer>(detailNoiseStagingBuffer)->getBuffer());

    // Release the staging buffer
    rm.releaseResource(detailNoiseStagingBuffer.getUid());

    // Load the curl noise
    ScopedMemory curlNoiseData(Filesystem::readImage(assetPath("noise/curlNoise.png"), w, h, c, Filesystem::ImageFormat::R16G16B16A16_SFLOAT));

    // Create host visible staging buffer on device
    ResourceHandle curlNoiseStagingBuffer = rm.createResource<Buffer>(
        "curl-staging-buffer",
        BufferDesc{
            .instanceSize = sizeof(float16),
            .instanceCount = static_cast<uint32_t>(w * h * c),
            .usageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .allocationFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        }
    );

    // Write the data into the staging buffer
    rm.getResource<Buffer>(curlNoiseStagingBuffer)->map();
    rm.getResource<Buffer>(curlNoiseStagingBuffer)->writeToBuffer(curlNoiseData.get());

    // Create image resource
    compute.curlNoise = rm.createResource<Image>(
        "curl-noise",
        ImageDesc{
            .width = static_cast<uint32_t>(w),
            .height = static_cast<uint32_t>(h),
            .channels = static_cast<uint32_t>(c),
            .format = VK_FORMAT_R16G16B16A16_SFLOAT,
            .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            .imageType = VK_IMAGE_TYPE_2D,
            .imageViewType = VK_IMAGE_VIEW_TYPE_2D,
        }
    );

    // Copy the data from buffer into the image
    rm.getResource<Image>(compute.curlNoise)->queueImageLayoutTransition(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    rm.getResource<Image>(compute.curlNoise)->queueCopyFromBuffer(rm.getResource<Buffer>(curlNoiseStagingBuffer)->getBuffer());
    // Image will be transitioned into SHADER_READ_ONLY_OPTIMAL by the render graph automatically

    // Release the staging buffer
    rm.releaseResource(curlNoiseStagingBuffer.getUid());


    // Other resource are managed by the render graph
    buildRenderGraph();

    // Finally build the pipelines
    buildPipelines();

    // In the constructor if IScene, device is waiting after the initialization so that the queues are finished before rendering
    // No need to do it here again
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
        });

    // The storage buffer created earlier
    renderGraph->addStaticResource<ResourceNode::Type::StorageBuffer>("compute-storage-buffer", compute.storageBuffer);

    // Base noise
    renderGraph->addStaticResource<ResourceNode::Type::SampledImage>("compute-base-noise", compute.baseNoise);

    // Detail noise
    renderGraph->addStaticResource<ResourceNode::Type::SampledImage>("compute-detail-noise", compute.detailNoise);

    // Curl noise
    renderGraph->addStaticResource<ResourceNode::Type::SampledImage>("compute-curl-noise", compute.curlNoise);

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
                .resourceName = "compute-base-noise",
                .requiredLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            })
            .read(ResourceAccess{
                .resourceName = "compute-detail-noise",
                .requiredLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            })
            .read(ResourceAccess{
                .resourceName = "compute-curl-noise",
                .requiredLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            })
            .descriptor(0, {
                            {0, {"compute-storage-image"}, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT},
                            {1, {"compute-uniform-buffer"}, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT},
                            {2, {"compute-storage-buffer"}, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT},
                            {3, {"compute-base-noise"}, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT},
                            {4, {"compute-detail-noise"}, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT},
                            {5, {"compute-curl-noise"}, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT},
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

                // Then we dispatch the compute shader
                // This way we can render in parallel which makes the raymarching way quicker than doing this in frag shader
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
                // In case of swapchain image we always need to specify final layout
                // In this case we declare that the layout will not change as this is not the last pass before present
                .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            })
            .execute([&](RenderPassContext context)-> void {
                // The composition pass is straight forward
                composition.pipeline->bind(context.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS);
                context.bindDescriptorSet(0, 0, composition.pipeline->pipelineLayout,
                                          VK_PIPELINE_BIND_POINT_GRAPHICS);

                // Even though there is no vertex buffer, this call is safe as it does not actually read the vertices in the shader
                // This only triggers fullscreen effect in vert shader that runs fragment shader for each pixel of the screen
                vkCmdDraw(context.commandBuffer, 3, 1, 0, 0);
            });

    // Last is the UI pass
    // Because it uses SwapChain's VkRenderPass and VkFrameBuffer, we mark it as such -> autoBeginRenderingDisabled()
    renderGraph->addPass<CommandQueueFamily::Graphics, RelativeViewPortSize::SwapChainRelative>("user-interface-pass")
            .autoBeginRenderingDisabled() // This will tell the render graph to not call begin rendering
            .write(ResourceAccess{
                .resourceName = "swap-color-image",
                .requiredLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, // Last pass before present
            })
            .execute([this](RenderPassContext context)-> void {
                // Clarification on why calling vkCmdBeginRenderPass:
                // The rendergraph by default uses dynamic rendering but the ImGUI vulkan backend requires valid VkRednerPass object
                // In dynamic rendering workflow, no VkRenderPasses and VkFramebuffer are created or used
                // That is why we need to tell the rendergraph to skip the dynamic rendering in this pass and begin the pass ourselves
                VkRenderPassBeginInfo renderPassInfo{};
                renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                renderPassInfo.renderPass = fm.getSwapChain()->getRenderPass();
                renderPassInfo.framebuffer = fm.getSwapChain()->getFramebuffer(fm.getSwapChainImageIndex());
                renderPassInfo.renderArea.offset = {0, 0};
                renderPassInfo.renderArea.extent = fm.getSwapChain()->getSwapChainExtent();
                VkClearValue clearColor = {.color = {{0.0f, 0.0f, 0.0f, 1.0f}}};
                renderPassInfo.clearValueCount = 1;
                renderPassInfo.pClearValues = &clearColor;

                // Begin the render pass
                vkCmdBeginRenderPass(context.commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

                // Draw the UI
                ui.get()->beginUserInterface();

                static bool showCam = true;
                static bool showPerf = true;

                // Main menu bar
                if (ImGui::BeginMainMenuBar()) {
                    if (ImGui::BeginMenu("Editors")) {
                        if (ImGui::MenuItem("Show property editor", NULL, showCam)) {
                            showCam = !showCam;
                        }

                        if (ImGui::MenuItem("Show performance overview", NULL, showPerf)) {
                            showPerf = !showPerf;
                        }

                        ImGui::EndMenu();
                    }
                    ImGui::EndMainMenuBar();
                }


                if (showCam) {
                    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
                    ImGui::SetNextWindowPos({0, static_cast<float>(window.getExtent().height)}, 0, {0, 1});
                    ImGui::Begin("Editor options", (bool *) false,
                                 ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove |
                                 ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                                 ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoDecoration);

                    ImGui::SeparatorText("Camera");
                    ImGui::DragFloat3("Position", &cameraPosition.Elements[0], 0.01f);
                    ImGui::DragFloat("Yaw", &yaw, 0.01f);
                    ImGui::DragFloat("Pitch", &pitch, 0.01f);

                    ImGui::SeparatorText("Lighting");
                    ImGui::DragFloat3("Light position", &uniformBufferData.sun.position.Elements[0], 0.1f);
                    ImGui::ColorEdit3("Light color", &uniformBufferData.sun.color.Elements[0]);

                    ImGui::SeparatorText("Clouds");
                    ImGui::SliderFloat("Absorption", &uniformBufferData.rendering.absorptionCoef, 0.0f, 1.0f);
                    ImGui::SliderFloat("Scattering", &uniformBufferData.rendering.scatteringCoef, 0.0f, 1.0f);
                    ImGui::SliderFloat("Henye Greenstein g", &uniformBufferData.rendering.phase, -.955f, .955f);

                    ImGui::PopStyleVar();
                    ImGui::End();
                }

                if (showPerf) {
                    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
                    ImGui::SetNextWindowPos({static_cast<float>(window.getExtent().width), static_cast<float>(window.getExtent().height)}, 0, {1, 1});
                    ImGui::Begin("Performance analysis", (bool *) false,
                                 ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove |
                                 ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                                 ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoDecoration);
                    ImGui::SeparatorText("Performance");
                    ImGui::Text("%.1f FPS ", 1.0f / frameTime);
                    ImGui::Text("Frametime: %.2f ms", frameTime * 1000.0f);
                    ImGui::PlotLines("Frame Times", frameTimes, FRAMETIME_BUFFER_SIZE, frameTimeFrameIndex, nullptr, 0.0f, 33.0f,
                                     ImVec2(0, 80));
                    ImGui::PopStyleVar();
                    ImGui::End();
                }


                ui.get()->endUserInterface(context.commandBuffer);

                // End the render pass
                vkCmdEndRenderPass(context.commandBuffer);
            });

    // Finally, build the rendergraph
    // Rendergraph will prepare resources, optimize the execution and will handle the synchronization
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
    // Movement and rotation speeds (adjust these as needed)
    const float movementSpeed = 10.0f;    // Units per frame
    const float rotationSpeed = 2.0f;   // Radians per frame

    // Process rotation input using arrow keys.
    // Rotate left/right (yaw)
    if (window.isKeyDown(Surfer::KeyCode::ArrowLeft))
        yaw -= rotationSpeed * frameTime;
    if (window.isKeyDown(Surfer::KeyCode::ArrowRight))
        yaw += rotationSpeed * frameTime;

    // Rotate up/down (pitch)
    if (window.isKeyDown(Surfer::KeyCode::ArrowUp))
        pitch += rotationSpeed * frameTime;
    if (window.isKeyDown(Surfer::KeyCode::ArrowDown))
        pitch -= rotationSpeed * frameTime;

    // Clamp pitch to avoid excessive rotation (e.g., limit to +/- 89 degrees in radians)
    const float pitchLimit = 1.55334f; // ~89 degrees in radians
    if (pitch > pitchLimit)
        pitch = pitchLimit;
    if (pitch < -pitchLimit)
        pitch = -pitchLimit;

    // Compute the forward direction vector from yaw and pitch.
    const float cosPitch = std::cos(pitch);
    const float sinPitch = std::sin(pitch);
    const float cosYaw   = std::cos(yaw);
    const float sinYaw   = std::sin(yaw);
    const HmckVec3 direction = HmckVec3{ cosYaw * cosPitch, sinPitch, sinYaw * cosPitch };

    // Get the up vector from the projection (assumed constant)
    const HmckVec3 up = Projection().upPosY();

    // Compute the right vector (perpendicular to both direction and up)
    const HmckVec3 right = HmckNorm(HmckCross(direction, up));

    // Process translation input (WASD keys)
    if (window.isKeyDown(Surfer::KeyCode::KeyW))
        cameraPosition += direction * movementSpeed * frameTime;
    if (window.isKeyDown(Surfer::KeyCode::KeyS))
        cameraPosition -= direction * movementSpeed * frameTime;
    if (window.isKeyDown(Surfer::KeyCode::KeyA))
        cameraPosition -= right * movementSpeed * frameTime;
    if (window.isKeyDown(Surfer::KeyCode::KeyD))
        cameraPosition += right * movementSpeed * frameTime;

    if (window.isKeyDown(Surfer::KeyCode::Space))
        cameraPosition += up * movementSpeed * frameTime;
    if (window.isKeyDown(Surfer::KeyCode::LeftShift))
        cameraPosition -= up * movementSpeed * frameTime;

    // Compute the target point from the camera position and forward direction.
    const HmckVec3 target = cameraPosition + direction;

    // Create the inverse view matrix based on the updated camera parameters.
    const HmckMat4 view = Projection().view(cameraPosition, target, up);

    uniformBufferData.camera.position = HmckVec4{ cameraPosition, 0.0f };
    uniformBufferData.camera.inverseView = HmckInvGeneral(view);
}



void SkyScene::render() {
    auto currentTime = std::chrono::high_resolution_clock::now();
    while (!window.shouldClose()) {
        // Timing
        auto newTime = std::chrono::high_resolution_clock::now();
        frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
        currentTime = newTime;

        // Update frame time tracking
        frameTimes[frameTimeFrameIndex] = frameTime * 1000.0f;
        frameTimeFrameIndex = (frameTimeFrameIndex + 1) % FRAMETIME_BUFFER_SIZE;

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
