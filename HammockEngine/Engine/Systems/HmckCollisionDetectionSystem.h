#pragma once

#include "HmckDevice.h"
#include "HmckPipeline.h"
#include "HmckFrameInfo.h"

#include <vector>
#include <utility>

#ifndef SHADERS_DIR
#define SHADERS_DIR "../../HammockEngine/Engine/Shaders/"
#endif


namespace Hmck
{
	struct HmckBoundingBoxPushConstant
	{
		glm::vec2 x{};
		glm::vec2 y{};
		glm::vec2 z{};
	};

	class HmckCollisionDetectionSystem
	{
	public:

		HmckCollisionDetectionSystem(HmckDevice& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
		~HmckCollisionDetectionSystem();

		// delete copy constructor and copy destructor
		HmckCollisionDetectionSystem(const HmckCollisionDetectionSystem&) = delete;
		HmckCollisionDetectionSystem& operator=(const HmckCollisionDetectionSystem&) = delete;

		void renderBoundingBoxes(HmckFrameInfo& frameInfo);
		
		bool intersect(HmckGameObject& obj1, HmckGameObject& obj2);


	private:
		void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
		void createPipeline(VkRenderPass renderPass);


		HmckDevice& hmckDevice;

		std::unique_ptr<HmckPipeline> hmckPipeline;
		VkPipelineLayout pipelineLayout;

	};
}