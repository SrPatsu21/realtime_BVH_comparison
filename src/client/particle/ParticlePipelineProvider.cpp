#include "ParticlePipelineProvider.hpp"
#include "../graphics_pipeline/GraphicsPipelineManager.hpp"
#include "../graphics_pipeline/GraphicsPipelineHelper.hpp"

void ParticlePipelineProvider::createPipelines(
    GraphicsPipelineManager& manager,
    const PipelineCreationContext& ctx)
{
    ShaderLoader shader(
        ctx.device,
        "shaders/particle.vert.glsl.spv",
        "shaders/particle.frag.glsl.spv"
    );

    VkPipelineShaderStageCreateInfo shaderStages[2];
    GraphicsPipelineHelper::createVertexStage(
        shader.getVertModule(),
        shaderStages[0]
    );
    GraphicsPipelineHelper::createFragmentStage(
        shader.getFragModule(),
        shaderStages[1]
    );

    //* layout
    VkPipelineLayout pipelineLayout;
    GraphicsPipelineHelper::createPipelineLayout(
        ctx.device,
        sizeof(ParticleData),
        {
            ctx.globalLayout,
            ctx.particleLayout
        },
        pipelineLayout
    );

    manager.createLayout(
        GraphicsPipelineManager::PIPE_TOPO_POINTS,
        pipelineLayout
    );

    //* empty vertex input
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    //* viewport
    VkPipelineViewportStateCreateInfo viewportState;
    GraphicsPipelineHelper::createViewportState(
        manager.getViewport(),
        manager.getScissor(),
        viewportState
    );

    //* multisampling
    VkPipelineMultisampleStateCreateInfo multisampling;
    GraphicsPipelineHelper::createMultisampleState(
        ctx.msaa,
        multisampling
    );

    //* dynamic states
    VkPipelineDynamicStateCreateInfo dynamicState;
    std::vector<VkDynamicState> dynamicStates =
    {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    GraphicsPipelineHelper::createDynamicState(
        dynamicStates,
        dynamicState
    );

    //* depth
    VkPipelineDepthStencilStateCreateInfo depthStencil;
    GraphicsPipelineHelper::createDepthStencilState(depthStencil);
    depthStencil.depthWriteEnable = VK_FALSE;

    //* blend
    VkPipelineColorBlendAttachmentState blendAttachment{
        .blendEnable = VK_TRUE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT |
            VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT
    };

    VkPipelineColorBlendStateCreateInfo colorBlending;
    GraphicsPipelineHelper::createColorBlendState(
        blendAttachment,
        colorBlending
    );

    //* input assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState;
    GraphicsPipelineHelper::createInputAssemblyState(
        VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
        inputAssemblyState
    );

    //* rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizationState;
    GraphicsPipelineHelper::createRasterizerState(
        VK_CULL_MODE_NONE,
        VK_POLYGON_MODE_FILL,
        rasterizationState
    );

    //* pipeline
    VkPipeline pipeline;
    GraphicsPipelineHelper::createPipeline(
        ctx.device,
        ctx.renderPass,
        pipelineLayout,
        shaderStages,
        vertexInputInfo,
        inputAssemblyState,
        viewportState,
        rasterizationState,
        multisampling,
        depthStencil,
        colorBlending,
        dynamicState,
        pipeline
    );

    manager.createPipeline(
        GraphicsPipelineManager::PIPE_TOPO_POINTS |
        GraphicsPipelineManager::PIPE_CULL_NONE |
        GraphicsPipelineManager::PIPE_DEPTH_TEST |
        GraphicsPipelineManager::PIPE_BLEND,
        pipeline
    );
}