#include "LightingPass.hpp"

void LightingPass::record(
    VkCommandBuffer cmd,
    GraphicsPipelineManager* graphicsPipeline
) {
    const auto pipelineFlags =
        GraphicsPipelineManager::PIPE_TOPO_TRIANGLES |
        GraphicsPipelineManager::PIPE_CULL_NONE;

    VkPipelineLayout layout =
        graphicsPipeline->getLayout(
            GraphicsPipelineManager::PIPE_TOPO_TRIANGLES
        );

    vkCmdBindPipeline(
        cmd,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        graphicsPipeline->getPipeline(pipelineFlags)
    );

    vkCmdDraw(
        cmd,
        3,
        1,
        0,
        0
    );
}