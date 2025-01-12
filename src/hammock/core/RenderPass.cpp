#include "hammock/core/RenderPass.h"

void hammock::rendergraph::RenderPass::createRenderPass(ResourceManager &manager) {
    // collect the attachments
    std::vector<Attachment *> attachments{};
    for (const auto &attachmentWrite: colorAttachmentWrites) {
        const auto &attachmentUid = attachmentWrite.first;
        const auto &write = attachmentWrite.second;
        const auto &node = write.first;
        const auto &access = write.second;
        attachments.push_back(manager.getResource(node.handle));
    }

    for (const auto &attachmentWrite: depthStencilAttachmentWrites) {
        const auto &attachmentUid = attachmentWrite.first;
        const auto &write = attachmentWrite.second;
        const auto &node = write.first;
        const auto &access = write.second;
        attachments.push_back(manager.getResource(node.handle));
    }

    // collect attachment descriptions
    std::vector<VkAttachmentDescription> attachmentDescriptions;
    for (const auto &attachment: attachments) {
        attachmentDescriptions.push_back(attachment->description);
    }

    // Collect attachment references
    std::vector<VkAttachmentReference> colorReferences;
    VkAttachmentReference depthReference = {};
    bool hasDepth = false;
    bool hasColor = false;

    uint32_t attachmentIndex = 0;
    for (auto &attachment: attachments) {
        if (attachment->isDepthStencil()) {
            // Only one depth attachment allowed
            assert(!hasDepth);
            depthReference.attachment = attachmentIndex;
            depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            hasDepth = true;
        } else {
            colorReferences.push_back({attachmentIndex, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
            hasColor = true;
        }
        attachmentIndex++;
    };

    // Default render pass setup uses only one subpass
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    if (hasColor) {
        subpass.pColorAttachments = colorReferences.data();
        subpass.colorAttachmentCount = static_cast<uint32_t>(colorReferences.size());
    }
    if (hasDepth) {
        subpass.pDepthStencilAttachment = &depthReference;
    }

    // Use subpass dependencies for attachment layout transitions
    std::array<VkSubpassDependency, 2> dependencies{};

    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    // Create render pass
    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.pAttachments = attachmentDescriptions.data();
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 2;
    renderPassInfo.pDependencies = dependencies.data();
    if (vkCreateRenderPass(device.device(), &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create render pass");
    }

    std::vector<VkImageView> attachmentViews;
    for (auto& at: attachments) {
        attachmentViews.push_back(at->getImageView());
    }

    // Find. max number of layers across attachments
    uint32_t maxLayers = 0;
    for (const auto&at: attachments) {
        if (at->subresourceRange.layerCount > maxLayers) {
            maxLayers = at->subresourceRange.layerCount;
        }
    }

    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.pAttachments = attachmentViews.data();
    framebufferInfo.attachmentCount = static_cast<uint32_t>(attachmentViews.size());
    framebufferInfo.width = width;
    framebufferInfo.height = height;
    framebufferInfo.layers = maxLayers;
    if (vkCreateFramebuffer(device.device(), &framebufferInfo, nullptr, &framebuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create framebuffer");
    }
}

hammock::rendergraph::RenderPass::~RenderPass() {
    vkDestroyRenderPass(device.device(), renderPass, nullptr);
    vkDestroyFramebuffer(device.device(), framebuffer, nullptr);
}
