#pragma once

#include "../HmckPipeline.h"
#include "../HmckDevice.h"
#include "../HmckGameObject.h"
#include "../HmckCamera.h"
#include "../HmckFrameInfo.h"
#include "../HmckDescriptors.h"
#include "../HmckSwapChain.h"

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



/*
 *	A system is a process which acts on all GameObjects with the desired components.
 */

/*
	At this point app functions as a Master Render System
	and this class function as subsystem

	subject to change in near future
*/

namespace Hmck
{
	// scalar float: N = 4 Bytes
	// vec2: 2N = 8 Bytes
	// vec3 (or vec4): 4N = 16 Bytes
	// taken from 15.6.4 Offset and Stride Assignment
	struct HmckPushConstantData {
		glm::mat4 modelMatrix{ 1.f };
		glm::mat4 normalMatrix{ 1.f };
	};

	class HmckRenderSystem
	{
	public:

		HmckRenderSystem(HmckDevice& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
		~HmckRenderSystem();

		// delete copy constructor and copy destructor
		HmckRenderSystem(const HmckRenderSystem&) = delete;
		HmckRenderSystem& operator=(const HmckRenderSystem&) = delete;

		void renderGameObjects(HmckFrameInfo& frameInfo);

	private:
		void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
		void createPipeline(VkRenderPass renderPass);
		

		HmckDevice& hmckDevice;
		std::unique_ptr<HmckPipeline> hmckPipeline;
		VkPipelineLayout pipelineLayout;

		//std::unique_ptr<HmckDescriptorPool> descriptorPool{};
		//std::unique_ptr<HmckDescriptorSetLayout> descriptSetLayout;
	};

}
