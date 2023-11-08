#include "HmckRenderer.h"

Hmck::Renderer::Renderer(Window& window, Device& device) : window{window}, device{device}
{
	recreateSwapChain();
	createCommandBuffer();
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

void Hmck::Renderer::renderEntity(
	std::unique_ptr<Scene>& scene, 
	VkCommandBuffer commandBuffer, 
	std::shared_ptr<Entity>& entity, 
	VkPipelineLayout pipelineLayout,
	std::unique_ptr<Buffer>& transformBuffer,
	std::unique_ptr<Buffer>& materialPropertyBuffer)
{
	// don't render invisible nodes
	if (!entity->visible) { return; }

	if (std::dynamic_pointer_cast<Entity3D>(entity)->mesh.primitives.size() > 0) {
		// Pass the node's matrix via push constants
		// Traverse the node hierarchy to the top-most parent to get the final matrix of the current node
		glm::mat4 model = entity->transform.mat4();
		std::shared_ptr<Entity> currentParent = entity->parent;
		while (currentParent) {
			model = currentParent->transform.mat4() * model;
			currentParent = currentParent->parent;
		}

		// Reduce writes by checking if material changed
		for (Primitive& primitive : std::dynamic_pointer_cast<Entity3D>(entity)->mesh.primitives) {
			if (primitive.indexCount > 0) {
				// Get the material index for this primitive
				Material& material = scene->materials[primitive.materialIndex];

				MaterialPropertyUbo materialPropertyData{
					.baseColorFactor = material.baseColorFactor,
					.baseColorTextureIndex = scene->textures[material.baseColorTextureIndex].imageIndex,
					.normalTextureIndex = scene->textures[material.normalTextureIndex].imageIndex,
					.metallicRoughnessTextureIndex = scene->textures[material.metallicRoughnessTextureIndex].imageIndex,
					.occlusionTextureIndex = scene->textures[material.occlusionTextureIndex].imageIndex,
					.alphaCutoff = material.alphaCutOff
				};
				materialPropertyBuffer->writeToBuffer(&materialPropertyData);

				TransformUbo transformData{
					.model = model,
					.normal = glm::transpose(glm::inverse(model))
				};
				transformBuffer->writeToBuffer(&transformData);

				// draw
				vkCmdDrawIndexed(commandBuffer, primitive.indexCount, 1, primitive.firstIndex, 0, 0);
			}
		}
	}
	for (auto& child : entity->children) {
		renderEntity(scene, commandBuffer, child, pipelineLayout, materialPropertyBuffer, transformBuffer);
	}
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
	renderPassInfo.renderArea.extent = { framebuffer->width, framebuffer->height};
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

void Hmck::Renderer::render(
	std::unique_ptr<Hmck::Scene>& scene, 
	VkCommandBuffer commandBuffer, 
	VkPipelineLayout pipelineLayout,
	std::unique_ptr<Buffer>& transformBuffer,
	std::unique_ptr<Buffer>& materialPropertyBuffer)
{
	VkDeviceSize offsets[] = { 0 };
	VkBuffer buffers[] = { scene->vertexBuffer->getBuffer() };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
	vkCmdBindIndexBuffer(commandBuffer, scene->indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);

	// Render all nodes at top-level
	for (auto& entity : scene->entities) {
		renderEntity(scene, commandBuffer, entity, pipelineLayout, materialPropertyBuffer, transformBuffer);
	}
}
