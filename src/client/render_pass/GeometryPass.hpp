#pragma once

#include "../CoreVulkan.hpp"
#include "../graphics_pipeline/GraphicsPipeline.hpp"
#include "../graphics_pipeline/GlobalDescriptorManager.hpp"
#include "../batch/RenderInstanceManager.hpp"
#include "../batch/instance/InstanceDescriptorManager.hpp"

// TODO transform into a real flex G-buffer

class GeometryPass
{
public:

    static void record(
        VkCommandBuffer cmd,
        uint32_t currentFrame,
        GraphicsPipeline* graphicsPipeline,
        VkDescriptorSet globalSet,
        InstanceDescriptorManager* instanceDescriptorManager,
        RenderInstanceManager* renderInstanceManager
    );
};