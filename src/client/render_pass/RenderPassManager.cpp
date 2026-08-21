#include "RenderPassManager.hpp"

#include "RenderPassHelper.hpp"

#include <stdexcept>

// ================================================================
// AttachmentDesc
// ================================================================

RenderPassManager::AttachmentDesc::AttachmentDesc(
    VkFormat format,
    VkSampleCountFlagBits samples,
    VkAttachmentLoadOp loadOp,
    VkAttachmentStoreOp storeOp,
    VkImageLayout initialLayout,
    VkImageLayout finalLayout
) :
    format(format),
    samples(samples),
    loadOp(loadOp),
    storeOp(storeOp),
    initialLayout(initialLayout),
    finalLayout(finalLayout)
{
}


// ================================================================
// RenderPassManager
// ================================================================

RenderPassManager::RenderPassManager(
    VkDevice device,
    Description description
)
    : device(device)
{
    if (device == VK_NULL_HANDLE)
    {
        throw std::runtime_error(
            "Cannot create RenderPassManager "
            "with null VkDevice"
        );
    }

    // ------------------------------------------------------------
    // Validation
    // ------------------------------------------------------------

    RenderPassHelper::validate(
        description
    );

    // ------------------------------------------------------------
    // Vulkan render pass
    // ------------------------------------------------------------

    renderPass =
        RenderPassHelper::create(
            device,
            description
        );
}


RenderPassManager::~RenderPassManager()
{
    if (renderPass != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(
            device,
            renderPass,
            nullptr
        );

        renderPass = VK_NULL_HANDLE;
    }
}