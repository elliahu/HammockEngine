#include "HmckDeferredRenderSystem.h"

Hmck::HmckDeferredRenderSystem::HmckDeferredRenderSystem(
	HmckDevice& device, VkRenderPass renderPass, 
	std::vector<VkDescriptorSetLayout>& setLayouts): HmckIRenderSystem(device)
{
	prepareDescriptors();
	createPipelineLayout(setLayouts);
	createPipeline(renderPass);
}

Hmck::HmckDeferredRenderSystem::~HmckDeferredRenderSystem()
{
	vkDestroyPipelineLayout(hmckDevice.device(), pipelineLayout, nullptr);
}

void Hmck::HmckDeferredRenderSystem::createPipelineLayout(std::vector<VkDescriptorSetLayout>& setLayouts)
{
	std::vector<VkDescriptorSetLayout> layouts{ setLayouts };
	layouts.push_back(descriptorLayout->getDescriptorSetLayout());

	VkPushConstantRange pushConstantRange{};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(HmckMeshPushConstantData);

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
	pipelineLayoutInfo.pSetLayouts = layouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

	if (vkCreatePipelineLayout(hmckDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create pipeline layout");
	}
}

void Hmck::HmckDeferredRenderSystem::createPipeline(VkRenderPass renderPass)
{
	assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

	HmckPipelineConfigInfo pipelineConfig{};
	HmckPipeline::defaultHmckPipelineConfigInfo(pipelineConfig);
	pipelineConfig.renderPass = renderPass;
	pipelineConfig.pipelineLayout = pipelineLayout;
	pipeline = std::make_unique<HmckPipeline>(
		hmckDevice,
		std::string(SHADERS_DIR) + "Compiled/deferred.vert.spv",
		std::string(SHADERS_DIR) + "Compiled/deferred.frag.spv",
		pipelineConfig
	);
}


void Hmck::HmckDeferredRenderSystem::render(HmckFrameInfo& frameInfo)
{
	pipeline->bind(frameInfo.commandBuffer);

	// bind descriptor set
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

		if (obj.materialComponent != nullptr)
		{
			vkCmdBindDescriptorSets(
				frameInfo.commandBuffer,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipelineLayout,
				1, 1,
				&obj.descriptorSetComponent->set,
				0,
				nullptr
			);
		}

		vkCmdBindDescriptorSets(
			frameInfo.commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipelineLayout,
			2, 1,
			&descriptorSet,
			0,
			nullptr
		);

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

void Hmck::HmckDeferredRenderSystem::prepareDescriptors()
{
	descriptorPool = HmckDescriptorPool::Builder(hmckDevice)
		.setMaxSets(100)
		.addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)
		.build();

	descriptorLayout = HmckDescriptorSetLayout::Builder(hmckDevice)
		.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.build();
}

void Hmck::HmckDeferredRenderSystem::writeToDescriptorSet(VkDescriptorImageInfo& imageInfo)
{
	auto writer = HmckDescriptorWriter(*descriptorLayout, *descriptorPool)
		.writeImage(0, &imageInfo)
		.build(descriptorSet);
}
