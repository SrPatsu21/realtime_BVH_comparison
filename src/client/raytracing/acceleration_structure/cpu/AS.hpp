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

    const AABB& getBounds() const
    {
        static AABB bounds;

        bounds.reset();

        if (nodes.empty())
            return bounds;

        const auto& root =
            nodes[0];

        for (uint32_t i = 0u; i < root.childCount; ++i)
        {
            bounds.expand(
                root.childBounds[i]
            );
        }

        return bounds;
    }
    bool empty() const { return nodes.empty(); }
    void clear()
    {
        nodes.clear();
        instances.clear();
    }
};