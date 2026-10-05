#pragma once

#include <vector>
#include <array>
#include <cstdint>
#include <algorithm>
#include <limits>
#include <cmath>

#include "../../utils/BVHUtils.hpp"

template<
    typename TNodeType,
    uint32_t BIN_COUNT = 16
>
class BinnedSAHBuilder
{
public:

    using NodeType = TNodeType;

    template<typename PrimitiveType>
    static void build(
        std::vector<NodeType>& nodes,
        std::vector<PrimitiveType>& primitives
    );

private:

    struct Bin
    {
        AABB bounds;
        uint32_t count = 0;
        bool valid = false;
    };

    static AABB mergeBounds(
        const AABB& a,
        const AABB& b
    );

    static float surfaceArea(
        const AABB& bounds
    );
};

template<typename NodeType, uint32_t BIN_COUNT>
AABB BinnedSAHBuilder<NodeType, BIN_COUNT>::mergeBounds(
    const AABB& a,
    const AABB& b
)
{
    AABB result = a;

    result.min.x = std::min(result.min.x, b.min.x);
    result.min.y = std::min(result.min.y, b.min.y);
    result.min.z = std::min(result.min.z, b.min.z);

    result.max.x = std::max(result.max.x, b.max.x);
    result.max.y = std::max(result.max.y, b.max.y);
    result.max.z = std::max(result.max.z, b.max.z);

    return result;
}

template<typename NodeType, uint32_t BIN_COUNT>
float BinnedSAHBuilder<NodeType, BIN_COUNT>::surfaceArea(
    const AABB& bounds
)
{
    const float dx =
        std::max(
            bounds.max.x - bounds.min.x,
            0.0f
        );

    const float dy =
        std::max(
            bounds.max.y - bounds.min.y,
            0.0f
        );

    const float dz =
        std::max(
            bounds.max.z - bounds.min.z,
            0.0f
        );

    return
        2.0f *
        (
            dx * dy +
            dx * dz +
            dy * dz
        );
}

template<typename NodeType, uint32_t BIN_COUNT>
template<typename PrimitiveType>
void BinnedSAHBuilder<NodeType, BIN_COUNT>::build(
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

    struct Range
    {
        uint32_t begin;
        uint32_t end;

        uint32_t parent;
        uint32_t childSlot;
    };

    constexpr uint32_t INVALID_NODE = 0xFFFFFFFFu;

    std::vector<Range> stack;

    stack.reserve(
        primitives.size()
    );

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
            nodes[range.parent].children[range.childSlot] = nodeIndex;
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

        const int preferredAxis =
            BVHUtils::selectSplitAxis(
                centroidBounds
            );

        float bestCost = std::numeric_limits<float>::max();

        int bestAxis = -1;
        uint32_t bestSplit = 0;

        for (int axisOffset = 0; axisOffset < 3; ++axisOffset)
        {
            const int axis = (preferredAxis + axisOffset) % 3;
            const float minCoord = centroidBounds.min[axis];
            const float maxCoord = centroidBounds.max[axis];

            const float extent = maxCoord - minCoord;

            if (extent <= 0.000001f)
                continue;

            std::array<Bin, BIN_COUNT> bins{};

            for (uint32_t i = begin; i < end; ++i)
            {
                const float centroid =
                    primitives[i]
                        .getBounds()
                        .getCenterAxis(axis);

                float normalized =
                    (centroid - minCoord) /
                    extent;

                normalized =
                    std::clamp(
                        normalized,
                        0.0f,
                        0.999999f
                    );

                uint32_t binIndex =
                    static_cast<uint32_t>(
                        normalized *
                        static_cast<float>(
                            BIN_COUNT
                        )
                    );

                binIndex =
                    std::min(
                        binIndex,
                        BIN_COUNT - 1
                    );

                Bin& bin =
                    bins[binIndex];

                if (!bin.valid)
                {
                    bin.bounds =
                        primitives[i]
                            .getBounds();

                    bin.valid = true;
                }
                else
                {
                    bin.bounds =
                        mergeBounds(
                            bin.bounds,
                            primitives[i]
                                .getBounds()
                        );
                }

                ++bin.count;
            }

            std::array<AABB, BIN_COUNT> prefixBounds;
            std::array<uint32_t, BIN_COUNT> prefixCount{};
            std::array<bool, BIN_COUNT> prefixValid{};

            std::array<AABB, BIN_COUNT> suffixBounds;
            std::array<uint32_t, BIN_COUNT> suffixCount{};
            std::array<bool, BIN_COUNT> suffixValid{};

            for (uint32_t i = 0; i < BIN_COUNT; ++i)
            {
                if (bins[i].valid)
                {
                    prefixBounds[i] = bins[i].bounds;
                    prefixCount[i] = bins[i].count;
                    prefixValid[i] = true;
                }

                if (i > 0)
                {
                    if (prefixValid[i - 1])
                    {
                        if (prefixValid[i])
                        {
                            prefixBounds[i] = mergeBounds(prefixBounds[i - 1], prefixBounds[i]);
                            prefixCount[i] += prefixCount[i - 1];
                        }
                        else
                        {
                            prefixBounds[i] = prefixBounds[i - 1];
                            prefixCount[i] = prefixCount[i - 1];
                            prefixValid[i] = true;
                        }
                    }
                }
            }

            for (int i = static_cast<int>(BIN_COUNT) - 1; i >= 0; --i)
            {
                if (bins[i].valid)
                {
                    suffixBounds[i] = bins[i].bounds;
                    suffixCount[i] = bins[i].count;
                    suffixValid[i] = true;
                }

                if (i + 1 < static_cast<int>(BIN_COUNT))
                {
                    if (suffixValid[i + 1])
                    {
                        if (suffixValid[i])
                        {
                            suffixBounds[i] =
                                mergeBounds(
                                    suffixBounds[i],
                                    suffixBounds[i + 1]
                                );

                            suffixCount[i] += suffixCount[i + 1];
                        }
                        else
                        {
                            suffixBounds[i] = suffixBounds[i + 1];
                            suffixCount[i] = suffixCount[i + 1];
                            suffixValid[i] = true;
                        }
                    }
                }
            }

            for (uint32_t split = 0; split + 1 < BIN_COUNT; ++split)
            {
                if (!prefixValid[split])
                    continue;

                if (!suffixValid[split + 1])
                    continue;

                const uint32_t leftCount = prefixCount[split];
                const uint32_t rightCount = suffixCount[split + 1];

                if (leftCount == 0 || rightCount == 0)
                    continue;

                const float cost =
                    surfaceArea(
                        prefixBounds[split]
                    ) *
                    static_cast<float>(
                        leftCount
                    )
                    +
                    surfaceArea(
                        suffixBounds[split + 1]
                    ) *
                    static_cast<float>(
                        rightCount
                    );

                if (cost < bestCost)
                {
                    bestCost = cost;
                    bestAxis = axis;
                    bestSplit = split;
                }
            }
        }

        uint32_t mid = 0;

        if (bestAxis >= 0)
        {
            const float minCoord = centroidBounds.min[bestAxis];
            const float maxCoord = centroidBounds.max[bestAxis];
            const float extent = maxCoord - minCoord;

            auto getBin =
                [&](const PrimitiveType& primitive)
                {
                    float normalized =
                        (
                            primitive.getBounds().getCenterAxis(bestAxis) -
                            minCoord
                        ) /
                        extent;

                    normalized = std::clamp(normalized, 0.0f, 0.999999f);

                    uint32_t binIndex = static_cast<uint32_t>(normalized * static_cast<float>(BIN_COUNT));

                    return std::min(binIndex, BIN_COUNT - 1);
                };

            auto middle =
                std::partition(
                    primitives.begin() + begin,
                    primitives.begin() + end,
                    [=](
                        const PrimitiveType& primitive
                    )
                    {
                        return getBin(primitive) <= bestSplit;
                    }
                );

            mid =
                static_cast<uint32_t>(
                    middle -
                    primitives.begin()
                );

            if (mid == begin || mid == end)
            {
                mid = begin + count / 2;

                std::nth_element(
                    primitives.begin() + begin,
                    primitives.begin() + mid,
                    primitives.begin() + end,
                    [bestAxis](
                        const PrimitiveType& a,
                        const PrimitiveType& b
                    )
                    {
                        return
                            a.getBounds().getCenterAxis(bestAxis)
                            <
                            b.getBounds().getCenterAxis(bestAxis);
                    }
                );
            }
        }
        else
        {
            mid = begin + count / 2;

            std::nth_element(
                primitives.begin() + begin,
                primitives.begin() + mid,
                primitives.begin() + end,
                [preferredAxis](
                    const PrimitiveType& a,
                    const PrimitiveType& b
                )
                {
                    return
                        a.getBounds().getCenterAxis(preferredAxis)
                        <
                        b.getBounds().getCenterAxis(preferredAxis);
                }
            );
        }

        node.leaf = 0;
        node.pad0 = 0;

        node.childBounds[0] =
            BVHUtils::computeBounds(
                primitives,
                begin,
                mid
            );

        node.childBounds[1] =
            BVHUtils::computeBounds(
                primitives,
                mid,
                end
            );

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