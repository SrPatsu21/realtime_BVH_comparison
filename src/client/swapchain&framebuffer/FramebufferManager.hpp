#pragma once

#include "../CoreVulkan.hpp"
#include "../ConfigTable.hpp"

class FramebufferManager
{

private:

    VkDevice device = VK_NULL_HANDLE;

    std::vector<VkFramebuffer> framebuffers;

public:

    FramebufferManager(
        VkDevice device,
        VkRenderPass renderPass,
        std::size_t swapchainImageViewsSize,
        VkExtent2D swapchainExtent,
        std::vector<std::vector<VkImageView>>& attachmentsVector
    );

    ~FramebufferManager();

    FramebufferManager(const FramebufferManager&) = delete;
    FramebufferManager& operator=(const FramebufferManager&) = delete;

    VkFramebuffer get(size_t index) const noexcept
    {
        return framebuffers[index];
    }

    const std::vector<VkFramebuffer>& getFramebuffers() const noexcept
    {
        return framebuffers;
    }
};