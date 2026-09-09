#include "CompositeRecord.hpp"

void CompositeRecord::record(
    VkCommandBuffer cmd,
    GraphicsPipelineManager* graphicsPipeline,
    VkDescriptorSet deferredLightingSet
)
{

    const auto pipelineFlags =
        GraphicsPipelineManager::PIPE_COMPOSITE;

    VkPipelineLayout layout =
        graphicsPipeline->getLayout(
            GraphicsPipelineManager::PIPE_COMPOSITE
        );

    vkCmdBindPipeline(
        cmd,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        graphicsPipeline->getPipeline(pipelineFlags)
    );

    VkDescriptorSet descriptorSets[] = {
        deferredLightingSet
    };

    vkCmdBindDescriptorSets(
        cmd,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        layout,
        0,
        1,
        descriptorSets,
        0,
        nullptr
    );

    vkCmdDraw(
        cmd,
        3,
        1,
        0,
        0
    );
}