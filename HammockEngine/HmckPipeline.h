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

		VkViewport viewport;
		VkRect2D scissor;
		VkPipelineViewportStateCreateInfo viewportInfo;
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
		VkPipelineRasterizationStateCreateInfo rasterizationInfo;
		VkPipelineMultisampleStateCreateInfo multisampleInfo;
		VkPipelineColorBlendAttachmentState colorBlendAttachment;
		VkPipelineColorBlendStateCreateInfo colorBlendInfo;
		VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
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
		void operator =(const HmckPipeline&) = delete;

		static void defaultHmckPipelineConfigInfo(HmckPipelineConfigInfo& configInfo, uint32_t width, uint32_t height);

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

