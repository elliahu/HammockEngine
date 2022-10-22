#include "HmckSimpleRenderSystem.h"

Hmck::HmckSimpleRenderSystem::HmckSimpleRenderSystem(HmckDevice& device, VkRenderPass renderPass) : hmckDevice{ device }
{
	createPipelineLayout();
	createPipeline(renderPass);
}

Hmck::HmckSimpleRenderSystem::~HmckSimpleRenderSystem()
{
	vkDestroyPipelineLayout(hmckDevice.device(), pipelineLayout, nullptr);
}



void Hmck::HmckSimpleRenderSystem::createPipelineLayout()
{
	VkPushConstantRange pushConstantRange{};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(HmckSimplePushConstantData);


	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 0;
	pipelineLayoutInfo.pSetLayouts = nullptr;
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
		"Shaders/shader.vert.spv",
		"Shaders/shader.frag.spv",
		pipelineConfig
		);
}


void Hmck::HmckSimpleRenderSystem::renderGameObjects(
	VkCommandBuffer commandBuffer, 
	std::vector<HmckGameObject>& gameObjects, 
	const HmckCamera& camera)
{
	hmckPipeline->bind(commandBuffer);

	auto projectionView = camera.getProjection() * camera.getView();

	for (auto& obj : gameObjects)
	{
		HmckSimplePushConstantData push{};
		auto modelMatrix = obj.transform.mat4();
		push.transform = projectionView * modelMatrix;
		push.normalMatrix = obj.transform.normalMatrix();

		vkCmdPushConstants(
			commandBuffer,
			pipelineLayout,
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			0,
			sizeof(HmckSimplePushConstantData),
			&push
		);

		obj.model->bind(commandBuffer);
		obj.model->draw(commandBuffer);
	}
}
