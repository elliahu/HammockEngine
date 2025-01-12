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
    auto colorAttachment = manager.createResource<rendergraph::Attachment>(AttachmentDescription{
        .name = "color",
        .width = 1920, .height = 1080, .channels = 4,
        .format = VK_FORMAT_R8G8B8A8_SRGB,
        .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
    });
    auto colorAttachmentNode = RenderGraphResourceNode<rendergraph::Attachment>(ResourceType::FramebufferAttachment, "color1_node", colorAttachment);

    RenderGraph graph;
    graph
        .addRenderPass(
            RenderPass("first", RenderPassType::Graphics)
            .writeColorAttachment(colorAttachmentNode, { .type = ResourceAccessType::Write, .requiredState = ImageState::ColorAttachment})
            .build());

    graph.execute();
}