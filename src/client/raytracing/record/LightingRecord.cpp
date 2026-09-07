#include "LightingRecord.hpp"

void LightingRecord::record(
    VkCommandBuffer cmd,
    GraphicsPipelineManager* graphicsPipeline,
    VkDescriptorSet globalSet,
    VkDescriptorSet gBufferSet,
    VkDescriptorSet lightSet,
    VkDescriptorSet transparentGBufferSet,
    const Config::ConfigTable& config
)
{
    const auto pipelineFlags =
        GraphicsPipelineManager::PIPE_LIGHTING |
        GraphicsPipelineManager::PIPE_TOPO_TRIANGLES |
        GraphicsPipelineManager::PIPE_CULL_NONE;

    VkPipelineLayout layout =
        graphicsPipeline->getLayout(
            GraphicsPipelineManager::PIPE_LIGHTING
        );

    vkCmdBindPipeline(
        cmd,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        graphicsPipeline->getPipeline(pipelineFlags)
    );

    VkDescriptorSet descriptorSets[] = {
        globalSet,
        gBufferSet,
        lightSet,
        transparentGBufferSet
    };

    vkCmdBindDescriptorSets(
        cmd,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        layout,
        0,
        4,
        descriptorSets,
        0,
        nullptr
    );

    // ============================================================
    // Fullscreen triangle
    // ============================================================

    vkCmdDraw(
        cmd,
        3,
        1,
        0,
        0
    );
}