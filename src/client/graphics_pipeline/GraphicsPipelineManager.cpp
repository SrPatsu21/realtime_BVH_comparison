#include "GraphicsPipelineManager.hpp"
#include "pipelines/GeometryMeshPipelineProvider.hpp"
#include "pipelines/ParticlePipelineProvider.hpp"
#include "pipelines/LightingPipelineProvider.hpp"
#include "pipelines/ForwardMeshPipelineProvider.hpp"

GraphicsPipelineManager::GraphicsPipelineManager(
    VkDevice device,
    VkExtent2D swapchainExtent,
    VkRenderPass renderPass,
    VkDescriptorSetLayout globalLayout,
    VkDescriptorSetLayout materialLayout,
    VkDescriptorSetLayout instanceLayout,
    VkDescriptorSetLayout particleLayout,
    VkSampleCountFlagBits msaaSamples,
    const Config::ConfigTable& config,
    VkPhysicalDeviceVulkan12Features supportedFeatures12
) :
    device(device)
{
    viewport = {0.0f, 0.0f, static_cast<float>(swapchainExtent.width), static_cast<float>(swapchainExtent.height), 0.0f, 1.0f};
    scissor = { {0, 0}, swapchainExtent };

    GeometryMeshPipelineProvider geometryMeshPipelineProvider;
    ParticlePipelineProvider particleProvider;
    LightingPipelineProvider lightingProvider;
    ForwardMeshPipelineProvider forwardMeshPipelineProvider;

    PipelineCreationContext ctx{
        .device = device,
        .renderPass = renderPass,
        .globalLayout = globalLayout,
        .materialLayout = materialLayout,
        .particleLayout = particleLayout,
        .instanceLayout = instanceLayout,
        .msaa = msaaSamples,
        .supportedFeatures12 = supportedFeatures12
    };

    switch (config.render.mode)
    {
        case Config::RenderMode::Forward:
            forwardMeshPipelineProvider.createPipelines(*this, ctx);
            break;
        case Config::RenderMode::GeometryGbuffer:
            geometryMeshPipelineProvider.createPipelines(*this, ctx);
            lightingProvider.createPipelines(*this, ctx);
            break;
        default:
            throw std::runtime_error(
                "Unknown render mode"
            );
    }

    particleProvider.createPipelines(*this, ctx);
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
