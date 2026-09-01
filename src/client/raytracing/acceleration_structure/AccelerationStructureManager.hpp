#pragma once

#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
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

    using BLAS = AS<BLNodeType, BLASInstance>;
    using TLAS = AS<TLNodeType, TLASInstance>;

private:

    std::vector<
        std::shared_ptr<BLAS>
    > blasVector;

    std::unordered_map<
        const Mesh*,
        std::shared_ptr<BLAS>
    > blasMap;

    TLAS tlas;

public:

    std::shared_ptr<BLAS> getBLAS(
        const Mesh* mesh
    );

    void recreateTLAS(
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

    template<typename NodeType>
    static void printBVH(
        const std::vector<NodeType>& nodes,
        uint32_t index = 0
    );
};


// =========================================================
// getBLAS
// =========================================================

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
)
{
    auto it = blasMap.find(mesh);

    if (it != blasMap.end())
        return it->second;

    std::shared_ptr<BLAS> blas = std::make_shared<BLAS>();
    BLAS* as = blas.get();

    BLASInstanceBuilder::build(
        *mesh,
        as->nodes,
        as->instances
    );

    as->index = blasVector.size();

    blasVector.emplace_back(
        blas
    );

    blasMap.emplace(
        mesh,
        blas
    );
    return blas;
}

// =========================================================
// recreateTLAS
// =========================================================

template<
    typename TLBuilderType,
    typename BLBuilderType
>
void
AccelerationStructureManager<
    TLBuilderType,
    BLBuilderType
>::recreateTLAS(
    const std::vector<TLASBuildInput>& inputs
)
{
    tlas.nodes.clear();
    tlas.instances.clear();

    if (inputs.empty())
    {
        std::cout
            << "No instances to build TLAS"
            << std::endl;

        return;
    }

    std::vector<uint32_t> blasIndices;

    blasIndices.reserve(
        inputs.size()
    );

    for (const TLASBuildInput& input : inputs)
    {
        if (!input.blas)
        {
            throw std::runtime_error(
                "TLAS input contains null BLAS"
            );
        }

        blasIndices.emplace_back(
            input.blas->index
        );
    }

    std::vector<PrimitiveRef> primitives;

    TLASInstanceBuilder::build(
        inputs,
        blasIndices,
        primitives,
        tlas.nodes,
        tlas.instances
    );
}

// =========================================================
// debug
// =========================================================

template<
    typename TLBuilderType,
    typename BLBuilderType
>
template<typename NodeType>
void
AccelerationStructureManager<
    TLBuilderType,
    BLBuilderType
>::printBVH(
    const std::vector<NodeType>& nodes,
    uint32_t index
)
{
    if (index >= nodes.size())
        return;

    const NodeType& node = nodes[index];

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