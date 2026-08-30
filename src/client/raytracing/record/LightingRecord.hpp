#pragma once

#include "../../CoreVulkan.hpp"
#include "../../graphics_pipeline/GraphicsPipelineManager.hpp"

class LightingRecord
{
public:

    static void record(
        VkCommandBuffer cmd,
        GraphicsPipelineManager* graphicsPipeline,
        VkDescriptorSet globalSet,
        VkDescriptorSet gBufferSet,
        VkDescriptorSet lightSet,
        const Config::ConfigTable& config
    );
};