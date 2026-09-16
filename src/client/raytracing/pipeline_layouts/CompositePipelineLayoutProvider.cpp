#include "CompositePipelineLayoutProvider.hpp"
#include "../../graphics_pipeline/GraphicsPipelineHelper.hpp"

GraphicsPipelineManager::PipelineFlags
CompositePipelineLayoutProvider::createPipelineLayouts(
    GraphicsPipelineManager& manager,
    const PipelineCreationContext& ctx
)
{
    constexpr auto layoutFlags =
        GraphicsPipelineManager::PIPE_COMPOSITE;

    if (manager.hasLayout(layoutFlags))
        return layoutFlags;

    VkPipelineLayout pipelineLayout;

    std::vector<VkDescriptorSetLayout> descriptorLayouts = {
        ctx.deferredLightingLayout
    };

    GraphicsPipelineHelper::createPipelineLayout(
        ctx.device,
        0,
        VK_SHADER_STAGE_VERTEX_BIT,
        descriptorLayouts,
        pipelineLayout
    );

    manager.createLayout(
        layoutFlags,
        pipelineLayout
    );

    return layoutFlags;
}