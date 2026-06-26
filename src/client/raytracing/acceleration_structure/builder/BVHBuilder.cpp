#include "BVHBuilder.hpp"
#include <cstdint>
#include "../utils/BVHUtils.hpp"

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
    uint32_t nodeIndex = static_cast<uint32_t>(nodes.size());

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
        node.leaf = true;
        node.firstPrimitive = begin;
        node.primitiveCount = count;

        return nodeIndex;
    }

    AABB centroidBounds = BVHUtils::computeCentroidBounds(
            primitives,
            begin,
            end
        );

    int axis = BVHUtils::selectSplitAxis(centroidBounds);

    uint32_t mid = begin + count / 2;

    std::nth_element(
        primitives.begin() + begin,
        primitives.begin() + mid,
        primitives.begin() + end,
        [axis](
            const PrimitiveType& a,
            const PrimitiveType& b
        )
        {
            return a.getBounds().getCentroidAxis(axis)
                <
                b.getBounds().getCentroidAxis(axis);
        }
    );

    node.leaf = false;

    node.left = buildRecursive(
        nodes,
        primitives,
        begin,
        mid
    );

    node.right = buildRecursive(
        nodes,
        primitives,
        mid,
        end
    );

    return nodeIndex;
}