#pragma once

#include "../../CoreVulkan.hpp"

class LightingFramebufferProvider
{
public:

    static void build(
        const std::vector<VkImageView>& swapchainImageViews,
        std::size_t swapchainImageViewsSize,
        std::vector<std::vector<VkImageView>>& attachmentsVector
    );
};