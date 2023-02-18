#include "HmckLightSystem.h"

Hmck::HmckLightSystem::HmckLightSystem(
	HmckDevice& device, 
	VkRenderPass renderPass, 
	std::vector<VkDescriptorSetLayout>& setLayouts) : HmckIRenderSystem(device)
{
	createPipelineLayout(setLayouts);
	createPipeline(renderPass);
}

Hmck::HmckLightSystem::~HmckLightSystem()
{
	vkDestroyPipelineLayout(hmckDevice.device(), pipelineLayout, nullptr);
}

void Hmck::HmckLightSystem::createPipelineLayout(std::vector<VkDescriptorSetLayout>& setLayouts)
{
	VkPushConstantRange pushConstantRange{};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(PointLightPushConstant);

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

void Hmck::HmckLightSystem::createPipeline(VkRenderPass renderPass)
{
	assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

	HmckPipelineConfigInfo pipelineConfig{};
	HmckPipeline::defaultHmckPipelineConfigInfo(pipelineConfig);
	HmckPipeline::enableAlphaBlending(pipelineConfig);
	pipelineConfig.attributeDescriptions.clear();
	pipelineConfig.bindingDescriptions.clear();
	pipelineConfig.renderPass = renderPass;
	pipelineConfig.pipelineLayout = pipelineLayout;
	pipeline = std::make_unique<HmckPipeline>(
		hmckDevice,
		std::string(SHADERS_DIR) + "Compiled/point_light.vert.spv",
		std::string(SHADERS_DIR) + "Compiled/point_light.frag.spv",
		pipelineConfig
	);
}

void Hmck::HmckLightSystem::render(HmckFrameInfo& frameInfo)
{
	// sort the lights
	std::map<float, HmckGameObject::id_t> sorted;
	for (auto& kv : frameInfo.gameObjects)
	{
		auto& obj = kv.second;
		if (obj.pointLightComponent == nullptr) continue;

		// calculate distance
		auto offset = frameInfo.camera.getPosition() - obj.transformComponent.translation;
		float disSqared = glm::dot(offset, offset);
		sorted[disSqared] = obj.getId();
	}

	pipeline->bind(frameInfo.commandBuffer);

	vkCmdBindDescriptorSets(
		frameInfo.commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		pipelineLayout,
		0, 1,
		&frameInfo.globalDescriptorSet,
		0,
		nullptr
	);

	// iterate over sorted map in reverse order
	for (auto it = sorted.rbegin(); it != sorted.rend(); ++it)
	{
		auto& obj = frameInfo.gameObjects.at(it->second);

		PointLightPushConstant push{};
		push.position = glm::vec4(obj.transformComponent.translation, 1.f);
		push.color = glm::vec4(obj.colorComponent, obj.pointLightComponent->lightIntensity);
		push.radius = obj.transformComponent.scale.x;

		vkCmdPushConstants(
			frameInfo.commandBuffer,
			pipelineLayout,
			VK_SHADER_STAGE_VERTEX_BIT |  VK_SHADER_STAGE_FRAGMENT_BIT,
			0,
			sizeof(PointLightPushConstant),
			&push
		);
		vkCmdDraw(frameInfo.commandBuffer, 6, 1, 0, 0);
	}

	
}


void Hmck::HmckLightSystem::update(HmckFrameInfo& frameInfo, HmckGlobalUbo& ubo)
{
	auto rotateLight = glm::rotate(
		glm::mat4(1.f),
		frameInfo.frameTime,
		{ 0.f, -1.f, 0.f }
	);

	int dirLightIdx = 0;
	int lightIndex = 0;
	for (auto& kv : frameInfo.gameObjects)
	{
		auto& obj = kv.second;
		
		if (obj.pointLightComponent != nullptr)
		{
			assert(lightIndex < MAX_LIGHTS && "Point light limit exeeded");

			// update lights
			obj.transformComponent.translation = glm::vec3(rotateLight * glm::vec4(obj.transformComponent.translation, 1.f));

			ubo.pointLights[lightIndex].position = glm::vec4(obj.transformComponent.translation, 1.f);
			ubo.pointLights[lightIndex].color = glm::vec4(obj.colorComponent, obj.pointLightComponent->lightIntensity);
			ubo.pointLights[lightIndex].lightTerms.x = obj.pointLightComponent->quadraticTerm;
			ubo.pointLights[lightIndex].lightTerms.y = obj.pointLightComponent->linearTerm;
			ubo.pointLights[lightIndex].lightTerms.z = obj.pointLightComponent->constantTerm;

			lightIndex += 1;
		}

		if (obj.directionalLightComponent != nullptr)
		{
			// directional light

			assert(dirLightIdx < 1 && "Directional light limit exeeded. There can only be one directional light");

			ubo.directionalLight.color = glm::vec4(obj.colorComponent, obj.directionalLightComponent->lightIntensity);
			ubo.directionalLight.direction = glm::vec4(obj.transformComponent.rotation, 1.f);

			dirLightIdx += 1;
		}
	}
	ubo.numLights = lightIndex;
}