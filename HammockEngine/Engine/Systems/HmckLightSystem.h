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
#include <map>

#include "HmckIRenderSystem.h"

#ifndef SHADERS_DIR
	#define SHADERS_DIR "../../HammockEngine/Engine/Shaders/"
#endif

/*
 *	A system is a process which acts on all entities with the desired components.
 */

namespace Hmck
{
	class HmckLightSystem : public HmckIRenderSystem
	{

		struct PointLightPushConstant
		{
			glm::vec4 position{};
			glm::vec4 color{};
			float radius;
		};

	public:

		HmckLightSystem(HmckDevice& device, VkRenderPass renderPass, std::vector<VkDescriptorSetLayout>& setLayouts);
		~HmckLightSystem();

		// delete copy constructor and copy destructor
		HmckLightSystem(const HmckLightSystem&) = delete;
		HmckLightSystem& operator=(const HmckLightSystem&) = delete;

		void update(HmckFrameInfo& frameInfo, HmckGlobalUbo& ubo);
		void render(HmckFrameInfo& frameInfo);

	private:
		void createPipelineLayout(std::vector<VkDescriptorSetLayout>& setLayouts);
		void createPipeline(VkRenderPass renderPass);
	};

}
