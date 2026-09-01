#pragma once

#include <deque>
#include <vector>
#include <cstdint>

#include "TLAS.hpp"
#include "BLAS.hpp"
#include "BVHNode.hpp"
#include "primitives/TLASBuildInstance.hpp"

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
        std::vector<TLASBuildInstance>& instances,
        BufferManager* bufferManager,
        TLAS<TLNodeType>& tlas
    );

private:

    void buildVulkanAccelerationStructureGPU(
        const std::vector<TLASBuildInstance>& instances,
        BufferManager* bufferManager,
        AccelerationStructureGPU& gpu
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
    std::vector<TLASBuildInstance>& instances,
    BufferManager* bufferManager,
    TLAS<TLNodeType>& tlas
)
{
    tlas.accelerationStructure.nodes.clear();

    if (!instances.size())
    {
        std::cout
            << "No instances to build TLAS"
            << std::endl;

        return;
    }

    // =========================================================
    // Build CPU TLAS
    // =========================================================

    TLBuilderType::build(
        tlas.accelerationStructure.nodes,
        instances
    );

    // =========================================================
    // Upload TLAS nodes
    // =========================================================

    buildVulkanTLAS(
        bufferManager,
        tlas
    );

    // =========================================================
    // Build GPU acceleration data
    // =========================================================

    buildVulkanAccelerationStructureGPU(
        instances,
        bufferManager,
        tlas.gpuData
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