#pragma once

#include <vector>
#include <cstdint>
#include <algorithm>

#include "../../utils/BVHUtils.hpp"

template<typename TNodeType>
class BVHBuilder
{
public:

    using NodeType = TNodeType;

    template<typename PrimitiveType>
    static void build(
        std::vector<NodeType>& nodes,
        std::vector<PrimitiveType>& primitives
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

    nodes.reserve(primitives.size() * 2);

    struct Range
    {
        uint32_t begin;
        uint32_t end;

        uint32_t parent;
        uint32_t childSlot;
    };

    constexpr uint32_t INVALID_NODE = 0xFFFFFFFFu;

    std::vector<Range> stack;

    stack.reserve(primitives.size());

    stack.push_back(
        {
            0,
            static_cast<uint32_t>(
                primitives.size()
            ),
            INVALID_NODE,
            0
        }
    );

    while (!stack.empty())
    {
        const Range range = stack.back();

        stack.pop_back();

        const uint32_t begin = range.begin;
        const uint32_t end = range.end;
        const uint32_t count = end - begin;
        const uint32_t nodeIndex = static_cast<uint32_t>(nodes.size());

        nodes.emplace_back();

        NodeType& node = nodes[nodeIndex];

        node.children[0] = 0;
        node.children[1] = 0;

        node.leaf = 0;
        node.pad0 = 0;

        if (range.parent != INVALID_NODE)
        {
            nodes[range.parent].children[range.childSlot] =
                nodeIndex;

            nodes[range.parent].childBounds[range.childSlot] =
                BVHUtils::computeBounds(
                    primitives,
                    begin,
                    end
                );
        }

        if (count <= 2)
        {
            node.leaf = 1;

            node.children[0] = begin;

            node.children[1] =
                count == 1
                ? begin
                : begin + 1;

            continue;
        }

        const AABB centroidBounds =
            BVHUtils::computeCentroidBounds(
                primitives,
                begin,
                end
            );

        const int axis =
            BVHUtils::selectSplitAxis(
                centroidBounds
            );

        const uint32_t mid =
            begin +
            count / 2;

        std::nth_element(
            primitives.begin() + begin,
            primitives.begin() + mid,
            primitives.begin() + end,
            [axis](
                const PrimitiveType& a,
                const PrimitiveType& b
            )
            {
                return
                    a.getBounds().getCenterAxis(axis)
                    <
                    b.getBounds().getCenterAxis(axis);
            }
        );

        node.leaf = 0;

        stack.push_back(
            {
                mid,
                end,
                nodeIndex,
                1
            }
        );

        stack.push_back(
            {
                begin,
                mid,
                nodeIndex,
                0
            }
        );
    }
}