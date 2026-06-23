#include "ImageColor.hpp"

ImageColor::ImageColor(
    VkPhysicalDevice physicalDevice,
    VkDevice device,
    VkFormat swapchainImageFormat,
    VkExtent2D swapchainExtent,
    VkSampleCountFlagBits msaaSamples
)
: device(device)
{
    createImage(
        physicalDevice,
        device,
        swapchainExtent.width,
        swapchainExtent.height,
        1,
        msaaSamples,
        swapchainImageFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        colorImage,
        colorImageMemory
    );
    colorImageView = createImageView(
        device,
        colorImage,
        swapchainImageFormat,
        VK_IMAGE_ASPECT_COLOR_BIT,
        1
    );
}

ImageColor::~ImageColor()
{
    vkDestroyImageView(device, colorImageView, nullptr);
    vkDestroyImage(device, colorImage, nullptr);
    vkFreeMemory(device, colorImageMemory, nullptr);
}