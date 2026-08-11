#pragma once

#include "AccelerationStructure.hpp"
#include "vulkan/VulkanBLAS.hpp"
class Mesh;

template<typename NodeType>
struct BLAS
{
    const Mesh* mesh;

    AccelerationStructure<NodeType> accelerationStructure;

    VulkanBLAS gpu;

    void destroy(VkDevice device)
    {
        gpu.destroy(device);
    }
};