#include "GraphicsPipeline.hpp"
#include <stdexcept>

GraphicsPipeline::GraphicsPipeline(
    VkDevice device,
    VkExtent2D swapchainExtent,
    VkRenderPass renderPass,
    VkDescriptorSetLayout globalLayout,
    VkDescriptorSetLayout materialLayout,
    VkDescriptorSetLayout instanceLayout,
    VkDescriptorSetLayout particleLayout,
    VkSampleCountFlagBits msaaSamples,
    VkPhysicalDeviceVulkan12Features SupportedFeatures12
) :
    device(device)
{
    // Load shaders
    ShaderLoader* shaderLoader = new ShaderLoader(device, "shaders/triangle.vert.glsl.spv", "shaders/triangle.frag.glsl.spv");
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = shaderLoader->getVertModule();
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = shaderLoader->getFragModule();
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

//* create layouts
    VkDescriptorSetLayout meshMaterialLayout = materialLayout; // bindlessMode ? bindlessLayout :
    pipelineLayouts[GraphicsPipeline::PIPE_TOPO_TRIANGLES] = createPipelineLayout(
        static_cast<uint32_t>(sizeof(InstanceData)),
        {
            globalLayout,
            meshMaterialLayout,
            instanceLayout
        }
    );

    pipelineLayouts[GraphicsPipeline::PIPE_TOPO_POINTS] = createPipelineLayout(
        static_cast<uint32_t>(sizeof(ParticleData)),
        {
            globalLayout,
            particleLayout
        }
    );

//* create info
    VkVertexInputBindingDescription bindingDescription = Vertex::getBindingDescription();
    std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions = Vertex::getAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = createVertexInputState(
        bindingDescription,
        attributeDescriptions
    );
    viewport = {0.0f, 0.0f, static_cast<float>(swapchainExtent.width), static_cast<float>(swapchainExtent.height), 0.0f, 1.0f};
    scissor = { {0, 0}, swapchainExtent };
    VkPipelineViewportStateCreateInfo viewportState = createViewportState(viewport, scissor);
    VkPipelineMultisampleStateCreateInfo multisampling = createMultisampleState(msaaSamples);
    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicState = createDynamicState(dynamicStates);
    VkPipelineDepthStencilStateCreateInfo depthStencil = createDepthStencilState();
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {
        .blendEnable = VK_TRUE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,

        .colorWriteMask = (VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT)
    };
    VkPipelineColorBlendStateCreateInfo colorBlending = createColorBlendState(colorBlendAttachment);

    graphicsPipelines[
        GraphicsPipeline::PIPE_TOPO_TRIANGLES |
        GraphicsPipeline::PIPE_CULL_NONE |
        GraphicsPipeline::PIPE_DEPTH_TEST |
        GraphicsPipeline::PIPE_DEPTH_WRITE |
        GraphicsPipeline::PIPE_BLEND
    ] =
        createPipeline(
            renderPass,
            pipelineLayouts[GraphicsPipeline::PIPE_TOPO_TRIANGLES],
            shaderStages,
            vertexInputInfo,
            createInputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST),
            viewportState,
            createRasterizerState(VK_CULL_MODE_NONE, VK_POLYGON_MODE_FILL),
            multisampling,
            depthStencil,
            colorBlending,
            dynamicState
        );

    graphicsPipelines[
        GraphicsPipeline::PIPE_TOPO_TRIANGLES |
        GraphicsPipeline::PIPE_CULL_BACK |
        GraphicsPipeline::PIPE_DEPTH_TEST |
        GraphicsPipeline::PIPE_DEPTH_WRITE |
        GraphicsPipeline::PIPE_BLEND
    ] =
        createPipeline(
            renderPass,
            pipelineLayouts[GraphicsPipeline::PIPE_TOPO_TRIANGLES],
            shaderStages,
            vertexInputInfo,
            createInputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST),
            viewportState,
            createRasterizerState(VK_CULL_MODE_BACK_BIT, VK_POLYGON_MODE_FILL),
            multisampling,
            depthStencil,
            colorBlending,
            dynamicState
        );

    graphicsPipelines[
        GraphicsPipeline::PIPE_TOPO_TRIANGLES |
        GraphicsPipeline::PIPE_CULL_FRONT |
        GraphicsPipeline::PIPE_DEPTH_TEST |
        GraphicsPipeline::PIPE_DEPTH_WRITE |
        GraphicsPipeline::PIPE_BLEND
    ] =
        createPipeline(
            renderPass,
            pipelineLayouts[GraphicsPipeline::PIPE_TOPO_TRIANGLES],
            shaderStages,
            vertexInputInfo,
            createInputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST),
            viewportState,
            createRasterizerState(VK_CULL_MODE_FRONT_BIT, VK_POLYGON_MODE_FILL),
            multisampling,
            depthStencil,
            colorBlending,
            dynamicState
        );


    graphicsPipelines[
        GraphicsPipeline::PIPE_TOPO_LINES |
        GraphicsPipeline::PIPE_CULL_NONE |
        GraphicsPipeline::PIPE_DEPTH_TEST |
        GraphicsPipeline::PIPE_DEPTH_WRITE |
        GraphicsPipeline::PIPE_BLEND
    ] =
        createPipeline(
            renderPass,
            pipelineLayouts[GraphicsPipeline::PIPE_TOPO_TRIANGLES],
            shaderStages,
            vertexInputInfo,
            createInputAssemblyState(VK_PRIMITIVE_TOPOLOGY_LINE_LIST),
            viewportState,
            createRasterizerState(VK_CULL_MODE_NONE, VK_POLYGON_MODE_FILL),
            multisampling,
            depthStencil,
            colorBlending,
            dynamicState
        );

    //* fix for points
    delete shaderLoader;
    shaderLoader = new ShaderLoader(
        device,
        "shaders/particle.vert.glsl.spv",
        "shaders/particle.frag.glsl.spv"
    );

    // update shader stages
    vertShaderStageInfo.module = shaderLoader->getVertModule();
    fragShaderStageInfo.module = shaderLoader->getFragModule();

    shaderStages[0] = vertShaderStageInfo;
    shaderStages[1] = fragShaderStageInfo;

    // empty Vertex Input
    VkPipelineVertexInputStateCreateInfo emptyVertexInput{};
    emptyVertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    emptyVertexInput.vertexBindingDescriptionCount = 0;
    emptyVertexInput.pVertexBindingDescriptions = nullptr;
    emptyVertexInput.vertexAttributeDescriptionCount = 0;
    emptyVertexInput.pVertexAttributeDescriptions = nullptr;

    // blending for particles
    VkPipelineColorBlendAttachmentState particleBlendAttachment{
        .blendEnable = VK_TRUE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = (VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT)
    };

    VkPipelineColorBlendStateCreateInfo particleColorBlending = createColorBlendState(particleBlendAttachment);

    // depth: test yes, write no
    VkPipelineDepthStencilStateCreateInfo particleDepth = depthStencil;
    particleDepth.depthWriteEnable = VK_FALSE;

    dynamicStates.push_back(VK_DYNAMIC_STATE_LINE_WIDTH);

    dynamicState = createDynamicState(
        dynamicStates
    );

    graphicsPipelines[
        GraphicsPipeline::PIPE_TOPO_POINTS |
        GraphicsPipeline::PIPE_CULL_NONE |
        GraphicsPipeline::PIPE_DEPTH_TEST |
        GraphicsPipeline::PIPE_BLEND
    ] =
        createPipeline(
            renderPass,
            pipelineLayouts[GraphicsPipeline::PIPE_TOPO_POINTS],
            shaderStages,
            emptyVertexInput,
            createInputAssemblyState(VK_PRIMITIVE_TOPOLOGY_POINT_LIST),
            viewportState,
            createRasterizerState(VK_CULL_MODE_NONE, VK_POLYGON_MODE_FILL),
            multisampling,
            particleDepth,
            particleColorBlending,
            dynamicState
        );

    //shaders is not required anymore
    delete(shaderLoader);
}

GraphicsPipeline::~GraphicsPipeline() {
    for (auto& pair : graphicsPipelines) {
        if (pair.second != VK_NULL_HANDLE) {
            vkDestroyPipeline(device, pair.second, nullptr);
        }
    }

    for (auto& pair : pipelineLayouts) {
        if (pair.second != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(device, pair.second, nullptr);
        }
    }
}

VkPipelineLayout GraphicsPipeline::createPipelineLayout(
    uint32_t pushConstantRangeSize,
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts
) {
    VkPipelineLayout pipelineLayout;

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
    return pipelineLayout;
}

VkPipelineVertexInputStateCreateInfo GraphicsPipeline::createVertexInputState(
    VkVertexInputBindingDescription& bindingDescription,
    std::array<VkVertexInputAttributeDescription, 4>& attributeDescriptions
) {
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    return vertexInputInfo;
}

VkPipelineInputAssemblyStateCreateInfo GraphicsPipeline::createInputAssemblyState(
    VkPrimitiveTopology topology
) {
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = topology;
    inputAssembly.primitiveRestartEnable = VK_FALSE;
    return inputAssembly;
}

VkPipelineViewportStateCreateInfo GraphicsPipeline::createViewportState(
    VkViewport& viewport,
    VkRect2D& scissor
) {
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    return viewportState;
}

VkPipelineRasterizationStateCreateInfo GraphicsPipeline::createRasterizerState(
    VkCullModeFlags cullMode,
    VkPolygonMode polygonMode
) {
    VkPipelineRasterizationStateCreateInfo rasterizer{};
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
    return rasterizer;
}

VkPipelineMultisampleStateCreateInfo GraphicsPipeline::createMultisampleState(VkSampleCountFlagBits msaaSamples) {
    VkPipelineMultisampleStateCreateInfo multisampling{};
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
    return multisampling;
}

VkPipelineDepthStencilStateCreateInfo GraphicsPipeline::createDepthStencilState() {
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
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

    return depthStencil;
}

VkPipelineColorBlendStateCreateInfo GraphicsPipeline::createColorBlendState(
    VkPipelineColorBlendAttachmentState& colorBlendAttachment
) {
    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    return colorBlending;
}

VkPipelineDynamicStateCreateInfo GraphicsPipeline::createDynamicState(
    const std::vector<VkDynamicState>& dynamicStates
) {
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    return dynamicState;
}

VkPipeline GraphicsPipeline::createPipeline(
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
    const VkPipelineDynamicStateCreateInfo& dynamicState
) {
    VkPipeline graphicsPipeline;

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

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    return graphicsPipeline;
}