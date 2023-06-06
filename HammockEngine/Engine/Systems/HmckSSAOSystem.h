#pragma once

#include <random>
#include <cmath>

#include <HmckIRenderSystem.h>
#include "../HmckDescriptors.h"
#include "../HmckUtils.h"

#define SSAO_KERNEL_SIZE 64
#define SSAO_RADIUS 0.3f
#define SSAO_NOISE_DIM 4

namespace Hmck
{
	class HmckSSAOSystem
	{

	public:
		HmckSSAOSystem(HmckDevice& device, VkRenderPass ssaoPass, VkRenderPass ssaoBlurPass, std::vector<VkDescriptorSetLayout>& setLayouts);
		~HmckSSAOSystem();

		// delete copy constructor and copy destructor
		HmckSSAOSystem(const HmckSSAOSystem&) = delete;
		HmckSSAOSystem& operator=(const HmckSSAOSystem&) = delete;

		void ssao(HmckFrameInfo& frameInfo);
		void ssaoBlur(HmckFrameInfo& frameInfo);

		void updateSSAODescriptorSet(std::vector<VkDescriptorImageInfo> imageInfos);
		void updateSSAOBlurDescriptorSet(VkDescriptorImageInfo& imageInfo);

	private:
		void createSSAOPipelineLayout(std::vector<VkDescriptorSetLayout>& setLayouts);
		void createSSAOPipeline(VkRenderPass renderPass);
		void createSSAOBlurPipelineLayout(std::vector<VkDescriptorSetLayout>& setLayouts);
		void createSSAOBlurPipeline(VkRenderPass renderPass);
		void createUBOAndNoise();
		void prepareDescriptors();

		HmckDevice& hmckDevice;
		std::unique_ptr<HmckPipeline> ssaoPipeline;
		VkPipelineLayout ssaoPipelineLayout = nullptr;
		std::unique_ptr<HmckPipeline> ssaoBlurPipeline;
		VkPipelineLayout ssaoBlurPipelineLayout = nullptr;

		std::unique_ptr<HmckDescriptorPool> descriptorPool{};
		std::unique_ptr<HmckDescriptorSetLayout> ssaoLayout{};
		std::unique_ptr<HmckDescriptorSetLayout> ssaoBlurLayout{};

		VkDescriptorSet ssaoDescriptorSet;
		VkDescriptorSet ssaoBlurDescriptorSet;

		std::vector<glm::vec4> ssaoKernel{ SSAO_KERNEL_SIZE };
		std::unique_ptr<HmckBuffer> ssaoKernelBuffer;

		HmckTexture2D ssaoNoiseTexture{};
	};
}
