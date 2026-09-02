#pragma once

#include <vector>

#include "../AABB.hpp"

template<
    typename NodeType,
    typename InstanceType
>
struct AS
{
    std::vector<NodeType> nodes;
    std::vector<InstanceType> instances;
    uint32_t index;

    // GPU
    uint32_t nodeOffset;
    uint32_t nodeCount;

    uint32_t instanceOffset;
    uint32_t instanceCount;

    const AABB& getBounds() const { return nodes[0].bounds; }
    bool empty() const { return nodes.empty(); }
    void clear()
    {
        nodes.clear();
        instances.clear();
    }
};