#pragma once

#include "../../CoreVulkan.hpp"

class DeferredLightingFramebufferProvider
{
public:

    static void build(
        VkImageView lightingView,
        std::size_t framebufferCount,
        std::vector<std::vector<VkImageView>>& attachmentsVector
    );
};