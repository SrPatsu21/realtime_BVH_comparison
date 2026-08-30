#include "LightingPipelineLayoutProvider.hpp"
#include "../../graphics_pipeline/GraphicsPipelineHelper.hpp"

GraphicsPipelineManager::PipelineFlags
LightingPipelineLayoutProvider::createPipelineLayouts(
    GraphicsPipelineManager& manager,
    const PipelineCreationContext& ctx
)
{
    constexpr auto layoutFlags = GraphicsPipelineManager::PIPE_LIGHTING;

    if (manager.hasLayout(layoutFlags))
        return layoutFlags;

    VkPipelineLayout pipelineLayout;

    std::vector<VkDescriptorSetLayout> descriptorLayouts = {
        ctx.globalLayout,
        ctx.gBufferLayout,
        ctx.lightingLayout
    };

    GraphicsPipelineHelper::createPipelineLayout(
        ctx.device,
        0,
        descriptorLayouts,
        pipelineLayout
    );

    manager.createLayout(
        layoutFlags,
        pipelineLayout
    );

    return layoutFlags;
}