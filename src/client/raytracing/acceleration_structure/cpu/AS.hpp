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

        if (nodes.empty())
            return bounds;

        bounds = nodes[0].childBounds[0];

        bounds.min.x =
            std::min(
                bounds.min.x,
                nodes[0].childBounds[1].min.x
            );

        bounds.min.y =
            std::min(
                bounds.min.y,
                nodes[0].childBounds[1].min.y
            );

        bounds.min.z =
            std::min(
                bounds.min.z,
                nodes[0].childBounds[1].min.z
            );

        bounds.max.x =
            std::max(
                bounds.max.x,
                nodes[0].childBounds[1].max.x
            );

        bounds.max.y =
            std::max(
                bounds.max.y,
                nodes[0].childBounds[1].max.y
            );

        bounds.max.z =
            std::max(
                bounds.max.z,
                nodes[0].childBounds[1].max.z
            );

        return bounds;
    }
    bool empty() const { return nodes.empty(); }
    void clear()
    {
        nodes.clear();
        instances.clear();
    }
};