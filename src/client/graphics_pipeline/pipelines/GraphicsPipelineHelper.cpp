#include "GraphicsPipelineHelper.hpp"
#include <stdexcept>

void GraphicsPipelineHelper::createVertexStage(
    VkShaderModule vertModule,
    VkPipelineShaderStageCreateInfo& out
) {
    out = {};
    out.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    out.stage = VK_SHADER_STAGE_VERTEX_BIT;
    out.module = vertModule;
    out.pName = "main";
};

void GraphicsPipelineHelper::createFragmentStage(
    VkShaderModule fragModule,
    VkPipelineShaderStageCreateInfo& out
) {
    out = {};
    out.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    out.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    out.module = fragModule;
    out.pName = "main";
};

void GraphicsPipelineHelper::createVertexInputState(
    VkVertexInputBindingDescription& bindingDescription,
    std::array<VkVertexInputAttributeDescription, 4>& attributeDescriptions,
    VkPipelineVertexInputStateCreateInfo& vertexInputState
) {
    vertexInputState = {};
    vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputState.vertexBindingDescriptionCount = 1;
    vertexInputState.pVertexBindingDescriptions = &bindingDescription;
    vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputState.pVertexAttributeDescriptions = attributeDescriptions.data();
}

void GraphicsPipelineHelper::createPipelineLayout(
    VkDevice device,
    uint32_t pushConstantRangeSize,
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts,
    VkPipelineLayout& pipelineLayout
) {
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();

    VkPushConstantRange pushConstantRange{};

    if (pushConstantRangeSize > 0) {
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = pushConstantRangeSize;

        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    } else {
        pipelineLayoutInfo.pushConstantRangeCount = 0;
        pipelineLayoutInfo.pPushConstantRanges = nullptr;
    }

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }
}

void GraphicsPipelineHelper::createInputAssemblyState(
    VkPrimitiveTopology topology,
    VkPipelineInputAssemblyStateCreateInfo& inputAssemblyState
) {
    inputAssemblyState = {};
    inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyState.topology = topology;
    inputAssemblyState.primitiveRestartEnable = VK_FALSE;
}

void GraphicsPipelineHelper::createViewportState(
    const VkViewport& viewport,
    const VkRect2D& scissor,
    VkPipelineViewportStateCreateInfo& viewportState
) {
    viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;
}

void GraphicsPipelineHelper::createRasterizerState(
    VkCullModeFlags cullMode,
    VkPolygonMode polygonMode,
    VkPipelineRasterizationStateCreateInfo& rasterizer
) {
    rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = polygonMode;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = cullMode;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f;
    rasterizer.depthBiasClamp = 0.0f;
    rasterizer.depthBiasSlopeFactor = 0.0f;
}

void GraphicsPipelineHelper::createMultisampleState(
    VkSampleCountFlagBits msaaSamples,
    VkPipelineMultisampleStateCreateInfo& multisampling
) {
    multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = msaaSamples;
    if (msaaSamples != VK_SAMPLE_COUNT_1_BIT) {
        multisampling.sampleShadingEnable = VK_TRUE; // enable sample shading in the pipeline
        multisampling.minSampleShading = 0.2f; // min fraction for sample shading; closer to one is smoother
    } else {
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.minSampleShading = 1.0f;
    }
    multisampling.pSampleMask = nullptr;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;
}

void GraphicsPipelineHelper::createDynamicState(
    const std::vector<VkDynamicState>& dynamicStates,
    VkPipelineDynamicStateCreateInfo& dynamicState
) {
    dynamicState = {};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();
}

void GraphicsPipelineHelper::createDepthStencilState(
    VkPipelineDepthStencilStateCreateInfo& depthStencil
) {
    depthStencil = {};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f; // Optional
    depthStencil.maxDepthBounds = 1.0f; // Optional
    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.front = {}; // Optional
    depthStencil.back = {}; // Optional
}

void GraphicsPipelineHelper::createColorBlendState(
    VkPipelineColorBlendAttachmentState& colorBlendAttachment,
    VkPipelineColorBlendStateCreateInfo& colorBlending
) {
    colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;
}

void GraphicsPipelineHelper::createPipeline(
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
    VkPipeline& graphicsPipeline
) {
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInput;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlend;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }
}

void GraphicsPipelineHelper::createEmptyVertexInputState(
    VkPipelineVertexInputStateCreateInfo& vertexInputState
) {
    vertexInputState = {};
    vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputState.vertexBindingDescriptionCount = 0;
    vertexInputState.pVertexBindingDescriptions = nullptr;
    vertexInputState.vertexAttributeDescriptionCount = 0;
    vertexInputState.pVertexAttributeDescriptions = nullptr;
}