#pragma once

#include "../../CoreVulkan.hpp"
#include "TransparentGBuffer.hpp"

class TransparentGBufferDescriptorManager
{
private:

    VkDevice device = VK_NULL_HANDLE;

    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

public:

    TransparentGBufferDescriptorManager(
        VkDevice device,
        TransparentGBuffer* transparentGBuffer
    );

    ~TransparentGBufferDescriptorManager();

    VkDescriptorSetLayout getLayout() const noexcept
    {
        return descriptorSetLayout;
    }

    VkDescriptorSet getDescriptorSet() const noexcept
    {
        return descriptorSet;
    }
};