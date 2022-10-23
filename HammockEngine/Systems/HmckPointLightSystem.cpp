#include "HmckPointLightSystem.h"

Hmck::HmckPointLightSystem::HmckPointLightSystem(HmckDevice& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout) : hmckDevice{ device }
{
	createPipelineLayout(globalSetLayout);
	createPipeline(renderPass);
}

Hmck::HmckPointLightSystem::~HmckPointLightSystem()
{
	vkDestroyPipelineLayout(hmckDevice.device(), pipelineLayout, nullptr);
}



void Hmck::HmckPointLightSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout)
{
	// VkPushConstantRange pushConstantRange{};
	// pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	// pushConstantRange.offset = 0;
	// pushConstantRange.size = sizeof(HmckSimplePushConstantData);

	std::vector<VkDescriptorSetLayout> descriptorSetLayouts{ globalSetLayout };

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = nullptr;

	if (vkCreatePipelineLayout(hmckDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create pipeline layout");
	}
}

void Hmck::HmckPointLightSystem::createPipeline(VkRenderPass renderPass)
{
	assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

	HmckPipelineConfigInfo pipelineConfig{};
	HmckPipeline::defaultHmckPipelineConfigInfo(pipelineConfig);
	pipelineConfig.attributeDescriptions.clear();
	pipelineConfig.bindingDescriptions.clear();
	pipelineConfig.renderPass = renderPass;
	pipelineConfig.pipelineLayout = pipelineLayout;
	hmckPipeline = std::make_unique<HmckPipeline>(
		hmckDevice,
		"Shaders/point_light.vert.spv",
		"Shaders/point_light.frag.spv",
		pipelineConfig
		);
}


void Hmck::HmckPointLightSystem::render(HmckFrameInfo& frameInfo)
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

	vkCmdDraw(frameInfo.commandBuffer, 6, 1, 0, 0);
}
