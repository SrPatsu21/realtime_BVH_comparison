#pragma once

#include "../../../CoreVulkan.hpp"

struct VulkanBLAS
{
    VkBuffer nodeBuffer = VK_NULL_HANDLE;
    VkDeviceMemory nodeMemory = VK_NULL_HANDLE;
    VkDeviceAddress nodeAddress = 0;

    void destroy(VkDevice device)
    {
        if (nodeBuffer != VK_NULL_HANDLE)
        {
            vkDestroyBuffer(device, nodeBuffer, nullptr);
            nodeBuffer = VK_NULL_HANDLE;
        }

        if (nodeMemory != VK_NULL_HANDLE)
        {
            vkFreeMemory(device, nodeMemory, nullptr);
            nodeMemory = VK_NULL_HANDLE;
        }

        nodeAddress = 0;
    }
};