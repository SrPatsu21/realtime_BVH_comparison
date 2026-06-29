#pragma once

#include "AccelerationStructure.hpp"
class Mesh;

template<typename NodeType>
struct BLAS
{
    const Mesh* mesh;

    AccelerationStructure<NodeType> accelerationStructure;

    VulkanBLAS gpu;
};