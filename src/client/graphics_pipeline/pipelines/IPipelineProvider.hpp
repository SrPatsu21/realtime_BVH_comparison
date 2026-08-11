#pragma once
#include "../../CoreVulkan.hpp"
#include "../GraphicsPipelineManager.hpp"

struct PipelineCreationContext
{
    VkDevice device;
    VkRenderPass renderPass;

    VkDescriptorSetLayout globalLayout;
    VkDescriptorSetLayout materialLayout;
    VkDescriptorSetLayout particleLayout;
    VkDescriptorSetLayout instanceLayout;

    VkSampleCountFlagBits msaa;
    VkPhysicalDeviceVulkan12Features supportedFeatures12;
};

struct IPipelineProvider
{
    virtual ~IPipelineProvider() = default;

    virtual void createPipelines(
        GraphicsPipelineManager& manager,
        const PipelineCreationContext& ctx
    ) = 0;
};