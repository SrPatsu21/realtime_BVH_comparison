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

#include "gpu/AccelerationStructureGPU.hpp"

#include "../../batch/mesh/Mesh.hpp"
#include "../../BufferManager.hpp"

template<
    typename TLBuilderType,
    typename BLBuilderType
>
class AccelerationStructureManager
{
public:

    BufferManager* bufferManager;

    static constexpr uint32_t BLAS_NODE_CAPACITY = 1000000;
    static constexpr uint32_t BLAS_INSTANCE_CAPACITY = 500000;
    static constexpr uint32_t TLAS_NODE_CAPACITY = 100000;
    static constexpr uint32_t TLAS_INSTANCE_CAPACITY = 50000;

    using TLNodeType = typename TLBuilderType::NodeType;
    using BLNodeType = typename BLBuilderType::NodeType;

    using BLAS = AS<BLNodeType, BLASInstance>;
    using TLAS = AS<TLNodeType, TLASInstance>;

private:

    std::vector<std::shared_ptr<BLAS>> blasVector;

    std::unordered_map<
        const Mesh*,
        std::shared_ptr<BLAS>
    > blasMap;

    TLAS tlas;

    // GPU
    AccelerationStructureGPU* blasBuffer;
    AccelerationStructureGPU* blasInstanceBuffer;
    AccelerationStructureGPU* tlasGPU;
    AccelerationStructureGPU* tlasInstanceGPU;

    uint32_t uploadedBLASCount = 0;

    uint32_t blasNodeCount = 0;
    uint32_t blasInstanceCount = 0;

    void uploadBLAS();
    void uploadTLAS();

public:

    explicit AccelerationStructureManager(
        BufferManager* bufferManager
    );

    ~AccelerationStructureManager();

    AccelerationStructureManager(
        const AccelerationStructureManager&
    ) = delete;

    AccelerationStructureManager& operator=(
        const AccelerationStructureManager&
    ) = delete;

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
// constructor and destructor
// =========================================================

template<
    typename TLBuilderType,
    typename BLBuilderType
>
AccelerationStructureManager<
    TLBuilderType,
    BLBuilderType
>::AccelerationStructureManager(
    BufferManager* bufferManager
)
    : bufferManager(bufferManager)
{
    if (!bufferManager)
    {
        throw std::invalid_argument(
            "AccelerationStructureManager: "
            "bufferManager is null"
        );
    }

    blasBuffer = new AccelerationStructureGPU();
    blasInstanceBuffer = new AccelerationStructureGPU();

    tlasGPU = new AccelerationStructureGPU();
    tlasInstanceGPU = new AccelerationStructureGPU();

    VkDevice device = bufferManager->getDevice();

    // =====================================================
    // BLAS NODE BUFFER
    // =====================================================

    bufferManager->createBuffer(
        static_cast<VkDeviceSize>(BLAS_NODE_CAPACITY) * sizeof(BLNodeType),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        blasBuffer->buffer
    );

    bufferManager->allocateBufferMemory(blasBuffer->buffer, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, blasBuffer->memory, true);

    if (vkBindBufferMemory(device, blasBuffer->buffer, blasBuffer->memory, 0) != VK_SUCCESS)
        throw std::runtime_error("failed to bind BLAS node buffer memory!");

    blasBuffer->address = bufferManager->getBufferDeviceAddress(blasBuffer->buffer);

    // =====================================================
    // BLAS INSTANCE BUFFER
    // =====================================================

    bufferManager->createBuffer(
        static_cast<VkDeviceSize>(BLAS_INSTANCE_CAPACITY) * sizeof(BLASInstance),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        blasInstanceBuffer->buffer
    );

    bufferManager->allocateBufferMemory(blasInstanceBuffer->buffer, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, blasInstanceBuffer->memory, true);

    if (vkBindBufferMemory( device, blasInstanceBuffer->buffer, blasInstanceBuffer->memory, 0) != VK_SUCCESS)
        throw std::runtime_error("failed to bind BLAS instance buffer memory!");

    blasInstanceBuffer->address = bufferManager->getBufferDeviceAddress(blasInstanceBuffer->buffer);

    // =====================================================
    // TLAS NODE BUFFER
    // =====================================================

    bufferManager->createBuffer(
        static_cast<VkDeviceSize>(TLAS_NODE_CAPACITY) * sizeof(TLNodeType),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        tlasGPU->buffer
    );

    bufferManager->allocateBufferMemory(tlasGPU->buffer, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, tlasGPU->memory, true);

    if (vkBindBufferMemory(device, tlasGPU->buffer, tlasGPU->memory, 0) != VK_SUCCESS)
        throw std::runtime_error("failed to bind TLAS node buffer memory!");

    tlasGPU->address = bufferManager->getBufferDeviceAddress(tlasGPU->buffer);

    // =====================================================
    // TLAS INSTANCE BUFFER
    // =====================================================

    bufferManager->createBuffer(
        static_cast<VkDeviceSize>(TLAS_INSTANCE_CAPACITY) * sizeof(TLASInstance),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        tlasInstanceGPU->buffer
    );

    bufferManager->allocateBufferMemory(tlasInstanceGPU->buffer, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, tlasInstanceGPU->memory, true);

    if (vkBindBufferMemory(device, tlasInstanceGPU->buffer, tlasInstanceGPU->memory, 0) != VK_SUCCESS)
        throw std::runtime_error("failed to bind TLAS instance buffer memory!");

    tlasInstanceGPU->address = bufferManager->getBufferDeviceAddress(tlasInstanceGPU->buffer);
}

template<
    typename TLBuilderType,
    typename BLBuilderType
>
AccelerationStructureManager<
    TLBuilderType,
    BLBuilderType
>::~AccelerationStructureManager()
{
    VkDevice device =
        bufferManager->getDevice();

    if (blasBuffer)
    {
        blasBuffer->destroy(device);
        delete blasBuffer;
        blasBuffer = nullptr;
    }

    if (blasInstanceBuffer)
    {
        blasInstanceBuffer->destroy(device);
        delete blasInstanceBuffer;
        blasInstanceBuffer = nullptr;
    }

    if (tlasGPU)
    {
        tlasGPU->destroy(device);
        delete tlasGPU;
        tlasGPU = nullptr;
    }

    if (tlasInstanceGPU)
    {
        tlasInstanceGPU->destroy(device);
        delete tlasInstanceGPU;
        tlasInstanceGPU = nullptr;
    }
}


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

    uploadBLAS();

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

    uploadTLAS();
}

// =========================================================
// upload
// =========================================================

template<
    typename TLBuilderType,
    typename BLBuilderType
>
void
AccelerationStructureManager<
    TLBuilderType,
    BLBuilderType
>::uploadBLAS()
{
    if (uploadedBLASCount >= blasVector.size())
        return;

    uint32_t nodeOffset = blasNodeCount;

    uint32_t instanceOffset = blasInstanceCount;

    for (
        uint32_t i = uploadedBLASCount;
        i < blasVector.size();
        ++i
    )
    {
        BLAS& blas = *blasVector[i];

        blas.nodeOffset = nodeOffset;
        blas.nodeCount = static_cast<uint32_t>(blas.nodes.size());

        blas.instanceOffset = instanceOffset;
        blas.instanceCount = static_cast<uint32_t>(blas.instances.size());

        nodeOffset += blas.nodeCount;
        instanceOffset += blas.instanceCount;

        if (!blas.nodes.empty())
        {
            VkDeviceSize offset =
                static_cast<VkDeviceSize>(
                    blas.nodeOffset
                ) * sizeof(BLNodeType);

            VkDeviceSize size =
                static_cast<VkDeviceSize>(
                    blas.nodes.size()
                ) * sizeof(BLNodeType);

            bufferManager->uploadBuffer(
                blasBuffer->buffer,
                blas.nodes.data(),
                size,
                offset
            );
        }

        if (!blas.instances.empty())
        {
            VkDeviceSize offset =
                static_cast<VkDeviceSize>(
                    blas.instanceOffset
                ) * sizeof(BLASInstance);

            VkDeviceSize size =
                static_cast<VkDeviceSize>(
                    blas.instances.size()
                ) * sizeof(BLASInstance);

            bufferManager->uploadBuffer(
                blasInstanceBuffer->buffer,
                blas.instances.data(),
                size,
                offset
            );
        }
    }

    blasNodeCount = nodeOffset;

    blasInstanceCount = instanceOffset;

    uploadedBLASCount = static_cast<uint32_t>(blasVector.size());
}


template<
    typename TLBuilderType,
    typename BLBuilderType
>
void
AccelerationStructureManager<
    TLBuilderType,
    BLBuilderType
>::uploadTLAS()
{
    tlas.nodeOffset = 0;

    tlas.nodeCount = static_cast<uint32_t>(
            tlas.nodes.size()
        );

    tlas.instanceOffset = 0;

    tlas.instanceCount = static_cast<uint32_t>(
            tlas.instances.size()
        );

    if (!tlas.nodes.empty())
    {
        VkDeviceSize size =
            static_cast<VkDeviceSize>(
                tlas.nodes.size()
            ) * sizeof(TLNodeType);

        bufferManager->uploadBuffer(
            tlasGPU->buffer,
            tlas.nodes.data(),
            size,
            0
        );
    }

    if (!tlas.instances.empty())
    {
        VkDeviceSize size =
            static_cast<VkDeviceSize>(
                tlas.instances.size()
            ) * sizeof(TLASInstance);

        bufferManager->uploadBuffer(
            tlasInstanceGPU->buffer,
            tlas.instances.data(),
            size,
            0
        );
    }
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