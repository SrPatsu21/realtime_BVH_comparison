#pragma once

#include <vector>
#include <cstdint>
#include <algorithm>

#include "../../utils/BVHUtils.hpp"
#include "../node/BVH8Node.hpp"

template<typename TNodeType>
class BVH8Builder
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
void BVH8Builder<NodeType>::build(
    std::vector<NodeType>& nodes,
    std::vector<PrimitiveType>& primitives
)
{
    nodes.clear();

    if (primitives.empty())
        return;

    nodes.reserve(primitives.size());

    constexpr uint32_t MAX_CHILDREN = 8;

    auto emit =
        [&](
            auto&& self,
            uint32_t begin,
            uint32_t end
        ) -> uint32_t
        {
            const uint32_t nodeIndex = static_cast<uint32_t>(nodes.size());

            nodes.emplace_back();

            NodeType& node = nodes[nodeIndex];

            for (uint32_t i = 0; i < MAX_CHILDREN; ++i)
            {
                node.children[i] = 0;
            }

            node.pad0 = 0;
            node.pad1 = 0;

            node.bounds =
                BVHUtils::computeBounds(
                    primitives,
                    begin,
                    end
                );

            const uint32_t count = end - begin;

            if (count <= MAX_CHILDREN)
            {
                node.leaf = 1;

                node.childCount = count;

                for (uint32_t i = 0; i < count; ++i)
                {
                    node.children[i] = begin + i;
                }

                return nodeIndex;
            }

            // =================================================
            // Internal node
            // =================================================

            node.leaf = 0;

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

            std::sort(
                primitives.begin() + begin,
                primitives.begin() + end,
                [axis](
                    const PrimitiveType& a,
                    const PrimitiveType& b
                )
                {
                    return a.getBounds().getCenterAxis(axis)
                        < b.getBounds().getCenterAxis(axis);
                }
            );

            const uint32_t childCount =
                std::min(
                    MAX_CHILDREN,
                    count
                );

            node.childCount = childCount;

            const uint32_t baseSize = count / childCount;

            const uint32_t remainder = count % childCount;

            uint32_t childBegin = begin;

            for (uint32_t i = 0; i < childCount; ++i)
            {
                const uint32_t childSize =
                    baseSize +
                    (i < remainder ? 1u : 0u);

                const uint32_t childEnd =
                    childBegin +
                    childSize;

                node.children[i] =
                    self(
                        self,
                        childBegin,
                        childEnd
                    );

                childBegin = childEnd;
            }

            return nodeIndex;
        };

    emit(
        emit,
        0,
        static_cast<uint32_t>(
            primitives.size()
        )
    );
}