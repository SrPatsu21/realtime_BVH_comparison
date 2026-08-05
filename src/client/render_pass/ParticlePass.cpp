#include "ParticlePass.hpp"

void ParticlePass::record(
    VkCommandBuffer cmd,
    uint32_t currentFrame,
    GraphicsPipeline* graphicsPipeline,
    VkDescriptorSet globalSet,
    ParticleInstanceDescriptorManager* particleInstanceDescriptorManager,
    const std::vector<ParticleData>& particles
)
{
    uint32_t currentOffset = 0;
    VkPipelineLayout layout = graphicsPipeline->getLayout(GraphicsPipeline::PIPE_TOPO_POINTS);

    // Bind particle pipeline
    vkCmdBindPipeline(
        cmd,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        graphicsPipeline->getPipeline(
            GraphicsPipeline::PIPE_TOPO_POINTS |
            GraphicsPipeline::PIPE_CULL_NONE |
            GraphicsPipeline::PIPE_DEPTH_TEST |
            GraphicsPipeline::PIPE_BLEND
        )
    );

    // * use the same view port I think
    // replicate viewport/scissor
    // setViewportAndScissor(
    //     cmd,
    //     graphicsPipeline,
    //     viewportProviders,
    //     scissorProviders
    // );

    particleInstanceDescriptorManager->update(
        currentFrame,
        currentOffset,
        particles
    );

    // set 0 = global UBO
    vkCmdBindDescriptorSets(
        cmd,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        layout,
        0,
        1,
        &globalSet,
        0,
        nullptr
    );

    VkDescriptorSet particleSet = particleInstanceDescriptorManager->getDescriptorSets()[currentFrame];

    vkCmdBindDescriptorSets(
        cmd,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        layout,
        1,
        1,
        &particleSet,
        0,
        nullptr
    );

    // Draw 1 vertex, 1 instance
    vkCmdDraw(cmd, 1, particles.size(), 0, 0);
}