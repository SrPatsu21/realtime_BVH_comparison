#include "GeometryMeshPipelineProvider.hpp"

#include "../../graphics_pipeline/GraphicsPipelineManager.hpp"
#include "../../graphics_pipeline/GraphicsPipelineHelper.hpp"
#include "../../graphics_pipeline/layouts/MeshPipelineLayoutProvider.hpp"

void GeometryMeshPipelineProvider::createPipelines(
    GraphicsPipelineManager& manager,
    const PipelineCreationContext& ctx
)
{
    ShaderLoader opaqueShader(
        ctx.device,
        "shaders/geometry_mesh.vert.glsl.spv",
        "shaders/geometry_mesh_opaque.frag.glsl.spv"
    );

    ShaderLoader maskShader(
        ctx.device,
        "shaders/geometry_mesh.vert.glsl.spv",
        "shaders/geometry_mesh_mask.frag.glsl.spv"
    );

    ShaderLoader blendShader(
        ctx.device,
        "shaders/geometry_mesh.vert.glsl.spv",
        "shaders/geometry_mesh_blend.frag.glsl.spv"
    );

    VkPipelineShaderStageCreateInfo opaqueStages[2];
    GraphicsPipelineHelper::createVertexStage(
        opaqueShader.getVertModule(),
        opaqueStages[0]
    );
    GraphicsPipelineHelper::createFragmentStage(
        opaqueShader.getFragModule(),
        opaqueStages[1]
    );

    VkPipelineShaderStageCreateInfo maskStages[2];
    GraphicsPipelineHelper::createVertexStage(
        maskShader.getVertModule(),
        maskStages[0]
    );
    GraphicsPipelineHelper::createFragmentStage(
        maskShader.getFragModule(),
        maskStages[1]
    );

    VkPipelineShaderStageCreateInfo blendStages[2];
    GraphicsPipelineHelper::createVertexStage(
        blendShader.getVertModule(),
        blendStages[0]
    );
    GraphicsPipelineHelper::createFragmentStage(
        blendShader.getFragModule(),
        blendStages[1]
    );

    // --------------------------------------------------
    // Layout
    // --------------------------------------------------

    MeshPipelineLayoutProvider meshPipelineLayoutProvider;

    VkPipelineLayout pipelineLayout =
        manager.getLayout(
            meshPipelineLayoutProvider.createPipelineLayouts(
                manager,
                ctx
            )
        );

    // --------------------------------------------------
    // Vertex input
    // --------------------------------------------------

    VkVertexInputBindingDescription bindingDescription = Vertex::getBindingDescription();
    std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions =
        Vertex::getAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo;
    GraphicsPipelineHelper::createVertexInputState(
        bindingDescription,
        attributeDescriptions,
        vertexInputInfo
    );

    // --------------------------------------------------
    // Viewport
    // --------------------------------------------------

    VkPipelineViewportStateCreateInfo viewportState;
    GraphicsPipelineHelper::createViewportState(
        manager.getViewport(),
        manager.getScissor(),
        viewportState
    );

    // --------------------------------------------------
    // Multisampling
    // --------------------------------------------------

    VkPipelineMultisampleStateCreateInfo multiSampling;

    GraphicsPipelineHelper::createMultisampleState(
        ctx.msaa,
        multiSampling
    );

    // --------------------------------------------------
    // Dynamic state
    // --------------------------------------------------

    std::vector<VkDynamicState> dynamicStates =
    {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState;
    GraphicsPipelineHelper::createDynamicState(
        dynamicStates,
        dynamicState
    );

    // --------------------------------------------------
    // Depth / stencil
    // --------------------------------------------------

    VkPipelineDepthStencilStateCreateInfo depthStencil;
    GraphicsPipelineHelper::createDepthStencilState(
        depthStencil
    );

    // --------------------------------------------------
    // GBuffer color attachments
    // --------------------------------------------------

    std::array<VkPipelineColorBlendAttachmentState, 4> colorBlendAttachments;

    for (size_t i = 0; i < 4; i++)
    {
        colorBlendAttachments[i] = {};
        colorBlendAttachments[i].blendEnable = VK_FALSE;
        colorBlendAttachments[i].colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT |
            VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT;
    }

    VkPipelineColorBlendStateCreateInfo colorBlending;

    GraphicsPipelineHelper::createColorBlendState(
        colorBlendAttachments,
        colorBlending
    );

    // --------------------------------------------------
    // Input assembly
    // --------------------------------------------------

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState;
    GraphicsPipelineHelper::createInputAssemblyState(
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        inputAssemblyState
    );

    VkPipelineRasterizationStateCreateInfo rasterizationState;

    // ==================================================
    // OPAQUE MATERIAL / TRIANGLES / CULL BACK
    // ==================================================

    GraphicsPipelineHelper::createRasterizerState(
        VK_CULL_MODE_BACK_BIT,
        VK_POLYGON_MODE_FILL,
        rasterizationState
    );

    VkPipeline pipelineOpaque;

    GraphicsPipelineHelper::createPipeline(
        ctx.device,
        ctx.renderPass,
        pipelineLayout,
        opaqueStages,
        vertexInputInfo,
        inputAssemblyState,
        viewportState,
        rasterizationState,
        multiSampling,
        depthStencil,
        colorBlending,
        dynamicState,
        pipelineOpaque
    );

    manager.createPipeline(
        GraphicsPipelineManager::PIPE_TOPO_TRIANGLES |
        GraphicsPipelineManager::PIPE_CULL_BACK |
        GraphicsPipelineManager::PIPE_DEPTH_TEST |
        GraphicsPipelineManager::PIPE_DEPTH_WRITE |
        GraphicsPipelineManager::PIPE_GEOMETRY,
        pipelineOpaque
    );

    // ==================================================
    // MASK MATERIAL / TRIANGLES / CULL BACK
    // ==================================================

    VkPipeline pipelineMask;

    GraphicsPipelineHelper::createPipeline(
        ctx.device,
        ctx.renderPass,
        pipelineLayout,
        maskStages,
        vertexInputInfo,
        inputAssemblyState,
        viewportState,
        rasterizationState,
        multiSampling,
        depthStencil,
        colorBlending,
        dynamicState,
        pipelineMask
    );

    manager.createPipeline(
        GraphicsPipelineManager::PIPE_TOPO_TRIANGLES |
        GraphicsPipelineManager::PIPE_CULL_BACK |
        GraphicsPipelineManager::PIPE_DEPTH_TEST |
        GraphicsPipelineManager::PIPE_DEPTH_WRITE |
        GraphicsPipelineManager::PIPE_ALPHA_TEST |
        GraphicsPipelineManager::PIPE_GEOMETRY,
        pipelineMask
    );

    // ==================================================
    // BLEND TRIANGLES / CULL NONE
    // ==================================================

    GraphicsPipelineHelper::createRasterizerState(
        VK_CULL_MODE_NONE,
        VK_POLYGON_MODE_FILL,
        rasterizationState
    );

    std::array<VkPipelineColorBlendAttachmentState, 4> blendAttachments;

    for (size_t i = 0; i < 4; i++)
    {
        blendAttachments[i] = {};
        blendAttachments[i].blendEnable = VK_TRUE;
        blendAttachments[i].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        blendAttachments[i].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blendAttachments[i].colorBlendOp = VK_BLEND_OP_ADD;
        blendAttachments[i].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        blendAttachments[i].dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blendAttachments[i].alphaBlendOp = VK_BLEND_OP_ADD;
        blendAttachments[i].colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT |
            VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT;
    }

    GraphicsPipelineHelper::createColorBlendState(
        blendAttachments,
        colorBlending
    );

    VkPipeline pipelineBlend;

    // fix depth write off for this pipeline
    depthStencil.depthWriteEnable = VK_FALSE;

    GraphicsPipelineHelper::createPipeline(
        ctx.device,
        ctx.renderPass,
        pipelineLayout,
        blendStages,
        vertexInputInfo,
        inputAssemblyState,
        viewportState,
        rasterizationState,
        multiSampling,
        depthStencil,
        colorBlending,
        dynamicState,
        pipelineBlend
    );

    manager.createPipeline(
        GraphicsPipelineManager::PIPE_TOPO_TRIANGLES |
        GraphicsPipelineManager::PIPE_CULL_NONE |
        GraphicsPipelineManager::PIPE_DEPTH_TEST |
        GraphicsPipelineManager::PIPE_BLEND |
        GraphicsPipelineManager::PIPE_GEOMETRY,
        pipelineBlend
    );
}