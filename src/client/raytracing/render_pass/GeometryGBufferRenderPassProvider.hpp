#pragma once

#include "../../CoreVulkan.hpp"
#include "../../render_pass/RenderPassManager.hpp"

class GeometryGBufferRenderPassProvider
{
public:

    static RenderPassManager::Description build(
        VkSampleCountFlagBits msaaSamples,
        VkFormat depthFormat
    );
};