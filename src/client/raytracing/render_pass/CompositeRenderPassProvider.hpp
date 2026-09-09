#pragma once

#include "../../CoreVulkan.hpp"
#include "../../render_pass/RenderPassManager.hpp"

class CompositeRenderPassProvider
{
public:

    static void build(
        RenderPassManager::Description& description,
        VkFormat swapchainFormat
    );
};