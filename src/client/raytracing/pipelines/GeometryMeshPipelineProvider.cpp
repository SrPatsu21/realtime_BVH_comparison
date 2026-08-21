#include "GeometryMeshPipelineProvider.hpp"

#include "../../graphics_pipeline/GraphicsPipelineManager.hpp"
#include "../../graphics_pipeline/GraphicsPipelineHelper.hpp"
#include "../../graphics_pipeline/layouts/MeshPipelineLayoutProvider.hpp"


void GeometryMeshPipelineProvider::createPipelines(
    GraphicsPipelineManager& manager,
    const PipelineCreationContext& ctx
)
{
    ShaderLoader shader(
        ctx.device,
        "shaders/geometry_mesh.vert.glsl.spv",
        "shaders/geometry_mesh.frag.glsl.spv"
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

    MeshPipelineLayoutProvider meshPipelineLayoutProvider;

    VkPipelineLayout pipelineLayout =
        manager.getLayout(
            meshPipelineLayoutProvider.createPipelineLayouts(
                manager,
                ctx
            )
        );

    //* create info
    VkVertexInputBindingDescription bindingDescription = Vertex::getBindingDescription();
    std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions =
        Vertex::getAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo;
    GraphicsPipelineHelper::createVertexInputState(
        bindingDescription,
        attributeDescriptions,
        vertexInputInfo
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

    VkPipelineDepthStencilStateCreateInfo depthStencil;
    GraphicsPipelineHelper::createDepthStencilState(
        depthStencil
    );


    // --------------------------------------------------
    // GBuffer color attachments
    //
    // Position
    // Normal
    // Albedo
    // Material
    // --------------------------------------------------

    std::array<VkPipelineColorBlendAttachmentState, 4> colorBlendAttachments;
    for (auto& attachment : colorBlendAttachments)
    {
        attachment = {};
        attachment.blendEnable = VK_FALSE;
        attachment.colorWriteMask =
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

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState;
    GraphicsPipelineHelper::createInputAssemblyState(
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        inputAssemblyState
    );

    VkPipelineRasterizationStateCreateInfo rasterizationState;
    GraphicsPipelineHelper::createRasterizerState(
        VK_CULL_MODE_NONE,
        VK_POLYGON_MODE_FILL,
        rasterizationState
    );


    // ==================================================
    // TRIANGLES / CULL NONE
    // ==================================================

    VkPipeline pipelineTriangleCullNone;

    GraphicsPipelineHelper::createPipeline(
        ctx.device,
        ctx.renderPass,
        pipelineLayout,
        shaderStages,
        vertexInputInfo,
        inputAssemblyState,
        viewportState,
        rasterizationState,
        multiSampling,
        depthStencil,
        colorBlending,
        dynamicState,
        pipelineTriangleCullNone
    );

    manager.createPipeline(
        GraphicsPipelineManager::PIPE_TOPO_TRIANGLES |
        GraphicsPipelineManager::PIPE_CULL_NONE |
        GraphicsPipelineManager::PIPE_DEPTH_TEST |
        GraphicsPipelineManager::PIPE_DEPTH_WRITE |
        GraphicsPipelineManager::PIPE_GEOMETRY,
        pipelineTriangleCullNone
    );


    // ==================================================
    // TRIANGLES / CULL BACK
    // ==================================================

    GraphicsPipelineHelper::createRasterizerState(
        VK_CULL_MODE_BACK_BIT,
        VK_POLYGON_MODE_FILL,
        rasterizationState
    );

    VkPipeline pipelineTriangleCullBack;

    GraphicsPipelineHelper::createPipeline(
        ctx.device,
        ctx.renderPass,
        pipelineLayout,
        shaderStages,
        vertexInputInfo,
        inputAssemblyState,
        viewportState,
        rasterizationState,
        multiSampling,
        depthStencil,
        colorBlending,
        dynamicState,
        pipelineTriangleCullBack
    );

    manager.createPipeline(
        GraphicsPipelineManager::PIPE_TOPO_TRIANGLES |
        GraphicsPipelineManager::PIPE_CULL_BACK |
        GraphicsPipelineManager::PIPE_DEPTH_TEST |
        GraphicsPipelineManager::PIPE_DEPTH_WRITE |
        GraphicsPipelineManager::PIPE_GEOMETRY,
        pipelineTriangleCullBack
    );


    // ==================================================
    // TRIANGLES / CULL FRONT
    // ==================================================

    GraphicsPipelineHelper::createRasterizerState(
        VK_CULL_MODE_FRONT_BIT,
        VK_POLYGON_MODE_FILL,
        rasterizationState
    );

    VkPipeline pipelineTriangleCullFront;

    GraphicsPipelineHelper::createPipeline(
        ctx.device,
        ctx.renderPass,
        pipelineLayout,
        shaderStages,
        vertexInputInfo,
        inputAssemblyState,
        viewportState,
        rasterizationState,
        multiSampling,
        depthStencil,
        colorBlending,
        dynamicState,
        pipelineTriangleCullFront
    );

    manager.createPipeline(
        GraphicsPipelineManager::PIPE_TOPO_TRIANGLES |
        GraphicsPipelineManager::PIPE_CULL_FRONT |
        GraphicsPipelineManager::PIPE_DEPTH_TEST |
        GraphicsPipelineManager::PIPE_DEPTH_WRITE |
        GraphicsPipelineManager::PIPE_GEOMETRY,
        pipelineTriangleCullFront
    );
}