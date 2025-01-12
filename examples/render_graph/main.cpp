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
    auto colorAttachment = manager.createResource<rendergraph::Attachment>("color1", AttachmentDescription{
        .width = 1920, .height = 1080, .channels = 4,
        .layout = VK_IMAGE_LAYOUT_UNDEFINED,
        .format = VK_FORMAT_R8G8B8A8_SRGB,
    });
    auto colorAttachmentNode = RenderGraphResourceNode<rendergraph::Attachment>(ResourceType::FramebufferAttachment, "color1_node", colorAttachment);

    RenderGraph graph;
    graph
        .addRenderPass(
            RenderPass("first", RenderPassType::Graphics)
            .writeColorAttachment(colorAttachmentNode, { .type = ResourceAccessType::Write, .requiredState = ImageState::ColorAttachment})
            .get());

    graph.execute();
}