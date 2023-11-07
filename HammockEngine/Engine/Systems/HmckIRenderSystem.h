#pragma once

#include "HmckFrameInfo.h"
#include "HmckPipeline.h"

#ifndef SHADERS_DIR
#define SHADERS_DIR "../../HammockEngine/Engine/Shaders/"
#endif


namespace Hmck
{
	// Renders system interface
	class IRenderSystem
	{
	public:
		IRenderSystem(Device& device): device{device}{}

		virtual void render(FrameInfo& frameInfo) = 0;

		VkPipelineLayout getPipelineLayout() { return pipelineLayout; }
		std::unique_ptr<Pipeline> pipeline;

	protected:
		virtual void createPipelineLayout(std::vector<VkDescriptorSetLayout>& setLayouts) = 0;
		virtual void createPipeline(VkRenderPass renderPass) = 0;

		Device& device;
		VkPipelineLayout pipelineLayout = nullptr;
	};
}