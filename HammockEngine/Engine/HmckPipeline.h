#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <stdexcept>
#include <iostream>
#include <cassert>
#include "HmckDevice.h"
#include "HmckVertex.h"

namespace Hmck 
{
	class GraphicsPipeline
	{
		struct GraphicsPipelineConfig
		{
			GraphicsPipelineConfig() = default;
			GraphicsPipelineConfig(const GraphicsPipelineConfig&) = delete;
			GraphicsPipelineConfig& operator=(const GraphicsPipelineConfig&) = delete;

			VkPipelineViewportStateCreateInfo viewportInfo;
			VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
			VkPipelineRasterizationStateCreateInfo rasterizationInfo;
			VkPipelineMultisampleStateCreateInfo multisampleInfo;
			VkPipelineColorBlendAttachmentState colorBlendAttachment;
			VkPipelineColorBlendStateCreateInfo colorBlendInfo;
			VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
			std::vector<VkDynamicState> dynamicStateEnables;
			VkPipelineDynamicStateCreateInfo dynamicStateInfo;
		};

		struct GraphicsPipelineCreateInfo
		{
			std::string debugName;
			Device& device; 

			struct ShaderModuleInfo
			{
				const std::vector<char>& byteCode;
				std::string entryFunc = "main";
			};

			ShaderModuleInfo VS;
			ShaderModuleInfo FS;

			std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
			std::vector <VkPushConstantRange> pushConstantRanges;

			struct GraphicsStateInfo
			{
				VkBool32 depthTest;
				VkCompareOp depthTestCompareOp;
				std::vector<VkPipelineColorBlendAttachmentState> blendAtaAttachmentStates;
				struct VertexBufferBindingsInfo
				{
					std::vector <VkVertexInputBindingDescription> vertexBindingDescriptions;
					std::vector<VkVertexInputAttributeDescription> vertexAttributeDescriptions;
				}vertexBufferBindings;

			}graphicsState;

			VkRenderPass renderPass;
			uint32_t subpass = 0;
		};

	public:
		GraphicsPipeline(GraphicsPipelineCreateInfo& createInfo);
		static GraphicsPipeline createGraphicsPipeline(GraphicsPipelineCreateInfo createInfo);
		static std::unique_ptr<GraphicsPipeline> createGraphicsPipelinePtr(GraphicsPipelineCreateInfo createInfo);
		void bind(VkCommandBuffer commandBuffer);

		~GraphicsPipeline();

		GraphicsPipeline(const GraphicsPipeline&) = delete;
		GraphicsPipeline& operator =(const GraphicsPipeline&) = delete;

		VkPipelineLayout graphicsPipelineLayout;

	private:
		void defaultRenderPipelineConfig(GraphicsPipelineConfig& configInfo);
		void createShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule);
		Device& device;
		VkPipeline graphicsPipeline;
		VkShaderModule vertShaderModule;
		VkShaderModule fragShaderModule;
	};
}

