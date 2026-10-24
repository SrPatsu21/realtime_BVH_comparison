#pragma once

#include <algorithm>
#include <cstdint>
#include <vector>

#include <oneapi/tbb/parallel_for.h>
#include <oneapi/tbb/parallel_sort.h>

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

    struct BuildTask
    {
        uint32_t node;
        uint32_t begin;
        uint32_t end;
    };

    struct SplitResult
    {
        uint32_t mid;
        uint32_t leftChild;
        uint32_t rightChild;
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
        const uint32_t x =
            static_cast<uint32_t>(
                std::clamp(
                    position.x * 1024.0f,
                    0.0f,
                    1023.0f
                )
            );

        const uint32_t y =
            static_cast<uint32_t>(
                std::clamp(
                    position.y * 1024.0f,
                    0.0f,
                    1023.0f
                )
            );

        const uint32_t z =
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
        const std::vector<MortonPrimitive>& primitives,
        int a,
        int b
    )
    {
        const uint32_t codeA =
            primitives[a].code;

        const uint32_t codeB =
            primitives[b].code;

        if (codeA != codeB)
        {
            return __builtin_clz(
                codeA ^ codeB
            );
        }

        return
            32 +
            __builtin_clz(
                static_cast<uint32_t>(a ^ b)
            );
    }

    static uint32_t determineSplit(
        const std::vector<MortonPrimitive>& primitives,
        uint32_t begin,
        uint32_t end
    )
    {
        const int first =
            static_cast<int>(begin);

        const int last =
            static_cast<int>(end - 1);

        const int common =
            commonPrefix(
                primitives,
                first,
                last
            );

        int split = first;
        int step = last - first;

        do
        {
            step = (step + 1) >> 1;

            const int candidate =
                split + step;

            if (candidate < last)
            {
                const int candidateCommon =
                    commonPrefix(
                        primitives,
                        first,
                        candidate
                    );

                if (candidateCommon > common)
                    split = candidate;
            }
        }
        while (step > 1);

        return static_cast<uint32_t>(
            split + 1
        );
    }

    static AABB computeRangeBounds(
        const std::vector<MortonPrimitive>& primitives,
        uint32_t begin,
        uint32_t end
    )
    {
        AABB bounds;

        for (uint32_t i = begin; i < end; ++i)
        {
            bounds.expand(
                primitives[i]
                    .primitive
                    .getBounds()
            );
        }

        return bounds;
    }

    static void initializeLeaf(
        NodeType& node,
        const std::vector<MortonPrimitive>& primitives,
        uint32_t begin,
        uint32_t end
    )
    {
        node.leaf = 1;
        node.pad0 = 0;

        node.children[0] = begin;

        node.childBounds[0] =
            primitives[begin]
                .primitive
                .getBounds();

        if (end - begin == 2)
        {
            node.children[1] = begin + 1;

            node.childBounds[1] =
                primitives[begin + 1]
                    .primitive
                    .getBounds();
        }
        else
        {
            node.children[1] = begin;

            node.childBounds[1] =
                node.childBounds[0];
        }
    }

    static void initializeInternal(
        NodeType& node,
        const std::vector<MortonPrimitive>& primitives,
        const BuildTask& task,
        uint32_t leftChild,
        uint32_t rightChild,
        uint32_t mid
    )
    {
        node.leaf = 0;
        node.pad0 = 0;

        node.children[0] = leftChild;
        node.children[1] = rightChild;

        node.childBounds[0] =
            computeRangeBounds(
                primitives,
                task.begin,
                mid
            );

        node.childBounds[1] =
            computeRangeBounds(
                primitives,
                mid,
                task.end
            );
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

        const AABB centroidBounds =
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

        extent.x = std::max(extent.x, 0.000001f);
        extent.y = std::max(extent.y, 0.000001f);
        extent.z = std::max(extent.z, 0.000001f);

        std::vector<MortonPrimitive>
            mortonPrimitives(primitiveCount);

        oneapi::tbb::parallel_for(
            uint32_t(0),
            primitiveCount,
            [&](uint32_t i)
            {
                const PrimitiveRef& primitive =
                    primitives[i];

                const glm::vec3 centroid =
                    primitive
                        .getBounds()
                        .getCenter();

                glm::vec3 normalized =
                    (
                        centroid -
                        glm::vec3(centroidBounds.min)
                    ) / extent;

                normalized =
                    glm::clamp(
                        normalized,
                        glm::vec3(0.0f),
                        glm::vec3(1.0f)
                    );

                mortonPrimitives[i] =
                {
                    morton3D(normalized),
                    primitive
                };
            }
        );

        oneapi::tbb::parallel_sort(
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

        for (uint32_t i = 0; i < primitiveCount; ++i)
        {
            primitives[i] =
                mortonPrimitives[i].primitive;
        }

        nodes.resize(primitiveCount);

        std::vector<BuildTask> currentLevel;
        std::vector<BuildTask> nextLevel;

        currentLevel.reserve(primitiveCount);
        nextLevel.reserve(primitiveCount);

        currentLevel.push_back(
            {
                0,
                0,
                primitiveCount
            }
        );

        uint32_t nextNode = 1;

        while (!currentLevel.empty())
        {

            std::vector<SplitResult> splits(
                currentLevel.size()
            );

            std::vector<uint32_t> childCounts(
                currentLevel.size(),
                0
            );

            oneapi::tbb::parallel_for(
                size_t(0),
                currentLevel.size(),
                [&](size_t i)
                {
                    const BuildTask& task =
                        currentLevel[i];

                    const uint32_t count =
                        task.end - task.begin;

                    if (count <= 2)
                        return;

                    const uint32_t mid =
                        determineSplit(
                            mortonPrimitives,
                            task.begin,
                            task.end
                        );

                    splits[i].mid = mid;

                    childCounts[i] = 2;
                }
            );

            uint32_t allocation =
                nextNode;

            for (size_t i = 0; i < currentLevel.size(); ++i)
            {
                if (childCounts[i] == 0)
                    continue;

                splits[i].leftChild =
                    allocation++;

                splits[i].rightChild =
                    allocation++;
            }

            nextNode = allocation;

            std::vector<std::vector<BuildTask>> generated(
                currentLevel.size()
            );

            oneapi::tbb::parallel_for(
                size_t(0),
                currentLevel.size(),
                [&](size_t i)
                {
                    const BuildTask& task =
                        currentLevel[i];

                    NodeType& node =
                        nodes[task.node];

                    const uint32_t count =
                        task.end - task.begin;

                    if (count <= 2)
                    {
                        initializeLeaf(
                            node,
                            mortonPrimitives,
                            task.begin,
                            task.end
                        );

                        return;
                    }

                    const uint32_t mid =
                        splits[i].mid;

                    const uint32_t leftChild =
                        splits[i].leftChild;

                    const uint32_t rightChild =
                        splits[i].rightChild;

                    initializeInternal(
                        node,
                        mortonPrimitives,
                        task,
                        leftChild,
                        rightChild,
                        mid
                    );

                    generated[i].push_back(
                        {
                            leftChild,
                            task.begin,
                            mid
                        }
                    );

                    generated[i].push_back(
                        {
                            rightChild,
                            mid,
                            task.end
                        }
                    );
                }
            );

            nextLevel.clear();

            for (const auto& children : generated)
            {
                nextLevel.insert(
                    nextLevel.end(),
                    children.begin(),
                    children.end()
                );
            }

            currentLevel.swap(nextLevel);
        }

        nodes.resize(nextNode);
    }
};