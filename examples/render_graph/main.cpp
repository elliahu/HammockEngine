#include <hammock/hammock.h>

using namespace hammock;


int main() {
    VulkanInstance instance{};
    hammock::Window window{instance, "Render Graph", 1920, 1080};
    Device device{instance, window.getSurface()};
    DeviceStorage storage{device};
    // TODO decouple context and window
    RenderContext renderContext{window, device};

    const auto graph = std::make_unique<RenderGraph>(device, renderContext);

    graph->addResource(ResourceNode{
        .type = ResourceNode::Type::SwapChainImage,
        .name = "swap-image",
        .desc = renderContext.getSwapChain()->getSwapChainImageDesc(),
        .refs = renderContext.getSwapChain()->getSwapChainImageRefs(),
        .isExternal = true
    });

    // This image has no resource ref so it will be created and manged by RenderGraph
    graph->addResource(ResourceNode{
        .type = ResourceNode::Type::Image,
        .name = "color-image",
        .desc = ImageDesc{
            .size = {1.0f, 1.0f}, // SwapChain relative by default
            .format = renderContext.getSwapChain()->getSwapChainImageFormat(),
            .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
        }
    });

    graph->addPass(RenderPassNode{
        .type = RenderPassNode::Type::Graphics,
        .name = "color-pass",
        .extent = renderContext.getSwapChain()->getSwapChainExtent(),
        .inputs = {},
        .outputs = {
            {
                "color-image", VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                VK_ATTACHMENT_STORE_OP_STORE
            }
        },
        .executeFunc = [&](RenderPassContext context) {
            std::cout << "Color pass executed" << std::endl;
        }
    });

    graph->addPresentPass(RenderPassNode{
        .type = RenderPassNode::Type::Graphics,
        .name = "composition-pass",
        .extent = renderContext.getSwapChain()->getSwapChainExtent(),
        .inputs = {
            {
                "color-image", VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ATTACHMENT_LOAD_OP_LOAD
            }
        },
        .outputs = {
            {
                "swap-image", VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_LOAD_OP_DONT_CARE
            }
        },
        .executeFunc = [&](RenderPassContext context) {
            std::cout << "Composition pass executed" << std::endl;
        }
    });

    graph->compile();

    while (!window.shouldClose()) {
        window.pollEvents();

        graph->execute();
    }
    device.waitIdle();
}
