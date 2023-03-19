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
		if (obj.pointLightComponent == nullptr && obj.directionalLightComponent == nullptr) continue;

		// calculate distance
		auto offset = frameInfo.camera.getPosition() - obj.transformComponent.translation;
		float disSqared = glm::dot(offset, offset);
		sorted[disSqared] = obj.getId();
	}

	pipeline->bind(frameInfo.commandBuffer);

	// UBO
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
		if(obj.directionalLightComponent == nullptr)
			push.color = glm::vec4(obj.colorComponent, obj.pointLightComponent->lightIntensity);
		if(obj.pointLightComponent == nullptr)
			push.color = glm::vec4(obj.colorComponent, obj.directionalLightComponent->lightIntensity);
		push.radius = obj.transformComponent.scale.x;

		// psh constant
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

	glm::vec3 cameraPos = {
		ubo.inverseView[3].x,
		ubo.inverseView[3].y,
		ubo.inverseView[3].z,
	};


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

			ubo.pointLights[lightIndex].position = ubo.view * glm::vec4(obj.transformComponent.translation, 1.f);
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

			glm::mat3 view = glm::mat3(ubo.view);

			ubo.directionalLight.color = glm::vec4(obj.colorComponent, obj.directionalLightComponent->lightIntensity);
			ubo.directionalLight.direction = glm::vec4(
				 glm::normalize(obj.directionalLightComponent->target - obj.transformComponent.translation),
				obj.directionalLightComponent->fov);

			// TODO make this not ubo as it is not used in all systems
			glm::mat4 depthProjectionMatrix = glm::perspective(
				obj.directionalLightComponent->fov, 1.0f, 
				obj.directionalLightComponent->_near, 
				obj.directionalLightComponent->_far);
			glm::mat4 depthViewMatrix = glm::lookAt(obj.transformComponent.translation, obj.directionalLightComponent->target, glm::vec3(0, 1, 0));
			glm::mat4 depthModelMatrix = glm::mat4(1.0f);
			ubo.depthBiasMVP = depthProjectionMatrix * depthViewMatrix * depthModelMatrix;

			dirLightIdx += 1;
		}
	}
	ubo.numLights = lightIndex;
}