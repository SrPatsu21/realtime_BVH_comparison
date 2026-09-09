#include "DeferredLightingRenderPassProvider.hpp"

#include "../../render_pass/RenderPassHelper.hpp"

void DeferredLightingRenderPassProvider::build(
    RenderPassManager::Description& description,
    VkSampleCountFlagBits msaaSamples
) {
    const uint32_t colorAttachment =
        RenderPassHelper::addColorAttachment(
            description,
            VK_FORMAT_R16G16B16A16_SFLOAT,
            msaaSamples,
            VK_ATTACHMENT_STORE_OP_STORE,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
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
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependency.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    RenderPassHelper::addDependency(
        description,
        dependency
    );
}