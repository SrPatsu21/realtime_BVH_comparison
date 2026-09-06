#include "TransparentGBufferRenderPassProvider.hpp"
#include "../../render_pass/RenderPassHelper.hpp"

void TransparentGBufferRenderPassProvider::build(
    RenderPassManager::Description& description,
    VkSampleCountFlagBits msaaSamples,
    VkFormat depthFormat
)
{
    const uint32_t position =
        RenderPassHelper::addColorAttachment(
            description,
            VK_FORMAT_R16G16B16A16_SFLOAT,
            msaaSamples,
            VK_ATTACHMENT_STORE_OP_STORE,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        );

    const uint32_t normal =
        RenderPassHelper::addColorAttachment(
            description,
            VK_FORMAT_R16G16B16A16_SFLOAT,
            msaaSamples,
            VK_ATTACHMENT_STORE_OP_STORE,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        );

    const uint32_t albedo =
        RenderPassHelper::addColorAttachment(
            description,
            VK_FORMAT_R8G8B8A8_UNORM,
            msaaSamples,
            VK_ATTACHMENT_STORE_OP_STORE,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        );

    const uint32_t material =
        RenderPassHelper::addColorAttachment(
            description,
            VK_FORMAT_R8G8B8A8_UNORM,
            msaaSamples,
            VK_ATTACHMENT_STORE_OP_STORE,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        );

    const uint32_t depth =
        RenderPassHelper::addDepthAttachment(
            description,
            depthFormat,
            msaaSamples,
            VK_ATTACHMENT_LOAD_OP_LOAD,
            VK_ATTACHMENT_STORE_OP_STORE,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
        );

    const uint32_t subpass =
        RenderPassHelper::addSubpass(
            description
        );

    RenderPassHelper::addColorAttachment(
        description,
        subpass,
        position
    );

    RenderPassHelper::addColorAttachment(
        description,
        subpass,
        normal
    );

    RenderPassHelper::addColorAttachment(
        description,
        subpass,
        albedo
    );

    RenderPassHelper::addColorAttachment(
        description,
        subpass,
        material
    );

    RenderPassHelper::setDepthAttachment(
        description,
        subpass,
        depth
    );

    RenderPassHelper::addExternalDependency(
        description,
        subpass
    );
}