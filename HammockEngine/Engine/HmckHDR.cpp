#include "HmckHDR.h"

void Hmck::EnvironmentLoader::load(std::string filepath, Texture2DHandle& texture, VkFormat format)
{
	stbi_set_flip_vertically_on_load(true);
	unsigned char* pixels = stbi_load(filepath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
	stbi_set_flip_vertically_on_load(false);
	if (!pixels)
	{
		throw std::runtime_error("Failed to load image from disk");
	}

	channels = 4;

    texture = memory.createTexture2DFromBuffer({
		.buffer = pixels,
		.bufferSize = static_cast<uint32_t>(width * height * channels),
		.width = static_cast<uint32_t>(width), .height = static_cast<uint32_t>(height),
		.format = format
		});


	// Free the memory allocated by stb_image
	stbi_image_free(pixels);
}

void Hmck::EnvironmentLoader::loadHDR(std::string filepath, Texture2DHandle& texture, VkFormat format)
{
	stbi_set_flip_vertically_on_load(true);
	float* pixels = stbi_loadf(filepath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
	stbi_set_flip_vertically_on_load(false);
	if (!pixels)
	{
		throw std::runtime_error("Failed to load image from disk");
	}

	channels = 4;

	texture = memory.createHDRTexture2DFromBuffer({
		.buffer = pixels,
		.bufferSize = static_cast<uint32_t>(width * height * channels),
		.width = static_cast<uint32_t>(width), .height = static_cast<uint32_t>(height),
		.format = format
	});

	// Free the memory allocated by stb_image
	stbi_image_free(pixels);
}

void Hmck::Environment::generateIrradianceSphere(Device& device, MemoryManager& memory, VkFormat format)
{
	assert(environmentSphere > 0 && "Env map not set");
	auto tStart = std::chrono::high_resolution_clock::now();
	uint32_t width = memory.getTexture2D(environmentSphere)->width;
	uint32_t height = memory.getTexture2D(environmentSphere)->height;
	std::unique_ptr<GraphicsPipeline> pipeline{};
	irradianceSphere = memory.createTexture2D();

	// image, view, sampler
	VkImageCreateInfo imageCI = Init::imageCreateInfo();
	imageCI.imageType = VK_IMAGE_TYPE_2D;
	imageCI.format = format;
	imageCI.extent.width = width;
	imageCI.extent.height = height;
	imageCI.extent.depth = 1;
	imageCI.mipLevels = 1;
	imageCI.arrayLayers = 1;
	imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCI.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	device.createImageWithInfo(imageCI, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memory.getTexture2D(irradianceSphere)->image, memory.getTexture2D(irradianceSphere)->memory);

	VkImageViewCreateInfo viewCI = Init::imageViewCreateInfo();
	viewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewCI.format = format;
	viewCI.subresourceRange = {};
	viewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewCI.subresourceRange.levelCount = 1;
	viewCI.subresourceRange.layerCount = 1;
	viewCI.image = memory.getTexture2D(irradianceSphere)->image;
	checkResult(vkCreateImageView(device.device(), &viewCI, nullptr, &memory.getTexture2D(irradianceSphere)->view));

	memory.getTexture2D(irradianceSphere)->createSampler(device, VK_FILTER_LINEAR);
	memory.getTexture2D(irradianceSphere)->layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	memory.getTexture2D(irradianceSphere)->updateDescriptor();

	// FB, RP , Att
	VkAttachmentDescription attDesc = {};
	attDesc.format = format;
	attDesc.samples = VK_SAMPLE_COUNT_1_BIT;
	attDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attDesc.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	VkAttachmentReference colorReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

	VkSubpassDescription subpassDescription = {};
	subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescription.colorAttachmentCount = 1;
	subpassDescription.pColorAttachments = &colorReference;

	// Use subpass dependencies for layout transitions
	std::array<VkSubpassDependency, 2> dependencies;
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

	// Create the actual renderpass

	VkRenderPassCreateInfo renderPassCI = Init::renderPassCreateInfo();
	renderPassCI.attachmentCount = 1;
	renderPassCI.pAttachments = &attDesc;
	renderPassCI.subpassCount = 1;
	renderPassCI.pSubpasses = &subpassDescription;
	renderPassCI.dependencyCount = 2;
	renderPassCI.pDependencies = dependencies.data();

	VkRenderPass renderpass;
	checkResult(vkCreateRenderPass(device.device(), &renderPassCI, nullptr, &renderpass));

	VkFramebufferCreateInfo framebufferCI = Init::framebufferCreateInfo();
	framebufferCI.renderPass = renderpass;
	framebufferCI.attachmentCount = 1;
	framebufferCI.pAttachments = &memory.getTexture2D(irradianceSphere)->view;
	framebufferCI.width = width;
	framebufferCI.height = height;
	framebufferCI.layers = 1;

	VkFramebuffer framebuffer;
	checkResult(vkCreateFramebuffer(device.device(), &framebufferCI, nullptr, &framebuffer));
	
	// Descriptors
	VkDescriptorSetLayout descriptorsetlayout;
	std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
		Init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0),
	};
	VkDescriptorSetLayoutCreateInfo descriptorsetlayoutCI = Init::descriptorSetLayoutCreateInfo(setLayoutBindings);
	checkResult(vkCreateDescriptorSetLayout(device.device(), &descriptorsetlayoutCI, nullptr, &descriptorsetlayout));

	// Descriptor Pool
	std::vector<VkDescriptorPoolSize> poolSizes = { Init::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1) };
	VkDescriptorPoolCreateInfo descriptorPoolCI = Init::descriptorPoolCreateInfo(poolSizes, 2);
	VkDescriptorPool descriptorpool;
	checkResult(vkCreateDescriptorPool(device.device(), &descriptorPoolCI, nullptr, &descriptorpool));

	// Descriptor sets
	VkDescriptorSet descriptorset;
	VkDescriptorSetAllocateInfo allocInfo = Init::descriptorSetAllocateInfo(descriptorpool, &descriptorsetlayout, 1);
	checkResult(vkAllocateDescriptorSets(device.device(), &allocInfo, &descriptorset));
	VkWriteDescriptorSet writeDescriptorSet = Init::writeDescriptorSet(descriptorset, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &memory.getTexture2D(environmentSphere)->descriptor);
	vkUpdateDescriptorSets(device.device(), 1, &writeDescriptorSet, 0, nullptr);

	pipeline = GraphicsPipeline::createGraphicsPipelinePtr({
		.debugName = "IrradianceMap_generation",
		.device = device,
		.VS {
			.byteCode = Hmck::Filesystem::readFile("../../HammockEngine/Engine/Shaders/Compiled/fullscreen_headless.vert.spv"),
			.entryFunc = "main"
		},
		.FS {
			.byteCode = Hmck::Filesystem::readFile("../../HammockEngine/Engine/Shaders/Compiled/generate_irradiancemap.frag.spv"),
			.entryFunc = "main"
		},
		.descriptorSetLayouts = {
			descriptorsetlayout
		},
		.pushConstantRanges {
			{
				.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
				.offset = 0,
				.size =  2 * sizeof(float)
			}
		},
		.graphicsState {
			.depthTest = VK_FALSE,
			.cullMode = VK_CULL_MODE_NONE,
			.blendAtaAttachmentStates {},
			.vertexBufferBindings{}
		},
		.renderPass = renderpass
		});

	// Render
	VkClearValue clearValues[1];
	clearValues[0].color = { { 0.0f, 0.0f, 0.2f, 1.0f } };


	VkRenderPassBeginInfo renderPassBeginInfo = Init::renderPassBeginInfo();
	renderPassBeginInfo.renderPass = renderpass;
	renderPassBeginInfo.renderArea.extent.width = width;
	renderPassBeginInfo.renderArea.extent.height = height;
	renderPassBeginInfo.clearValueCount = 1;
	renderPassBeginInfo.pClearValues = clearValues;
	renderPassBeginInfo.framebuffer = framebuffer;

	VkImageSubresourceRange subresourceRange = {};
	subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount = 1;
	subresourceRange.layerCount = 1;

	VkCommandBuffer commandBuffer = device.beginSingleTimeCommands();


	vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	VkViewport viewport = Init::viewport((float)width, (float)height, 0.0f, 1.0f);
	VkRect2D scissor = Init::rect2D(width, height, 0, 0);
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
	pipeline->bind(commandBuffer);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->graphicsPipelineLayout, 0, 1, &descriptorset, 0, NULL);
	struct { float resX, resY; } data;
	data.resX = static_cast<float>(width);
	data.resY = static_cast<float>(height);
	vkCmdPushConstants(commandBuffer, pipeline->graphicsPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 2 * sizeof(float), &data);
	vkCmdDraw(commandBuffer, 3, 1, 0, 0);
	vkCmdEndRenderPass(commandBuffer);

	device.endSingleTimeCommands(commandBuffer);

	vkQueueWaitIdle(device.graphicsQueue());

	vkDestroyRenderPass(device.device(), renderpass, nullptr);
	vkDestroyFramebuffer(device.device(), framebuffer, nullptr);
	vkDestroyDescriptorSetLayout(device.device(), descriptorsetlayout, nullptr);
	vkDestroyDescriptorPool(device.device(), descriptorpool, nullptr);

	auto tEnd = std::chrono::high_resolution_clock::now();
	auto tDiff = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
	Logger::log(LogLevel::HMCK_LOG_LEVEL_DEBUG, "Generating irradiance sphere took %f ms\n", tDiff);
}

void Hmck::Environment::generateBRDFLUT(Device& device, MemoryManager& memory, uint32_t dim, VkFormat format)
{
	auto tStart = std::chrono::high_resolution_clock::now();
	std::unique_ptr<GraphicsPipeline> brdfLUTPipeline{};
	brdfLUT = memory.createTexture2D();

	// image, view, sampler
    VkImageCreateInfo imageCI = Init::imageCreateInfo();
    imageCI.imageType = VK_IMAGE_TYPE_2D;
    imageCI.format = format;
    imageCI.extent.width = dim;
    imageCI.extent.height = dim;
    imageCI.extent.depth = 1;
    imageCI.mipLevels = 1;
    imageCI.arrayLayers = 1;
    imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCI.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	device.createImageWithInfo(imageCI, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memory.getTexture2D(brdfLUT)->image, memory.getTexture2D(brdfLUT)->memory);

	VkImageViewCreateInfo viewCI = Init::imageViewCreateInfo();
	viewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewCI.format = format;
	viewCI.subresourceRange = {};
	viewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewCI.subresourceRange.levelCount = 1;
	viewCI.subresourceRange.layerCount = 1;
	viewCI.image = memory.getTexture2D(brdfLUT)->image;
	checkResult(vkCreateImageView(device.device(), &viewCI, nullptr, &memory.getTexture2D(brdfLUT)->view));

	memory.getTexture2D(brdfLUT)->createSampler(device, VK_FILTER_LINEAR);
	memory.getTexture2D(brdfLUT)->layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	memory.getTexture2D(brdfLUT)->updateDescriptor();

	// FB, RP , Att
	VkAttachmentDescription attDesc = {};
	attDesc.format = format;
	attDesc.samples = VK_SAMPLE_COUNT_1_BIT;
	attDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attDesc.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	VkAttachmentReference colorReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

	VkSubpassDescription subpassDescription = {};
	subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescription.colorAttachmentCount = 1;
	subpassDescription.pColorAttachments = &colorReference;

	// Use subpass dependencies for layout transitions
	std::array<VkSubpassDependency, 2> dependencies;
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

	// Create the actual renderpass

	VkRenderPassCreateInfo renderPassCI = Init::renderPassCreateInfo();
	renderPassCI.attachmentCount = 1;
	renderPassCI.pAttachments = &attDesc;
	renderPassCI.subpassCount = 1;
	renderPassCI.pSubpasses = &subpassDescription;
	renderPassCI.dependencyCount = 2;
	renderPassCI.pDependencies = dependencies.data();

	VkRenderPass renderpass;
	checkResult(vkCreateRenderPass(device.device(), &renderPassCI, nullptr, &renderpass));

	VkFramebufferCreateInfo framebufferCI = Init::framebufferCreateInfo();
	framebufferCI.renderPass = renderpass;
	framebufferCI.attachmentCount = 1;
	framebufferCI.pAttachments = &memory.getTexture2D(brdfLUT)->view;
	framebufferCI.width = dim;
	framebufferCI.height = dim;
	framebufferCI.layers = 1;

	VkFramebuffer framebuffer;
	checkResult(vkCreateFramebuffer(device.device(), &framebufferCI, nullptr, &framebuffer));

	// Descriptors
	VkDescriptorSetLayout descriptorsetlayout;
	std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {};
	VkDescriptorSetLayoutCreateInfo descriptorsetlayoutCI = Init::descriptorSetLayoutCreateInfo(setLayoutBindings);
	checkResult(vkCreateDescriptorSetLayout(device.device(), &descriptorsetlayoutCI, nullptr, &descriptorsetlayout));

	// Descriptor Pool
	std::vector<VkDescriptorPoolSize> poolSizes = { Init::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1) };
	VkDescriptorPoolCreateInfo descriptorPoolCI = Init::descriptorPoolCreateInfo(poolSizes, 2);
	VkDescriptorPool descriptorpool;
	checkResult(vkCreateDescriptorPool(device.device(), &descriptorPoolCI, nullptr, &descriptorpool));

	// Descriptor sets
	VkDescriptorSet descriptorset;
	VkDescriptorSetAllocateInfo allocInfo = Init::descriptorSetAllocateInfo(descriptorpool, &descriptorsetlayout, 1);
	checkResult(vkAllocateDescriptorSets(device.device(), &allocInfo, &descriptorset));

	brdfLUTPipeline = GraphicsPipeline::createGraphicsPipelinePtr({
		.debugName = "BRDFLUT_generation",
		.device = device,
		.VS {
			.byteCode = Hmck::Filesystem::readFile("../../HammockEngine/Engine/Shaders/Compiled/fullscreen_headless.vert.spv"),
			.entryFunc = "main"
		},
		.FS {
			.byteCode = Hmck::Filesystem::readFile("../../HammockEngine/Engine/Shaders/Compiled/generate_brdflut.frag.spv"),
			.entryFunc = "main"
		},
		.descriptorSetLayouts = {
			descriptorsetlayout
		},
		.pushConstantRanges {},
		.graphicsState {
			.depthTest = VK_FALSE,
			.cullMode = VK_CULL_MODE_NONE,
			.blendAtaAttachmentStates {},
			.vertexBufferBindings{}
		},
		.renderPass = renderpass
	});

	// Render
	VkClearValue clearValues[1];
	clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };

	VkRenderPassBeginInfo renderPassBeginInfo = Init::renderPassBeginInfo();
	renderPassBeginInfo.renderPass = renderpass;
	renderPassBeginInfo.renderArea.extent.width = dim;
	renderPassBeginInfo.renderArea.extent.height = dim;
	renderPassBeginInfo.clearValueCount = 1;
	renderPassBeginInfo.pClearValues = clearValues;
	renderPassBeginInfo.framebuffer = framebuffer;

	VkCommandBuffer commandBuffer = device.beginSingleTimeCommands();
	vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	VkViewport viewport = Init::viewport((float)dim, (float)dim, 0.0f, 1.0f);
	VkRect2D scissor = Init::rect2D(dim, dim, 0, 0);
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
	brdfLUTPipeline->bind(commandBuffer);
	vkCmdDraw(commandBuffer, 3, 1, 0, 0);
	vkCmdEndRenderPass(commandBuffer);
	device.endSingleTimeCommands(commandBuffer);

	vkQueueWaitIdle(device.graphicsQueue());

	vkDestroyRenderPass(device.device(), renderpass, nullptr);
	vkDestroyFramebuffer(device.device(), framebuffer, nullptr);
	vkDestroyDescriptorSetLayout(device.device(), descriptorsetlayout, nullptr);
	vkDestroyDescriptorPool(device.device(), descriptorpool, nullptr);

	auto tEnd = std::chrono::high_resolution_clock::now();
	auto tDiff = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
	Logger::log(LogLevel::HMCK_LOG_LEVEL_DEBUG, "Generating BRDF LUT took %f ms\n", tDiff);
}

void Hmck::Environment::destroy(MemoryManager& memory)
{
    if(environmentSphere > 0)
	    memory.destroyTexture2D(environmentSphere);
    if (irradianceSphere > 0)
        memory.destroyTexture2D(irradianceSphere);
    if (brdfLUT > 0)
        memory.destroyTexture2D(brdfLUT);
}
