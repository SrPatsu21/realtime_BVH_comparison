#include "LightingRecord.hpp"

void LightingRecord::record(
    VkCommandBuffer cmd,
    GraphicsPipelineManager* graphicsPipeline,
    VkDescriptorSet globalSet,
    VkDescriptorSet gbufferSet,
    const Config::ConfigTable& config
)
{
    if (config.render.mode != Config::RenderMode::GeometryGbuffer)
        return;

    const auto pipelineFlags =
        GraphicsPipelineManager::PIPE_LIGHTING |
        GraphicsPipelineManager::PIPE_TOPO_TRIANGLES |
        GraphicsPipelineManager::PIPE_CULL_NONE;

    VkPipelineLayout layout =
        graphicsPipeline->getLayout(
            GraphicsPipelineManager::PIPE_LIGHTING
        );

    // ============================================================
    // Pipeline
    // ============================================================

    vkCmdBindPipeline(
        cmd,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        graphicsPipeline->getPipeline(pipelineFlags)
    );

    // ============================================================
    // GBuffer + global descriptors
    // ============================================================

    VkDescriptorSet descriptorSets[] = {
        globalSet,
        gbufferSet
    };

    vkCmdBindDescriptorSets(
        cmd,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        layout,
        0,
        2,
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