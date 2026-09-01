#include "TLASInstanceBuilder.hpp"

#include "../builder/BVHBuilder.hpp"

void TLASInstanceBuilder::build(
    const std::vector<TLASBuildInput>& inputs,
    std::vector<PrimitiveRef>& primitives,
    std::vector<BVHNode>& nodes,
    std::vector<TLASInstance>& instances
)
{
    createPrimitives(
        inputs,
        primitives
    );

    nodes.clear();
    instances.clear();

    if (primitives.empty())
        return;

    BVHBuilder<BVHNode>::build(
        nodes,
        primitives
    );

    createInstances(
        inputs,
        primitives,
        nodes,
        instances
    );
}

void TLASInstanceBuilder::createPrimitives(
    const std::vector<TLASBuildInput>& inputs,
    std::vector<PrimitiveRef>& primitives
)
{
    primitives.clear();
    primitives.reserve(inputs.size());

    for (uint32_t i = 0; i < inputs.size(); ++i)
    {
        PrimitiveRef primitive{};

        primitive.bounds =
            inputs[i].bounds;

        primitive.index =
            i;

        primitives.emplace_back(
            primitive
        );
    }
}

void TLASInstanceBuilder::createInstances(
    const std::vector<TLASBuildInput>& inputs,
    const std::vector<PrimitiveRef>& primitives,
    const std::vector<BVHNode>& nodes,
    std::vector<TLASInstance>& instances
)
{
    instances.clear();

    for (const BVHNode& node : nodes)
    {
        if (!node.leaf)
            continue;

        if (node.primitiveCount == 0)
            continue;

        for (
            uint32_t i = 0;
            i < node.primitiveCount;
            ++i
        )
        {
            const uint32_t primitiveIndex =
                node.firstPrimitive + i;

            const uint32_t inputIndex =
                primitives[primitiveIndex].index;

            const TLASBuildInput& input =
                inputs[inputIndex];

            TLASInstance instance{};

            instance.bounds = input.bounds;
            instance.inverseTransform = input.inverseTransform;
            instance.blasIndex = input.blasIndex;
            instance.pad0 = 0;
            instance.pad1 = 0;
            instance.pad2 = 0;

            instances.emplace_back(
                instance
            );
        }
    }
}