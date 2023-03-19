#pragma once

#include "../HmckPipeline.h"
#include "../HmckDevice.h"
#include "../HmckGameObject.h"
#include "../HmckCamera.h"
#include "../HmckFrameInfo.h"
#include "../HmckDescriptors.h"
#include "../HmckSwapChain.h"
#include "../HmckMesh.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <memory>
#include <vector>
#include <stdexcept>

#include "HmckIRenderSystem.h"

#ifndef SHADERS_DIR
#define SHADERS_DIR "../../HammockEngine/Engine/Shaders/"
#endif

namespace Hmck
{
	class HmckShadowmapSystem : public HmckIRenderSystem
	{
		struct OffscreenPushConstantData 
		{
			glm::mat4 offscreenMVP;
		};

	public:

		HmckShadowmapSystem(HmckDevice& device, VkRenderPass renderPass, std::vector<VkDescriptorSetLayout>& setLayouts);
		~HmckShadowmapSystem();

		// delete copy constructor and copy destructor
		HmckShadowmapSystem(const HmckShadowmapSystem&) = delete;
		HmckShadowmapSystem& operator=(const HmckShadowmapSystem&) = delete;

		void render(HmckFrameInfo& frameInfo);

	private:
		void createPipelineLayout(std::vector<VkDescriptorSetLayout>& setLayouts);
		void createPipeline(VkRenderPass renderPass);

		// Depth bias (and slope) are used to avoid shadowing artifacts
		// Constant depth bias factor (always applied)
		float depthBiasConstant = 1.25f;
		// Slope depth bias factor, applied depending on polygon's slope
		float depthBiasSlope = 1.75f;
	};
}


