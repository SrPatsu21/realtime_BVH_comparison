#pragma once

#include "AccelerationStructure.hpp"
#include "vulkan/VulkanTLAS.hpp"

template<typename NodeType>
struct TLAS
{
    AccelerationStructure<NodeType> accelerationStructure;

    VulkanTLAS gpu;

    void destroy(VkDevice device)
    {
        gpu.destroy(device);
    }
};