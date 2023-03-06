#include "HmckGbufferRenderSystem.h"

Hmck::HmckGbufferRenderSystem::HmckGbufferRenderSystem(HmckDevice& device, VkRenderPass renderPass, std::vector<VkDescriptorSetLayout>& setLayouts) : HmckIRenderSystem(device)
{
	createPipelineLayout(setLayouts);
	createPipeline(renderPass);
}

Hmck::HmckGbufferRenderSystem::~HmckGbufferRenderSystem()
{
	vkDestroyPipelineLayout(hmckDevice.device(), pipelineLayout, nullptr);
}

void Hmck::HmckGbufferRenderSystem::render(HmckFrameInfo& frameInfo)
{
	pipeline->bind(frameInfo.commandBuffer);

	// bind global descriptor set (UBO)
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
		// dont render object that would not be visible
		if (obj.wavefrontObjComponent == nullptr && obj.glTFComponent == nullptr) continue;	

		if (obj.wavefrontObjComponent != nullptr)
		{			
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

			// bind MTL materials
			vkCmdBindDescriptorSets(
				frameInfo.commandBuffer,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipelineLayout,
				1, 1,
				&obj.descriptorSetComponent->set,
				0,
				nullptr
			);
			obj.wavefrontObjComponent->mesh->bind(frameInfo.commandBuffer);
			obj.wavefrontObjComponent->mesh->draw(frameInfo.commandBuffer);
		}
		else if (obj.glTFComponent != nullptr)
		{
			obj.glTFComponent->glTF->draw(frameInfo.commandBuffer, pipelineLayout);
		}
	}
}

void Hmck::HmckGbufferRenderSystem::createPipelineLayout(std::vector<VkDescriptorSetLayout>& setLayouts)
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

void Hmck::HmckGbufferRenderSystem::createPipeline(VkRenderPass renderPass)
{
	assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

	HmckPipelineConfigInfo pipelineConfig{};
	HmckPipeline::defaultHmckPipelineConfigInfo(pipelineConfig);
	HmckPipeline::enableAlphaBlending(pipelineConfig);

	std::array<VkPipelineColorBlendAttachmentState, 4> blendAttachmentStates =
	{
		Hmck::Init::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
		Hmck::Init::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
		Hmck::Init::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
		Hmck::Init::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
	};

	HmckPipeline::enableGbuffer(pipelineConfig, blendAttachmentStates);
	pipelineConfig.renderPass = renderPass;
	pipelineConfig.pipelineLayout = pipelineLayout;
	pipeline = std::make_unique<HmckPipeline>(
		hmckDevice,
		std::string(SHADERS_DIR) + "Compiled/gbuffer.vert.spv",
		std::string(SHADERS_DIR) + "Compiled/gbuffer.frag.spv",
		pipelineConfig
		);
}
