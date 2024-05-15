#include "HmckLights.h"

void Hmck::OmniLight::prepareRenderPass(Device& device)
{
	VkAttachmentDescription osAttachments[2] = {};

	auto depthFormat = device.findSupportedFormat(
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

	osAttachments[0].format = shadwoCubeMapFaceImageFormat;
	osAttachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
	osAttachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	osAttachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	osAttachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	osAttachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	osAttachments[0].initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	osAttachments[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	// Depth attachment
	osAttachments[1].format = depthFormat;
	osAttachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
	osAttachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	osAttachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	osAttachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	osAttachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	osAttachments[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	osAttachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorReference = {};
	colorReference.attachment = 0;
	colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthReference = {};
	depthReference.attachment = 1;
	depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorReference;
	subpass.pDepthStencilAttachment = &depthReference;

	VkRenderPassCreateInfo renderPassCreateInfo = Init::renderPassCreateInfo();
	renderPassCreateInfo.attachmentCount = 2;
	renderPassCreateInfo.pAttachments = osAttachments;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpass;

	checkResult(vkCreateRenderPass(device.device(), &renderPassCreateInfo, nullptr, &renderPass.renderPass));
}

void Hmck::OmniLight::prepareFrameBuffer(Device& device)
{
	renderPass.width = shadowCubeMapFaceSize;
	renderPass.height = shadowCubeMapFaceSize;

	auto depthFormat = device.findSupportedFormat(
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

	// Color attachment
	VkImageCreateInfo imageCreateInfo = Init::imageCreateInfo();
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.format = shadwoCubeMapFaceImageFormat;
	imageCreateInfo.extent.width = renderPass.width;
	imageCreateInfo.extent.height = renderPass.height;
	imageCreateInfo.extent.depth = 1;
	imageCreateInfo.mipLevels = 1;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	// Image of the framebuffer is blit source
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageCreateInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkImageViewCreateInfo colorImageView = Init::imageViewCreateInfo();
	colorImageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
	colorImageView.format = shadwoCubeMapFaceImageFormat;
	colorImageView.flags = 0;
	colorImageView.subresourceRange = {};
	colorImageView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	colorImageView.subresourceRange.baseMipLevel = 0;
	colorImageView.subresourceRange.levelCount = 1;
	colorImageView.subresourceRange.baseArrayLayer = 0;
	colorImageView.subresourceRange.layerCount = 1;

	VkCommandBuffer layoutCmd = device.beginSingleTimeCommands();

	// Depth stencil attachment
	imageCreateInfo.format = depthFormat;
	imageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

	VkImageViewCreateInfo depthStencilView = Init::imageViewCreateInfo();
	depthStencilView.viewType = VK_IMAGE_VIEW_TYPE_2D;
	depthStencilView.format = depthFormat;
	depthStencilView.flags = 0;
	depthStencilView.subresourceRange = {};
	depthStencilView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	/*if (depthFormat >= VK_FORMAT_D16_UNORM_S8_UINT) {
		depthStencilView.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
	}*/
	depthStencilView.subresourceRange.baseMipLevel = 0;
	depthStencilView.subresourceRange.levelCount = 1;
	depthStencilView.subresourceRange.baseArrayLayer = 0;
	depthStencilView.subresourceRange.layerCount = 1;

	checkResult(vkCreateImage(device.device(), &imageCreateInfo, nullptr, &renderPass.depth.image));

	VkMemoryRequirements memReqs;
	vkGetImageMemoryRequirements(device.device(), renderPass.depth.image, &memReqs);

	VkMemoryAllocateInfo memAlloc = Init::memoryAllocateInfo();
	memAlloc.allocationSize = memReqs.size;
	memAlloc.memoryTypeIndex = device.findMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	checkResult(vkAllocateMemory(device.device(), &memAlloc, nullptr, &renderPass.depth.mem));
	checkResult(vkBindImageMemory(device.device(), renderPass.depth.image, renderPass.depth.mem, 0));

	setImageLayout(
		layoutCmd,
		renderPass.depth.image,
		VK_IMAGE_ASPECT_DEPTH_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	device.endSingleTimeCommands(layoutCmd);

	depthStencilView.image = renderPass.depth.image;
	checkResult(vkCreateImageView(device.device(), &depthStencilView, nullptr, &renderPass.depth.view));

	VkImageView attachments[2];
	attachments[1] = renderPass.depth.view;

	VkFramebufferCreateInfo fbufCreateInfo = Init::framebufferCreateInfo();
	fbufCreateInfo.renderPass = renderPass.renderPass;
	fbufCreateInfo.attachmentCount = 2;
	fbufCreateInfo.pAttachments = attachments;
	fbufCreateInfo.width = renderPass.width;
	fbufCreateInfo.height = renderPass.height;
	fbufCreateInfo.layers = 1;

	for (uint32_t i = 0; i < 6; i++)
	{
		attachments[0] = shadowCubeMapFaceImageViews[i];
		checkResult(vkCreateFramebuffer(device.device(), &fbufCreateInfo, nullptr, &renderPass.frameBuffers[i]));
	}
}

void Hmck::OmniLight::prepareCubeMap(Device& device)
{
	shadowCubeMap.width = shadowCubeMapFaceSize;
	shadowCubeMap.height = shadowCubeMapFaceSize;

	// Cube map image description
	VkImageCreateInfo imageCreateInfo = Init::imageCreateInfo();
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.format = shadwoCubeMapFaceImageFormat;
	imageCreateInfo.extent = { static_cast<uint32_t>(shadowCubeMap.width), static_cast<uint32_t>(shadowCubeMap.height), 1};
	imageCreateInfo.mipLevels = 1;
	imageCreateInfo.arrayLayers = 6;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

	VkMemoryAllocateInfo memAllocInfo = Init::memoryAllocateInfo();
	VkMemoryRequirements memReqs;

	VkCommandBuffer layoutCmd = device.beginSingleTimeCommands();

	// Create cube map image
	checkResult(vkCreateImage(device.device(), &imageCreateInfo, nullptr, &shadowCubeMap.image));

	vkGetImageMemoryRequirements(device.device(), shadowCubeMap.image, &memReqs);

	memAllocInfo.allocationSize = memReqs.size;
	memAllocInfo.memoryTypeIndex = device.findMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	checkResult(vkAllocateMemory(device.device(), &memAllocInfo, nullptr, &shadowCubeMap.memory));
	checkResult(vkBindImageMemory(device.device(), shadowCubeMap.image, shadowCubeMap.memory, 0));

	// Image barrier for optimal image (target)
	VkImageSubresourceRange subresourceRange = {};
	subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount = 1;
	subresourceRange.layerCount = 6;
	setImageLayout(
		layoutCmd,
		shadowCubeMap.image,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		subresourceRange);

	device.endSingleTimeCommands(layoutCmd);

	// Create sampler
	VkSamplerCreateInfo sampler = Init::samplerCreateInfo();
	sampler.magFilter = VK_FILTER_LINEAR;
	sampler.minFilter = VK_FILTER_LINEAR;
	sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	sampler.addressModeV = sampler.addressModeU;
	sampler.addressModeW = sampler.addressModeU;
	sampler.mipLodBias = 0.0f;
	sampler.maxAnisotropy = 1.0f;
	sampler.compareOp = VK_COMPARE_OP_NEVER;
	sampler.minLod = 0.0f;
	sampler.maxLod = 1.0f;
	sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	checkResult(vkCreateSampler(device.device(), &sampler, nullptr, &shadowCubeMap.sampler));

	// Create image view
	VkImageViewCreateInfo view = Init::imageViewCreateInfo();
	view.image = VK_NULL_HANDLE;
	view.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
	view.format = shadwoCubeMapFaceImageFormat;
	view.components = { VK_COMPONENT_SWIZZLE_R };
	view.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	view.subresourceRange.layerCount = 6;
	view.image = shadowCubeMap.image;
	checkResult(vkCreateImageView(device.device(), &view, nullptr, &shadowCubeMap.view));

	view.viewType = VK_IMAGE_VIEW_TYPE_2D;
	view.subresourceRange.layerCount = 1;
	view.image = shadowCubeMap.image;

	for (uint32_t i = 0; i < 6; i++)
	{
		view.subresourceRange.baseArrayLayer = i;
		checkResult(vkCreateImageView(device.device(), &view, nullptr, &shadowCubeMapFaceImageViews[i]));
	}

	shadowCubeMap.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	shadowCubeMap.updateDescriptor();
}

void Hmck::OmniLight::beginCubeMapFaceUpdate(uint32_t faceIndex, VkCommandBuffer commandBuffer)
{
	VkClearValue clearValues[2];
	clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
	clearValues[1].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo renderPassBeginInfo = Init::renderPassBeginInfo();
	// Reuse render pass from example pass
	renderPassBeginInfo.renderPass = renderPass.renderPass;
	renderPassBeginInfo.framebuffer = renderPass.frameBuffers[faceIndex];
	renderPassBeginInfo.renderArea.extent.width = renderPass.width;
	renderPassBeginInfo.renderArea.extent.height = renderPass.height;
	renderPassBeginInfo.clearValueCount = 2;
	renderPassBeginInfo.pClearValues = clearValues;

	// Update view matrix via push constant

	glm::mat4 viewMatrix = glm::mat4(1.0f);
	switch (faceIndex)
	{
	case 0: // POSITIVE_X
		viewMatrix = glm::rotate(viewMatrix, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		viewMatrix = glm::rotate(viewMatrix, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		break;
	case 1:	// NEGATIVE_X
		viewMatrix = glm::rotate(viewMatrix, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		viewMatrix = glm::rotate(viewMatrix, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		break;
	case 2:	// POSITIVE_Y
		viewMatrix = glm::rotate(viewMatrix, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		break;
	case 3:	// NEGATIVE_Y
		viewMatrix = glm::rotate(viewMatrix, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		break;
	case 4:	// POSITIVE_Z
		viewMatrix = glm::rotate(viewMatrix, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		break;
	case 5:	// NEGATIVE_Z
		viewMatrix = glm::rotate(viewMatrix, glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		break;
	}

	viewMatrix = glm::translate(viewMatrix, -transform.translation);

	// Render scene from cube face's point of view
	vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	PushData data
	{
		.view = viewMatrix,
		.lightPosition = glm::vec4(transform.translation, 1.0f)
	};

	// Update shader push constant block
	// Contains current face view matrix
	vkCmdPushConstants(
		commandBuffer,
		pipeline->graphicsPipelineLayout,
		VK_SHADER_STAGE_VERTEX_BIT,
		0,
		sizeof(PushData),
		&data);

	pipeline->bind(commandBuffer);

}

void Hmck::OmniLight::preparePipeline(Device& device, MemoryManager& memory)
{
	pipeline = GraphicsPipeline::createGraphicsPipelinePtr({
		.debugName = "light_pass",
		.device = device,
		.VS {
			.byteCode = Hmck::Filesystem::readFile("../../HammockEngine/Engine/Shaders/Compiled/light_pass.vert.spv"),
			.entryFunc = "main"
		},
		.FS {
			.byteCode = Hmck::Filesystem::readFile("../../HammockEngine/Engine/Shaders/Compiled/light_pass.frag.spv"),
			.entryFunc = "main"
		},
		.descriptorSetLayouts = {
			memory.getDescriptorSetLayout(descriptorSetLayout).getDescriptorSetLayout()
		},
		.pushConstantRanges {
			{
				.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
				.offset = 0,
				.size = sizeof(OmniLight::PushData)
			}
		},
		.graphicsState {
			.depthTest = VK_TRUE,
			.depthTestCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
			.cullMode = VK_CULL_MODE_BACK_BIT,
			.frontFace = VK_FRONT_FACE_CLOCKWISE,
			.blendAtaAttachmentStates {
				Hmck::Init::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
			},
			.vertexBufferBindings
			{
					.vertexBindingDescriptions =
					{
						{
							.binding = 0,
							.stride = sizeof(Vertex),
							.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
						}
					},
					.vertexAttributeDescriptions =
					{
						{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)},
						{1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color)},
						{2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)},
						{3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)},
						{4, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, tangent)}
					}
				}
		},
		.renderPass = renderPass.renderPass
	});
}

void Hmck::OmniLight::endCubeMapFaceUpdate(VkCommandBuffer commandBuffer)
{
	vkCmdEndRenderPass(commandBuffer);
}

void Hmck::OmniLight::prepareDescriptor(MemoryManager& memory)
{
	descriptorSetLayout = memory.createDescriptorSetLayout({
		.bindings = {
			{.binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS},
		}
	});

	buffer = memory.createBuffer({
		.instanceSize = sizeof(UniformData),
		.instanceCount = 1,
		.usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		.memoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	});

	auto bufferInfo = memory.getBuffer(buffer)->descriptorInfo();
	descriptorSet = memory.createDescriptorSet({
		.descriptorSetLayout = descriptorSetLayout,
		.bufferWrites = {{0,bufferInfo}},
	});
}
