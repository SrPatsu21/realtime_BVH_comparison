#pragma once

#include <vulkan/vulkan.h>

class DeferredLightingBuffer
{
private:

    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;

    VkExtent2D extent{};
    VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;

public:

    void create(
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkExtent2D extent,
        VkSampleCountFlagBits samples
    );

    void destroy(VkDevice device);

    VkImage getImage() const { return image; };
    VkImageView getView() const { return view; };
    VkSampler getSample() const { return sampler; };

    VkExtent2D getExtent() const { return extent; };
    VkSampleCountFlagBits getSamples() const { return samples; };
};