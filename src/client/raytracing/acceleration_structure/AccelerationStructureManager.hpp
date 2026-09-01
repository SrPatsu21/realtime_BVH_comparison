#pragma once

#include <cstdint>
#include <iostream>
#include <vector>

#include "BVHNode.hpp"

#include "builder/BLASInstanceBuilder.hpp"
#include "builder/TLASInstanceBuilder.hpp"
#include "builder/TLASBuildInput.hpp"

#include "../../batch/mesh/Mesh.hpp"

template<
    typename TLBuilderType,
    typename BLBuilderType
>
class AccelerationStructureManager
{
public:

    using TLNodeType =
        typename TLBuilderType::NodeType;

    using BLNodeType =
        typename BLBuilderType::NodeType;

    void createBLAS(
        const Mesh* mesh,
        std::vector<PrimitiveRef>& primitives,
        BufferManager* bufferManager,
        BLAS<BLNodeType>& blas
    );

    void createTLAS(
        const std::vector<TLASBuildInput>& inputs,
        std::vector<PrimitiveRef>& primitives,
        BufferManager* bufferManager,
        TLAS<TLNodeType>& tlas
    );
};

template<
    typename TLBuilderType,
    typename BLBuilderType
>
void
AccelerationStructureManager<
    TLBuilderType,
    BLBuilderType
>::createBLAS(
    const Mesh* mesh,
    std::vector<PrimitiveRef>& primitives,
    BufferManager* bufferManager,
    BLAS<BLNodeType>& blas
)
{
    blas.mesh = mesh;

    blas.accelerationStructure
        .accelerationStructure.clear();

    blas.accelerationStructure
        .instances.clear();

    BLASInstanceBuilder::build(
        *mesh,
        primitives,
        blas.accelerationStructure.accelerationStructure,
        blas.accelerationStructure.instances
    );
}

template<
    typename TLBuilderType,
    typename BLBuilderType
>
void
AccelerationStructureManager<
    TLBuilderType,
    BLBuilderType
>::createTLAS(
    const std::vector<TLASBuildInput>& inputs,
    std::vector<PrimitiveRef>& primitives,
    BufferManager* bufferManager,
    TLAS<TLNodeType>& tlas
)
{
    tlas.accelerationStructure
        .accelerationStructure.clear();

    tlas.accelerationStructure
        .instances.clear();

    if (inputs.empty())
    {
        std::cout
            << "No instances to build TLAS"
            << std::endl;

        return;
    }

    TLASInstanceBuilder::build(
        inputs,
        primitives,
        tlas.accelerationStructure.accelerationStructure,
        tlas.accelerationStructure.instances
    );
}

template<typename NodeType>
void printBVH(
    const std::vector<NodeType>& nodes,
    uint32_t index = 0
)
{
    if (index >= nodes.size())
        return;

    const NodeType& node =
        nodes[index];

    std::cout
        << "["
        << index
        << "] "
        << "min=("
        << node.bounds.min.x
        << ", "
        << node.bounds.min.y
        << ", "
        << node.bounds.min.z
        << ") "
        << "max=("
        << node.bounds.max.x
        << ", "
        << node.bounds.max.y
        << ", "
        << node.bounds.max.z
        << ") ";

    if (node.leaf)
    {
        std::cout
            << "LEAF first="
            << node.firstPrimitive
            << " count="
            << node.primitiveCount;
    }
    else
    {
        std::cout
            << "INTERNAL left="
            << node.left
            << " right="
            << node.right;
    }

    std::cout << '\n';

    if (!node.leaf)
    {
        printBVH(
            nodes,
            node.left
        );

        printBVH(
            nodes,
            node.right
        );
    }
}