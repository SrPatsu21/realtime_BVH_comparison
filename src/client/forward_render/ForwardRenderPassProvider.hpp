#pragma once

#include "../CoreVulkan.hpp"
#include "../render_pass/RenderPassManager.hpp"

class ForwardRenderPassProvider
{
public:

    static RenderPassManager::Description build(
        VkFormat swapchainImageFormat,
        VkSampleCountFlagBits msaaSamples,
        VkFormat depthFormat
    );
};