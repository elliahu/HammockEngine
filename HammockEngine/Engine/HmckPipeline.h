#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <stdexcept>
#include <iostream>
#include <cassert>
#include "HmckDevice.h"
#include "HmckGLTF.h"

namespace Hmck 
{
	struct PipelineConfigInfo
	{
		PipelineConfigInfo() = default;
		PipelineConfigInfo(const PipelineConfigInfo&) = delete;
		PipelineConfigInfo& operator=(const PipelineConfigInfo&) = delete;

		std::vector<VkVertexInputBindingDescription> bindingDescriptions{};
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};
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


	class Pipeline
	{
	public:
		Pipeline(
			Device& device, 
			const std::string& vertexShaderFilePath, 
			const std::string& fragmentShaderFilePath, 
			const PipelineConfigInfo& configInfo);
		~Pipeline();

		Pipeline(const Pipeline&) = delete;
		Pipeline& operator =(const Pipeline&) = delete;

		static void defaultHmckPipelineConfigInfo(PipelineConfigInfo& configInfo);
		static void enableAlphaBlending(PipelineConfigInfo& configInfo);
		static void enablePolygonModeLine(PipelineConfigInfo& configInfo);
		static void enableGbuffer(PipelineConfigInfo& configInfo, std::array<VkPipelineColorBlendAttachmentState, 4> blendAttachmentStates);
		static void disableDepthTest(PipelineConfigInfo& configInfo);

		void bind(VkCommandBuffer commandBuffer);

	private:
		static std::vector<char> readFile(const std::string& filePath);

		void createGraphicsPipeline(const std::string& vertexShaderFilePath, 
			const std::string& fragmentShaderFilePath, 
			const PipelineConfigInfo& configInfo);
		
		void createShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule);
		
		Device& hmckDevice; // TODO potentionaly memmory unsafe
		VkPipeline graphicsPipeline;
		VkShaderModule vertShaderModule;
		VkShaderModule fragShaderModule;
	};

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

		static GraphicsPipeline createGraphicsPipeline(GraphicsPipelineCreateInfo createInfo);
		void bind(VkCommandBuffer commandBuffer);

		~GraphicsPipeline();

		GraphicsPipeline(const GraphicsPipeline&) = delete;
		GraphicsPipeline& operator =(const GraphicsPipeline&) = delete;

		VkPipelineLayout graphicsPipelineLayout;

	private:
		GraphicsPipeline(GraphicsPipelineCreateInfo& createInfo);
		void defaultRenderPipelineConfig(GraphicsPipelineConfig& configInfo);
		void createShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule);
		Device& device;
		VkPipeline graphicsPipeline;
		VkShaderModule vertShaderModule;
		VkShaderModule fragShaderModule;
	};
}

