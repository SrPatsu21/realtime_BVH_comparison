#pragma once

#include "../../CoreVulkan.hpp"

struct PipelineCreationContext
{
    VkDevice device;
    VkRenderPass gBufferRenderPass;
    VkRenderPass lightRenderPass;

    VkDescriptorSetLayout globalLayout;
    VkDescriptorSetLayout materialLayout;
    VkDescriptorSetLayout particleLayout;
    VkDescriptorSetLayout instanceLayout;

    VkDescriptorSetLayout gBufferLayout;
    VkDescriptorSetLayout transparentGBufferLayout;
    VkDescriptorSetLayout lightingLayout;

    VkSampleCountFlagBits msaa;

    VkPhysicalDeviceVulkan12Features supportedFeatures12;

    const Config::ConfigTable* config;
};
