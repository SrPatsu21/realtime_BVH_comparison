#pragma once

#include <deque>
#include <vector>
#include <cstdint>

#include "TLAS.hpp"
#include "BLAS.hpp"
#include "BVHNode.hpp"
#include "primitives/BLASInstance.hpp"

#include "../../batch/mesh/Mesh.hpp"

#include "vulkan/BuildVulkanBLAS.hpp"
#include "vulkan/BuildVulkanTLAS.hpp"

template<
    typename TLBuilderType,
    typename BLBuilderType
>
class AccelerationStructureManager
{
public:

    using TLNodeType = typename TLBuilderType::NodeType;
    using BLNodeType = typename BLBuilderType::NodeType;

    template<typename Primitive>
    void createBLAS(
        const Mesh* mesh,
        std::vector<Primitive>& primitives,
        BufferManager* bufferManager,
        BLAS<BLNodeType>& blas
    );

    void createTLAS(
        std::vector<BLASInstance>& instances,
        BufferManager* bufferManager,
        TLAS<TLNodeType>& tlas
    );
};

//*======================
//* buildBLAS
//*======================

template<
    typename TLBuilderType,
    typename BLBuilderType
>
template<typename Primitive>
void
AccelerationStructureManager<
    TLBuilderType,
    BLBuilderType
>::createBLAS(
    const Mesh* mesh,
    std::vector<Primitive>& primitives,
    BufferManager* bufferManager,
    BLAS<BLNodeType>& blas
)
{
    blas.mesh = mesh;

    blas.accelerationStructure.nodes.clear();
    BLBuilderType::build(
        blas.accelerationStructure.nodes,
        primitives
    );

    buildVulkanBLAS(
        bufferManager,
        blas
    );
}

//*======================
//* buildTLAS
//*======================

template<
    typename TLBuilderType,
    typename BLBuilderType
>
void
AccelerationStructureManager<
    TLBuilderType,
    BLBuilderType
>::createTLAS(
    std::vector<BLASInstance>& instances,
    BufferManager* bufferManager,
    TLAS<TLNodeType>& tlas
)
{
    tlas.accelerationStructure.nodes.clear();
    TLBuilderType::build(
        tlas.accelerationStructure.nodes,
        instances
    );

    buildVulkanTLAS(
        bufferManager,
        tlas
    );
}

//*======================
//* Helpers
//*======================

template<typename NodeType>
void printBVH(
    const std::vector<NodeType>& nodes,
    uint32_t index = 0
)
{
    if (index >= nodes.size())
        return;

    const NodeType& node = nodes[index];

    std::cout
        << "[" << index << "] "
        << "min=("
        << node.bounds.min.x << ", "
        << node.bounds.min.y << ", "
        << node.bounds.min.z << ") "
        << "max=("
        << node.bounds.max.x << ", "
        << node.bounds.max.y << ", "
        << node.bounds.max.z << ") ";

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
        printBVH(nodes, node.left);
        printBVH(nodes, node.right);
    }
}