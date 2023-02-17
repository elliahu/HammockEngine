#include "HmckRenderer.h"

Hmck::HmckRenderer::HmckRenderer(HmckWindow& window, HmckDevice& device) : hmckWindow{window}, hmckDevice{device}
{
	recreateSwapChain();
	recreateOffscreenRenderPass();
	createCommandBuffer();
}

Hmck::HmckRenderer::~HmckRenderer()
{
	freeCommandBuffers();
	freeOffscreenRenderPass();
}


void Hmck::HmckRenderer::createCommandBuffer()
{
	commandBuffers.resize(HmckSwapChain::MAX_FRAMES_IN_FLIGHT);

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = hmckDevice.getCommandPool();
	allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

	if (vkAllocateCommandBuffers(hmckDevice.device(), &allocInfo, commandBuffers.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate command buffer");
	}
}


void Hmck::HmckRenderer::freeCommandBuffers()
{
	vkFreeCommandBuffers(
		hmckDevice.device(),
		hmckDevice.getCommandPool(),
		static_cast<uint32_t>(commandBuffers.size()),
		commandBuffers.data());
	commandBuffers.clear();
}

void Hmck::HmckRenderer::freeOffscreenRenderPass()
{
	vkDestroyImageView(hmckDevice.device(), offscreenRenderPass.depth.view, nullptr);
	vkDestroyImage(hmckDevice.device(), offscreenRenderPass.depth.image, nullptr);
	vkFreeMemory(hmckDevice.device(), offscreenRenderPass.depth.mem, nullptr);

	vkDestroyRenderPass(hmckDevice.device(), offscreenRenderPass.renderPass, nullptr);
	vkDestroySampler(hmckDevice.device(), offscreenRenderPass.sampler, nullptr);
	vkDestroyFramebuffer(hmckDevice.device(), offscreenRenderPass.frameBuffer, nullptr);
}


void Hmck::HmckRenderer::recreateSwapChain()
{
	auto extent = hmckWindow.getExtent();
	while (extent.width == 0 || extent.height == 0)
	{
		extent = hmckWindow.getExtent();
		glfwWaitEvents();
	}
	vkDeviceWaitIdle(hmckDevice.device());

	if (hmckSwapChain == nullptr)
	{
		hmckSwapChain = std::make_unique<HmckSwapChain>(hmckDevice, extent);
	}
	else {
		std::shared_ptr<HmckSwapChain> oldSwapChain = std::move(hmckSwapChain);
		hmckSwapChain = std::make_unique<HmckSwapChain>(hmckDevice, extent, oldSwapChain);

		if (!oldSwapChain->compareSwapFormats(*hmckSwapChain.get()))
		{
			throw std::runtime_error("Swapchain image (or detph) format has changed");
		}
	}

	//
}

VkCommandBuffer Hmck::HmckRenderer::beginFrame()
{
	assert(!isFrameStarted && "Cannot call beginFrame while already in progress");

	auto result = hmckSwapChain->acquireNextImage(&currentImageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		recreateSwapChain();
		return nullptr;
	}

	if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		throw std::runtime_error("failed to aquire swap chain image!");
	}

	isFrameStarted = true;

	auto commandBuffer = getCurrentCommandBuffer();
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to begin recording commadn buffer");
	}
	return commandBuffer;
}

void Hmck::HmckRenderer::endFrame()
{
	assert(isFrameStarted && "Cannot call endFrame while frame is not in progress");
	auto commandBuffer = getCurrentCommandBuffer();

	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to record command buffer");
	}

	auto result = hmckSwapChain->submitCommandBuffers(&commandBuffer, &currentImageIndex);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || hmckWindow.wasWindowResized())
	{
		// Window was resized (resolution was changed)
		hmckWindow.resetWindowResizedFlag();
		recreateSwapChain();

		freeOffscreenRenderPass();
		recreateOffscreenRenderPass();
	}

	else if (result != VK_SUCCESS)
	{
		throw std::runtime_error("failed to present swap chain image");
	}

	isFrameStarted = false;

	currentFrameIndex = (currentFrameIndex + 1) % HmckSwapChain::MAX_FRAMES_IN_FLIGHT;
}

void Hmck::HmckRenderer::beginSwapChainRenderPass(VkCommandBuffer commandBuffer)
{
	assert(isFrameInProgress() && "Cannot call beginSwapChainRenderPass if frame is not in progress");
	assert(commandBuffer == getCurrentCommandBuffer() && "Cannot begin render pass on command buffer from a different frame");

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = hmckSwapChain->getRenderPass();
	renderPassInfo.framebuffer = hmckSwapChain->getFrameBuffer(currentImageIndex);

	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = hmckSwapChain->getSwapChainExtent();

	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = HMCK_CLEAR_COLOR; // clear color
	clearValues[1].depthStencil = { 1.0f, 0 };
	
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(hmckSwapChain->getSwapChainExtent().width);
	viewport.height = static_cast<float>(hmckSwapChain->getSwapChainExtent().height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	VkRect2D scissor{ {0,0}, hmckSwapChain->getSwapChainExtent() };
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void Hmck::HmckRenderer::endSwapChainRenderPass(VkCommandBuffer commandBuffer)
{
	assert(isFrameInProgress() && "Cannot call endSwapChainRenderPass if frame is not in progress");
	assert(commandBuffer == getCurrentCommandBuffer() && "Cannot end render pass on command buffer from a different frame");

	vkCmdEndRenderPass(commandBuffer);
}

void Hmck::HmckRenderer::beginOffscreenRenderPass(VkCommandBuffer commandBuffer)
{
	assert(isFrameInProgress() && "Cannot call beginOffscreenRenderPass if frame is not in progress");
	assert(commandBuffer == getCurrentCommandBuffer() && "Cannot beginOffscreenRenderPass on command buffer from a different frame");

	// TODO 

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = offscreenRenderPass.renderPass;
	renderPassInfo.framebuffer = offscreenRenderPass.frameBuffer;

	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = hmckSwapChain->getSwapChainExtent();

	std::array<VkClearValue, 1> clearValues{};
	clearValues[0].depthStencil = { 1.0f, 0 };

	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(hmckSwapChain->getSwapChainExtent().width);
	viewport.height = static_cast<float>(hmckSwapChain->getSwapChainExtent().height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	VkRect2D scissor{ {0,0}, hmckSwapChain->getSwapChainExtent() };
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void Hmck::HmckRenderer::endOffscreenRenderPass(VkCommandBuffer commandBuffer)
{
	assert(isFrameInProgress() && "Cannot call endOffscreenRenderPass if frame is not in progress");
	assert(commandBuffer == getCurrentCommandBuffer() && "Cannot endOffscreenRenderPass on command buffer from a different frame");

	vkCmdEndRenderPass(commandBuffer);
}

void Hmck::HmckRenderer::recreateOffscreenRenderPass()
{
	offscreenRenderPass.width = hmckSwapChain->width();
	offscreenRenderPass.height = hmckSwapChain->height();

	// Find a suitable depth format
	VkFormat fbDepthFormat = hmckSwapChain->findDepthFormat();

	// Create sampler to sample from the attachment in the fragment shader
	VkSamplerCreateInfo samplerInfo = Hmck::Init::samplerCreateInfo();
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = samplerInfo.addressModeU;
	samplerInfo.addressModeW = samplerInfo.addressModeU;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.maxAnisotropy = 1.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 1.0f;
	samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	if (vkCreateSampler(hmckDevice.device(), &samplerInfo, nullptr, &offscreenRenderPass.sampler) != VK_SUCCESS) {
		throw std::runtime_error("failed to create sampler");
	}

	// Depth stencil attachment
	VkImageCreateInfo image = Hmck::Init::imageCreateInfo();
	image.imageType = VK_IMAGE_TYPE_2D;
	image.extent.width = offscreenRenderPass.width;
	image.extent.height = offscreenRenderPass.height;
	image.extent.depth = 1;
	image.mipLevels = 1;
	image.arrayLayers = 1;
	image.samples = VK_SAMPLE_COUNT_1_BIT;
	image.tiling = VK_IMAGE_TILING_OPTIMAL;
	image.format = fbDepthFormat;																// Depth stencil attachment
	image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

	if (vkCreateImage(hmckDevice.device(), &image, nullptr, &offscreenRenderPass.depth.image) != VK_SUCCESS) {
		throw std::runtime_error("failed to create image");
	}
	VkMemoryAllocateInfo memAlloc = Hmck::Init::memoryAllocateInfo();
	VkMemoryRequirements memReqs;
	vkGetImageMemoryRequirements(hmckDevice.device(), offscreenRenderPass.depth.image, &memReqs);
	memAlloc.allocationSize = memReqs.size;
	memAlloc.memoryTypeIndex = hmckDevice.findMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	if (vkAllocateMemory(hmckDevice.device(), &memAlloc, nullptr, &offscreenRenderPass.depth.mem) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate memory");
	}
	if (vkBindImageMemory(hmckDevice.device(), offscreenRenderPass.depth.image, offscreenRenderPass.depth.mem, 0) != VK_SUCCESS) {
		throw std::runtime_error("failed to bind image memory");
	}

	VkImageViewCreateInfo depthStencilView = Hmck::Init::imageViewCreateInfo();
	depthStencilView.viewType = VK_IMAGE_VIEW_TYPE_2D;
	depthStencilView.format = fbDepthFormat;
	depthStencilView.subresourceRange = {};
	depthStencilView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	depthStencilView.subresourceRange.baseMipLevel = 0;
	depthStencilView.subresourceRange.levelCount = 1;
	depthStencilView.subresourceRange.baseArrayLayer = 0;
	depthStencilView.subresourceRange.layerCount = 1;
	depthStencilView.image = offscreenRenderPass.depth.image;
	if (vkCreateImageView(hmckDevice.device(), &depthStencilView, nullptr, &offscreenRenderPass.depth.view) != VK_SUCCESS) {
		throw std::runtime_error("failed to create image view");
	}

	// Create a separate render pass for the offscreen rendering as it may differ from the one used for scene rendering

	VkAttachmentDescription attachmentDescription{};
	attachmentDescription.format = fbDepthFormat;
	attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
	attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;							// Clear depth at beginning of the render pass
	attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;						// We will read from depth, so it's important to store the depth attachment results
	attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;					// We don't care about initial layout of the attachment
	attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;// Attachment will be transitioned to shader read at render pass end

	VkAttachmentReference depthReference = {};
	depthReference.attachment = 0;
	depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 0;													// No color attachments
	subpass.pDepthStencilAttachment = &depthReference;

	// Use subpass dependencies for layout transitions
	std::array<VkSubpassDependency, 2> dependencies;

	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	// Create the actual renderpass
	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &attachmentDescription;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
	renderPassInfo.pDependencies = dependencies.data();

	if (vkCreateRenderPass(hmckDevice.device(), &renderPassInfo, nullptr, &offscreenRenderPass.renderPass) != VK_SUCCESS) {
		throw std::runtime_error("failed to create offscreen renderpass");
	}

	VkImageView attachments[1];
	attachments[0] = offscreenRenderPass.depth.view;

	VkFramebufferCreateInfo fbufCreateInfo = Hmck::Init::framebufferCreateInfo();
	fbufCreateInfo.renderPass = offscreenRenderPass.renderPass;
	fbufCreateInfo.attachmentCount = 1;
	fbufCreateInfo.pAttachments = attachments;
	fbufCreateInfo.width = offscreenRenderPass.width;
	fbufCreateInfo.height = offscreenRenderPass.height;
	fbufCreateInfo.layers = 1;

	if (vkCreateFramebuffer(hmckDevice.device(), &fbufCreateInfo, nullptr, &offscreenRenderPass.frameBuffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to create offscreen framebuffer");
	}

	// Fill a descriptor for later use in a descriptor set
	offscreenRenderPass.descriptor.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
	offscreenRenderPass.descriptor.imageView = offscreenRenderPass.depth.view;
	offscreenRenderPass.descriptor.sampler = offscreenRenderPass.sampler;
}