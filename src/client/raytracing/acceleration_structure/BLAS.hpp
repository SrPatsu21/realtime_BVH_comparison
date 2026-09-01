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

    const AABB& getBounds() const { return accelerationStructure.nodes[0].bounds; }

    void destroy(VkDevice device)
    {
        gpu.destroy(device);
    }
};