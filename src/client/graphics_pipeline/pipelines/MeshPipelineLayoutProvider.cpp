#include "MeshPipelineLayoutProvider.hpp"
#include "GraphicsPipelineHelper.hpp"

GraphicsPipelineManager::PipelineFlags MeshPipelineLayoutProvider::createPipelineLayouts(
    GraphicsPipelineManager& manager,
    const PipelineCreationContext& ctx
)
{
    constexpr auto layoutFlags =
        GraphicsPipelineManager::PIPE_TOPO_TRIANGLES;

    if (manager.hasLayout(layoutFlags))
        return layoutFlags;

    VkPipelineLayout pipelineLayout;

    GraphicsPipelineHelper::createPipelineLayout(
        ctx.device,
        sizeof(InstanceData),
        {
            ctx.globalLayout,
            ctx.materialLayout,
            ctx.instanceLayout
        },
        pipelineLayout
    );

    manager.createLayout(
        layoutFlags,
        pipelineLayout
    );

    return layoutFlags;
}