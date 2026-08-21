#include "ForwardRenderPassProvider.hpp"
#include "../render_pass/RenderPassHelper.hpp"

RenderPassManager::Description
ForwardRenderPassProvider::build(
    VkFormat swapchainImageFormat,
    VkSampleCountFlagBits msaaSamples,
    VkFormat depthFormat
)
{
    RenderPassManager::Description description;

    const bool useMSAA =
        msaaSamples != VK_SAMPLE_COUNT_1_BIT;

    // ============================================================
    // Color
    // ============================================================

    const uint32_t color =
        RenderPassHelper::addColorAttachment(
            description,
            swapchainImageFormat,
            useMSAA
                ? msaaSamples
                : VK_SAMPLE_COUNT_1_BIT
        );

    // ============================================================
    // Depth
    // ============================================================

    const uint32_t depth =
        RenderPassHelper::addDepthAttachment(
            description,
            depthFormat,
            msaaSamples
        );

    // ============================================================
    // Resolve
    // ============================================================

    std::optional<uint32_t> resolve;

    if (useMSAA)
    {
        resolve =
            RenderPassHelper::addResolveAttachment(
                description,
                swapchainImageFormat
            );
    }

    // ============================================================
    // Subpass
    // ============================================================

    const uint32_t subpass =
        RenderPassHelper::addSubpass(
            description
        );

    RenderPassHelper::addColorAttachment(
        description,
        subpass,
        color
    );

    RenderPassHelper::setDepthAttachment(
        description,
        subpass,
        depth
    );

    if (resolve.has_value())
    {
        RenderPassHelper::addResolveAttachment(
            description,
            subpass,
            *resolve
        );
    }

    // ============================================================
    // Dependency
    // ============================================================

    RenderPassHelper::addExternalDependency(
        description,
        subpass
    );

    return description;
}