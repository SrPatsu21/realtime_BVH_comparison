#include "GraphicsPipelineManager.hpp"
#include "../forward_render/ForwardMeshPipelineProvider.hpp"
#include "../particle/ParticlePipelineProvider.hpp"
#include "../raytracing/pipelines/GeometryMeshPipelineProvider.hpp"
#include "../raytracing/pipelines/LightingPipelineProvider.hpp"

GraphicsPipelineManager::GraphicsPipelineManager(
    VkDevice device,
    VkExtent2D swapchainExtent,
    const PipelineCreationContext& ctx,
    const Config::ConfigTable& config
) :
    device(device)
{
    viewport = {0.0f, 0.0f, static_cast<float>(swapchainExtent.width), static_cast<float>(swapchainExtent.height), 0.0f, 1.0f};
    scissor = { {0, 0}, swapchainExtent };

    GeometryMeshPipelineProvider geometryMeshPipelineProvider;
    ParticlePipelineProvider particleProvider;
    LightingPipelineProvider lightingProvider;
    ForwardMeshPipelineProvider forwardMeshPipelineProvider;

    switch (config.render.mode)
    {
        case Config::RenderMode::Forward:
            forwardMeshPipelineProvider.createPipelines(*this, ctx);
            break;
        case Config::RenderMode::GeometryGBuffer:
            geometryMeshPipelineProvider.createPipelines(*this, ctx);
            lightingProvider.createPipelines(*this, ctx);
            break;
        default:
            throw std::runtime_error(
                "Unknown render mode"
            );
    }
    //! culpado
    // particleProvider.createPipelines(*this, ctx);
}

GraphicsPipelineManager::~GraphicsPipelineManager() {
    for (auto& pair : graphicsPipelines) {
        if (pair.second != VK_NULL_HANDLE) {
            vkDestroyPipeline(device, pair.second, nullptr);
        }
    }

    for (auto& pair : pipelineLayouts) {
        if (pair.second != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(device, pair.second, nullptr);
        }
    }
}

bool GraphicsPipelineManager::createPipeline(
    PipelineFlags flags,
    VkPipeline pipeline
) {
    auto [it, inserted] = graphicsPipelines.try_emplace(flags, pipeline);

    return inserted;
}

bool GraphicsPipelineManager::createLayout(
    PipelineFlags flags,
    VkPipelineLayout layout
) {
    auto [it, inserted] = pipelineLayouts.try_emplace(flags, layout);

    return inserted;
}

bool GraphicsPipelineManager::hasLayout(PipelineFlags flags) const
{
    return pipelineLayouts.find(flags) != pipelineLayouts.end();
}

bool GraphicsPipelineManager::hasPipeline(PipelineFlags flags) const
{
    return graphicsPipelines.find(flags) != graphicsPipelines.end();
}
