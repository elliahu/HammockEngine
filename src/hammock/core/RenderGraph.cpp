#include "hammock/core/RenderGraph.h"

void hammock::Barrier::apply() const {
    if (node.type == ResourceNode::Type::Image) {
        // Verify that we're dealing with an image resource
        ASSERT(std::holds_alternative<ImageResourceRef>(node.refs[frameIndex].resource),
               "Node is image node but does not hold image ref");

        // Get the image reference for the current frame
        ImageResourceRef &ref = std::get<ImageResourceRef>(node.refs[frameIndex].resource);

        ASSERT(std::holds_alternative<ImageDesc>(node.desc),
               "Node is image node yet does not hold image desc.");
        const ImageDesc &desc = std::get<ImageDesc>(node.desc);

        // Skip if no transition is needed, this should already be checked but anyway...
        if (ref.currentLayout == access.requiredLayout) {
            return;
        }

        // Create image memory barrier
        VkImageMemoryBarrier imageBarrier{};
        imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageBarrier.oldLayout = ref.currentLayout;
        imageBarrier.newLayout = access.requiredLayout;
        imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageBarrier.image = ref.image;

        // Define image subresource range
        imageBarrier.subresourceRange.aspectMask =
                isDepthStencil(desc.format)
                    ? VK_IMAGE_ASPECT_DEPTH_BIT
                    : VK_IMAGE_ASPECT_COLOR_BIT;
        imageBarrier.subresourceRange.baseMipLevel = 0;
        imageBarrier.subresourceRange.levelCount = desc.mips;
        imageBarrier.subresourceRange.baseArrayLayer = 0;
        imageBarrier.subresourceRange.layerCount = desc.layers;

        // Set access masks based on layout transitions
        switch (ref.currentLayout) {
            case VK_IMAGE_LAYOUT_UNDEFINED:
                imageBarrier.srcAccessMask = 0;
                break;
            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                break;
            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                imageBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
                break;
            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                imageBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                break;
            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                imageBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                break;
            default:
                imageBarrier.srcAccessMask = 0;
        }

        switch (access.requiredLayout) {
            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                imageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                break;
            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                imageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                break;
            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                imageBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                break;
            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                imageBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                break;
            default:
                imageBarrier.dstAccessMask = 0;
        }

        // Issue the pipeline barrier command
        vkCmdPipelineBarrier(
            commandBuffer, // commandBuffer
            access.stageFlags, // srcStageMask
            access.stageFlags, // dstStageMask
            0, // dependencyFlags
            0, nullptr, // Memory barriers
            0, nullptr, // Buffer memory barriers
            1, &imageBarrier // Image memory barriers
        );

        // Update the current layout
        ref.currentLayout = access.requiredLayout;
    }
    // TODO support for buffer transition
}

void hammock::RenderGraph::createResource(ResourceNode &resourceNode, ResourceAccess& access) {
    ASSERT(resourceNode.type != ResourceNode::Type::SwapChainImage,
           "Cannot create resource for SwapChain. This should not happen.")
    ;
    ASSERT(resourceNode.refs.empty(),
           "Resource node contains resource refs. Cannot create resource for node that already contains refs.")
    ;


    // check if resource should be buffered (one for each frame in flight)
    if (resourceNode.isBuffered) {
        resourceNode.refs.resize(maxFramesInFlight);
    } else
        resourceNode.refs.resize(1);

    // Check for resource type
    if (resourceNode.type == ResourceNode::Type::Buffer) {
        ASSERT(std::holds_alternative<BufferDesc>(resourceNode.desc),
               "Buffer node without buffer desc");
        BufferDesc& desc = std::get<BufferDesc>(resourceNode.desc);

        // create actual resources and corresponding refs
        for (int i = 0; i < resourceNode.refs.size(); ++i) {
            createBuffer(resourceNode.refs[i], desc);
        }
    } else if (resourceNode.type == ResourceNode::Type::Image) {
        ASSERT(std::holds_alternative<ImageDesc>(resourceNode.desc),
               "Image node without image desc");

        ImageDesc& desc = std::get<ImageDesc>(resourceNode.desc);
        // create actual resources and corresponding refs
        for (int i = 0; i < resourceNode.refs.size(); ++i) {
            createImage(resourceNode.refs[i], desc, access);

            // Fill attachment description
            desc.attachmentDesc = {};
            desc.attachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
            desc.attachmentDesc.loadOp = access.loadOp;
            desc.attachmentDesc.storeOp = access.storeOp;
            desc.attachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            desc.attachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            desc.attachmentDesc.format = desc.format;
            desc.attachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            // Final layout
            // If not, final layout depends on attachment type
            if (isDepthStencil(desc.format)) {
                desc.attachmentDesc.finalLayout = access.requiredLayout;
            } else {
                desc.attachmentDesc.finalLayout = access.requiredLayout;

            }
        }
    }
}

void hammock::RenderGraph::createBuffer(ResourceRef &resourceRef, BufferDesc &bufferDesc) const {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = alignSize(bufferDesc.instanceSize, bufferDesc.minOffsetAlignment) * bufferDesc.
                      instanceCount;
    bufferInfo.usage = bufferDesc.usageFlags;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

    BufferResourceRef ref{};
    checkResult(vmaCreateBuffer(device.allocator(), &bufferInfo, &allocInfo, &ref.buffer, &ref.allocation,
                                nullptr));

    resourceRef.resource = ref;
}

void hammock::RenderGraph::createImage(ResourceRef &resourceRef, ImageDesc &desc, ResourceAccess& access) const {
    ImageResourceRef ref{};

    // Create the VkImage
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = desc.imageType;
    // TODO make framebuffer relative possible
    VkExtent2D extent = renderContext.getSwapChain()->getSwapChainExtent();
    imageInfo.extent.width = static_cast<uint32_t>(static_cast<float>(extent.width) * desc.size.X);
    imageInfo.extent.height = static_cast<uint32_t>(static_cast<float>(extent.height) * desc.size.Y);
    imageInfo.extent.depth = desc.depth;
    imageInfo.mipLevels = desc.mips;
    imageInfo.arrayLayers = desc.layers;
    imageInfo.format = desc.format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = desc.usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.flags = 0;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

    // Allocate memory
    checkResult(vmaCreateImage(device.allocator(), &imageInfo, &allocInfo, &ref.image, &ref.allocation, nullptr));

    // Select aspect mask and layout depending on usage
    VkImageAspectFlags aspectMask = 0;
    // Color attachment
    if (desc.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
        aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    // Depth (and/or stencil) attachment
    if (desc.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
        if (isDepthFormat(desc.format)) {
            aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        }
        if (isStencilFormat(desc.format)) {
            aspectMask = aspectMask | VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    }

    assert(aspectMask > 0);

    // create image view
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = ref.image;
    viewInfo.viewType = desc.imageViewType;
    viewInfo.format = desc.format;
    viewInfo.subresourceRange.aspectMask = aspectMask;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = desc.mips;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = desc.layers;

    checkResult(vkCreateImageView(device.device(), &viewInfo, nullptr, &ref.view));

    // Create sampler
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = desc.filter;
    samplerInfo.minFilter = desc.filter;
    samplerInfo.addressModeU = desc.addressMode;
    samplerInfo.addressModeV = desc.addressMode;
    samplerInfo.addressModeW = desc.addressMode;
    samplerInfo.anisotropyEnable = VK_TRUE;

    // retrieve max anisotropy from physical device
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(device.getPhysicalDevice(), &properties);

    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = desc.borderColor;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = desc.mipmapMode;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = desc.mips;

    checkResult(vkCreateSampler(device.device(), &samplerInfo, nullptr, &ref.sampler));
    resourceRef.resource = ref;
}

void hammock::RenderGraph::destroyTransientResources() {
    for (auto& resource : resources) {
        auto& resourceNode = resource.second;
        if (resourceNode.isTransient) {
            destroyResource(resourceNode);
        }
    }
}

void hammock::RenderGraph::destroyResource(ResourceNode &resourceNode) const {
    // We need to destroy each ResourceRef of the node
    for (auto &resourceRef: resourceNode.refs) {
        // Check for resource type
        if (resourceNode.type == ResourceNode::Type::Buffer) {
            ASSERT(std::holds_alternative<BufferResourceRef>(resourceRef.resource),
                   "Node of type Buffer does not hold BufferResourceRef.");
            BufferResourceRef& bufferRef = std::get<BufferResourceRef>(resourceRef.resource);

            ASSERT(bufferRef.buffer != VK_NULL_HANDLE,
                   "buffer handle is null. This should not happen.");
            ASSERT(bufferRef.allocation != VK_NULL_HANDLE,
                   "buffer allocation is null. This should not happen.");

            vmaDestroyBuffer(device.allocator(), bufferRef.buffer, bufferRef.allocation);
        } else if (resourceNode.type == ResourceNode::Type::Image) {
            ASSERT(std::holds_alternative<ImageResourceRef>(resourceRef.resource),
                   "Node of type Image does not hold ImageResourceRef.");
            ImageResourceRef& imageRef = std::get<ImageResourceRef>(resourceRef.resource);

            ASSERT(imageRef.view != VK_NULL_HANDLE,
                   "Image view is null. This should not happen.");
            vkDestroyImageView(device.device(), imageRef.view, nullptr);

            // Sampler is optional
            if (imageRef.sampler != VK_NULL_HANDLE) {
                vkDestroySampler(device.device(), imageRef.sampler, nullptr);
            }

            ASSERT(imageRef.image != VK_NULL_HANDLE,
                   "Image is null. This should not happen.");
            ASSERT(imageRef.allocation != VK_NULL_HANDLE,
                   "Image allocation is null. This should not happen.");
            vmaDestroyImage(device.allocator(), imageRef.image, imageRef.allocation);
        }
    }
}

void hammock::RenderGraph::createRenderPassAndFramebuffers(RenderPassNode &renderPassNode) const {
    std::vector<VkAttachmentDescription> attachmentDescriptions;
    std::vector<VkAttachmentReference> colorReferences;
    VkAttachmentReference depthReference = {};
    std::vector<std::vector<VkImageView>> attachmentViews;
    attachmentViews.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);

    bool hasDepth = false;
    bool hasColor = false;
    uint32_t attachmentIndex = 0;
    uint32_t maxLayers = 0;

    ASSERT(!renderPassNode.outputs.empty(), "Render pass has no outputs");

    // loop over outputs to collect attachmetn descriptions of color/depth targets
    for (auto &access: renderPassNode.outputs) {
        ResourceNode resourceNode = resources.at(access.resourceName);

        // We skip non image outputs as they are not part of the framebuffer
        if (resourceNode.type != ResourceNode::Type::Image) { continue; }

        ImageDesc& imageDesc = std::get<ImageDesc>(resourceNode.desc);

        attachmentDescriptions.push_back(imageDesc.attachmentDesc);

        if (isDepthStencil(imageDesc.format)) {
            ASSERT(!hasDepth, "Only one depth attachment allowed");
            depthReference.attachment = attachmentIndex;
            depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            hasDepth = true;
        } else {
            colorReferences.push_back({attachmentIndex, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
            hasColor = true;
        }

        if (imageDesc.layers > maxLayers) {
            maxLayers = imageDesc.layers;
        }

        attachmentIndex++;
    }

    // collect image views
    for (int i = 0; i < attachmentViews.size(); i++) {
        std::vector<VkImageView> views = attachmentViews[i];
        for (auto &access: renderPassNode.outputs) {
            ResourceNode resourceNode = resources.at(access.resourceName);
            // We skip non image outputs as they are not part of the framebuffer
            if (resourceNode.type != ResourceNode::Type::Image) { continue; }

            ImageResourceRef& imageRef = std::get<ImageResourceRef>(resourceNode.refs[i].resource);
            views.push_back(imageRef.view);
        }
        attachmentViews[i] = views;
    }

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
    checkResult(vkCreateRenderPass(device.device(), &renderPassInfo, nullptr, &renderPassNode.renderPass));

    // create framebuffer for each frame in flight
    renderPassNode.framebuffers.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < renderPassNode.framebuffers.size(); i++) {
        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPassNode.renderPass;
        framebufferInfo.pAttachments = attachmentViews[i].data();
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachmentViews[i].size());
        framebufferInfo.width = renderPassNode.extent.width;
        framebufferInfo.height = renderPassNode.extent.height;
        framebufferInfo.layers = maxLayers;
        checkResult(vkCreateFramebuffer(device.device(), &framebufferInfo, nullptr, &renderPassNode.framebuffers[i]));
    }

}
