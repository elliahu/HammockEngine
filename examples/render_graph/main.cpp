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
    // TODO decouple context and window
    RenderContext renderContext{window, device};

    struct UniformBuffer {
        HmckMat4 mvp;
    } ubo;

    // Load geometry
    Geometry geometry{};
    Loader(geometry, device, storage).loadglTF(assetPath("models/Sphere/Sphere.glb"));

    // Create vertex and index buffers
    ResourceHandle<Buffer> vertexBuffer = storage.createVertexBuffer({
        .vertexSize = sizeof(geometry.vertices[0]),
        .vertexCount = static_cast<uint32_t>(geometry.vertices.size()),
        .data = static_cast<void *>(geometry.vertices.data())
    });

    ResourceHandle<Buffer> indexBuffer = storage.createIndexBuffer({
        .indexSize = sizeof(geometry.indices[0]),
        .indexCount = static_cast<uint32_t>(geometry.indices.size()),
        .data = static_cast<void *>(geometry.indices.data())
    });

    // Descriptors
    std::vector<ResourceHandle<VkDescriptorSet> > descriptorSets{};
    descriptorSets.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
    std::vector<ResourceHandle<Buffer> > buffers{};
    buffers.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
    ResourceHandle<DescriptorSetLayout> descriptorSetLayout = storage.createDescriptorSetLayout({
        .bindings = {
            {
                .binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS
            },
        }
    });

    for (int i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        buffers[i] = storage.createBuffer({
            .instanceSize = sizeof(UniformBuffer),
            .instanceCount = 1,
            .usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .memoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        });
    }

    for (int i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        auto fbufferInfo = storage.getBuffer(buffers[i])->descriptorInfo();
        descriptorSets[i] = storage.createDescriptorSet({
            .descriptorSetLayout = descriptorSetLayout,
            .bufferWrites = {{0, fbufferInfo}},
        });
    }

    const auto graph = std::make_unique<RenderGraph>(device, renderContext);

    ImageDesc swapImageDesc = renderContext.getSwapChain()->getSwapChainColorImageDesc();
    ImageDesc swapDepthDesc = renderContext.getSwapChain()->getSwapChainDepthImageDesc();

    graph->addResource(ResourceNode{
        .type = ResourceNode::Type::SwapChainColorAttachment,
        .name = "swap-image",
        .desc = swapImageDesc,
        .refs = renderContext.getSwapChain()->getSwapChainColorAttachmetsRefs(),
        .isExternal = true
    });

    graph->addResource(ResourceNode{
        .type = ResourceNode::Type::SwapChainDepthStencilAttachment,
        .name = "swap-depth-image",
        .desc = swapDepthDesc,
        .refs = renderContext.getSwapChain()->getSwapChainDepthAttachmetsRefs(),
        .isExternal = true
    });

    // // This image has no resource ref so it will be created and manged by RenderGraph
    // graph->addResource(ResourceNode{
    //     .type = ResourceNode::Type::Image,
    //     .name = "color-image",
    //     .desc = ImageDesc{
    //         .size = {1.0f, 1.0f}, // SwapChain relative by default
    //         .format = renderContext.getSwapChain()->getSwapChainImageFormat(),
    //         .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
    //     }
    // });

    // graph->addPass(RenderPassNode{
    //     .type = RenderPassNode::Type::Graphics,
    //     .name = "color-pass",
    //     .extent = renderContext.getSwapChain()->getSwapChainExtent(),
    //     .inputs = {},
    //     .outputs = {
    //         {
    //             "color-image", VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    //             VK_ATTACHMENT_STORE_OP_STORE
    //         }
    //     },
    //     .executeFunc = [&](RenderPassContext context) {
    //         std::cout << "Color pass executed" << std::endl;
    //     }
    // });

    std::unique_ptr<GraphicsPipeline> presentPipeline = GraphicsPipeline::createGraphicsPipelinePtr({
        .debugName = "present-pipeline",
        .device = device,
        .VS
        {.byteCode = Filesystem::readFile(compiledShaderPath("fullscreen.vert")),},
        .FS
        {.byteCode = Filesystem::readFile(compiledShaderPath("rendergraph.frag")),},
        .descriptorSetLayouts = {
            storage.getDescriptorSetLayout(descriptorSetLayout).getDescriptorSetLayout()
        },
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
            .colorAttachmentFormats = {swapImageDesc.format},
            .depthAttachmentFormat = renderContext.getSwapChain()->getSwapChainDepthFormat(),
        }
    });

    graph->addPresentPass(RenderPassNode{
        .type = RenderPassNode::Type::Graphics,
        .name = "present-pass",
        .extent = renderContext.getSwapChain()->getSwapChainExtent(),
        // .inputs = {
        //     {
        //         "color-image", VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ATTACHMENT_LOAD_OP_LOAD
        //     }
        // },
        .outputs = {
            {
                "swap-image", VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                VK_ATTACHMENT_LOAD_OP_DONT_CARE
            },
            {
                "swap-depth-image", VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                VK_ATTACHMENT_LOAD_OP_DONT_CARE
            }
        },
        .executeFunc = [&](RenderPassContext context) {
            storage.bindVertexBuffer(vertexBuffer, indexBuffer, context.commandBuffer);
            presentPipeline->bind(context.commandBuffer);

            // Update buffer data

            storage.getBuffer(buffers[context.frameIndex])->writeToBuffer(&ubo);

            storage.bindDescriptorSet(
                context.commandBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                presentPipeline->graphicsPipelineLayout,
                0, 1,
                descriptorSets[context.frameIndex],
                0,
                nullptr);

            vkCmdDraw(context.commandBuffer, 3, 1, 0, 0);
        }
    });

    graph->compile();


    // std::unique_ptr<GraphicsPipeline> colorPipeline = GraphicsPipeline::createGraphicsPipelinePtr({
    //     .debugName = "color-pipeline",
    //     .device = device,
    //     .VS
    //     {.byteCode = Filesystem::readFile(compiledShaderPath("rendergraph.vert")),},
    //     .FS
    //     {.byteCode = Filesystem::readFile(compiledShaderPath("rendergraph.frag")),},
    //     .descriptorSetLayouts = {
    //         storage.getDescriptorSetLayout(descriptorSetLayout).getDescriptorSetLayout()
    //     },
    //     .pushConstantRanges{},
    //     .graphicsState{
    //         .cullMode = VK_CULL_MODE_NONE,
    //         .vertexBufferBindings{
    //             .vertexBindingDescriptions = Vertex::vertexInputBindingDescriptions(),
    //             .vertexAttributeDescriptions = Vertex::vertexInputAttributeDescriptions()
    //         }
    //     },
    //     .renderPass = graph->getRenderPass("color-pass"),
    // });


    while (!window.shouldClose()) {
        window.pollEvents();

        graph->execute();
    }
    device.waitIdle();
}
