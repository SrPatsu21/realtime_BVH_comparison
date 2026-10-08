#pragma once

#include <vector>
#include <cstdint>
#include <algorithm>

#include "../../utils/BVHUtils.hpp"

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

    constexpr uint32_t MAX_CHILDREN   = 8u;
    constexpr uint32_t MAX_LEAF_PRIMS = 8u;

    nodes.reserve(primitives.size());

    auto emit =
        [&](auto&& self, uint32_t begin, uint32_t end) -> uint32_t
        {
            const uint32_t nodeIndex =
                static_cast<uint32_t>(nodes.size());

            nodes.emplace_back();

            NodeType node{};

            node.childCount = 0u;
            node.leaf = 0u;
            node.pad0 = 0u;
            node.pad1 = 0u;

            for (uint32_t i = 0u; i < MAX_CHILDREN; ++i)
            {
                node.children[i] = 0u;

                node.childBounds[i] =
                    AABB{};
            }

            const uint32_t count =
                end - begin;

            // =====================================================
            // Leaf
            // =====================================================

            if (count <= MAX_LEAF_PRIMS)
            {
                node.leaf =
                    1u;

                node.childCount =
                    count;

                for (uint32_t i = 0u; i < count; ++i)
                {
                    node.children[i] =
                        begin + i;

                    node.childBounds[i] =
                        primitives[
                            begin + i
                        ].getBounds();
                }

                nodes[nodeIndex] = node;

                return nodeIndex;
            }

            // =====================================================
            // Internal node
            // =====================================================

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
                    return
                        a.getBounds().getCenterAxis(axis)
                        <
                        b.getBounds().getCenterAxis(axis);
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

            for (uint32_t child = 0u; child < childCount; ++child)
            {
                const uint32_t childSize =
                    baseSize +
                    (child < remainder ? 1u : 0u);

                const uint32_t childEnd =
                    childBegin +
                    childSize;

                node.childBounds[child] =
                    BVHUtils::computeBounds(
                        primitives,
                        childBegin,
                        childEnd
                    );

                const uint32_t childNode =
                    self(
                        self,
                        childBegin,
                        childEnd
                    );

                node.children[child] =
                    childNode;

                childBegin =
                    childEnd;
            }

            nodes[nodeIndex] = node;

            return nodeIndex;
        };

    emit(
        emit,
        0u,
        static_cast<uint32_t>(
            primitives.size()
        )
    );
}