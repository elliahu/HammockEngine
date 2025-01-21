#include "hammock/core/RenderGraph.h"

void hammock::PipelineBarrier::apply() const {
    if (node.isRenderingAttachment()) {
        // Verify that we're dealing with an image resource
        ASSERT(std::holds_alternative<ImageResourceRef>(node.refs[frameIndex].resource),
               "Node is image node but does not hold image ref");

        // Get the image reference for the current frame
        ImageResourceRef &ref = std::get<ImageResourceRef>(node.refs[frameIndex].resource);

        ASSERT(std::holds_alternative<ImageDesc>(node.desc),
               "Node is image node yet does not hold image desc.");
        const ImageDesc &desc = std::get<ImageDesc>(node.desc);
        VkImageLayout newLayout =  VK_IMAGE_LAYOUT_UNDEFINED;

        if (transitionStage == TransitionStage::RequiredLayout) {
            if (ref.currentLayout == access.requiredLayout) {
                return;
            }
            newLayout = access.requiredLayout;
        }
        else if (transitionStage == TransitionStage::FinalLayout) {
            if (ref.currentLayout == access.finalLayout) {
                return;
            }
            newLayout = access.finalLayout;
        }

        // Create image memory barrier
        VkImageMemoryBarrier imageBarrier{};
        imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageBarrier.oldLayout = ref.currentLayout;
        imageBarrier.newLayout = newLayout;
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

        switch (newLayout) {
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
        ref.currentLayout = newLayout;
    }
    // TODO support for buffer transition
}

void hammock::RenderGraph::createResource(ResourceNode &resourceNode) {
    ASSERT(resourceNode.type != ResourceNode::Type::SwapChainColorAttachment,
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
    if (resourceNode.isBuffer()) {
        ASSERT(std::holds_alternative<BufferDesc>(resourceNode.desc),
               "Buffer node without buffer desc");
        BufferDesc& desc = std::get<BufferDesc>(resourceNode.desc);

        // create actual resources and corresponding refs
        for (int i = 0; i < resourceNode.refs.size(); ++i) {
            createBuffer(resourceNode.refs[i], desc);
        }
    } else if (resourceNode.isRenderingAttachment()) {
        ASSERT(std::holds_alternative<ImageDesc>(resourceNode.desc),
               "Image node without image desc");

        ImageDesc& desc = std::get<ImageDesc>(resourceNode.desc);
        // create actual resources and corresponding refs
        for (int i = 0; i < resourceNode.refs.size(); ++i) {
            createImage(resourceNode.refs[i], desc);
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

void hammock::RenderGraph::createImage(ResourceRef &resourceRef, ImageDesc &desc) const {
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
        if (resourceNode.isBuffer()) {
            ASSERT(std::holds_alternative<BufferResourceRef>(resourceRef.resource),
                   "Node of type Buffer does not hold BufferResourceRef.");
            BufferResourceRef& bufferRef = std::get<BufferResourceRef>(resourceRef.resource);

            ASSERT(bufferRef.buffer != VK_NULL_HANDLE,
                   "buffer handle is null. This should not happen.");
            ASSERT(bufferRef.allocation != VK_NULL_HANDLE,
                   "buffer allocation is null. This should not happen.");

            vmaDestroyBuffer(device.allocator(), bufferRef.buffer, bufferRef.allocation);
        } else if (resourceNode.isRenderingAttachment()) {
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

