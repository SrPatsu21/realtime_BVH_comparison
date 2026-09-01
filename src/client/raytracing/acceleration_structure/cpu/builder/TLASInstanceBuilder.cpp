#include "TLASInstanceBuilder.hpp"

#include "../builder/BVHBuilder.hpp"

void TLASInstanceBuilder::build(
    const std::vector<TLASBuildInput>& inputs,
    const std::vector<uint32_t>& blasIndices,
    std::vector<PrimitiveRef>& primitives,
    std::vector<BVHNode>& nodes,
    std::vector<TLASInstance>& instances
)
{
    primitives.clear();
    nodes.clear();
    instances.clear();

    if (inputs.empty())
        return;

    if (blasIndices.size() != inputs.size())
        throw std::runtime_error("TLASInstanceBuilder: blasIndices size mismatch");

    createPrimitives(
        inputs,
        primitives
    );

    BVHBuilder<BVHNode>::build(
        nodes,
        primitives
    );

    createInstances(
        inputs,
        blasIndices,
        primitives,
        nodes,
        instances
    );

    primitives.clear();
}

void TLASInstanceBuilder::createPrimitives(
    const std::vector<TLASBuildInput>& inputs,
    std::vector<PrimitiveRef>& primitives
)
{
    primitives.reserve(inputs.size());

    for (uint32_t i = 0;
         i < static_cast<uint32_t>(inputs.size());
         ++i)
    {
        PrimitiveRef primitive{};

        primitive.bounds =
            inputs[i].bounds;

        /*
         * Identidade da entrada original.
         *
         * O BVHBuilder pode reordenar os PrimitiveRef,
         * então este índice não pode ser substituído pela
         * posição atual dentro do vetor.
         */
        primitive.index = i;

        primitives.emplace_back(
            primitive
        );
    }
}

void TLASInstanceBuilder::createInstances(
    const std::vector<TLASBuildInput>& inputs,
    const std::vector<uint32_t>& blasIndices,
    const std::vector<PrimitiveRef>& primitives,
    const std::vector<BVHNode>& nodes,
    std::vector<TLASInstance>& instances
)
{
    instances.reserve(inputs.size());

    for (const BVHNode& node : nodes)
    {
        if (!node.leaf)
            continue;

        if (node.primitiveCount == 0)
            continue;

        for (uint32_t i = 0;
             i < node.primitiveCount;
             ++i)
        {
            const uint32_t primitiveIndex =
                node.firstPrimitive + i;

            const PrimitiveRef& primitive =
                primitives[primitiveIndex];

            const uint32_t inputIndex =
                primitive.index;

            const TLASBuildInput& input =
                inputs[inputIndex];

            TLASInstance instance{};

            instance.bounds =
                input.bounds;

            instance.inverseTransform =
                input.inverseTransform;

            instance.blasIndex =
                blasIndices[inputIndex];

            instance.pad0 = 0;
            instance.pad1 = 0;
            instance.pad2 = 0;

            instances.emplace_back(
                instance
            );
        }
    }
}