#include "FramebufferManager.hpp"

#include <cassert>
#include <stdexcept>

FramebufferManager::FramebufferManager(
    VkDevice device,
    VkRenderPass renderPass,
    std::size_t swapchainImageViewsSize,
    VkExtent2D swapchainExtent,
    std::vector<std::vector<VkImageView>>& attachmentsVector
)
    : device(device)
{
    #ifndef NDEBUG
        if (device == VK_NULL_HANDLE)
        {
            throw std::runtime_error(
                "FramebufferManager: invalid Vulkan device"
            );
        }

        if (renderPass == VK_NULL_HANDLE)
        {
            throw std::runtime_error(
                "FramebufferManager: invalid render pass"
            );
        }

        if (swapchainImageViewsSize)
        {
            throw std::runtime_error(
                "FramebufferManager: no swapchain image views"
            );
        }

        if (swapchainExtent.width == 0 ||
            swapchainExtent.height == 0)
        {
            throw std::runtime_error(
                "FramebufferManager: invalid framebuffer extent"
            );
        }
    #endif

    framebuffers.resize(swapchainImageViewsSize);

    for (size_t i = 0; i < swapchainImageViewsSize; ++i)
    {
        std::vector<VkImageView> attachments = attachmentsVector[i];

        VkFramebufferCreateInfo framebufferInfo{};

        framebufferInfo.sType =VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = swapchainExtent.width;
        framebufferInfo.height = swapchainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(
                device,
                &framebufferInfo,
                nullptr,
                &framebuffers[i]
            ) != VK_SUCCESS)
        {
            throw std::runtime_error(
                "FramebufferManager: "
                "failed to create framebuffer"
            );
        }
    }
}

// ================================================================
// Destructor
// ================================================================

FramebufferManager::~FramebufferManager()
{
    for (VkFramebuffer& framebuffer : framebuffers)
    {
        if (framebuffer != VK_NULL_HANDLE)
        {
            vkDestroyFramebuffer(
                device,
                framebuffer,
                nullptr
            );

            framebuffer = VK_NULL_HANDLE;
        }
    }

    framebuffers.clear();
}