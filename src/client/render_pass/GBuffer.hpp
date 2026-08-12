#pragma once

#include "../CoreVulkan.hpp"
#include <array>

class GBuffer
{
public:

    enum class Attachment
    {
        Position,
        Normal,
        Albedo,
        Material,
        Depth
    };

public:

    void create(
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkExtent2D extent,
        VkSampleCountFlagBits samples
    );

    void destroy(VkDevice device);

    VkImage getImage(Attachment attachment) const;
    VkImageView getView(Attachment attachment) const;
    VkFormat getFormat(Attachment attachment) const;

    VkExtent2D getExtent() const;
    VkSampleCountFlagBits getSamples() const;

    std::array<VkImageView, 4> getColorViews() const;
    VkImageView getDepthView() const;

private:

    struct Image
    {
        VkImage image = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;

        VkFormat format = VK_FORMAT_UNDEFINED;
        VkImageAspectFlags aspect = 0;
    };

    void createAttachment(
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        Image& attachment,
        VkFormat format,

        VkImageUsageFlags usage,
        VkImageAspectFlags aspect
    );

    void destroyAttachment(
        VkDevice device,
        Image& attachment
    );

private:

    Image position;
    Image normal;
    Image albedo;
    Image material;
    Image depth;

    VkExtent2D extent{};
    VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
};