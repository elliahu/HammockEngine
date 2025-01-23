#include "hammock/core/RenderGraph.h"

void hammock::PipelineBarrier::apply() const {
    if (node.isRenderingAttachment()) {
        VkImageLayout newLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        if (transitionStage == TransitionStage::RequiredLayout) {
            newLayout = access.requiredLayout;
        } else if (transitionStage == TransitionStage::FinalLayout) {
            newLayout = access.finalLayout;
        }

        if (node.isSwapChainAttachment()) {
            VkImageSubresourceRange subresourceRange = {};
            subresourceRange.aspectMask = node.isDepthAttachment()
                    ? VK_IMAGE_ASPECT_DEPTH_BIT
                    : VK_IMAGE_ASPECT_COLOR_BIT;
            subresourceRange.baseMipLevel = 0;
            subresourceRange.levelCount = 1;
            subresourceRange.baseArrayLayer = 0;
            subresourceRange.layerCount = 1;



            transitionImageLayout(
                renderContext.getCurrentCommandBuffer(),
                node.isDepthAttachment() ? renderContext.getSwapChain()->getDepthImage(renderContext.getImageIndex()) : renderContext.getSwapChain()->getImage(renderContext.getImageIndex()),
                VK_IMAGE_LAYOUT_UNDEFINED, newLayout, subresourceRange);
            return;
        }

        const experimental::ResourceHandle handle = node.resolve(rm, renderContext.getFrameIndex());
        auto * image = rm.getResource<experimental::Image>(handle);

        image->transitionLayout(renderContext.getCurrentCommandBuffer(), newLayout);
    }
    else if (node.isBuffer()) {
        // TODO support for buffer transition
    }
}
