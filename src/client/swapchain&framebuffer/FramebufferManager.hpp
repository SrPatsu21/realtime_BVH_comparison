#pragma once

#include "../CoreVulkan.hpp"
#include "SwapchainManager.hpp"
#include "DepthBufferManager.hpp"
#include <cstring>

/**
 * @brief Manages framebuffer creation for each swapchain image.
 *
 * FramebufferManager creates one VkFramebuffer per swapchain image,
 * binding together:
 * - The multisampled color attachment
 * - The depth (and optional stencil) attachment
 * - The resolve / presentation swapchain image
 *
 * The framebuffer configuration must exactly match the attachments
 * declared in the associated render pass.
 *
 * This class owns all created framebuffers and is responsible for
 * their lifetime.
 */
class FramebufferManager
{
private:
    VkDevice device;
    std::vector<VkFramebuffer> swapchainFramebuffers;

public:
    /**
     * @brief Creates a framebuffer for each swapchain image.
     *
     * Attachment order must match the render pass attachment order:
     * 0 - Multisampled color attachment
     * 1 - Depth (or depth-stencil) attachment
     * 2 - Resolve / swapchain image
     *
     * @param device Logical Vulkan device.
     * @param renderPass Render pass compatible with the attachments.
     * @param swapchainImageViews Image views of the swapchain images.
     * @param colorImageView Multisampled color image view.
     * @param depthImageView Depth (or depth-stencil) image view.
     * @param swapChainExtent Framebuffer dimensions.
     */
    FramebufferManager(
        VkDevice device,
        VkRenderPass renderPass,
        std::vector<VkImageView> swapchainImageViews,
        const VkImageView colorImageView,
        const VkImageView depthImageView,
        const VkExtent2D swapChainExtent,
        bool useMSAA
    );

    /**
     * @brief Destroys all framebuffers.
     */
    ~FramebufferManager();

    const std::vector<VkFramebuffer>& getFramebuffers() const { return this->swapchainFramebuffers; }
};