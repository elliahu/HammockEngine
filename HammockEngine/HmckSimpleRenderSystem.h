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
	/// scalar float: N = 4 Bytes
	/// vec2: 2N = 8 Bytes
	/// vec3 (or vec4): 4N = 16 Bytes
	/// taken from 15.6.4 Offset and Stride Assignment
	struct HmckSimplePushConstantData {
		glm::mat4 modelMatrix{ 1.f };
		glm::mat4 normalMatrix{ 1.f };
	};

	class HmckSimpleRenderSystem
	{
	public:

		HmckSimpleRenderSystem(HmckDevice& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
		~HmckSimpleRenderSystem();

		// delete copy constructor and copy destructor
		HmckSimpleRenderSystem(const HmckSimpleRenderSystem&) = delete;
		HmckSimpleRenderSystem& operator=(const HmckSimpleRenderSystem&) = delete;

		void renderGameObjects(HmckFrameInfo& frameInfo, std::vector<HmckGameObject>& gameObjects);

	private:
		void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
		void createPipeline(VkRenderPass renderPass);
		

		HmckDevice& hmckDevice;

		std::unique_ptr<HmckPipeline> hmckPipeline;
		VkPipelineLayout pipelineLayout;
	};

}
