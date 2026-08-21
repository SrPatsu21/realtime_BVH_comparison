#include "LightingPipelineProvider.hpp"
#include "../../graphics_pipeline/GraphicsPipelineManager.hpp"
#include "../../graphics_pipeline/GraphicsPipelineHelper.hpp"
#include "../pipeline_layouts/LightingPipelineLayoutProvider.hpp"

void LightingPipelineProvider::createPipelines(
    GraphicsPipelineManager& manager,
    const PipelineCreationContext& ctx
)
{
    #ifndef NDEBUG
        if (ctx.config == nullptr)
        {
            throw std::runtime_error(
                "DeferredLightingPipelineProvider requires ConfigTable"
            );
        }

        if (ctx.gBufferLayout == VK_NULL_HANDLE)
        {
            throw std::runtime_error(
                "DeferredLightingPipelineProvider requires GBuffer layout"
            );
        }
    #endif

    ShaderLoader shader(
        ctx.device,
        "shaders/deferred_lighting.vert.glsl.spv",
        "shaders/deferred_lighting.frag.glsl.spv"
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

    LightingPipelineLayoutProvider lightingPipelineLayoutProvider;

    VkPipelineLayout pipelineLayout =
        manager.getLayout(
            lightingPipelineLayoutProvider.createPipelineLayouts(
                manager,
                ctx
            )
        );

    // ============================================================
    // Create info
    // ============================================================
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

    VkPipelineMultisampleStateCreateInfo multiSampling;
    GraphicsPipelineHelper::createMultisampleState(
        ctx.msaa,
        multiSampling
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

    // ============================================================
    // Pipeline
    // ============================================================

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
        multiSampling,
        depthStencil,
        colorBlending,
        dynamicState,
        lightingPipeline
    );

    // ============================================================
    // Pipeline flags
    // ============================================================

    GraphicsPipelineManager::PipelineFlags lightingFlags =
        GraphicsPipelineManager::PIPE_LIGHTING |
        GraphicsPipelineManager::PIPE_TOPO_TRIANGLES |
        GraphicsPipelineManager::PIPE_CULL_NONE;

    manager.createPipeline(
        lightingFlags,
        lightingPipeline
    );
}