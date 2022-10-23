#pragma once

#include "HmckPipeline.h"
#include "HmckDevice.h"
#include "HmckGameObject.h"
#include "HmckCamera.h"
#include "HmckFrameInfo.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <memory>
#include <vector>
#include <stdexcept>

namespace Hmck
{
	class HmckPointLightSystem
	{
	public:

		HmckPointLightSystem(HmckDevice& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
		~HmckPointLightSystem();

		// delete copy constructor and copy destructor
		HmckPointLightSystem(const HmckPointLightSystem&) = delete;
		HmckPointLightSystem& operator=(const HmckPointLightSystem&) = delete;

		void render(HmckFrameInfo& frameInfo);

	private:
		void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
		void createPipeline(VkRenderPass renderPass);


		HmckDevice& hmckDevice;

		std::unique_ptr<HmckPipeline> hmckPipeline;
		VkPipelineLayout pipelineLayout;
	};

}
