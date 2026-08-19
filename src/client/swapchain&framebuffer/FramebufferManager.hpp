#pragma once

#include "../CoreVulkan.hpp"
#include "../ConfigTable.hpp"

class FramebufferManager
{
public:

    struct ForwardAttachments
    {
        // MSAA color attachment.
        //
        // Required when msaaSamples != VK_SAMPLE_COUNT_1_BIT.
        // Ignored when msaaSamples == VK_SAMPLE_COUNT_1_BIT.
        VkImageView color = VK_NULL_HANDLE;

        // Depth attachment.
        VkImageView depth = VK_NULL_HANDLE;
    };

    struct GBufferAttachments
    {
        VkImageView position = VK_NULL_HANDLE;
        VkImageView albedo = VK_NULL_HANDLE;
        VkImageView normal = VK_NULL_HANDLE;
        VkImageView material = VK_NULL_HANDLE;
        VkImageView depth = VK_NULL_HANDLE;
    };

private:

    VkDevice device = VK_NULL_HANDLE;

    std::vector<VkFramebuffer> framebuffers;

public:

    FramebufferManager(
        VkDevice device,
        VkRenderPass renderPass,
        const std::vector<VkImageView>& swapchainImageViews,
        VkExtent2D swapchainExtent,
        const Config::ConfigTable& config,
        VkSampleCountFlagBits msaaSamples,
        const ForwardAttachments& forwardAttachments,
        const GBufferAttachments& gBufferAttachments
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

private:

    static std::vector<VkImageView> buildForwardAttachments(
        const std::vector<VkImageView>& swapchainImageViews,
        size_t index,
        const ForwardAttachments& attachments,
        VkSampleCountFlagBits msaaSamples
    );

    static std::vector<VkImageView> buildGBufferAttachments(
        const GBufferAttachments& attachments
    );

    static void validateForwardAttachments(
        const ForwardAttachments& attachments,
        VkSampleCountFlagBits msaaSamples
    );

    static void validateGBufferAttachments(
        const GBufferAttachments& attachments
    );
};