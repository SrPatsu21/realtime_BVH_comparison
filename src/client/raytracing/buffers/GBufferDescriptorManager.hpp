#pragma once

#include "../../CoreVulkan.hpp"
#include "GBuffer.hpp"

class GBufferDescriptorManager
{
private:

    VkDevice device = VK_NULL_HANDLE;

    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

public:

    GBufferDescriptorManager(
        VkDevice device,
        GBuffer* gbuffer
    );

    ~GBufferDescriptorManager();

    VkDescriptorSetLayout getLayout() const noexcept
    {
        return descriptorSetLayout;
    }

    VkDescriptorPool getPool() const noexcept
    {
        return descriptorPool;
    }

    VkDescriptorSet getDescriptorSet() const noexcept
    {
        return descriptorSet;
    }
};