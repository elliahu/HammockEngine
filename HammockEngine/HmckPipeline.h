#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <stdexcept>
#include <iostream>
#include <cassert>
#include "HmckDevice.h"
#include "HmckModel.h"

namespace Hmck 
{
	struct HmckPipelineConfigInfo
	{
		HmckPipelineConfigInfo() = default;
		HmckPipelineConfigInfo(const HmckPipelineConfigInfo&) = delete;
		HmckPipelineConfigInfo& operator=(const HmckPipelineConfigInfo&) = delete;

		VkPipelineViewportStateCreateInfo viewportInfo;
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
		VkPipelineRasterizationStateCreateInfo rasterizationInfo;
		VkPipelineMultisampleStateCreateInfo multisampleInfo;
		VkPipelineColorBlendAttachmentState colorBlendAttachment;
		VkPipelineColorBlendStateCreateInfo colorBlendInfo;
		VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
		std::vector<VkDynamicState> dynamicStateEnables;
		VkPipelineDynamicStateCreateInfo dynamicStateInfo;
		VkPipelineLayout pipelineLayout = nullptr;
		VkRenderPass renderPass = nullptr;
		uint32_t subpass = 0;
	};


	class HmckPipeline
	{
	public:
		HmckPipeline(
			HmckDevice& device, 
			const std::string& vertexShaderFilePath, 
			const std::string& fragmentShaderFilePath, 
			const HmckPipelineConfigInfo& configInfo);
		~HmckPipeline();

		HmckPipeline(const HmckPipeline&) = delete;
		HmckPipeline& operator =(const HmckPipeline&) = delete;

		static void defaultHmckPipelineConfigInfo(
			HmckPipelineConfigInfo& configInfo);

		void bind(VkCommandBuffer commandBuffer);

	private:
		static std::vector<char> readFile(const std::string& filePath);

		void createGraphicsPipeline(const std::string& vertexShaderFilePath, 
			const std::string& fragmentShaderFilePath, 
			const HmckPipelineConfigInfo& configInfo);
		
		void createShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule);
		
		HmckDevice& hmckDevice; // potentionaly memmory unsafe
		VkPipeline graphicsPipeline;
		VkShaderModule vertSahderModule;
		VkShaderModule fragShaderModule;
	};
}

