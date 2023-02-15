#pragma once

#include "../HmckPipeline.h"
#include "../HmckDevice.h"
#include "../HmckGameObject.h"
#include "../HmckCamera.h"
#include "../HmckFrameInfo.h"
#include "../HmckDescriptors.h"
#include "../HmckSwapChain.h"
#include "../HmckModel.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <memory>
#include <vector>
#include <stdexcept>

#ifndef SHADERS_DIR
#define SHADERS_DIR "../../HammockEngine/Engine/Shaders/"
#endif

namespace Hmck
{
	class HmckOffscreenRenderSystem
	{
	public:

		HmckOffscreenRenderSystem(HmckDevice& device, VkRenderPass renderPass, std::vector<VkDescriptorSetLayout>& setLayouts);
		~HmckOffscreenRenderSystem();

		// delete copy constructor and copy destructor
		HmckOffscreenRenderSystem(const HmckOffscreenRenderSystem&) = delete;
		HmckOffscreenRenderSystem& operator=(const HmckOffscreenRenderSystem&) = delete;

		void renderOffscreen(HmckFrameInfo& frameInfo);

	private:
		void createPipelineLayout(std::vector<VkDescriptorSetLayout>& setLayouts);
		void createPipeline(VkRenderPass renderPass);

		HmckDevice& hmckDevice;
		std::unique_ptr<HmckPipeline> pipeline;
		VkPipelineLayout pipelineLayout;
	};
}


