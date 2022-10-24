#include "HmckSimpleRenderSystem.h"

Hmck::HmckSimpleRenderSystem::HmckSimpleRenderSystem(HmckDevice& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout) : hmckDevice{ device }
{
	createPipelineLayout(globalSetLayout);
	createPipeline(renderPass);
}

Hmck::HmckSimpleRenderSystem::~HmckSimpleRenderSystem()
{
	vkDestroyPipelineLayout(hmckDevice.device(), pipelineLayout, nullptr);
}



void Hmck::HmckSimpleRenderSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout)
{
	VkPushConstantRange pushConstantRange{};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(HmckSimplePushConstantData);

	std::vector<VkDescriptorSetLayout> descriptorSetLayouts{ globalSetLayout };

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

	if (vkCreatePipelineLayout(hmckDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create pipeline layout");
	}
}

void Hmck::HmckSimpleRenderSystem::createPipeline(VkRenderPass renderPass)
{
	assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

	HmckPipelineConfigInfo pipelineConfig{};
	HmckPipeline::defaultHmckPipelineConfigInfo(pipelineConfig);
	pipelineConfig.renderPass = renderPass;
	pipelineConfig.pipelineLayout = pipelineLayout;
	hmckPipeline = std::make_unique<HmckPipeline>(
		hmckDevice,
		"Shaders/Compiled/shader.vert.spv",
		"Shaders/Compiled/shader.frag.spv",
		pipelineConfig
		);
}


void Hmck::HmckSimpleRenderSystem::renderGameObjects(HmckFrameInfo& frameInfo)
{
	hmckPipeline->bind(frameInfo.commandBuffer);

	vkCmdBindDescriptorSets(
		frameInfo.commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		pipelineLayout,
		0, 1,
		&frameInfo.globalDescriptorSet,
		0,
		nullptr
	);

	for (auto& kv : frameInfo.gameObjects)
	{
		auto& obj = kv.second;
		if (obj.model == nullptr) continue;
		HmckSimplePushConstantData push{};
		push.modelMatrix = obj.transform.mat4();
		push.normalMatrix = obj.transform.normalMatrix();

		vkCmdPushConstants(
			frameInfo.commandBuffer,
			pipelineLayout,
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			0,
			sizeof(HmckSimplePushConstantData),
			&push
		);

		obj.model->bind(frameInfo.commandBuffer);
		obj.model->draw(frameInfo.commandBuffer);
	}
}
