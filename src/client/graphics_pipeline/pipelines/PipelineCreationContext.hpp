#pragma once

#include "../../CoreVulkan.hpp"

struct PipelineCreationContext
{
    VkDevice device;
    VkRenderPass renderPass;

    VkDescriptorSetLayout globalLayout;
    VkDescriptorSetLayout materialLayout;
    VkDescriptorSetLayout particleLayout;
    VkDescriptorSetLayout instanceLayout;

    VkDescriptorSetLayout gbufferLayout;
    VkDescriptorSetLayout lightingLayout;

    VkSampleCountFlagBits msaa;

    VkPhysicalDeviceVulkan12Features supportedFeatures12;

    const Config::ConfigTable* config;
};
