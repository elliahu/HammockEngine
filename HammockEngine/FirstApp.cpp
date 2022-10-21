#include "FirstApp.h"

Hmck::FirstApp::FirstApp()
{
	loadModels();
	createPipelineLayout();
	recreateSwapChain();
	createCommandBuffer();
}

Hmck::FirstApp::~FirstApp()
{
	vkDestroyPipelineLayout(hmckDevice.device(), pipelineLayout, nullptr);
}

void Hmck::FirstApp::run()
{
	while (!hmckWindow.shouldClose())
	{
		glfwPollEvents();
		drawFrame();
	}

	vkDeviceWaitIdle(hmckDevice.device());
}

void Hmck::FirstApp::loadModels()
{
	std::vector<HmckModel::Vertex> vertices{
		//position			//color
		{{0.0f, -0.5f},		{1.0f , 0.0f, 0.0f}},
		{{0.5f, 0.5f},		{0.0f , 1.0f, 0.0f}},
		{{-0.5f, 0.5f},		{1.0f , 0.0f, 1.0f}}
	};

	hmckModel = std::make_unique<HmckModel>(hmckDevice, vertices);
}

void Hmck::FirstApp::createPipelineLayout()
{
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};

	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 0;
	pipelineLayoutInfo.pSetLayouts = nullptr;
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = nullptr;

	if (vkCreatePipelineLayout(hmckDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create pipeline layout");
	}
}

void Hmck::FirstApp::createPipeline()
{
	HmckPipelineConfigInfo pipelineConfig{};
	HmckPipeline::defaultHmckPipelineConfigInfo(
		pipelineConfig,
		hmckSwapChain->width(),
		hmckSwapChain->height());

	pipelineConfig.renderPass = hmckSwapChain->getRenderPass();
	pipelineConfig.pipelineLayout = pipelineLayout;
	hmckPipeline = std::make_unique<HmckPipeline>(
		hmckDevice,
		"Shaders/shader.vert.spv",
		"Shaders/shader.frag.spv",
		pipelineConfig
	);
}

void Hmck::FirstApp::createCommandBuffer()
{
	commandBuffers.resize(hmckSwapChain->imageCount());

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

void Hmck::FirstApp::recordCommandBuffer(int imageIndex)
{
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	if (vkBeginCommandBuffer(commandBuffers[imageIndex], &beginInfo) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to begin recording commadn buffer");
	}

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = hmckSwapChain->getRenderPass();
	renderPassInfo.framebuffer = hmckSwapChain->getFrameBuffer(imageIndex);

	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = hmckSwapChain->getSwapChainExtent();

	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = { 0.1f, 0.1f, 0.1f, 1.0f }; // clear color
	clearValues[1].depthStencil = { 1.0f, 0 };

	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(commandBuffers[imageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	hmckPipeline->bind(commandBuffers[imageIndex]);
	hmckModel->bind(commandBuffers[imageIndex]);
	hmckModel->draw(commandBuffers[imageIndex]);

	vkCmdEndRenderPass(commandBuffers[imageIndex]);

	if (vkEndCommandBuffer(commandBuffers[imageIndex]) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to record command buffer");
	}
}

void Hmck::FirstApp::drawFrame()
{
	uint32_t imageIndex;
	auto result = hmckSwapChain->acquireNextImage(&imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		recreateSwapChain();
		return;
	}

	if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		throw std::runtime_error("failed to aquire swap chain image!");
	}

	recordCommandBuffer(imageIndex);
	result = hmckSwapChain->submitCommandBuffers(&commandBuffers[imageIndex], &imageIndex);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || hmckWindow.wasWindowResized())
	{
		hmckWindow.resetWindowResizedFlag();
		recreateSwapChain();
		return;
	}

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("failed to present swap chain image");
	}
}

void Hmck::FirstApp::recreateSwapChain()
{
	auto extent = hmckWindow.getExtent();
	while (extent.width == 0 || extent.height == 0)
	{
		extent = hmckWindow.getExtent();
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(hmckDevice.device());
	hmckSwapChain = std::make_unique<HmckSwapChain>(hmckDevice, extent);
	createPipeline();
}
