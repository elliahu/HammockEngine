#include <hammock/hammock.h>

using namespace hammock;
using namespace rendergraph;


int main() {
    VulkanInstance instance{};
    hammock::Window window{instance, "Render Graph", 1920, 1080};
    Device device{instance, window.getSurface()};
    DeviceStorage storage{device};
    RenderContext context{window, device};

    std::unique_ptr<RenderGraph> graph = std::make_unique<RenderGraph>(device, SwapChain::MAX_FRAMES_IN_FLIGHT);

    ResourceNode swap;
    swap.name = "swap-image";
    swap.type = ResourceNode::Type::SwapChain;
    swap.isExternal = true; // this image is managed by swapchain
    swap.refs = context.getSwapChain()->getSwapChainImageRefs();
    graph->addResource(swap);

    ResourceNode depth;
    depth.name = "depth-image";
    depth.type = ResourceNode::Type::Image;
    graph->addResource(depth);

    RenderPassNode shadowPass;
    shadowPass.name = "shadow-pass";
    shadowPass.type = RenderPassNode::Type::Graphics;
    shadowPass.outputs.push_back({"depth-image"});
    shadowPass.executeFunc = [&](VkCommandBuffer commandBuffer, uint32_t frameINdex) {
    };
    graph->addPass(shadowPass);

    RenderPassNode compositionPass;
    compositionPass.name = "composition-pass";
    shadowPass.type = RenderPassNode::Type::Graphics;
    compositionPass.inputs.push_back({"depth-image"});
    compositionPass.outputs.push_back({"swap-image"});
    compositionPass.executeFunc = [&](VkCommandBuffer commandBuffer, uint32_t frameINdex) {
    };
    graph->addPass(compositionPass);
    graph->compile();

    while (!window.shouldClose()) {
        window.pollEvents();

        if (VkCommandBuffer cmd = context.beginFrame()) {
            uint32_t frameIndex = context.getFrameIndex();

            graph->execute(cmd, frameIndex);

            context.endFrame();
        }
    }
    device.waitIdle();
}
