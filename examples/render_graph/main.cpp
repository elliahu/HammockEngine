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
    experimental::ResourceManager rm{device};
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

    const auto graph = std::make_unique<RenderGraph>(device,rm, renderContext);

    graph->addSwapChainImageResource<ResourceNode::Type::SwapChainColorAttachment>("swap-color-image");

    std::unique_ptr<GraphicsPipeline> presentPipeline = GraphicsPipeline::createGraphicsPipelinePtr({
        .debugName = "present-pipeline",
        .device = device,
        .VS
        {.byteCode = Filesystem::readFile(compiledShaderPath("rendergraph.vert")),},
        .FS
        {.byteCode = Filesystem::readFile(compiledShaderPath("rendergraph.frag")),},
        .descriptorSetLayouts = {
            storage.getDescriptorSetLayout(descriptorSetLayout).getDescriptorSetLayout()
        },
        .pushConstantRanges{},
        .graphicsState{
            .depthTest = VK_FALSE,
            .blendAtaAttachmentStates{
                Init::pipelineColorBlendAttachmentState(0xf, VK_FALSE)
            },
            .vertexBufferBindings{
                .vertexBindingDescriptions = Vertex::vertexInputBindingDescriptions(),
                .vertexAttributeDescriptions = Vertex::vertexInputAttributeDescriptions()
            }
        },
        .dynamicRendering = {
            .enabled = true,
            .colorAttachmentCount = 1,
            .colorAttachmentFormats = {renderContext.getSwapChain()->getSwapChainImageFormat()},
            .depthAttachmentFormat = VK_FORMAT_UNDEFINED,
        }
    });

    graph->addPass<RenderPassType::Graphics>("present-pass", renderContext.getSwapChain()->getSwapChainExtent())
        .write(ResourceAccess{
            .resourceName = "swap-color-image",
            .requiredLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        })
        .execute([&](RenderPassContext context) {
            storage.bindVertexBuffer(vertexBuffer, indexBuffer, context.commandBuffer);
            presentPipeline->bind(context.commandBuffer);

            // Update buffer data
            HmckMat4 projection = Projection().perspective(45.f, renderContext.getAspectRatio(), 0.1f, 100.f);
            HmckMat4 view = Projection().view(HmckVec3{0.f, 0.f, 6.0f}, HmckVec3{0.f,0.f,0.f}, Projection().upPosY());
            ubo.mvp = projection * view;

            storage.getBuffer(buffers[context.frameIndex])->writeToBuffer(&ubo);

            storage.bindDescriptorSet(
                context.commandBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                presentPipeline->graphicsPipelineLayout,
                0, 1,
                descriptorSets[context.frameIndex],
                0,
                nullptr);

            vkCmdDrawIndexed(context.commandBuffer, geometry.indices.size(), 1, 0, 0,0);
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
