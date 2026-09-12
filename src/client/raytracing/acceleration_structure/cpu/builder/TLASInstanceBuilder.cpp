#include "TLASInstanceBuilder.hpp"

#include "../builder/BVHBuilder.hpp"

void TLASInstanceBuilder::build(
    const std::vector<TLASBuildInput>& inputs,
    const std::vector<uint32_t>& blasIndices,
    std::vector<PrimitiveRef>& primitives,
    std::vector<NodeType>& nodes,
    std::vector<TLASInstance>& instances
)
{
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

    if (primitives.empty())
        return;

    BuilderType::build(
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
    primitives.clear();

    primitives.reserve(inputs.size());

    for (uint32_t i = 0; i < static_cast<uint32_t>(inputs.size()); ++i)
    {
        PrimitiveRef primitive{};
        primitive.bounds = inputs[i].bounds;
        primitive.index = i;
        primitive.count = 1;
        primitives.emplace_back(
            primitive
        );
    }
}

void TLASInstanceBuilder::createInstances(
    const std::vector<TLASBuildInput>& inputs,
    const std::vector<uint32_t>& blasIndices,
    const std::vector<PrimitiveRef>& primitives,
    const std::vector<NodeType>& nodes,
    std::vector<TLASInstance>& instances
)
{
    instances.clear();

    if (primitives.empty())
        return;

    instances.reserve(primitives.size());

    for ( const PrimitiveRef& primitive : primitives)
    {
        const uint32_t inputIndex = primitive.index;

        if (inputIndex >= inputs.size())
            throw std::runtime_error("TLASInstanceBuilder: primitive input index out of range");

        const TLASBuildInput& input = inputs[inputIndex];

        TLASInstance instance{};
        instance.bounds = input.bounds;
        instance.inverseTransform = input.inverseTransform;
        instance.vertexAddress = input.vertexAddress;
        instance.indexAddress = input.indexAddress;
        instance.blasIndex = blasIndices[inputIndex];

        instance.nodeOffset = 0;
        instance.nodeCount = 0;
        instance.instanceOffset = 0;

        instances.emplace_back(
            instance
        );
    }
}