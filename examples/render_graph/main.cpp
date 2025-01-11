#include <hammock/hammock.h>

using namespace hammock;
using namespace rendergraph;

int main() {
    VulkanInstance instance{};
    hammock::Window window{instance, "Render Graph", 1920, 1080};
    Device device{instance, window.getSurface()};
    DeviceStorage storage{device};
    RenderContext context{window, device};
    UserInterface ui{device, context.getSwapChainRenderPass(), storage.getDescriptorPool(), window};

    ResourceManager manager{device};
    auto colorAttachment = manager.createResource<rendergraph::FramebufferAttachment>("color1", FramebufferAttachmentInfo{});
    auto colorAttachmentNode = RenderGraphResourceNode<rendergraph::FramebufferAttachment>(ResourceType::FramebufferAttachment, "color1_node", colorAttachment);

    RenderGraph graph;
    graph
        .addRenderPass(
            RenderPass("first", RenderPassType::Graphics)
            .writeColorAttachment(colorAttachmentNode, { .type = ResourceAccessType::Write, .requiredState = ImageState::ColorAttachment})
            .get());

    graph.execute();
}