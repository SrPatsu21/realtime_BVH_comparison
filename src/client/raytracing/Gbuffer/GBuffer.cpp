#include "GBuffer.hpp"
#include "../../image/VulkanImageUtils.hpp"

void GBuffer::create(
    VkDevice device,
    VkPhysicalDevice physicalDevice,
    VkExtent2D extent,
    VkSampleCountFlagBits samples
)
{
    this->extent = extent;
    this->samples = samples;

    createAttachment(
        device,
        physicalDevice,
        position,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
        VK_IMAGE_USAGE_SAMPLED_BIT |
        VK_IMAGE_USAGE_STORAGE_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT
    );

    createAttachment(
        device,
        physicalDevice,
        normal,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
        VK_IMAGE_USAGE_SAMPLED_BIT |
        VK_IMAGE_USAGE_STORAGE_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT
    );

    createAttachment(
        device,
        physicalDevice,
        albedo,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
        VK_IMAGE_USAGE_SAMPLED_BIT |
        VK_IMAGE_USAGE_STORAGE_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT
    );

    createAttachment(
        device,
        physicalDevice,
        material,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
        VK_IMAGE_USAGE_SAMPLED_BIT |
        VK_IMAGE_USAGE_STORAGE_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT
    );

    createAttachment(
        device,
        physicalDevice,
        depth,
        VK_FORMAT_D32_SFLOAT,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
        VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_DEPTH_BIT
    );
}

void GBuffer::createAttachment(
    VkDevice device,
    VkPhysicalDevice physicalDevice,
    Image& attachment,
    VkFormat format,
    VkImageUsageFlags usage,
    VkImageAspectFlags aspect
)
{
    attachment.format = format;
    attachment.aspect = aspect;

    createImage(
        physicalDevice,
        device,
        extent.width,
        extent.height,
        1,
        samples,
        format,
        VK_IMAGE_TILING_OPTIMAL,
        usage,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        attachment.image,
        attachment.memory
    );

    attachment.view = createImageView(
        device,
        attachment.image,
        attachment.format,
        attachment.aspect,
        1
    );
}

void GBuffer::destroy(VkDevice device)
{
    destroyAttachment(device, position);
    destroyAttachment(device, normal);
    destroyAttachment(device, albedo);
    destroyAttachment(device, material);
    destroyAttachment(device, depth);
}

void GBuffer::destroyAttachment(
    VkDevice device,
    Image& attachment
)
{
    if (attachment.view != VK_NULL_HANDLE) {
        vkDestroyImageView(device, attachment.view, nullptr);
        attachment.view = VK_NULL_HANDLE;
    }

    if (attachment.image != VK_NULL_HANDLE) {
        vkDestroyImage(device, attachment.image, nullptr);
        attachment.image = VK_NULL_HANDLE;
    }

    if (attachment.memory != VK_NULL_HANDLE) {
        vkFreeMemory(device, attachment.memory, nullptr);
        attachment.memory = VK_NULL_HANDLE;
    }
}

VkImage GBuffer::getImage(Attachment attachment) const
{
    switch (attachment)
    {
        case Attachment::Position: return position.image;
        case Attachment::Normal:   return normal.image;
        case Attachment::Albedo:   return albedo.image;
        case Attachment::Material: return material.image;
        case Attachment::Depth:    return depth.image;
    }

    return VK_NULL_HANDLE;
}

VkImageView GBuffer::getView(Attachment attachment) const
{
    switch (attachment)
    {
        case Attachment::Position: return position.view;
        case Attachment::Normal:   return normal.view;
        case Attachment::Albedo:   return albedo.view;
        case Attachment::Material: return material.view;
        case Attachment::Depth:    return depth.view;
    }

    return VK_NULL_HANDLE;
}

VkFormat GBuffer::getFormat(Attachment attachment) const
{
    switch (attachment)
    {
        case Attachment::Position: return position.format;
        case Attachment::Normal:   return normal.format;
        case Attachment::Albedo:   return albedo.format;
        case Attachment::Material: return material.format;
        case Attachment::Depth:    return depth.format;
    }

    return VK_FORMAT_UNDEFINED;
}

VkExtent2D GBuffer::getExtent() const
{
    return extent;
}

VkSampleCountFlagBits GBuffer::getSamples() const
{
    return samples;
}

std::array<VkImageView, 4> GBuffer::getColorViews() const
{
    return {
        position.view,
        normal.view,
        albedo.view,
        material.view
    };
}

VkImageView GBuffer::getDepthView() const
{
    return depth.view;
}