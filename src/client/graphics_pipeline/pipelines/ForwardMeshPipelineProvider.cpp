#include "ForwardMeshPipelineProvider.hpp"
#include "../GraphicsPipelineManager.hpp"
#include "GraphicsPipelineHelper.hpp"
#include "MeshPipelineLayoutProvider.hpp"


void ForwardMeshPipelineProvider::createPipelines(
    GraphicsPipelineManager& manager,
    const PipelineCreationContext& ctx
) {
    ShaderLoader shader(
        ctx.device,
        "shaders/triangle.vert.glsl.spv",
        "shaders/triangle.frag.glsl.spv"
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

    VkPipelineLayout pipelineLayout = manager.getLayout(
        meshPipelineLayoutProvider.createPipelineLayouts(
            manager,
            ctx
        )
    );

    //* create info
    VkVertexInputBindingDescription bindingDescription = Vertex::getBindingDescription();
    std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions = Vertex::getAttributeDescriptions();

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

    VkPipelineMultisampleStateCreateInfo multisampling;
    GraphicsPipelineHelper::createMultisampleState(
        ctx.msaa,
        multisampling
    );

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

    VkPipelineDepthStencilStateCreateInfo depthStencil;
    GraphicsPipelineHelper::createDepthStencilState(
        depthStencil
    );


    VkPipelineColorBlendStateCreateInfo colorBlending;
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
    GraphicsPipelineHelper::createColorBlendState(
        colorBlendAttachment,
        colorBlending
    );


//* Pipeline TRIANGLE_CULL_NONE
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

    VkPipeline graphicsPipeline_TRIANGLE_CULL_NONE;
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
        graphicsPipeline_TRIANGLE_CULL_NONE
    );
    manager.createPipeline(
        GraphicsPipelineManager::PIPE_TOPO_TRIANGLES |
        GraphicsPipelineManager::PIPE_CULL_NONE |
        GraphicsPipelineManager::PIPE_DEPTH_TEST |
        GraphicsPipelineManager::PIPE_DEPTH_WRITE |
        GraphicsPipelineManager::PIPE_BLEND |
        GraphicsPipelineManager::PIPE_GEOMETRY |
        GraphicsPipelineManager::PIPE_LIGHTING,
        graphicsPipeline_TRIANGLE_CULL_NONE
    );

//* Pipeline TRIANGLE_CULL_BACK
    GraphicsPipelineHelper::createRasterizerState(
        VK_CULL_MODE_BACK_BIT,
        VK_POLYGON_MODE_FILL,
        rasterizationState
    );

    VkPipeline graphicsPipeline_TRIANGLE_CULL_BACK;
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
        graphicsPipeline_TRIANGLE_CULL_BACK
    );
    manager.createPipeline(
        GraphicsPipelineManager::PIPE_TOPO_TRIANGLES |
        GraphicsPipelineManager::PIPE_CULL_BACK |
        GraphicsPipelineManager::PIPE_DEPTH_TEST |
        GraphicsPipelineManager::PIPE_DEPTH_WRITE |
        GraphicsPipelineManager::PIPE_BLEND |
        GraphicsPipelineManager::PIPE_GEOMETRY |
        GraphicsPipelineManager::PIPE_LIGHTING,
        graphicsPipeline_TRIANGLE_CULL_BACK
    );

//* Pipeline TRIANGLE_CULL_FRONT
    GraphicsPipelineHelper::createRasterizerState(
        VK_CULL_MODE_FRONT_BIT,
        VK_POLYGON_MODE_FILL,
        rasterizationState
    );

    VkPipeline graphicsPipeline_TRIANGLE_CULL_FRONT;
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
        graphicsPipeline_TRIANGLE_CULL_FRONT
    );
    manager.createPipeline(
        GraphicsPipelineManager::PIPE_TOPO_TRIANGLES |
        GraphicsPipelineManager::PIPE_CULL_FRONT |
        GraphicsPipelineManager::PIPE_DEPTH_TEST |
        GraphicsPipelineManager::PIPE_DEPTH_WRITE |
        GraphicsPipelineManager::PIPE_BLEND |
        GraphicsPipelineManager::PIPE_GEOMETRY |
        GraphicsPipelineManager::PIPE_LIGHTING,
        graphicsPipeline_TRIANGLE_CULL_FRONT
    );

//* debug lines LINES_CULL_NONE
    GraphicsPipelineHelper::createRasterizerState(
        VK_CULL_MODE_NONE,
        VK_POLYGON_MODE_FILL,
        rasterizationState
    );

    GraphicsPipelineHelper::createInputAssemblyState(
        VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
        inputAssemblyState
    );

    VkPipeline graphicsPipeline_LINES_CULL_NONE;
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
        graphicsPipeline_LINES_CULL_NONE
    );
    manager.createPipeline(
        GraphicsPipelineManager::PIPE_TOPO_LINES |
        GraphicsPipelineManager::PIPE_CULL_NONE |
        GraphicsPipelineManager::PIPE_DEPTH_TEST |
        GraphicsPipelineManager::PIPE_DEPTH_WRITE |
        GraphicsPipelineManager::PIPE_BLEND |
        GraphicsPipelineManager::PIPE_GEOMETRY |
        GraphicsPipelineManager::PIPE_LIGHTING,
        graphicsPipeline_LINES_CULL_NONE
    );

}