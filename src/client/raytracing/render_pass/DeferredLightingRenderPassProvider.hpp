#pragma once

#include "../../CoreVulkan.hpp"
#include "../../render_pass/RenderPassManager.hpp"

class DeferredLightingRenderPassProvider
{
public:
    static void build(
        RenderPassManager::Description& description,
        VkSampleCountFlagBits msaaSamples
    );
};