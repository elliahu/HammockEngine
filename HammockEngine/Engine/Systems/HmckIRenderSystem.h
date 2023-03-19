#pragma once

#include "HmckFrameInfo.h"
#include "HmckPipeline.h"

#ifndef SHADERS_DIR
#define SHADERS_DIR "../../HammockEngine/Engine/Shaders/"
#endif


namespace Hmck
{
	// Renders system interface
	class HmckIRenderSystem
	{
	public:
		HmckIRenderSystem(HmckDevice& device): hmckDevice{device}{}

		virtual void render(HmckFrameInfo& frameInfo) = 0;

	protected:
		virtual void createPipelineLayout(std::vector<VkDescriptorSetLayout>& setLayouts) = 0;
		virtual void createPipeline(VkRenderPass renderPass) = 0;

		HmckDevice& hmckDevice;
		std::unique_ptr<HmckPipeline> pipeline;
		VkPipelineLayout pipelineLayout = nullptr;
	};
}