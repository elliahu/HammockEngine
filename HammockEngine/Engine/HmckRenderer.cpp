#include "HmckRenderer.h"

Hmck::Renderer::Renderer(Window& window, Device& device, std::unique_ptr<Scene>& scene) : window{ window }, device{ device }, scene{ scene }
{
	recreateSwapChain();
	createCommandBuffer();
	createVertexBuffer();
	createIndexBuffer();
}

Hmck::Renderer::~Renderer()
{
	freeCommandBuffers();
}


void Hmck::Renderer::createCommandBuffer()
{
	commandBuffers.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = device.getCommandPool();
	allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

	if (vkAllocateCommandBuffers(device.device(), &allocInfo, commandBuffers.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate command buffer");
	}
}


void Hmck::Renderer::freeCommandBuffers()
{
	vkFreeCommandBuffers(
		device.device(),
		device.getCommandPool(),
		static_cast<uint32_t>(commandBuffers.size()),
		commandBuffers.data());
	commandBuffers.clear();
}

void Hmck::Renderer::createVertexBuffer()
{
	// copy data to staging memory on device, then copy from staging to v/i memory
	uint32_t vertexCount = static_cast<uint32_t>(scene->vertices.size());
	assert(vertexCount >= 3 && "Vertex count must be at least 3");
	VkDeviceSize bufferSize = sizeof(scene->vertices[0]) * vertexCount;
	uint32_t vertexSize = sizeof(scene->vertices[0]);

	Buffer stagingBuffer{
		device,
		vertexSize,
		vertexCount,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	};


	stagingBuffer.map();
	stagingBuffer.writeToBuffer((void*)scene->vertices.data());

	sceneVertexBuffer = std::make_unique<Buffer>(
		device,
		vertexSize,
		vertexCount,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		);

	device.copyBuffer(stagingBuffer.getBuffer(), sceneVertexBuffer->getBuffer(), bufferSize);

	if (scene->hasSkybox)
	{
		uint32_t vertexCount = static_cast<uint32_t>(scene->skyboxVertices.size());
		assert(vertexCount >= 3 && "Vertex count must be at least 3");
		VkDeviceSize bufferSize = sizeof(scene->skyboxVertices[0]) * vertexCount;
		uint32_t vertexSize = sizeof(scene->skyboxVertices[0]);

		Buffer stagingBuffer{
			device,
			vertexSize,
			vertexCount,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		};


		stagingBuffer.map();
		stagingBuffer.writeToBuffer((void*)scene->skyboxVertices.data());

		skyboxVertexBuffer = std::make_unique<Buffer>(
			device,
			vertexSize,
			vertexCount,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
			);

		device.copyBuffer(stagingBuffer.getBuffer(), skyboxVertexBuffer->getBuffer(), bufferSize);
	}
}

void Hmck::Renderer::createIndexBuffer()
{
	uint32_t indexCount = static_cast<uint32_t>(scene->indices.size());

	VkDeviceSize bufferSize = sizeof(scene->indices[0]) * indexCount;
	uint32_t indexSize = sizeof(scene->indices[0]);

	Buffer stagingBuffer{
		device,
		indexSize,
		indexCount,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	};

	stagingBuffer.map();
	stagingBuffer.writeToBuffer((void*)scene->indices.data());

	sceneIndexBuffer = std::make_unique<Buffer>(
		device,
		indexSize,
		indexCount,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		);


	device.copyBuffer(stagingBuffer.getBuffer(), sceneIndexBuffer->getBuffer(), bufferSize);

	if (scene->hasSkybox)
	{
		uint32_t indexCount = static_cast<uint32_t>(scene->skyboxIndices.size());

		VkDeviceSize bufferSize = sizeof(scene->skyboxIndices[0]) * indexCount;
		uint32_t indexSize = sizeof(scene->skyboxIndices[0]);

		Buffer stagingBuffer{
			device,
			indexSize,
			indexCount,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		};

		stagingBuffer.map();
		stagingBuffer.writeToBuffer((void*)scene->skyboxIndices.data());

		skyboxIndexBuffer = std::make_unique<Buffer>(
			device,
			indexSize,
			indexCount,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
			);


		device.copyBuffer(stagingBuffer.getBuffer(), skyboxIndexBuffer->getBuffer(), bufferSize);
	}
}



void Hmck::Renderer::recreateSwapChain()
{
	auto extent = window.getExtent();
	while (extent.width == 0 || extent.height == 0)
	{
		extent = window.getExtent();
		glfwWaitEvents();
	}
	vkDeviceWaitIdle(device.device());

	if (hmckSwapChain == nullptr)
	{
		hmckSwapChain = std::make_unique<SwapChain>(device, extent);
	}
	else {
		std::shared_ptr<SwapChain> oldSwapChain = std::move(hmckSwapChain);
		hmckSwapChain = std::make_unique<SwapChain>(device, extent, oldSwapChain);

		if (!oldSwapChain->compareSwapFormats(*hmckSwapChain.get()))
		{
			throw std::runtime_error("Swapchain image (or detph) format has changed");
		}
	}

	//
}


VkCommandBuffer Hmck::Renderer::beginFrame()
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

void Hmck::Renderer::endFrame()
{
	assert(isFrameStarted && "Cannot call endFrame while frame is not in progress");
	auto commandBuffer = getCurrentCommandBuffer();

	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to record command buffer");
	}

	auto result = hmckSwapChain->submitCommandBuffers(&commandBuffer, &currentImageIndex);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || window.wasWindowResized())
	{
		// Window was resized (resolution was changed)
		window.resetWindowResizedFlag();
		recreateSwapChain();
	}

	else if (result != VK_SUCCESS)
	{
		throw std::runtime_error("failed to present swap chain image");
	}

	isFrameStarted = false;

	currentFrameIndex = (currentFrameIndex + 1) % SwapChain::MAX_FRAMES_IN_FLIGHT;
}


void Hmck::Renderer::beginRenderPass(
	std::unique_ptr<Framebuffer>& framebuffer,
	VkCommandBuffer commandBuffer,
	std::vector<VkClearValue> clearValues)
{
	// TODO delete this method when no longer used
	assert(isFrameInProgress() && "Cannot call beginRenderPass if frame is not in progress");
	assert(commandBuffer == getCurrentCommandBuffer() && "Cannot begin render pass on command buffer from a different frame");
	assert(framebuffer->renderPass && "Provided framebuffer doesn't have a render pass");

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = framebuffer->renderPass;
	renderPassInfo.framebuffer = framebuffer->framebuffer;
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = { framebuffer->width, framebuffer->height };
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport = Hmck::Init::viewport(
		static_cast<float>(framebuffer->width),
		static_cast<float>(framebuffer->height),
		0.0f, 1.0f);
	VkRect2D scissor{ {0,0}, { framebuffer->width, framebuffer->height} };
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void Hmck::Renderer::beginRenderPass(Framebuffer& framebuffer, VkCommandBuffer commandBuffer, std::vector<VkClearValue> clearValues)
{
	assert(isFrameInProgress() && "Cannot call beginRenderPass if frame is not in progress");
	assert(commandBuffer == getCurrentCommandBuffer() && "Cannot begin render pass on command buffer from a different frame");
	assert(framebuffer.renderPass && "Provided framebuffer doesn't have a render pass");

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = framebuffer.renderPass;
	renderPassInfo.framebuffer = framebuffer.framebuffer;
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = { framebuffer.width, framebuffer.height };
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport = Hmck::Init::viewport(
		static_cast<float>(framebuffer.width),
		static_cast<float>(framebuffer.height),
		0.0f, 1.0f);
	VkRect2D scissor{ {0,0}, { framebuffer.width, framebuffer.height} };
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void Hmck::Renderer::beginSwapChainRenderPass(VkCommandBuffer commandBuffer)
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

	VkViewport viewport = Hmck::Init::viewport(
		static_cast<float>(hmckSwapChain->getSwapChainExtent().width),
		static_cast<float>(hmckSwapChain->getSwapChainExtent().height),
		0.0f, 1.0f);
	VkRect2D scissor{ {0,0}, hmckSwapChain->getSwapChainExtent() };
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void Hmck::Renderer::endRenderPass(VkCommandBuffer commandBuffer)
{
	assert(isFrameInProgress() && "Cannot call endActiveRenderPass if frame is not in progress");
	assert(commandBuffer == getCurrentCommandBuffer() && "Cannot end render pass on command buffer from a different frame");

	vkCmdEndRenderPass(commandBuffer);
}


void Hmck::Renderer::bindSkyboxVertexBuffer(VkCommandBuffer commandBuffer)
{
	// skybox
	VkDeviceSize offsets[] = { 0 };
	VkBuffer buffers[] = { skyboxVertexBuffer->getBuffer() };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
	vkCmdBindIndexBuffer(commandBuffer, skyboxIndexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
}
void Hmck::Renderer::bindVertexBuffer(VkCommandBuffer commandBuffer)
{
	VkDeviceSize offsets[] = { 0 };
	VkBuffer buffers[] = { sceneVertexBuffer->getBuffer() };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
	vkCmdBindIndexBuffer(commandBuffer, sceneIndexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
}
