#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <stdexcept>
#include <iostream>
#include <cassert>
#include "HmckDevice.h"
#include "HmckVertex.h"
#include <memory>

namespace Hmck {
    class GraphicsPipeline {
        struct GraphicsPipelineConfig {
            GraphicsPipelineConfig() = default;

            GraphicsPipelineConfig(const GraphicsPipelineConfig &) = delete;

            GraphicsPipelineConfig &operator=(const GraphicsPipelineConfig &) = delete;

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

        struct GraphicsPipelineCreateInfo {
            std::string debugName;
            Device &device;

            struct ShaderModuleInfo {
                const std::vector<char> &byteCode;
                std::string entryFunc = "main";
            };

            ShaderModuleInfo VS;
            ShaderModuleInfo FS;

            std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
            std::vector<VkPushConstantRange> pushConstantRanges;

            struct GraphicsStateInfo {
                VkBool32 depthTest;
                VkCompareOp depthTestCompareOp;
                VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;
                VkFrontFace frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
                std::vector<VkPipelineColorBlendAttachmentState> blendAtaAttachmentStates;

                struct VertexBufferBindingsInfo {
                    std::vector<VkVertexInputBindingDescription> vertexBindingDescriptions;
                    std::vector<VkVertexInputAttributeDescription> vertexAttributeDescriptions;
                } vertexBufferBindings;
            } graphicsState;

            struct DynamicStateInfo {
                std::vector<VkDynamicState> dynamicStateEnables{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
            } dynamicState;

            VkRenderPass renderPass;
            uint32_t subpass = 0;
        };

    public:
        GraphicsPipeline(GraphicsPipelineCreateInfo &createInfo);

        static GraphicsPipeline createGraphicsPipeline(GraphicsPipelineCreateInfo createInfo);

        static std::unique_ptr<GraphicsPipeline> createGraphicsPipelinePtr(GraphicsPipelineCreateInfo createInfo);

        void bind(VkCommandBuffer commandBuffer);

        ~GraphicsPipeline();

        GraphicsPipeline(const GraphicsPipeline &) = delete;

        GraphicsPipeline &operator =(const GraphicsPipeline &) = delete;

        VkPipelineLayout graphicsPipelineLayout;

    private:
        static void defaultRenderPipelineConfig(GraphicsPipelineConfig &configInfo);

        void createShaderModule(const std::vector<char> &code, VkShaderModule *shaderModule) const;

        Device &device;
        VkPipeline graphicsPipeline;
        VkShaderModule vertShaderModule;
        VkShaderModule fragShaderModule;
    };
}
