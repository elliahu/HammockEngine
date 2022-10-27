#include "HmckCollisionDetectionSystem.h"

Hmck::HmckCollisionDetectionSystem::HmckCollisionDetectionSystem(
	HmckDevice& device, VkRenderPass renderPass, 
	VkDescriptorSetLayout globalSetLayout) : hmckDevice{ device }
{
	createPipelineLayout(globalSetLayout);
	createPipeline(renderPass);
}

Hmck::HmckCollisionDetectionSystem::~HmckCollisionDetectionSystem()
{
	vkDestroyPipelineLayout(hmckDevice.device(), pipelineLayout, nullptr);
}

void Hmck::HmckCollisionDetectionSystem::renderBoundingBoxes(HmckFrameInfo& frameInfo)
{
	// TO DO
}


bool Hmck::HmckCollisionDetectionSystem::intersect(HmckGameObject& obj1, HmckGameObject& obj2)
{
	assert(obj1.boundingBox != nullptr && "Provided object 1 needs bounding box to calculate intersections !");
	assert(obj2.boundingBox != nullptr && "Provided object 2 needs bounding box to calculate intersections !");

	return (
		obj1.boundingBox->x.min + obj1.transform.translation.x <= obj2.boundingBox->x.max + obj2.transform.translation.x &&
		obj1.boundingBox->x.max + obj1.transform.translation.x >= obj2.boundingBox->x.min + obj2.transform.translation.x &&
		obj1.boundingBox->y.min + obj1.transform.translation.y <= obj2.boundingBox->y.max + obj2.transform.translation.y &&
		obj1.boundingBox->y.max + obj1.transform.translation.y >= obj2.boundingBox->y.min + obj2.transform.translation.y &&
		obj1.boundingBox->z.min + obj1.transform.translation.z <= obj2.boundingBox->z.max + obj2.transform.translation.z &&
		obj1.boundingBox->z.max + obj1.transform.translation.z >= obj2.boundingBox->z.min + obj2.transform.translation.z
	);
}


void Hmck::HmckCollisionDetectionSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout)
{
	VkPushConstantRange pushConstantRange{};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(HmckBoundingBoxPushConstant);

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

void Hmck::HmckCollisionDetectionSystem::createPipeline(VkRenderPass renderPass)
{
	assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

	HmckPipelineConfigInfo pipelineConfig{};
	HmckPipeline::defaultHmckPipelineConfigInfo(pipelineConfig);
	HmckPipeline::enablePolygonModeLine(pipelineConfig);
	pipelineConfig.attributeDescriptions.clear();
	pipelineConfig.bindingDescriptions.clear();
	pipelineConfig.renderPass = renderPass;
	pipelineConfig.pipelineLayout = pipelineLayout;
	hmckPipeline = std::make_unique<HmckPipeline>(
		hmckDevice,
		std::string(SHADERS_DIR) + "Compiled/bounding_boxes.vert.spv",
		std::string(SHADERS_DIR) + "Compiled/bounding_boxes.frag.spv",
		pipelineConfig
		);
}
