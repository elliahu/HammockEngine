#pragma once

#include "../HmckPipeline.h"
#include "../HmckDevice.h"
#include "../HmckGameObject.h"
#include "../HmckCamera.h"
#include "../HmckFrameInfo.h"
#include "../HmckDescriptors.h"
#include "../HmckSwapChain.h"
#include "../HmckMesh.h"
#include "../HmckUtils.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <memory>
#include <vector>
#include <stdexcept>

#include "HmckIRenderSystem.h"

namespace Hmck
{
	class HmckGbufferRenderSystem : public HmckIRenderSystem
	{
	public:
		HmckGbufferRenderSystem(HmckDevice& device, VkRenderPass renderPass, std::vector<VkDescriptorSetLayout>& setLayouts);
		~HmckGbufferRenderSystem();

		// delete copy constructor and copy destructor
		HmckGbufferRenderSystem(const HmckGbufferRenderSystem&) = delete;
		HmckGbufferRenderSystem& operator=(const HmckGbufferRenderSystem&) = delete;

		void render(HmckFrameInfo& frameInfo);

	private:
		void createPipelineLayout(std::vector<VkDescriptorSetLayout>& setLayouts);
		void createPipeline(VkRenderPass renderPass);
	};
}
