#include "HmckSSAOSystem.h"

Hmck::HmckSSAOSystem::HmckSSAOSystem(HmckDevice& device, VkRenderPass ssaoPass, VkRenderPass ssaoBlurPass, std::vector<VkDescriptorSetLayout>& setLayouts) : hmckDevice{ device }
{
	prepareDescriptors();
	createSSAOPipelineLayout(setLayouts);
	createSSAOPipeline(ssaoPass);
	createSSAOBlurPipelineLayout(setLayouts);
	createSSAOBlurPipeline(ssaoBlurPass);
	createUBOAndNoise();
}

Hmck::HmckSSAOSystem::~HmckSSAOSystem()
{
	vkDestroyPipelineLayout(hmckDevice.device(), ssaoPipelineLayout, nullptr);
	vkDestroyPipelineLayout(hmckDevice.device(), ssaoBlurPipelineLayout, nullptr);
	ssaoNoiseTexture.destroy(hmckDevice);
}


void Hmck::HmckSSAOSystem::ssao(HmckFrameInfo& frameInfo)
{
	ssaoPipeline->bind(frameInfo.commandBuffer);

	// bind global descriptor set (UBO)
	vkCmdBindDescriptorSets(
		frameInfo.commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		ssaoPipelineLayout,
		0, 1,
		&frameInfo.globalDescriptorSet,
		0,
		nullptr
	);

	vkCmdBindDescriptorSets(
		frameInfo.commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		ssaoPipelineLayout,
		1, 1,
		&ssaoDescriptorSet,
		0,
		nullptr
	);

	vkCmdDraw(frameInfo.commandBuffer, 3, 1, 0, 0);
}

void Hmck::HmckSSAOSystem::ssaoBlur(HmckFrameInfo& frameInfo)
{
	ssaoBlurPipeline->bind(frameInfo.commandBuffer);

	// bind global descriptor set(UBO)
	vkCmdBindDescriptorSets(
		frameInfo.commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		ssaoBlurPipelineLayout,
		0, 1,
		&frameInfo.globalDescriptorSet,
		0,
		nullptr
	);
	vkCmdBindDescriptorSets(
		frameInfo.commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		ssaoBlurPipelineLayout,
		1, 1,
		&ssaoBlurDescriptorSet,
		0,
		nullptr
	);

	vkCmdDraw(frameInfo.commandBuffer, 3, 1, 0, 0);
}

void Hmck::HmckSSAOSystem::updateSSAODescriptorSet(std::array<VkDescriptorImageInfo, 2> imageInfos)
{
	auto bufferInfo = ssaoKernelBuffer->descriptorInfo();
	auto writer = HmckDescriptorWriter(*ssaoLayout, *descriptorPool)
		.writeImage(0, &imageInfos[0])
		.writeImage(1, &imageInfos[1])
		.writeImage(2, &ssaoNoiseTexture.descriptor)
		.writeBuffer(3, &bufferInfo)
		.build(ssaoDescriptorSet);
}

void Hmck::HmckSSAOSystem::updateSSAOBlurDescriptorSet(VkDescriptorImageInfo& imageInfo)
{
	auto writer = HmckDescriptorWriter(*ssaoBlurLayout, *descriptorPool)
		.writeImage(0, &imageInfo)
		.build(ssaoBlurDescriptorSet);
}

void Hmck::HmckSSAOSystem::createSSAOPipelineLayout(std::vector<VkDescriptorSetLayout>& setLayouts)
{
	std::vector<VkDescriptorSetLayout> layouts{ setLayouts };
	layouts.push_back(ssaoLayout->getDescriptorSetLayout());

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
	pipelineLayoutInfo.pSetLayouts = layouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = nullptr;

	if (vkCreatePipelineLayout(hmckDevice.device(), &pipelineLayoutInfo, nullptr, &ssaoPipelineLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create pipeline layout");
	}
}

void Hmck::HmckSSAOSystem::createSSAOPipeline(VkRenderPass renderPass)
{
	assert(ssaoPipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

	HmckPipelineConfigInfo pipelineConfig{};
	HmckPipeline::defaultHmckPipelineConfigInfo(pipelineConfig);
	pipelineConfig.renderPass = renderPass;
	pipelineConfig.pipelineLayout = ssaoPipelineLayout;
	ssaoPipeline = std::make_unique<HmckPipeline>(
		hmckDevice,
		std::string(SHADERS_DIR) + "Compiled/fullscreen.vert.spv",
		std::string(SHADERS_DIR) + "Compiled/ssao.frag.spv",
		pipelineConfig
		);
}

void Hmck::HmckSSAOSystem::createSSAOBlurPipelineLayout(std::vector<VkDescriptorSetLayout>& setLayouts)
{
	std::vector<VkDescriptorSetLayout> layouts{ setLayouts };
	layouts.push_back(ssaoBlurLayout->getDescriptorSetLayout());

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
	pipelineLayoutInfo.pSetLayouts = layouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = nullptr;

	if (vkCreatePipelineLayout(hmckDevice.device(), &pipelineLayoutInfo, nullptr, &ssaoBlurPipelineLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create pipeline layout");
	}
}

void Hmck::HmckSSAOSystem::createSSAOBlurPipeline(VkRenderPass renderPass)
{
	assert(ssaoBlurPipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

	HmckPipelineConfigInfo pipelineConfig{};
	HmckPipeline::defaultHmckPipelineConfigInfo(pipelineConfig);
	pipelineConfig.renderPass = renderPass;
	pipelineConfig.pipelineLayout = ssaoBlurPipelineLayout;
	ssaoBlurPipeline = std::make_unique<HmckPipeline>(
		hmckDevice,
		std::string(SHADERS_DIR) + "Compiled/fullscreen.vert.spv",
		std::string(SHADERS_DIR) + "Compiled/blur.frag.spv",
		pipelineConfig
		);
}

void Hmck::HmckSSAOSystem::createUBOAndNoise()
{
	std::default_random_engine rndEngine((unsigned)time(nullptr));
	std::uniform_real_distribution<float> rndDist(0.0f, 1.0f);

	for (uint32_t i = 0; i < SSAO_KERNEL_SIZE; ++i)
	{
		glm::vec3 sample(rndDist(rndEngine) * 2.0 - 1.0, rndDist(rndEngine) * 2.0 - 1.0, rndDist(rndEngine));
		sample = glm::normalize(sample);
		sample *= rndDist(rndEngine);
		float scale = float(i) / float(SSAO_KERNEL_SIZE);
		scale = Hmck::Math::lerp(0.1f, 1.0f, scale * scale);
		ssaoKernel[i] = glm::vec4(sample * scale, 0.0f);
	}

	ssaoKernelBuffer = std::make_unique<HmckBuffer>(
		hmckDevice,
		ssaoKernel.size() * sizeof(glm::vec4),
		1,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		);
	ssaoKernelBuffer->map();
	ssaoKernelBuffer->writeToBuffer(ssaoKernel.data());

	// Random noise
	std::vector<glm::vec4> ssaoNoise(SSAO_NOISE_DIM * SSAO_NOISE_DIM);
	for (uint32_t i = 0; i < static_cast<uint32_t>(ssaoNoise.size()); i++)
	{
		ssaoNoise[i] = glm::vec4(rndDist(rndEngine) * 2.0f - 1.0f, rndDist(rndEngine) * 2.0f - 1.0f, 0.0f, 0.0f);
	}
	// Upload as texture
	ssaoNoiseTexture.loadFromBuffer((unsigned char *) ssaoNoise.data(), ssaoNoise.size() * sizeof(glm::vec4), SSAO_NOISE_DIM, SSAO_NOISE_DIM, hmckDevice, VK_FORMAT_R32G32B32A32_SFLOAT);
	ssaoNoiseTexture.createSampler(hmckDevice, VK_FILTER_NEAREST);
	ssaoNoiseTexture.updateDescriptor();
}


void Hmck::HmckSSAOSystem::prepareDescriptors()
{
	descriptorPool = HmckDescriptorPool::Builder(hmckDevice)
		.setMaxSets(100)
		.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1)
		.addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4)
		.build();

	ssaoLayout = HmckDescriptorSetLayout::Builder(hmckDevice)
		.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.addBinding(3, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.build();

	ssaoBlurLayout = HmckDescriptorSetLayout::Builder(hmckDevice)
		.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.build();
}
