#pragma once

#include "../CoreVulkan.hpp"
#include "../BufferManager.hpp"

/**
 * @brief Creates a 2D Vulkan image and allocates & binds its memory.
 *
 * This is a low-level utility function that:
 * - Creates a VkImage
 * - Allocates device memory for it
 * - Binds the memory to the image
 *
 * The image is created with an initial layout of VK_IMAGE_LAYOUT_UNDEFINED.
 * Layout transitions and usage synchronization are responsibility of the caller.
 *
 * Typical use cases:
 * - Color attachments
 * - Depth/stencil attachments
 * - Textures (before layout transitions)
 *
 * @param physicalDevice Physical device used for memory type selection.
 * @param device Logical Vulkan device.
 * @param width Image width in pixels.
 * @param height Image height in pixels.
 * @param mipLevels Number of mipmap levels.
 * @param numSamples Sample count (for MSAA images).
 * @param format Image format.
 * @param tiling Image tiling mode.
 * @param usage Usage flags describing how the image will be used.
 * @param properties Memory property flags (e.g. DEVICE_LOCAL).
 * @param image (out) Created VkImage handle.
 * @param imageMemory (out) Allocated VkDeviceMemory bound to the image.
 *
 * @throws std::runtime_error if image or memory allocation fails.
 */
void createImage(
    VkPhysicalDevice physicalDevice,
    VkDevice device,
    uint32_t width,
    uint32_t height,
    uint32_t mipLevels,
    VkSampleCountFlagBits numSamples,
    VkFormat format,
    VkImageTiling tiling,
    VkImageUsageFlags usage,
    VkMemoryPropertyFlags properties,
    VkImage& image,
    VkDeviceMemory& imageMemory
);

/**
 * @brief Creates a 2D image view for a Vulkan image.
 *
 * Image views describe how an image is accessed by the pipeline.
 * This function creates a basic 2D view covering all specified mip levels.
 *
 * Typical use cases:
 * - Color attachments
 * - Depth attachments
 * - Sampled textures
 *
 * @param device Logical Vulkan device.
 * @param image Image handle to create a view for.
 * @param format Image format.
 * @param aspectFlags Aspect mask (e.g. COLOR, DEPTH, STENCIL).
 * @param mipLevels Number of mip levels covered by the view.
 *
 * @return VkImageView The created image view.
 *
 * @throws std::runtime_error if image view creation fails.
 */
VkImageView createImageView(
    VkDevice device,
    VkImage image,
    VkFormat format,
    VkImageAspectFlags aspectFlags,
    uint32_t mipLevels
);

/**
 * @brief Generates mipmaps for a 2D image using GPU blitting.
 *
 * This function records and submits commands that:
 * - Transition mip levels between transfer and shader-read layouts
 * - Perform linear blits from higher-resolution mip levels to lower ones
 * - Transition all mip levels to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
 *
 * Requirements and assumptions:
 * - The image must have been created with:
 *     - VK_IMAGE_USAGE_TRANSFER_SRC_BIT
 *     - VK_IMAGE_USAGE_TRANSFER_DST_BIT
 * - The image format must support linear filtering for blitting
 * - The base mip level (level 0) must already be in
 *   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
 *
 * Synchronization:
 * - Uses BufferManager immediate submission
 * - Fully completes before returning
 *
 * @param physicalDevice Physical device used for format capability queries.
 * @param bufferManager BufferManager used for immediate command submission.
 * @param image Image to generate mipmaps for.
 * @param imageFormat Format of the image.
 * @param texWidth Width of the base mip level.
 * @param texHeight Height of the base mip level.
 * @param mipLevels Total number of mip levels.
 *
 * @throws std::runtime_error if the image format does not support linear blitting.
 */
void generateMipmaps(
    VkPhysicalDevice physicalDevice,
    BufferManager* bufferManager,
    VkImage image,
    VkFormat imageFormat,
    int32_t texWidth,
    int32_t texHeight,
    uint32_t mipLevels
);
