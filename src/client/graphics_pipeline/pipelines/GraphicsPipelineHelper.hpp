#pragma once
#include "../../CoreVulkan.hpp"
#include <array>

class GraphicsPipelineHelper {

public:

    static void createVertexStage(
        VkShaderModule vertModule,
        VkPipelineShaderStageCreateInfo& out
    );

    static void createFragmentStage(
        VkShaderModule fragModule,
        VkPipelineShaderStageCreateInfo& out
    );

    static void createVertexInputState(
        VkVertexInputBindingDescription& bindingDescription,
        std::array<VkVertexInputAttributeDescription, 4>& attributeDescriptions,
        VkPipelineVertexInputStateCreateInfo& out
    );

    static void createInputAssemblyState(
        VkPrimitiveTopology topology,
        VkPipelineInputAssemblyStateCreateInfo& out
    );

    static void createViewportState(
        const VkViewport& viewport,
        const VkRect2D& scissor,
        VkPipelineViewportStateCreateInfo& out
    );
    static void createRasterizerState(
        VkCullModeFlags cullMode,
        VkPolygonMode polygonMode,
        VkPipelineRasterizationStateCreateInfo& out
    );
    static void createMultisampleState(
        VkSampleCountFlagBits msaaSamples,
        VkPipelineMultisampleStateCreateInfo& out
    );
    static void createDynamicState(
        const std::vector<VkDynamicState>& dynamicState,
        VkPipelineDynamicStateCreateInfo& out
    );
    static void createDepthStencilState(
        VkPipelineDepthStencilStateCreateInfo& out
    );
    static void createColorBlendState(
        VkPipelineColorBlendAttachmentState& colorBlendAttachment,
        VkPipelineColorBlendStateCreateInfo& out
    );
    static void createColorBlendState(
        const std::array<VkPipelineColorBlendAttachmentState, 4>& colorBlendAttachments,
        VkPipelineColorBlendStateCreateInfo& out
    );

    static void createPipelineLayout(
        VkDevice device,
        uint32_t pushConstantRangeSize,
        std::vector<VkDescriptorSetLayout> descriptorSetLayouts,
        VkPipelineLayout& out
    );

    static void createPipeline(
        VkDevice device,
        const VkRenderPass renderPass,
        const VkPipelineLayout& pipelineLayout,
        const VkPipelineShaderStageCreateInfo* shaderStages,
        const VkPipelineVertexInputStateCreateInfo& vertexInput,
        const VkPipelineInputAssemblyStateCreateInfo& inputAssembly,
        const VkPipelineViewportStateCreateInfo& viewportState,
        const VkPipelineRasterizationStateCreateInfo& rasterizer,
        const VkPipelineMultisampleStateCreateInfo& multisampling,
        const VkPipelineDepthStencilStateCreateInfo& depthStencil,
        const VkPipelineColorBlendStateCreateInfo& colorBlend,
        const VkPipelineDynamicStateCreateInfo& dynamicState,
        VkPipeline& out
    );

    static void createEmptyVertexInputState(
        VkPipelineVertexInputStateCreateInfo& vertexInputState
    );
};
