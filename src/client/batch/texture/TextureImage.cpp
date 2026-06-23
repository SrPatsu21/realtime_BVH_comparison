#include "TextureImage.hpp"
#include "../../image/VulkanImageUtils.hpp"

void TextureImage::DefaultImageTransitionPolicy::transition(
    BufferManager* bufferManager,
    VkImage image,
    VkFormat format,
    VkImageLayout oldLayout,
    VkImageLayout newLayout,
    uint32_t mipLevels
) {
    VkCommandBuffer commandBuffer = bufferManager->beginImmediate();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;

    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    barrier.image = image;
    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        if (CoreVulkan::hasStencilComponent(format)) {
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    } else {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = 0;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;
    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    } else {
        throw std::invalid_argument("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage,
        destinationStage,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &barrier
    );

    bufferManager->endImmediate();
}

TextureImage::TextureImage(
    VkPhysicalDevice physicalDevice,
    VkDevice device,
    BufferManager* bufferManager,
    VkSampler sampler,
    const TextureAsset& asset,
    TextureImage::IImageTransitionPolicy* transitionPolicy
) :
    device(device),
    sampler(sampler)
{
    createImageFromAsset(
        physicalDevice,
        bufferManager,
        asset,
        transitionPolicy
    );

    VkImageAspectFlags aspect =
        CoreVulkan::hasDepthComponent(asset.getFormat())
        ? VK_IMAGE_ASPECT_DEPTH_BIT
        : VK_IMAGE_ASPECT_COLOR_BIT;

    createView(aspect);
}

TextureImage::TextureImage(
    VkDevice device,
    VkImage image,
    VkDeviceMemory memory,
    VkImageView view,
    VkSampler sampler,
    uint32_t mipLevels,
    uint32_t layers,
    VkFormat format
) :
    device(device),
    image(image),
    memory(memory),
    imageView(view),
    sampler(sampler),
    mipLevels(mipLevels),
    layers(layers),
    format(format)
{}

TextureImage::~TextureImage()
{
    if (imageView != VK_NULL_HANDLE)
        vkDestroyImageView(device, imageView, nullptr);

    if (image != VK_NULL_HANDLE)
        vkDestroyImage(device, image, nullptr);

    if (memory != VK_NULL_HANDLE)
        vkFreeMemory(device, memory, nullptr);
}

//todo Cubemap
void TextureImage::createImageFromAsset(
    VkPhysicalDevice physicalDevice,
    BufferManager* bufferManager,
    const TextureAsset& asset,
    TextureImage::IImageTransitionPolicy* transitionPolicy
)
{
    mipLevels = asset.getMipLevels();
    format = asset.getFormat();

    uint32_t faceCount  = asset.getFaces();
    uint32_t layerCount = asset.getLayers();
    bool isCube = (faceCount == 6);
    layers = layerCount * faceCount;

    VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

    if (mipLevels > 1)
        usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

    createImage(
        physicalDevice,
        device,
        asset.getWidth(),
        asset.getHeight(),
        mipLevels,
        VK_SAMPLE_COUNT_1_BIT,
        format,
        VK_IMAGE_TILING_OPTIMAL,
        usage,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        image,
        memory
    );

    // TRANSITION → TRANSFER_DST
    transitionPolicy->transition(
        bufferManager,
        image,
        format,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        mipLevels
    );

    // Upload mip levels
    const auto& mips = asset.getMipData();
    for (uint32_t level = 0; level < mipLevels; ++level)
    {
        const auto& mip = mips[level];
        for (uint32_t i = 0; i < mip.subresources.size(); ++i)
        {
            uint32_t layer = i / asset.getFaces();
            uint32_t face  = i % asset.getFaces();

            bufferManager->uploadToImageMipLevel(
                mip.subresources[i].data,
                mip.subresources[i].size,
                image,
                mip.width,
                mip.height,
                level,
                layer * asset.getFaces() + face,
                1
            );
        }
    }

    // TRANSITION → SHADER_READ
    transitionPolicy->transition(
        bufferManager,
        image,
        format,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        mipLevels
    );
}

void TextureImage::createView(
    VkImageAspectFlags aspectFlags
)
{
    imageView = createImageView(
        device,
        image,
        format,
        aspectFlags,
        mipLevels
    );
}

std::shared_ptr<TextureImage> TextureFactory::createSolidRGBA8(
    VkPhysicalDevice physicalDevice,
    VkDevice device,
    BufferManager* bufferManager,
    SamplerManager* samplerManager,
    VkFormat format,
    uint8_t r,
    uint8_t g,
    uint8_t b,
    uint8_t a
)
{
    uint32_t width = 1;
    uint32_t height = 1;
    uint32_t mipLevels = 1;

    uint8_t pixel[4] = { r, g, b, a };

    VkImage image;
    VkDeviceMemory memory;

    createImage(
        physicalDevice,
        device,
        width,
        height,
        mipLevels,
        VK_SAMPLE_COUNT_1_BIT,
        format,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        image,
        memory
    );

    // TRANSITION: UNDEFINED → TRANSFER_DST
    {
        VkCommandBuffer cmd = bufferManager->beginImmediate();

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        vkCmdPipelineBarrier(
            cmd,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        bufferManager->endImmediate();
    }

    // Upload pixel
    bufferManager->uploadToImageMipLevel(
        pixel,
        4,
        image,
        width,
        height,
        0,
        0,
        1
    );

    // ---- TRANSITION: TRANSFER_DST → SHADER_READ_ONLY ----
    {
        VkCommandBuffer cmd = bufferManager->beginImmediate();

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(
            cmd,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        bufferManager->endImmediate();
    }

    VkImageView view = createImageView(
        device,
        image,
        format,
        VK_IMAGE_ASPECT_COLOR_BIT,
        1
    );

    VkSampler sampler = samplerManager->getSampler(SamplerManager::LinearRepeat);

    return std::make_shared<TextureImage>(
        device,
        image,
        memory,
        view,
        sampler,
        mipLevels,
        1,
        format
    );
}