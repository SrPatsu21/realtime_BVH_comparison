#pragma once

#include <cstdint>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <vector>

#include "cpu/AS.hpp"

#include "cpu/node/BVHNode.hpp"
#include "cpu/node/BLASInstance.hpp"
#include "cpu/node/TLASInstance.hpp"

#include "cpu/primitives/PrimitiveRef.hpp"
#include "cpu/primitives/TLASBuildInput.hpp"

#include "cpu/builder/BLASInstanceBuilder.hpp"
#include "cpu/builder/TLASInstanceBuilder.hpp"
#include "cpu/builder/BVHBuilder.hpp"

#include "../../batch/mesh/Mesh.hpp"

template<
    typename TLBuilderType,
    typename BLBuilderType
>
class AccelerationStructureManager
{
public:

    using TLNodeType = typename TLBuilderType::NodeType;
    using BLNodeType = typename BLBuilderType::NodeType;

    using TLAS = AS<TLNodeType, TLASInstance>;
    using BLAS = AS< BLNodeType, BLASInstance>;

private:

    std::unordered_map<
        const Mesh*,
        std::shared_ptr<BLAS>
    > blasMap;

    TLAS tlas;

public:

    AccelerationStructureManager() = default;

    ~AccelerationStructureManager() = default;

    std::shared_ptr<BLAS> createBLAS(
        const Mesh* mesh
    );
    std::shared_ptr<BLAS> getBLAS(
        const Mesh* mesh
    ) const;

    void createTLAS(
        const std::vector<TLASBuildInput>& inputs
    );
    TLAS& getTLAS()
    {
        return tlas;
    }
    const TLAS& getTLAS() const
    {
        return tlas;
    }

    void clear();

private:

    void destroyBLAS(
        const Mesh* mesh
    );
};


/*
 * =============================================================
 * createBLAS
 * =============================================================
 */

template<
    typename TLBuilderType,
    typename BLBuilderType
>
std::shared_ptr<
    typename AccelerationStructureManager<
        TLBuilderType,
        BLBuilderType
    >::BLAS
>
AccelerationStructureManager<
    TLBuilderType,
    BLBuilderType
>::createBLAS(
    const Mesh* mesh
)
{
    if (!mesh)
        return nullptr;

    auto it = blasMap.find(mesh);

    if (it != blasMap.end())
        return it->second;

    auto blas = std::make_shared<BLAS>();

    std::vector<PrimitiveRef> primitives;

    BLASInstanceBuilder::buildPrimitives(
        *mesh,
        primitives
    );

    BLBuilderType::build(
        blas->accelerationStructure,
        primitives
    );

    BLASInstanceBuilder::buildInstances(
        blas->accelerationStructure,
        primitives,
        blas->instances
    );

    primitives.clear();

    blasMap.emplace(
        mesh,
        blas
    );

    return blas;
}


template<
    typename TLBuilderType,
    typename BLBuilderType
>
std::shared_ptr<
    typename AccelerationStructureManager<
        TLBuilderType,
        BLBuilderType
    >::BLAS
>
AccelerationStructureManager<
    TLBuilderType,
    BLBuilderType
>::getBLAS(
    const Mesh* mesh
) const
{
    auto it = blasMap.find(mesh);

    if (it == blasMap.end())
        return nullptr;

    return it->second;
}

/*
 * =============================================================
 * createTLAS
 * =============================================================
 */

template<
    typename TLBuilderType,
    typename BLBuilderType
>
void
AccelerationStructureManager<
    TLBuilderType,
    BLBuilderType
>::createTLAS(
    const std::vector<TLASBuildInput>& inputs
)
{
    tlas.accelerationStructure.clear();
    tlas.instances.clear();

    if (inputs.empty())
    {
        std::cout
            << "No instances to build TLAS"
            << std::endl;

        return;
    }

    std::vector<uint32_t> blasIndices;
    blasIndices.reserve(inputs.size());

    for (const TLASBuildInput& input : inputs)
    {
        if (!input.blas)
            throw std::runtime_error(
                "TLAS input contains null BLAS"
            );

        blasIndices.emplace_back(
            getBLASIndex(input.blas)
        );
    }

    std::vector<PrimitiveRef> primitives;

    TLASInstanceBuilder::build(
        inputs,
        blasIndices,
        primitives,
        tlas.accelerationStructure,
        tlas.instances
    );
}

/*
 * =============================================================
 * destroyBLAS
 * =============================================================
 */

template<
    typename TLBuilderType,
    typename BLBuilderType
>
void
AccelerationStructureManager<
    TLBuilderType,
    BLBuilderType
>::destroyBLAS(
    const Mesh* mesh
)
{
    auto it = blasMap.find(mesh);

    if (it == blasMap.end())
        return;

    blasMap.erase(it);
}


/*
 * =============================================================
 * clear
 * =============================================================
 */

template<
    typename TLBuilderType,
    typename BLBuilderType
>
void
AccelerationStructureManager<
    TLBuilderType,
    BLBuilderType
>::clear()
{
    tlas.accelerationStructure.clear();
    tlas.instances.clear();

    blasMap.clear();
}


/*
 * =============================================================
 * Debug
 * =============================================================
 */

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