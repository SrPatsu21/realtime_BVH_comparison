#pragma once

#include "../../../CoreVulkan.hpp"

struct VulkanBLAS
{
    VkBuffer nodeBuffer = VK_NULL_HANDLE;
    VkDeviceAddress nodeAddress = 0;
};