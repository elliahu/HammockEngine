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
	struct HmckPipelineConfigInfo
	{
		HmckPipelineConfigInfo() = default;
		HmckPipelineConfigInfo(const HmckPipelineConfigInfo&) = delete;
		HmckPipelineConfigInfo& operator=(const HmckPipelineConfigInfo&) = delete;

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

		static void defaultHmckPipelineConfigInfo(HmckPipelineConfigInfo& configInfo);
		static void enableAlphaBlending(HmckPipelineConfigInfo& configInfo);
		static void enablePolygonModeLine(HmckPipelineConfigInfo& configInfo);
		static void enableGbuffer(HmckPipelineConfigInfo& configInfo, std::array<VkPipelineColorBlendAttachmentState, 4> blendAttachmentStates);
		static void disableDepthTest(HmckPipelineConfigInfo& configInfo);

		void bind(VkCommandBuffer commandBuffer);

	private:
		static std::vector<char> readFile(const std::string& filePath);

		void createGraphicsPipeline(const std::string& vertexShaderFilePath, 
			const std::string& fragmentShaderFilePath, 
			const HmckPipelineConfigInfo& configInfo);
		
		void createShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule);
		
		HmckDevice& hmckDevice; // TODO potentionaly memmory unsafe
		VkPipeline graphicsPipeline;
		VkShaderModule vertShaderModule;
		VkShaderModule fragShaderModule;
	};

	class HmckGraphicsPipeline
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
			HmckDevice& device; 

			struct ShaderModuleInfo
			{
				const std::vector<char>& byteCode;
				std::string entryFunc = "main";
			};

			ShaderModuleInfo VS;
			ShaderModuleInfo FS;

			std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
			VkPushConstantRange pushConstantRange;

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

		static HmckGraphicsPipeline createGraphicsPipeline(GraphicsPipelineCreateInfo createInfo);
		void bind(VkCommandBuffer commandBuffer);

		~HmckGraphicsPipeline();

		HmckGraphicsPipeline(const HmckGraphicsPipeline&) = delete;
		HmckGraphicsPipeline& operator =(const HmckGraphicsPipeline&) = delete;

		VkPipelineLayout graphicsPipelineLayout;

	private:
		HmckGraphicsPipeline(GraphicsPipelineCreateInfo& createInfo);
		void defaultRenderPipelineConfig(GraphicsPipelineConfig& configInfo);
		void createShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule);
		HmckDevice& device;
		VkPipeline graphicsPipeline;
		VkShaderModule vertShaderModule;
		VkShaderModule fragShaderModule;
	};
}

