#pragma once

#include <algorithm>
#include <cstdint>
#include <limits>
#include <vector>

#include "../node/BVHNode.hpp"
#include "../primitives/PrimitiveRef.hpp"
#include "../../utils/BVHUtils.hpp"

template<typename TNodeType>
class LBVHBuilder
{
public:

    using NodeType = TNodeType;

private:

    struct MortonPrimitive
    {
        uint32_t code;
        PrimitiveRef primitive;
    };

    static uint32_t expandBits(
        uint32_t value
    )
    {
        value =
            (value * 0x00010001u) &
            0xFF0000FFu;

        value =
            (value * 0x00000101u) &
            0x0F00F00Fu;

        value =
            (value * 0x00000011u) &
            0xC30C30C3u;

        value =
            (value * 0x00000005u) &
            0x49249249u;

        return value;
    }

    static uint32_t morton3D(
        const glm::vec3& position
    )
    {
        uint32_t x =
            static_cast<uint32_t>(
                std::clamp(
                    position.x * 1024.0f,
                    0.0f,
                    1023.0f
                )
            );

        uint32_t y =
            static_cast<uint32_t>(
                std::clamp(
                    position.y * 1024.0f,
                    0.0f,
                    1023.0f
                )
            );

        uint32_t z =
            static_cast<uint32_t>(
                std::clamp(
                    position.z * 1024.0f,
                    0.0f,
                    1023.0f
                )
            );

        return
            (expandBits(x) << 2) |
            (expandBits(y) << 1) |
            expandBits(z);
    }

    static int commonPrefix(
        uint32_t a,
        uint32_t b
    )
    {
        if (a == b)
            return 32;

        return __builtin_clz(
            a ^ b
        );
    }

    static int determineSplit(
        const std::vector<MortonPrimitive>& primitives,
        int first,
        int last
    )
    {
        uint32_t firstCode =
            primitives[first].code;

        uint32_t lastCode =
            primitives[last].code;

        if (firstCode == lastCode)
            return (first + last) >> 1;

        int common =
            commonPrefix(
                firstCode,
                lastCode
            );

        int split =
            first;

        int step =
            last - first;

        do
        {
            step =
                (step + 1) >> 1;

            int candidate =
                split + step;

            if (candidate < last)
            {
                int candidateCommon =
                    commonPrefix(
                        firstCode,
                        primitives[candidate].code
                    );

                if (candidateCommon > common)
                    split = candidate;
            }
        }
        while (step > 1);

        return split;
    }

    static AABB computeRangeBounds(
        const std::vector<MortonPrimitive>& primitives,
        uint32_t begin,
        uint32_t end
    )
    {
        AABB bounds;

        for (
            uint32_t i = begin;
            i < end;
            ++i
        )
        {
            bounds.expand(
                primitives[i]
                    .primitive
                    .getBounds()
            );
        }

        return bounds;
    }

    static uint32_t buildRecursive(
        std::vector<NodeType>& nodes,
        const std::vector<MortonPrimitive>& primitives,
        uint32_t begin,
        uint32_t end
    )
    {
        const uint32_t nodeIndex =
            static_cast<uint32_t>(
                nodes.size()
            );

        nodes.emplace_back();

        NodeType& node =
            nodes[nodeIndex];

        node.leaf = 0;
        node.pad0 = 0;

        const uint32_t count =
            end - begin;

        if (count <= 2)
        {
            node.leaf = 1;

            node.children[0] =
                begin;

            node.childBounds[0] =
                primitives[begin]
                    .primitive
                    .getBounds();

            if (count == 2)
            {
                node.children[1] =
                    begin + 1;

                node.childBounds[1] =
                    primitives[begin + 1]
                        .primitive
                        .getBounds();
            }
            else
            {
                node.children[1] =
                    begin;

                node.childBounds[1] =
                    node.childBounds[0];
            }

            return nodeIndex;
        }

        const int first =
            static_cast<int>(
                begin
            );

        const int last =
            static_cast<int>(
                end - 1
            );

        const int split =
            determineSplit(
                primitives,
                first,
                last
            );

        const uint32_t mid =
            static_cast<uint32_t>(
                split + 1
            );

        const uint32_t leftChild =
            buildRecursive(
                nodes,
                primitives,
                begin,
                mid
            );

        const uint32_t rightChild =
            buildRecursive(
                nodes,
                primitives,
                mid,
                end
            );

        node.children[0] =
            leftChild;

        node.children[1] =
            rightChild;

        node.childBounds[0] =
            computeRangeBounds(
                primitives,
                begin,
                mid
            );

        node.childBounds[1] =
            computeRangeBounds(
                primitives,
                mid,
                end
            );

        return nodeIndex;
    }

public:

    static void build(
        std::vector<NodeType>& nodes,
        std::vector<PrimitiveRef>& primitives
    )
    {
        nodes.clear();

        if (primitives.empty())
            return;

        const uint32_t primitiveCount =
            static_cast<uint32_t>(
                primitives.size()
            );

        AABB centroidBounds =
            BVHUtils::computeCentroidBounds(
                primitives,
                0,
                primitiveCount
            );

        glm::vec3 extent =
            glm::vec3(
                centroidBounds.max -
                centroidBounds.min
            );

        extent.x =
            std::max(
                extent.x,
                0.000001f
            );

        extent.y =
            std::max(
                extent.y,
                0.000001f
            );

        extent.z =
            std::max(
                extent.z,
                0.000001f
            );

        std::vector<MortonPrimitive>
            mortonPrimitives;

        mortonPrimitives.reserve(
            primitives.size()
        );

        for (
            const PrimitiveRef& primitive :
            primitives
        )
        {
            glm::vec3 centroid =
                primitive
                    .getBounds()
                    .getCenter();

            glm::vec3 normalized =
                (
                    centroid -
                    glm::vec3(
                        centroidBounds.min
                    )
                ) / extent;

            normalized =
                glm::clamp(
                    normalized,
                    glm::vec3(0.0f),
                    glm::vec3(1.0f)
                );

            MortonPrimitive entry;

            entry.code =
                morton3D(
                    normalized
                );

            entry.primitive =
                primitive;

            mortonPrimitives.emplace_back(
                entry
            );
        }

        std::stable_sort(
            mortonPrimitives.begin(),
            mortonPrimitives.end(),
            [](
                const MortonPrimitive& a,
                const MortonPrimitive& b
            )
            {
                return a.code < b.code;
            }
        );

        primitives.clear();

        primitives.reserve(
            mortonPrimitives.size()
        );

        for (
            const MortonPrimitive& primitive :
            mortonPrimitives
        )
        {
            primitives.emplace_back(
                primitive.primitive
            );
        }

        nodes.reserve(
            primitives.size() * 2
        );

        buildRecursive(
            nodes,
            mortonPrimitives,
            0,
            static_cast<uint32_t>(
                mortonPrimitives.size()
            )
        );
    }
};