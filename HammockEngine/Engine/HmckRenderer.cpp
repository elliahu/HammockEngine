#include "HmckRenderer.h"

Hmck::HmckRenderer::HmckRenderer(HmckWindow& window, HmckDevice& device) : hmckWindow{window}, hmckDevice{device}
{
	recreateSwapChain();
	recreateShadowmapRenderPass();
	recreateGbufferRenderPass();
	createCommandBuffer();
}

Hmck::HmckRenderer::~HmckRenderer()
{
	freeCommandBuffers();
	shadowmapFramebuffer = nullptr;
	gbufferFramebuffer = nullptr;
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
	}

	else if (result != VK_SUCCESS)
	{
		throw std::runtime_error("failed to present swap chain image");
	}

	isFrameStarted = false;

	currentFrameIndex = (currentFrameIndex + 1) % HmckSwapChain::MAX_FRAMES_IN_FLIGHT;
}


void Hmck::HmckRenderer::beginRenderPass(
	std::unique_ptr<HmckFramebuffer>& framebuffer,
	VkCommandBuffer commandBuffer, 
	std::vector<VkClearValue> clearValues)
{
	assert(isFrameInProgress() && "Cannot call beginRenderPass if frame is not in progress");
	assert(commandBuffer == getCurrentCommandBuffer() && "Cannot begin render pass on command buffer from a different frame");
	assert(framebuffer->renderPass && "Provided framebuffer doesn't have a render pass");

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = framebuffer->renderPass;
	renderPassInfo.framebuffer = framebuffer->framebuffer;
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = { framebuffer->width, framebuffer->height};
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(framebuffer->width);
	viewport.height = static_cast<float>(framebuffer->height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	VkRect2D scissor{ {0,0}, { framebuffer->width, framebuffer->height} };
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
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

void Hmck::HmckRenderer::beginShadowmapRenderPass(VkCommandBuffer commandBuffer)
{
	std::vector<VkClearValue> clearValues{ 1 };
	clearValues[0].depthStencil = { 1.0f, 0 };

	beginRenderPass(shadowmapFramebuffer, commandBuffer, clearValues);
}

void Hmck::HmckRenderer::beginGbufferRenderPass(VkCommandBuffer commandBuffer)
{
	std::vector<VkClearValue> clearValues{ 8 };
	clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
	clearValues[1].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
	clearValues[2].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
	clearValues[3].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
	clearValues[5].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
	clearValues[5].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
	clearValues[6].depthStencil = { 1.0f, 0 };

	beginRenderPass(gbufferFramebuffer, commandBuffer, clearValues);
}

void Hmck::HmckRenderer::endRenderPass(VkCommandBuffer commandBuffer)
{
	assert(isFrameInProgress() && "Cannot call endActiveRenderPass if frame is not in progress");
	assert(commandBuffer == getCurrentCommandBuffer() && "Cannot end render pass on command buffer from a different frame");

	vkCmdEndRenderPass(commandBuffer);
}

void Hmck::HmckRenderer::recreateShadowmapRenderPass()
{
	shadowmapFramebuffer = std::make_unique<HmckFramebuffer>(hmckDevice);

	shadowmapFramebuffer->width = OFFSCREEN_RES_WIDTH;
	shadowmapFramebuffer->height = OFFSCREEN_RES_HEIGHT;

	// Find a suitable depth format
	VkFormat depthFormat = hmckSwapChain->findDepthFormat();

	// Create sampler to sample from depth attachment
	if (shadowmapFramebuffer->createSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create depth sampler");
	}

	// Create depth attachment
	HmckAttachmentCreateInfo attachmentInfo{};
	attachmentInfo.width = OFFSCREEN_RES_WIDTH;
	attachmentInfo.height = OFFSCREEN_RES_HEIGHT;
	attachmentInfo.layerCount = 1;
	attachmentInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	attachmentInfo.format = depthFormat;
	shadowmapFramebuffer->addAttachment(attachmentInfo);

	// create renderpass with framebuffer
	if (shadowmapFramebuffer->createRenderPass() != VK_SUCCESS) {
		throw std::runtime_error("Failed to create offscreen renderpass");
	}
}

void Hmck::HmckRenderer::recreateGbufferRenderPass()
{
	gbufferFramebuffer = std::make_unique<HmckFramebuffer>(hmckDevice);

	gbufferFramebuffer->width = hmckSwapChain->width();
	gbufferFramebuffer->height = hmckSwapChain->height();

	// Find a suitable depth format
	VkFormat depthFormat = hmckSwapChain->findDepthFormat();

	// Six attachments (6 color, 1 depth)
	HmckAttachmentCreateInfo attachmentInfo = {};
	attachmentInfo.width = gbufferFramebuffer->width;
	attachmentInfo.height = gbufferFramebuffer->height;
	attachmentInfo.layerCount = 1;
	attachmentInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

	// Color attachments
	// Attachment 0: position
	attachmentInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
	gbufferFramebuffer->addAttachment(attachmentInfo);

	// Attachment 1: albedo (color)
	attachmentInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	gbufferFramebuffer->addAttachment(attachmentInfo);

	// Attachment 2: normal
	attachmentInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
	gbufferFramebuffer->addAttachment(attachmentInfo);

	// Attachment 3: roughness
	attachmentInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
	gbufferFramebuffer->addAttachment(attachmentInfo);

	// Attachment 4: metalness
	attachmentInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
	gbufferFramebuffer->addAttachment(attachmentInfo);

	// Attachment 5: ambient occlusion
	attachmentInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
	gbufferFramebuffer->addAttachment(attachmentInfo);

	// Attachment 6: Depth attachment
	attachmentInfo.format = depthFormat;
	attachmentInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	gbufferFramebuffer->addAttachment(attachmentInfo);

	// Create sampler to sample from color attachments
	if (gbufferFramebuffer->createSampler(VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create depth sampler");
	}

	// create renderpass with framebuffer
	if (gbufferFramebuffer->createRenderPass() != VK_SUCCESS) {
		throw std::runtime_error("Failed to create offscreen renderpass");
	}
}
