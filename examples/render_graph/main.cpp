#include <hammock/hammock.h>

using namespace hammock;

std::string assetPath(const std::string &asset) {
    return "../../../data/" + asset;
}

std::string compiledShaderPath(const std::string &shader) {
    return "../../../src/hammock/shaders/compiled/" + shader + ".spv";
}

int main() {
    VulkanInstance instance{};
    hammock::Window window{instance, "Render Graph", 1920, 1080};
    Device device{instance, window.getSurface()};
    DeviceStorage storage{device};
    ResourceManager rm{device};
    // TODO decouple context and window
    FrameManager renderContext{window, device};
    std::unique_ptr<DescriptorPool> descriptorPool = DescriptorPool::Builder(device)
            .setMaxSets(20000)
            .setPoolFlags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)
            .addPoolSize(VK_DESCRIPTOR_TYPE_SAMPLER, 10000)
            .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10000)
            .addPoolSize(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 10000)
            .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 10000)
            .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 10000)
            .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 10000)
            .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10000)
            .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10000)
            .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 10000)
            .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 10000)
            .addPoolSize(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 10000)
            .build();

    struct UniformBuffer {
        HmckMat4 mvp;
    } ubo;

    // Load geometry
    Geometry geometry{};
    Loader(geometry, device, storage).loadglTF(assetPath("models/Sphere/Sphere.glb"));

    ResourceHandle vertexBuffer = rm.createVertexBuffer(
        sizeof(geometry.vertices[0]),
        static_cast<uint32_t>(geometry.vertices.size()),
        static_cast<void *>(geometry.vertices.data())
    );

    ResourceHandle indexBuffer = rm.createIndexBuffer(
        sizeof(geometry.indices[0]),
        static_cast<uint32_t>(geometry.indices.size()),
        static_cast<void *>(geometry.indices.data())
    );

    // Forward declare pipelines
    std::unique_ptr<GraphicsPipeline> halfResPipeline = nullptr;
    std::unique_ptr<GraphicsPipeline> presentPipeline = nullptr;

    // Create the actual render graph
    const auto renderGraph = std::make_unique<RenderGraph>(device, rm, renderContext, *descriptorPool);
    renderGraph->addStaticResource<ResourceNode::Type::VertexBuffer>("vertex-buffer", vertexBuffer);
    renderGraph->addStaticResource<ResourceNode::Type::IndexBuffer>("index-buffer", indexBuffer);
    renderGraph->addResource<ResourceNode::Type::UniformBuffer, Buffer, BufferDesc>(
        "uniform-buffer", BufferDesc{
            .instanceSize = sizeof(UniformBuffer),
            .instanceCount = 1,
            .usageFlags =
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .allocationFlags =
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
        });
    renderGraph->addSwapChainImageResource<ResourceNode::Type::SwapChainColorAttachment>("swap-color-image");
    renderGraph->addSwapChainDependentResource<ResourceNode::Type::ColorAttachment, Image, ImageDesc>(
        "half-res-image", [&](VkExtent2D swapchain)-> ImageDesc {
            return ImageDesc{
                .width = swapchain.width,
                .height = swapchain.height,
                .channels = 4,
                .format = VK_FORMAT_B8G8R8A8_UNORM,
                .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                .imageType = VK_IMAGE_TYPE_2D,
                .imageViewType = VK_IMAGE_VIEW_TYPE_2D,
            };
        });
    renderGraph->createSampler("sampler"); // this is our default sampler


    renderGraph->addPass<CommandQueueFamily::Graphics>("half-res-pass")
            .read(ResourceAccess{
                .resourceName = "vertex-buffer",
            })
            .read(ResourceAccess{
                .resourceName = "index-buffer",
            })
            .write(ResourceAccess{
                .resourceName = "half-res-image",
                .requiredLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            })
            .execute([&](RenderPassContext context)-> void {
                context.bindVertexBuffers({"vertex-buffer"},{0});
                context.bindIndexBuffer({"index-buffer"});
                halfResPipeline->bind(context.commandBuffer);
                vkCmdDraw(context.commandBuffer, 3, 1, 0, 0);
            });


    renderGraph->addPass<CommandQueueFamily::Graphics, RelativeViewPortSize::SwapChainRelative>("present-pass")
            .read(ResourceAccess{
                .resourceName = "uniform-buffer",
            })
            .read(ResourceAccess{
                .resourceName = "half-res-image",
                .requiredLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD
            })
            .descriptor(0, {
                            {0, {"uniform-buffer"}, VK_SHADER_STAGE_ALL_GRAPHICS},
                            {1, {"half-res-image"}, VK_SHADER_STAGE_ALL_GRAPHICS}
                        })
            .write(ResourceAccess{
                .resourceName = "swap-color-image",
                .requiredLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            })
            .execute([&](RenderPassContext context)-> void {
                presentPipeline->bind(context.commandBuffer);

                // Update buffer data
                HmckMat4 projection = Projection().perspective(45.f, renderContext.getAspectRatio(), 0.1f, 100.f);
                HmckMat4 view = Projection().view(HmckVec3{0.f, 0.f, 6.0f}, HmckVec3{0.f, 0.f, 0.f},
                                                  Projection().upPosY());
                ubo.mvp = projection * view;

                context.get<Buffer>("uniform-buffer")->writeToBuffer(&ubo);

                context.bindDescriptorSet(0, 0,presentPipeline->graphicsPipelineLayout, VK_PIPELINE_BIND_POINT_GRAPHICS);

                vkCmdDrawIndexed(context.commandBuffer, geometry.indices.size(), 1, 0, 0, 0);
            });

    renderGraph->addPass<CommandQueueFamily::Graphics>("to-be-purged-pass")
            .read(ResourceAccess{
                .resourceName = "swap-color-image",
                .requiredLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD
            })
            .execute([&](RenderPassContext context)-> void {
            });


    renderGraph->build();

    halfResPipeline = GraphicsPipeline::createGraphicsPipelinePtr({
        .debugName = "half-res-pipeline",
        .device = device,
        .VS
        {.byteCode = Filesystem::readFile(compiledShaderPath("fullscreen.vert")),},
        .FS
        {.byteCode = Filesystem::readFile(compiledShaderPath("fullscreen.frag")),},
        .descriptorSetLayouts = {},
        .pushConstantRanges{},
        .graphicsState{
            .cullMode = VK_CULL_MODE_NONE,
            .vertexBufferBindings{
                .vertexBindingDescriptions = Vertex::vertexInputBindingDescriptions(),
                .vertexAttributeDescriptions = Vertex::vertexInputAttributeDescriptions()
            }
        },
        .dynamicRendering = {
            .enabled = true,
            .colorAttachmentCount = 1,
            .colorAttachmentFormats = {renderContext.getSwapChain()->getSwapChainImageFormat()},
        }
    });

    presentPipeline = GraphicsPipeline::createGraphicsPipelinePtr({
        .debugName = "present-pipeline",
        .device = device,
        .VS
        {.byteCode = Filesystem::readFile(compiledShaderPath("rendergraph.vert")),},
        .FS
        {.byteCode = Filesystem::readFile(compiledShaderPath("rendergraph.frag")),},
        .descriptorSetLayouts = {
            renderGraph->getDescriptorSetLayouts("present-pass")
        },
        .pushConstantRanges{},
        .graphicsState{
            .vertexBufferBindings{
                .vertexBindingDescriptions = Vertex::vertexInputBindingDescriptions(),
                .vertexAttributeDescriptions = Vertex::vertexInputAttributeDescriptions()
            }
        },
        .dynamicRendering = {
            .enabled = true,
            .colorAttachmentCount = 1,
            .colorAttachmentFormats = {renderContext.getSwapChain()->getSwapChainImageFormat()},
        }
    });

    device.waitIdle();
    while (!window.shouldClose()) {
        window.pollEvents();

        renderGraph->execute();
    }
    device.waitIdle();
}
