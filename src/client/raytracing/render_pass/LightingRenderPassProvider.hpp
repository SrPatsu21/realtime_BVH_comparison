#pragma once

#include "../../CoreVulkan.hpp"
#include "../../render_pass/RenderPassManager.hpp"

class LightingRenderPassProvider
{
public:
    static void build(
        RenderPassManager::Description& description,
        VkFormat swapchainImageFormat
    );
};