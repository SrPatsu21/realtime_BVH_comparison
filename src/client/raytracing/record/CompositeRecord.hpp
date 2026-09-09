#pragma once

#include "../../CoreVulkan.hpp"
#include "../../graphics_pipeline/GraphicsPipelineManager.hpp"

class CompositeRecord
{
public:

    static void record(
        VkCommandBuffer cmd,
        GraphicsPipelineManager* graphicsPipeline,
        VkDescriptorSet deferredLightingSet
    );
};