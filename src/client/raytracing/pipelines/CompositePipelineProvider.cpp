#include "CompositePipelineProvider.hpp"

#include "../../graphics_pipeline/GraphicsPipelineManager.hpp"
#include "../../graphics_pipeline/GraphicsPipelineHelper.hpp"
#include "../pipeline_layouts/CompositePipelineLayoutProvider.hpp"

void CompositePipelineProvider::createPipelines(
    GraphicsPipelineManager& manager,
    const PipelineCreationContext& ctx
)
{
#ifndef NDEBUG
    if (ctx.deferredLightingLayout == VK_NULL_HANDLE)
        throw std::runtime_error("CompositePipelineProvider requires DeferredLighting layout");

    if (ctx.compositeRenderPass == VK_NULL_HANDLE)
        throw std::runtime_error("CompositePipelineProvider requires Composite render pass");
#endif

    ShaderLoader shader(
        ctx.device,
        "shaders/composite.vert.glsl.spv",
        "shaders/composite.frag.glsl.spv"
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

    // ============================================================
    // Pipeline layout
    // ============================================================

    CompositePipelineLayoutProvider compositePipelineLayoutProvider;

    VkPipelineLayout pipelineLayout =
        manager.getLayout(
            compositePipelineLayoutProvider.createPipelineLayouts(
                manager,
                ctx
            )
        );

    // ============================================================
    // Vertex input
    // ============================================================

    VkPipelineVertexInputStateCreateInfo vertexInput;

    GraphicsPipelineHelper::createEmptyVertexInputState(
        vertexInput
    );

    // ============================================================
    // Viewport
    // ============================================================

    VkPipelineViewportStateCreateInfo viewportState;

    GraphicsPipelineHelper::createViewportState(
        manager.getViewport(),
        manager.getScissor(),
        viewportState
    );

    // ============================================================
    // Multisampling
    // ============================================================

    VkPipelineMultisampleStateCreateInfo multiSampling;

    GraphicsPipelineHelper::createMultisampleState(
        VK_SAMPLE_COUNT_1_BIT,
        multiSampling
    );

    // ============================================================
    // Dynamic state
    // ============================================================

    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState;

    GraphicsPipelineHelper::createDynamicState(
        dynamicStates,
        dynamicState
    );

    // ============================================================
    // Depth / stencil
    // ============================================================

    VkPipelineDepthStencilStateCreateInfo depthStencil{};

    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_FALSE;
    depthStencil.depthWriteEnable = VK_FALSE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_ALWAYS;

    // ============================================================
    // Color blend
    // ============================================================

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

    // ============================================================
    // Input assembly
    // ============================================================

    VkPipelineInputAssemblyStateCreateInfo inputAssembly;

    GraphicsPipelineHelper::createInputAssemblyState(
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        inputAssembly
    );

    // ============================================================
    // Rasterization
    // ============================================================

    VkPipelineRasterizationStateCreateInfo rasterizationState;

    GraphicsPipelineHelper::createRasterizerState(
        VK_CULL_MODE_NONE,
        VK_POLYGON_MODE_FILL,
        rasterizationState
    );

    // ============================================================
    // Pipeline
    // ============================================================

    VkPipeline compositePipeline;

    GraphicsPipelineHelper::createPipeline(
        ctx.device,
        ctx.compositeRenderPass,
        pipelineLayout,
        shaderStages,
        vertexInput,
        inputAssembly,
        viewportState,
        rasterizationState,
        multiSampling,
        depthStencil,
        colorBlending,
        dynamicState,
        compositePipeline
    );

    // ============================================================
    // Pipeline flags
    // ============================================================

    constexpr auto compositeFlags =
        GraphicsPipelineManager::PIPE_COMPOSITE;

    manager.createPipeline(
        compositeFlags,
        compositePipeline
    );
}