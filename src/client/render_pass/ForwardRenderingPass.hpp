#pragma once

#include "../CoreVulkan.hpp"
#include "../graphics_pipeline/GraphicsPipelineManager.hpp"
#include "../graphics_pipeline/GlobalDescriptorManager.hpp"
#include "../batch/RenderInstanceManager.hpp"
#include "../batch/instance/InstanceDescriptorManager.hpp"

class ForwardRenderingPass
{
public:

    static void record(
        VkCommandBuffer cmd,
        uint32_t currentFrame,
        GraphicsPipelineManager* graphicsPipeline,
        VkDescriptorSet globalSet,
        InstanceDescriptorManager* instanceDescriptorManager,
        RenderInstanceManager* renderInstanceManager
    );
};

