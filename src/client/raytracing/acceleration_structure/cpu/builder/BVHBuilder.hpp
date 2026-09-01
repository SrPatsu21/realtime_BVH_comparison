#pragma once

#include <vector>
#include <cstdint>
#include "../../utils/BVHUtils.hpp"
#include <algorithm>


template<typename TNodeType>
class BVHBuilder
{
public:

    using NodeType = TNodeType;

    static constexpr uint32_t LEAF_SIZE = 4;

    template<typename PrimitiveType>
    static void build(
        std::vector<NodeType>& nodes,
        std::vector<PrimitiveType>& primitives
    );

private:

    template<typename PrimitiveType>
    static uint32_t buildRecursive(
        std::vector<NodeType>& nodes,
        std::vector<PrimitiveType>& primitives,
        uint32_t begin,
        uint32_t end
    );
};


template<typename NodeType>
template<typename PrimitiveType>
void BVHBuilder<NodeType>::build(
    std::vector<NodeType>& nodes,
    std::vector<PrimitiveType>& primitives
)
{
    nodes.clear();

    if (primitives.empty())
        return;

    nodes.reserve(
        primitives.size() * 2
    );

    buildRecursive(
        nodes,
        primitives,
        0,
        static_cast<uint32_t>(primitives.size())
    );
}


template<typename NodeType>
template<typename PrimitiveType>
uint32_t BVHBuilder<NodeType>::buildRecursive(
    std::vector<NodeType>& nodes,
    std::vector<PrimitiveType>& primitives,
    uint32_t begin,
    uint32_t end
)
{
    const uint32_t nodeIndex = static_cast<uint32_t>(nodes.size());

    nodes.emplace_back();

    NodeType& node = nodes[nodeIndex];

    node.bounds = BVHUtils::computeBounds(
            primitives,
            begin,
            end
        );

    const uint32_t count = end - begin;

    if (count <= LEAF_SIZE)
    {
        node.leaf = 1;
        node.firstPrimitive = begin;
        node.primitiveCount = count;

        return nodeIndex;
    }

    const AABB centroidBounds =
        BVHUtils::computeCentroidBounds(
            primitives,
            begin,
            end
        );

    const int axis = BVHUtils::selectSplitAxis(centroidBounds);

    const uint32_t mid = begin + count / 2;

    std::nth_element(
        primitives.begin() + begin,
        primitives.begin() + mid,
        primitives.begin() + end,
        [axis](const PrimitiveType& a, const PrimitiveType& b)
        {
            return a.getBounds().getCenterAxis(axis)
            < b.getBounds().getCenterAxis(axis);
        }
    );

    node.leaf = 0;

    const uint32_t left = buildRecursive(
            nodes,
            primitives,
            begin,
            mid
        );

    const uint32_t right = buildRecursive(
            nodes,
            primitives,
            mid,
            end
        );

    node.left = left;
    node.right = right;

    return nodeIndex;
}