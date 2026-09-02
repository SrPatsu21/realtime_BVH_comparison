#pragma once

#include "../../../CoreVulkan.hpp"

struct AccelerationStructureGPU
{
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkDeviceAddress address = 0;

    void destroy(VkDevice device)
    {
        if (buffer != VK_NULL_HANDLE)
            vkDestroyBuffer(device, buffer, nullptr);

        if (memory != VK_NULL_HANDLE)
            vkFreeMemory(device, memory, nullptr);

        buffer = VK_NULL_HANDLE;
        memory = VK_NULL_HANDLE;
        address = 0;
    }
};