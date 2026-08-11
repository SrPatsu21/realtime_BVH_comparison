#include "LightingPipelineProvider.hpp"

#include "../GraphicsPipelineManager.hpp"
#include "GraphicsPipelineHelper.hpp"

void LightingPipelineProvider::createPipelines(
    GraphicsPipelineManager& manager,
    const PipelineCreationContext& ctx
)
{
    ShaderLoader shader(
        ctx.device,
        "shaders/lighting_fullscreen.vert.glsl.spv",
        "shaders/lighting_fullscreen.frag.glsl.spv"
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
        0,
        {},
        pipelineLayout
    );
    manager.createLayout(
        GraphicsPipelineManager::PIPE_LIGHTING,
        pipelineLayout
    );

    //* create info
    VkPipelineVertexInputStateCreateInfo vertexInput;
    GraphicsPipelineHelper::createEmptyVertexInputState(
        vertexInput
    );

    VkPipelineViewportStateCreateInfo viewportState;
    GraphicsPipelineHelper::createViewportState(
        manager.getViewport(),
        manager.getScissor(),
        viewportState
    );

    VkPipelineMultisampleStateCreateInfo multisampling;
    GraphicsPipelineHelper::createMultisampleState(
        ctx.msaa,
        multisampling
    );

    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicState;
    GraphicsPipelineHelper::createDynamicState(
        dynamicStates,
        dynamicState
    );

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_FALSE;
    depthStencil.depthWriteEnable = VK_FALSE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_ALWAYS;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT |
        VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT |
        VK_COLOR_COMPONENT_A_BIT;
    VkPipelineColorBlendStateCreateInfo colorBlending;
    GraphicsPipelineHelper::createColorBlendState(
        colorBlendAttachment,
        colorBlending
    );

    VkPipelineInputAssemblyStateCreateInfo inputAssembly;
    GraphicsPipelineHelper::createInputAssemblyState(
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        inputAssembly
    );

    VkPipelineRasterizationStateCreateInfo rasterizationState;
    GraphicsPipelineHelper::createRasterizerState(
        VK_CULL_MODE_NONE,
        VK_POLYGON_MODE_FILL,
        rasterizationState
    );

    //* Pipeline TRIANGLE_CULL_NONE
    VkPipeline lightingPipeline;
    GraphicsPipelineHelper::createPipeline(
        ctx.device,
        ctx.renderPass,
        pipelineLayout,
        shaderStages,
        vertexInput,
        inputAssembly,
        viewportState,
        rasterizationState,
        multisampling,
        depthStencil,
        colorBlending,
        dynamicState,
        lightingPipeline
    );

    manager.createPipeline(
        GraphicsPipelineManager::PIPE_TOPO_TRIANGLES |
        GraphicsPipelineManager::PIPE_CULL_NONE,
        lightingPipeline
    );
}