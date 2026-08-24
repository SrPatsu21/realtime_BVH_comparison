#include "LightingRenderPassProvider.hpp"

#include "../../render_pass/RenderPassHelper.hpp"

// ================================================================
// Lighting Render Pass
// ================================================================

void LightingRenderPassProvider::build(
    RenderPassManager::Description& description,
    VkFormat swapchainImageFormat
)
{
    // ------------------------------------------------------------
    // Lighting output
    //
    // The lighting pass renders directly into the swapchain image.
    // ------------------------------------------------------------

    const uint32_t colorAttachment =
        RenderPassHelper::addColorAttachment(
            description,
            swapchainImageFormat,
            VK_SAMPLE_COUNT_1_BIT
        );

    // ------------------------------------------------------------
    // Subpass
    // ------------------------------------------------------------

    const uint32_t subpass =
        RenderPassHelper::addSubpass(
            description
        );

    RenderPassHelper::addColorAttachment(
        description,
        subpass,
        colorAttachment
    );

    // ------------------------------------------------------------
    // External -> Lighting dependency
    // ------------------------------------------------------------

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = subpass;
    dependency.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependency.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependency.dependencyFlags = 0;

    RenderPassHelper::addDependency(
        description,
        dependency
    );
}