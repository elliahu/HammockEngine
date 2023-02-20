#include "HmckOffscreenRenderSystem.h"

Hmck::HmckOffscreenRenderSystem::HmckOffscreenRenderSystem(
	HmckDevice& device, VkRenderPass renderPass, 
	std::vector<VkDescriptorSetLayout>& setLayouts) : HmckIRenderSystem(device)
{
	createPipelineLayout(setLayouts);
	createPipeline(renderPass);		
}

Hmck::HmckOffscreenRenderSystem::~HmckOffscreenRenderSystem()
{
	vkDestroyPipelineLayout(hmckDevice.device(), pipelineLayout, nullptr);
}

void Hmck::HmckOffscreenRenderSystem::render(HmckFrameInfo& frameInfo)
{
	pipeline->bind(frameInfo.commandBuffer);

	// Set depth bias (aka "Polygon offset")
	// Required to avoid shadow mapping artifacts
	vkCmdSetDepthBias(
		frameInfo.commandBuffer,
		depthBiasConstant,
		0.0f,
		depthBiasSlope);

	// bind global descriptor set
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
		if (obj.meshComponent == nullptr) continue;

		HmckMeshPushConstantData push{};
		push.modelMatrix = obj.transformComponent.mat4();
		push.normalMatrix = obj.transformComponent.normalMatrix();

		// push data using push constant
		vkCmdPushConstants(
			frameInfo.commandBuffer,
			pipelineLayout,
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			0,
			sizeof(HmckMeshPushConstantData),
			&push
		);

		obj.meshComponent->mesh->bind(frameInfo.commandBuffer);
		obj.meshComponent->mesh->draw(frameInfo.commandBuffer);
	}
}

void Hmck::HmckOffscreenRenderSystem::createPipelineLayout(std::vector<VkDescriptorSetLayout>& setLayouts)
{
	VkPushConstantRange pushConstantRange{};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(HmckMeshPushConstantData);

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
		std::string(SHADERS_DIR) + "Compiled/shadowmap.vert.spv",
		std::string(SHADERS_DIR) + "Compiled/shadowmap.frag.spv",
		pipelineConfig
		);
}



