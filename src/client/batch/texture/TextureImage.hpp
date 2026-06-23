#pragma once

#include "../../CoreVulkan.hpp"
#include "../../BufferManager.hpp"
#include "TextureAsset.hpp"
#include <memory>

class TextureImage
{
public:
    class IImageTransitionPolicy {
    public:
        virtual ~IImageTransitionPolicy() = default;

        /**
         * @brief Performs a layout transition for a Vulkan image.
         *
         * @param bufferManager Command submission helper.
         * @param image Image to transition.
         * @param format Image format.
         * @param oldLayout Current image layout.
         * @param newLayout Target image layout.
         * @param mipLevels Number of mip levels affected.
         */
        virtual void transition(
            BufferManager* bufferManager,
            VkImage image,
            VkFormat format,
            VkImageLayout oldLayout,
            VkImageLayout newLayout,
            uint32_t mipLevels
        ) = 0;
    };

    /**
     * @brief Default image layout transition implementation.
     *
     * Supports the following transitions:
     * - UNDEFINED -> TRANSFER_DST_OPTIMAL
     * - TRANSFER_DST_OPTIMAL -> SHADER_READ_ONLY_OPTIMAL
     * - UNDEFINED -> DEPTH_STENCIL_ATTACHMENT_OPTIMAL
     *
     * Uses immediate command buffers for simplicity.
     */
    class DefaultImageTransitionPolicy : public IImageTransitionPolicy{
    public:
        static DefaultImageTransitionPolicy& instance() {
            static DefaultImageTransitionPolicy policy;
            return policy;
        }

        void transition(
            BufferManager* bufferManager,
            VkImage image,
            VkFormat format,
            VkImageLayout oldLayout,
            VkImageLayout newLayout,
            uint32_t mipLevels
        ) override;

    private:
        DefaultImageTransitionPolicy() = default;
    };

    struct DefaultTextures
    {
        std::shared_ptr<TextureImage> white;
        std::shared_ptr<TextureImage> normal;
        std::shared_ptr<TextureImage> metallic;
    };

private:
    VkDevice device{VK_NULL_HANDLE};

    VkImage image{VK_NULL_HANDLE};
    VkDeviceMemory memory{VK_NULL_HANDLE};
    VkImageView imageView{VK_NULL_HANDLE};
    VkSampler sampler{VK_NULL_HANDLE};

    uint32_t mipLevels{0};
    uint32_t layers{1};
    VkFormat format{VK_FORMAT_UNDEFINED};

    /**
     * @brief Creates a Vulkan image from a TextureAsset and uploads its data.
     *
     * This function:
     * - Creates a VkImage with appropriate usage flags
     * - Allocates device-local memory
     * - Transitions the image to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
     * - Uploads all mip levels and subresources
     * - Transitions the image to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
     *
     * Supports layered textures and cubemaps.
     *
     * @param physicalDevice Physical device used for memory allocation.
     * @param bufferManager  Buffer manager responsible for staging uploads.
     * @param asset          Texture asset containing image data and metadata.
     * @param transitionPolicy Strategy object used to perform layout transitions.
     */
    void createImageFromAsset(
        VkPhysicalDevice physicalDevice,
        BufferManager* bufferManager,
        const TextureAsset& asset,
        TextureImage::IImageTransitionPolicy* transitionPolicy
    );


    /**
     * @brief Creates the VkImageView for the texture.
     *
     * The aspect flags determine whether the image is treated as a
     * color or depth texture.
     *
     * @param aspectFlags VkImageAspectFlags (e.g. VK_IMAGE_ASPECT_COLOR_BIT
     *                    or VK_IMAGE_ASPECT_DEPTH_BIT).
     */
    void createView(
        VkImageAspectFlags aspectFlags
    );

public:
    /**
     * @brief Constructs a TextureImage from a TextureAsset.
     *
     * Creates the Vulkan image, uploads all mip levels, performs
     * necessary layout transitions, and creates the image view.
     *
     * The final layout of the image will be:
     * VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
     *
     * @param physicalDevice Vulkan physical device.
     * @param device         Logical Vulkan device.
     * @param bufferManager  Buffer manager used for staging and transfers.
     * @param sampler        Sampler associated with this texture.
     * @param asset          Texture asset providing image data.
     * @param transitionPolicy Policy used for image layout transitions.
     */
    TextureImage(
        VkPhysicalDevice physicalDevice,
        VkDevice device,
        BufferManager* bufferManager,
        VkSampler sampler,
        const TextureAsset& asset,
        TextureImage::IImageTransitionPolicy* transitionPolicy
    );

    TextureImage(
        VkDevice device,
        VkImage image,
        VkDeviceMemory memory,
        VkImageView view,
        VkSampler sampler,
        uint32_t mipLevels,
        uint32_t layers,
        VkFormat format
    );


    /**
     * @brief Destroys the texture image and releases Vulkan resources.
     *
     * Frees:
     * - VkImageView
     * - VkImage
     * - Allocated VkDeviceMemory
     *
     * Safe to call even if handles are VK_NULL_HANDLE.
     */
    ~TextureImage();

    VkImageView getImageView() const { return imageView; }
    VkImage getImage() const { return image; }
    uint32_t getMipLevels() const { return mipLevels; }
    VkFormat getFormat() const { return format; }
    VkSampler getSampler() const { return sampler; }
};

class TextureFactory
{
public:
    static std::shared_ptr<TextureImage> createSolidRGBA8(
        VkPhysicalDevice physicalDevice,
        VkDevice device,
        BufferManager* bufferManager,
        SamplerManager* samplerManager,
        VkFormat format,
        uint8_t r,
        uint8_t g,
        uint8_t b,
        uint8_t a
    );
};