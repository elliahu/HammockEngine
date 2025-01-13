#include "hammock/core/RenderGraph.h"

void hammock::rendergraph::RenderGraph::createResource(ResourceNode &resourceNode,
    std::variant<BufferDesc, ImageDesc> descVariant) {
    ASSERT(resourceNode.type != ResourceNode::Type::SwapChain,
           "RenderGraph::createResource sanity check: Cannot create resource for SwapChain. This should not happen.")
    ;
    ASSERT(resourceNode.refs.empty(),
           "RenderGraph::createResource sanity check: Resource node contains resource refs. Cannot create resource for node that already contains refs.")
    ;


    // check if resource should be buffered (one for each frame in flight)
    if (resourceNode.isBuffered) {
        resourceNode.refs.resize(maxFramesInFlight);
    } else
        resourceNode.refs.resize(1);

    // Check for resource type
    if (resourceNode.type == ResourceNode::Type::Buffer) {
        ASSERT(std::holds_alternative<ImageDesc>(descVariant),
               "RenderGraph::createResource sanity check: descVariant missmatch.");
        auto desc = std::get<BufferDesc>(descVariant);

        // create actual resources and corresponding refs
        for (int i = 0; i < resourceNode.refs.size(); ++i) {
            createBuffer(resourceNode.refs[i], desc);
        }
    } else if (resourceNode.type == ResourceNode::Type::Image) {
        ASSERT(std::holds_alternative<BufferDesc>(descVariant),
               "RenderGraph::createResource sanity check: descVariant missmatch.");

        auto desc = std::get<ImageDesc>(descVariant);
        // create actual resources and corresponding refs
        for (int i = 0; i < resourceNode.refs.size(); ++i) {
            createImage(resourceNode.refs[i], desc);
        }
    }
}

void hammock::rendergraph::RenderGraph::createBuffer(ResourceRef &resourceRef, BufferDesc &bufferDesc) const {
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

void hammock::rendergraph::RenderGraph::createImage(ResourceRef &resourceRef, ImageDesc &desc) const {
    ImageResourceRef ref{};

    // Create the VkImage
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = desc.imageType;
    imageInfo.extent.width = desc.width;
    imageInfo.extent.height = desc.height;
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

    // Fill attachment description
    ref.attachmentDesc = {};
    ref.attachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
    ref.attachmentDesc.loadOp = desc.loadOp;
    ref.attachmentDesc.storeOp = desc.storeOp;
    ref.attachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    ref.attachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    ref.attachmentDesc.format = desc.format;
    ref.attachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    // Final layout
    // If not, final layout depends on attachment type
    if (isDepthStencil(desc.format)) {
        ref.attachmentDesc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    } else {
        ref.attachmentDesc.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

}

void hammock::rendergraph::RenderGraph::destroyTransientResources() {
    for (auto& resource : resources) {
        auto& resourceNode = resource.second;
        if (resourceNode.isTransient) {
            destroyResource(resourceNode);
        }
    }
}

void hammock::rendergraph::RenderGraph::destroyResource(ResourceNode &resourceNode) const {
    // We need to destroy each ResourceRef of the node
    for (auto &resourceRef: resourceNode.refs) {
        // Check for resource type
        if (resourceNode.type == ResourceNode::Type::Buffer) {
            ASSERT(std::holds_alternative<BufferResourceRef>(resourceRef.resource),
                   "~RenderGraph sanity check: Node of type Buffer does not hold BufferResourceRef.");
            auto bufferRef = std::get<BufferResourceRef>(resourceRef.resource);

            ASSERT(bufferRef.buffer != VK_NULL_HANDLE,
                   "~RenderGraph sanity check: buffer handle is null. This should not happen.");
            ASSERT(bufferRef.allocation != VK_NULL_HANDLE,
                   "~RenderGraph sanity check: buffer allocation is null. This should not happen.");

            vmaDestroyBuffer(device.allocator(), bufferRef.buffer, bufferRef.allocation);
        } else if (resourceNode.type == ResourceNode::Type::Image) {
            ASSERT(std::holds_alternative<ImageResourceRef>(resourceRef.resource),
                   "~RenderGraph sanity check: Node of type Image does not hold ImageResourceRef.");
            auto imageRef = std::get<ImageResourceRef>(resourceRef.resource);

            ASSERT(imageRef.view != VK_NULL_HANDLE,
                   "~RenderGraph sanity check: Image view is null. This should not happen.");
            vkDestroyImageView(device.device(), imageRef.view, nullptr);

            // Sampler is optional
            if (imageRef.sampler != VK_NULL_HANDLE) {
                vkDestroySampler(device.device(), imageRef.sampler, nullptr);
            }

            ASSERT(imageRef.image != VK_NULL_HANDLE,
                   "~RenderGraph sanity check: Image is null. This should not happen.");
            ASSERT(imageRef.allocation != VK_NULL_HANDLE,
                   "~RenderGraph sanity check: Image allocation is null. This should not happen.");
            vmaDestroyImage(device.allocator(), imageRef.image, imageRef.allocation);
        }
    }
}
