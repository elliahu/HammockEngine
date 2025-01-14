#include <hammock/hammock.h>

using namespace hammock;
using namespace rendergraph;


int main() {
    VulkanInstance instance{};
    hammock::Window window{instance, "Render Graph", 1920, 1080};
    Device device{instance, window.getSurface()};
    DeviceStorage storage{device};
    // TODO decouple context and window, this is totaly wrong
    RenderContext context{window, device};

    std::unique_ptr<RenderGraph> graph = std::make_unique<RenderGraph>(device, context);

    ResourceNode swap;
    swap.name = "swap-image";
    swap.type = ResourceNode::Type::SwapChain;
    swap.isExternal = true; // this image is managed by swapchain
    swap.refs = context.getSwapChain()->getSwapChainImageRefs();
    swap.desc = context.getSwapChain()->getSwapChainImageDesc();
    graph->addResource(swap);

    // This image has no resource ref as it will be created and manged by RenderGraph
    ResourceNode depth;
    depth.name = "depth-image";
    depth.type = ResourceNode::Type::Image;
    depth.desc = ImageDesc{
        .size = {1.0f, 1.0f}, // SwapChain relative by default
        .format = context.getSwapChain()->getSwapChainDepthFormat(),
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
    };
    graph->addResource(depth);

    RenderPassNode shadowPass;
    shadowPass.name = "shadow-pass";
    shadowPass.type = RenderPassNode::Type::Graphics;
    shadowPass.outputs.push_back({
        "depth-image", VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        VK_ATTACHMENT_STORE_OP_STORE
    });
    shadowPass.executeFunc = [&](VkCommandBuffer commandBuffer, uint32_t frameINdex) {
    };
    graph->addPass(shadowPass);

    RenderPassNode compositionPass;
    compositionPass.name = "composition-pass";
    shadowPass.type = RenderPassNode::Type::Graphics;
    compositionPass.inputs.push_back({
        "depth-image", VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ATTACHMENT_LOAD_OP_LOAD
    });
    compositionPass.outputs.push_back({
        "swap-image", VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_LOAD_OP_DONT_CARE
    });
    compositionPass.executeFunc = [&](VkCommandBuffer commandBuffer, uint32_t frameINdex) {
    };
    graph->addPass(compositionPass);
    graph->compile();

    while (!window.shouldClose()) {
        window.pollEvents();

        graph->execute();
    }
    device.waitIdle();
}
