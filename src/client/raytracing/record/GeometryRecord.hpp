#pragma once

#include <vulkan/vulkan.h>
#include <cstdint>

#include "../../graphics_pipeline/GraphicsPipelineManager.hpp"
#include "../../batch/RenderInstanceManager.hpp"
#include "../../batch/RenderBatch.hpp"
#include "../../batch/instance/InstanceDescriptorManager.hpp"

class GeometryRecord
{
public:

    static void record(
        VkCommandBuffer cmd,

        GraphicsPipelineManager* graphicsPipeline,

        VkDescriptorSet globalSet,
        VkDescriptorSet instanceSet,

        RenderInstanceManager* renderInstanceManager,

        uint32_t firstBatch,
        uint32_t lastBatch,
        uint32_t firstInstanceOffset
    );
};