#pragma once

#include "../CoreVulkan.hpp"
#include "VulkanImageUtils.hpp"
#include "../BufferManager.hpp"

/**
 * @brief Manages an MSAA color attachment image.
 *
 * ImageColor encapsulates the creation and lifetime of a color image
 * used as a multisampled color attachment in a render pass.
 *
 * Typical usage:
 * - Bound as the color attachment in a render pass
 * - Resolved into a single-sampled swapchain image
 *
 * This image is:
 * - Device-local
 * - Transient (implementation may discard contents after use)
 * - Not directly sampled by shaders
 *
 * The class exists to keep MSAA attachment management isolated
 * from swapchain and render pass logic.
 */
class ImageColor
{
private:
    VkDevice device;
    VkImage colorImage;
    VkDeviceMemory colorImageMemory;
    VkImageView colorImageView;

public:
    /**
     * @brief Creates a multisampled color attachment image.
     *
     * The image is created with:
     * - The same format and extent as the swapchain
     * - The specified MSAA sample count
     * - TRANSIENT + COLOR_ATTACHMENT usage flags
     *
     * @param physicalDevice Physical device used for memory selection.
     * @param device Logical Vulkan device.
     * @param swapchainImageFormat Format of the swapchain images.
     * @param swapchainExtent Resolution of the swapchain.
     * @param msaaSamples Sample count for multisampling.
     */
    ImageColor(
        VkPhysicalDevice physicalDevice,
        VkDevice device,
        VkFormat swapchainImageFormat,
        VkExtent2D swapchainExtent,
        VkSampleCountFlagBits msaaSamples
    );

    const VkImage& getColorImage() const { return colorImage; }
    const VkDeviceMemory& getColorImageMemory() const { return colorImageMemory; }
    const VkImageView& getColorImageView() const { return colorImageView; }

    ~ImageColor();
};