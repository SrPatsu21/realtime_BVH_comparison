#pragma once

#include "../CoreVulkan.hpp"
#include "../graphics_pipeline/GraphicsPipelineManager.hpp"

class LightingPass
{
public:

    static void record(
        VkCommandBuffer cmd,
        GraphicsPipelineManager* graphicsPipeline
    );
};