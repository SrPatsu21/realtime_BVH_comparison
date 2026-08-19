#pragma once

#include "../CoreVulkan.hpp"

/**
 * @brief Manages the depth (and optional stencil) attachment image.
 *
 * DepthBufferManager encapsulates the creation and lifetime of the
 * depth/stencil image used by the render pass.
 *
 * The depth buffer:
 * - Matches the swapchain extent
 * - Supports optional MSAA
 * - Is allocated in device-local memory
 *
 * This class intentionally hides Vulkan image creation details,
 * exposing only the resources required for framebuffer construction.
 */
class DepthBufferManager
{
private:
    VkDevice device;
    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;

public:
    /**
     * @brief Creates the depth (and optional stencil) buffer.
     *
     * The image is created with:
     * - The same extent as the swapchain
     * - The specified MSAA sample count
     * - VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
     *
     * If the chosen depth format contains a stencil component,
     * VK_IMAGE_ASPECT_STENCIL_BIT is automatically added to the
     * image view aspect mask.
     *
     * @param physicalDevice Physical device used for memory selection.
     * @param device Logical Vulkan device.
     * @param swapchainExtent Resolution of the swapchain.
     * @param msaaSamples Sample count for depth MSAA.
     * @param depthFormat Depth (or depth-stencil) image format.
     * @param aspect Initial aspect mask (typically DEPTH_BIT).
     */
    DepthBufferManager(
        VkPhysicalDevice physicalDevice,
        VkDevice device,
        VkExtent2D swapchainExtent,
        VkSampleCountFlagBits msaaSamples,
        VkFormat depthFormat,
        VkImageAspectFlags aspect
    );

    /**
     * @brief Destroys the depth image, image view and frees memory.
     */
    ~DepthBufferManager();

    VkImage getDepthImage() const { return depthImage; }
    VkDeviceMemory getDepthImageMemory() const { return depthImageMemory; }
    VkImageView getDepthImageView() const { return depthImageView; }
};