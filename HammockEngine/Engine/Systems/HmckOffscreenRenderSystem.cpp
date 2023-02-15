#include "HmckOffscreenRenderSystem.h"

Hmck::HmckOffscreenRenderSystem::HmckOffscreenRenderSystem(
	HmckDevice& device, VkRenderPass renderPass, 
	std::vector<VkDescriptorSetLayout>& setLayouts) : hmckDevice{ device }
{
	createPipelineLayout(setLayouts);
	createPipeline(renderPass);
}

Hmck::HmckOffscreenRenderSystem::~HmckOffscreenRenderSystem()
{
	vkDestroyPipelineLayout(hmckDevice.device(), pipelineLayout, nullptr);
}

void Hmck::HmckOffscreenRenderSystem::renderOffscreen(HmckFrameInfo& frameInfo)
{
	// TODO
}

void Hmck::HmckOffscreenRenderSystem::createPipelineLayout(std::vector<VkDescriptorSetLayout>& setLayouts)
{
	VkPushConstantRange pushConstantRange{};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(HmckModelPushConstantData);

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
	pipelineLayoutInfo.pSetLayouts = setLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

	if (vkCreatePipelineLayout(hmckDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create pipeline layout");
	}
}

void Hmck::HmckOffscreenRenderSystem::createPipeline(VkRenderPass renderPass)
{
	assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

	HmckPipelineConfigInfo pipelineConfig{};
	HmckPipeline::defaultHmckPipelineConfigInfo(pipelineConfig);
	pipelineConfig.renderPass = renderPass;
	pipelineConfig.pipelineLayout = pipelineLayout;
	pipeline = std::make_unique<HmckPipeline>(
		hmckDevice,
		std::string(SHADERS_DIR) + "Compiled/offscreen.vert.spv",
		std::string(SHADERS_DIR) + "Compiled/offscreen.frag.spv",
		pipelineConfig
		);
}
