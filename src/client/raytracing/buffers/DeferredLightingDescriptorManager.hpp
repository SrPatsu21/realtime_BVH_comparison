#pragma once

#include <vulkan/vulkan.h>
#include "DeferredLightingBuffer.hpp"

class DeferredLightingDescriptorManager
{
public:

    DeferredLightingDescriptorManager(
        VkDevice device,
        DeferredLightingBuffer* deferredLighting
    );

    ~DeferredLightingDescriptorManager();

    VkDescriptorSetLayout getLayout() const
    {
        return descriptorSetLayout;
    }

    VkDescriptorSet getDescriptorSet() const
    {
        return descriptorSet;
    }

private:

    VkDevice device{};

    VkDescriptorSetLayout descriptorSetLayout{};
    VkDescriptorPool descriptorPool{};
    VkDescriptorSet descriptorSet{};
};