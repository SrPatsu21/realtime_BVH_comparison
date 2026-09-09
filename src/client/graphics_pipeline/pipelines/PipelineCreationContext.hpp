#pragma once

#include "../../CoreVulkan.hpp"

struct PipelineCreationContext
{
    VkDevice device;
    VkRenderPass gBufferRenderPass;
    VkRenderPass lightRenderPass;
    VkRenderPass compositeRenderPass;

    VkDescriptorSetLayout globalLayout;
    VkDescriptorSetLayout materialLayout;
    VkDescriptorSetLayout particleLayout;
    VkDescriptorSetLayout instanceLayout;

    VkDescriptorSetLayout gBufferLayout;
    VkDescriptorSetLayout transparentGBufferLayout;
    VkDescriptorSetLayout lightingLayout;
    VkDescriptorSetLayout deferredLightingLayout;

    VkSampleCountFlagBits msaa;

    VkPhysicalDeviceVulkan12Features supportedFeatures12;

    const Config::ConfigTable* config;
};
