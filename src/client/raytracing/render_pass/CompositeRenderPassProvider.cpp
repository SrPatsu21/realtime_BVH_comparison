#include "CompositeRenderPassProvider.hpp"

#include "../../render_pass/RenderPassHelper.hpp"

void CompositeRenderPassProvider::build(
    RenderPassManager::Description& description,
    VkFormat swapchainFormat
)
{
    const uint32_t colorAttachment =
        RenderPassHelper::addColorAttachment(
            description,
            swapchainFormat,
            VK_SAMPLE_COUNT_1_BIT,
            VK_ATTACHMENT_STORE_OP_STORE,
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
        );

    const uint32_t subpass =
        RenderPassHelper::addSubpass(
            description
        );

    RenderPassHelper::addColorAttachment(
        description,
        subpass,
        colorAttachment
    );

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = subpass;
    dependency.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependency.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    RenderPassHelper::addDependency(
        description,
        dependency
    );
}