#pragma once

#include "../CoreVulkan.hpp"
#include "../render_pass/RenderPassManager.hpp"

class ForwardRenderPassProvider
{
public:

    static void build(
        RenderPassManager::Description& description,
        VkFormat swapchainImageFormat,
        VkSampleCountFlagBits msaaSamples,
        VkFormat depthFormat
    );
};