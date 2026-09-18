#pragma once

#include <iomanip>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include "cpu/AS.hpp"
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

    std::shared_ptr<BLAS> createBLAS(
        const Mesh* mesh,
        const std::vector<Vertex>& vertices,
        const std::vector<uint32_t>& indices,
        const std::vector<SubMesh>& subMeshes
    );

    // get

    TLAS& getTLAS() { return tlas; }

    const TLAS& getTLAS() const { return tlas; }

    AccelerationStructureGPU* getBLASBuffer() { return blasBuffer; }
    AccelerationStructureGPU* getBLASInstanceBuffer() { return blasInstanceBuffer; }
    AccelerationStructureGPU* getTLASGPU() { return tlasGPU; }
    AccelerationStructureGPU* getTLASInstanceGPU() { return tlasInstanceGPU; }

    // Debug

    template<typename NodeType>
    static void printBVH(
        const std::vector<NodeType>& nodes,
        uint32_t index = 0
    );

    static void printBLAS(
        const BLAS& blas
    );


    static void printTLAS(
        const TLAS& tlas
    );

    template<typename T>
    static void printRawBytes(
        const T& value
    );

    template<typename T>
    static void printVectorRawBytes(
        const std::vector<T>& values
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

    if (it == blasMap.end())
        return nullptr;

    return it->second;
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
>::createBLAS(
    const Mesh* mesh,
    const std::vector<Vertex>& vertices,
    const std::vector<uint32_t>& indices,
    const std::vector<SubMesh>& subMeshes
)
{
    auto it = blasMap.find(mesh);

    if (it != blasMap.end())
        return it->second;

    std::shared_ptr<BLAS> blas =
        std::make_shared<BLAS>();

    BLASInstanceBuilder::build(
        vertices,
        indices,
        subMeshes,
        blas->nodes,
        blas->instances
    );

    if (blas->nodes.empty())
        throw std::runtime_error("AccelerationStructureManager: BLAS contains no nodes");

    blas->index = static_cast<uint32_t>(blasVector.size());

    blasVector.emplace_back(
        blas
    );

    blasMap.emplace(
        mesh,
        blas
    );

    uploadBLAS();

    // printBLAS(
    //     *blas.get()
    // );

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
            throw std::runtime_error("TLAS input contains null BLAS");

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

    for (TLASInstance& instance : tlas.instances)
    {
        if (instance.blasIndex >= blasVector.size())
            throw std::runtime_error("TLAS instance contains invalid BLAS index");

        const BLAS& blas = *blasVector[instance.blasIndex];
        instance.nodeOffset = blas.nodeOffset;
        instance.nodeCount = blas.nodeCount;
        instance.instanceOffset = blas.instanceOffset;
    }

    uploadTLAS();

    // printTLAS(
    //     tlas
    // );
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
    tlas.nodeCount = static_cast<uint32_t>(
            tlas.nodes.size()
        );

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
    {
        std::cout
            << "    INVALID NODE INDEX: "
            << index
            << '\n';

        return;
    }

    const NodeType& node =
        nodes[index];

    std::cout
        << "\n----------------------------------------\n";

    std::cout
        << "NODE ["
        << index
        << "]\n";

    std::cout
        << "  sizeof(NodeType): "
        << sizeof(NodeType)
        << " bytes\n";

    std::cout
        << "  bounds.min = ("
        << node.bounds.min.x
        << ", "
        << node.bounds.min.y
        << ", "
        << node.bounds.min.z
        << ")\n";

    std::cout
        << "  bounds.max = ("
        << node.bounds.max.x
        << ", "
        << node.bounds.max.y
        << ", "
        << node.bounds.max.z
        << ")\n";

    std::cout
        << "  leaf = "
        << node.leaf
        << '\n';



    if constexpr (
        std::is_same_v<
            NodeType,
            BVH8Node
        >
    )
    {
        std::cout
            << "  childCount = "
            << node.childCount
            << '\n';

        for (
            uint32_t i = 0;
            i < node.childCount &&
            i < 8;
            ++i
        )
        {
            std::cout
                << "  children["
                << i
                << "] = "
                << node.children[i]
                << '\n';
        }

        std::cout
            << "  unused children:\n";

        for (
            uint32_t i = node.childCount;
            i < 8;
            ++i
        )
        {
            std::cout
                << "    children["
                << i
                << "] = "
                << node.children[i]
                << '\n';
        }
    }
    else
    {
        std::cout
            << "  left = "
            << node.left
            << '\n';

        std::cout
            << "  right = "
            << node.right
            << '\n';
    }

    std::cout
        << "  RAW BYTES:\n";

    printRawBytes(
        node
    );

    std::cout
        << "----------------------------------------\n";
}

template<
    typename TLBuilderType,
    typename BLBuilderType
>
void
AccelerationStructureManager<
    TLBuilderType,
    BLBuilderType
>::printBLAS(
    const BLAS& blas
)
{
    std::cout
        << "\n"
        << "========================================\n"
        << "                  BLAS\n"
        << "========================================\n";

    std::cout
        << "BLAS index: "
        << blas.index
        << '\n';

    std::cout
        << "sizeof(BLAS): "
        << sizeof(BLAS)
        << " bytes\n";

    std::cout
        << "\n--- GPU OFFSETS ---\n";

    std::cout
        << "nodeOffset: "
        << blas.nodeOffset
        << '\n';

    std::cout
        << "nodeCount: "
        << blas.nodeCount
        << '\n';

    std::cout
        << "instanceOffset: "
        << blas.instanceOffset
        << '\n';

    std::cout
        << "instanceCount: "
        << blas.instanceCount
        << '\n';

    std::cout
        << "\n--- CPU VECTORS ---\n";

    std::cout
        << "nodes.size(): "
        << blas.nodes.size()
        << '\n';

    std::cout
        << "nodes.capacity(): "
        << blas.nodes.capacity()
        << '\n';

    std::cout
        << "nodes bytes: "
        << blas.nodes.size() * sizeof(BLNodeType)
        << '\n';

    std::cout
        << "instances.size(): "
        << blas.instances.size()
        << '\n';

    std::cout
        << "instances.capacity(): "
        << blas.instances.capacity()
        << '\n';

    std::cout
        << "instances bytes: "
        << blas.instances.size() * sizeof(BLASInstance)
        << '\n';

    std::cout
        << "\n--- NODE TYPE ---\n";

    std::cout
        << "sizeof(BLNodeType): "
        << sizeof(BLNodeType)
        << '\n';

    std::cout
        << "\n--- INSTANCE TYPE ---\n";

    std::cout
        << "sizeof(BLASInstance): "
        << sizeof(BLASInstance)
        << '\n';

    std::cout
        << "\n--- NODES ---\n";

    for (
        uint32_t i = 0;
        i < blas.nodes.size();
        ++i
    )
    {
        const auto& node =
            blas.nodes[i];

        std::cout
            << "\nNODE ["
            << i
            << "]\n";

        std::cout
            << "  address: "
            << static_cast<const void*>(&node)
            << '\n';

        std::cout
            << "  min = ("
            << node.bounds.min.x
            << ", "
            << node.bounds.min.y
            << ", "
            << node.bounds.min.z
            << ")\n";

        std::cout
            << "  max = ("
            << node.bounds.max.x
            << ", "
            << node.bounds.max.y
            << ", "
            << node.bounds.max.z
            << ")\n";

        std::cout
            << "  leaf = "
            << node.leaf
            << '\n';



        if constexpr (
            std::is_same_v<
                BLNodeType,
                BVH8Node
            >
        )
        {
            std::cout
                << "  childCount = "
                << node.childCount
                << '\n';

            for (
                uint32_t child = 0;
                child < node.childCount &&
                child < 8;
                ++child
            )
            {
                std::cout
                    << "  children["
                    << child
                    << "] = "
                    << node.children[child]
                    << '\n';
            }
        }
        else
        {
            std::cout
                << "  left = "
                << node.left
                << '\n';

            std::cout
                << "  right = "
                << node.right
                << '\n';
        }

        std::cout
            << "  RAW:\n";

        printRawBytes(
            node
        );
    }

    std::cout
        << "\n--- BLAS INSTANCES ---\n";

    for (
        uint32_t i = 0;
        i < blas.instances.size();
        ++i
    )
    {
        const BLASInstance& instance =
            blas.instances[i];

        std::cout
            << "\nINSTANCE ["
            << i
            << "]\n";

        std::cout
            << "  address: "
            << static_cast<const void*>(&instance)
            << '\n';

        std::cout
            << "  min = ("
            << instance.bounds.min.x
            << ", "
            << instance.bounds.min.y
            << ", "
            << instance.bounds.min.z
            << ")\n";

        std::cout
            << "  max = ("
            << instance.bounds.max.x
            << ", "
            << instance.bounds.max.y
            << ", "
            << instance.bounds.max.z
            << ")\n";

        std::cout
            << "  firstTriangle = "
            << instance.firstTriangle
            << '\n';

        std::cout
            << "  triangleCount = "
            << instance.triangleCount
            << '\n';

        std::cout
            << "  materialOffset = "
            << instance.materialOffset
            << '\n';

        std::cout
            << "  sizeof(BLASInstance) = "
            << sizeof(BLASInstance)
            << " bytes\n";

        std::cout
            << "  RAW:\n";

        printRawBytes(
            instance
        );
    }

    std::cout
        << "\n========================================\n";
}

template<
    typename TLBuilderType,
    typename BLBuilderType
>
template<typename T>
void
AccelerationStructureManager<
    TLBuilderType,
    BLBuilderType
>::printRawBytes(
    const T& value
)
{
    const uint8_t* bytes =
        reinterpret_cast<const uint8_t*>(&value);

    std::cout
        << "    sizeof: "
        << sizeof(T)
        << " bytes\n";

    for (
        size_t i = 0;
        i < sizeof(T);
        ++i
    )
    {
        if (i % 16 == 0)
        {
            std::cout
                << "    "
                << std::setw(4)
                << std::setfill('0')
                << std::hex
                << i
                << ": ";
        }

        std::cout
            << std::setw(2)
            << std::setfill('0')
            << std::hex
            << static_cast<uint32_t>(bytes[i])
            << " ";

        if (i % 16 == 15 || i == sizeof(T) - 1)
        {
            std::cout
                << std::dec
                << '\n';
        }
    }

    std::cout
        << std::dec
        << std::setfill(' ');
}

template<
    typename TLBuilderType,
    typename BLBuilderType
>
template<typename T>
void
AccelerationStructureManager<
    TLBuilderType,
    BLBuilderType
>::printVectorRawBytes(
    const std::vector<T>& values
)
{
    std::cout
        << "    vector size: "
        << values.size()
        << '\n';

    std::cout
        << "    element size: "
        << sizeof(T)
        << " bytes\n";

    std::cout
        << "    total bytes: "
        << values.size() * sizeof(T)
        << '\n';

    for (
        size_t i = 0;
        i < values.size();
        ++i
    )
    {
        std::cout
            << "\n    element["
            << i
            << "]\n";

        printRawBytes(
            values[i]
        );
    }
}


template<
    typename TLBuilderType,
    typename BLBuilderType
>
void
AccelerationStructureManager<
    TLBuilderType,
    BLBuilderType
>::printTLAS(
    const TLAS& tlas
)
{
    std::cout
        << "\n"
        << "========================================\n"
        << "                  TLAS\n"
        << "========================================\n";

    std::cout
        << "sizeof(TLAS): "
        << sizeof(TLAS)
        << " bytes\n";

    std::cout
        << "sizeof(TLNodeType): "
        << sizeof(TLNodeType)
        << " bytes\n";

    std::cout
        << "sizeof(TLASInstance): "
        << sizeof(TLASInstance)
        << " bytes\n";

    // =====================================================
    // GPU OFFSETS
    // =====================================================

    std::cout
        << "\n--- GPU OFFSETS ---\n";

    std::cout
        << "nodeOffset: "
        << tlas.nodeOffset
        << '\n';

    std::cout
        << "nodeCount: "
        << tlas.nodeCount
        << '\n';

    std::cout
        << "instanceOffset: "
        << tlas.instanceOffset
        << '\n';

    std::cout
        << "instanceCount: "
        << tlas.instanceCount
        << '\n';

    // =====================================================
    // CPU VECTORS
    // =====================================================

    std::cout
        << "\n--- CPU VECTORS ---\n";

    std::cout
        << "nodes.size(): "
        << tlas.nodes.size()
        << '\n';

    std::cout
        << "nodes.capacity(): "
        << tlas.nodes.capacity()
        << '\n';

    std::cout
        << "nodes bytes: "
        << tlas.nodes.size() * sizeof(TLNodeType)
        << '\n';

    std::cout
        << "instances.size(): "
        << tlas.instances.size()
        << '\n';

    std::cout
        << "instances.capacity(): "
        << tlas.instances.capacity()
        << '\n';

    std::cout
        << "instances bytes: "
        << tlas.instances.size() * sizeof(TLASInstance)
        << '\n';

    // =====================================================
    // TLAS NODES
    // =====================================================

    std::cout
        << "\n"
        << "========================================\n"
        << "              TLAS NODES\n"
        << "========================================\n";

    for (
        uint32_t i = 0;
        i < tlas.nodes.size();
        ++i
    )
    {
        const auto& node =
            tlas.nodes[i];

        std::cout
            << "\nNODE ["
            << i
            << "]\n";

        std::cout
            << "  address: "
            << static_cast<const void*>(&node)
            << '\n';

        std::cout
            << "  offset in vector: "
            << i * sizeof(TLNodeType)
            << " bytes\n";

        std::cout
            << "  bounds.min = ("
            << node.bounds.min.x
            << ", "
            << node.bounds.min.y
            << ", "
            << node.bounds.min.z
            << ")\n";

        std::cout
            << "  bounds.max = ("
            << node.bounds.max.x
            << ", "
            << node.bounds.max.y
            << ", "
            << node.bounds.max.z
            << ")\n";

        std::cout
            << "  leaf = "
            << node.leaf
            << '\n';



        if constexpr (
            std::is_same_v<
                TLNodeType,
                BVH8Node
            >
        )
        {
            std::cout
                << "  childCount = "
                << node.childCount
                << '\n';

            for (
                uint32_t child = 0;
                child < node.childCount &&
                child < 8;
                ++child
            )
            {
                std::cout
                    << "  children["
                    << child
                    << "] = "
                    << node.children[child]
                    << '\n';

                std::cout
                    << "  children["
                    << child
                    << "] valid = "
                    << (
                        node.children[child] <
                        (
                            node.leaf ?
                            tlas.instances.size() :
                            tlas.nodes.size()
                        )
                    )
                    << '\n';
            }

            std::cout
                << "  unused children:\n";

            for (
                uint32_t child = node.childCount;
                child < 8;
                ++child
            )
            {
                std::cout
                    << "    children["
                    << child
                    << "] = "
                    << node.children[child]
                    << '\n';
            }
        }
        else
        {
            std::cout
                << "  left = "
                << node.left
                << '\n';

            std::cout
                << "  right = "
                << node.right
                << '\n';

            if (node.leaf)
            {
                std::cout
                    << "  left valid = "
                    << (
                        node.left <
                        tlas.instances.size()
                    )
                    << '\n';

                std::cout
                    << "  right valid = "
                    << (
                        node.right <
                        tlas.instances.size()
                    )
                    << '\n';
            }
            else
            {
                std::cout
                    << "  left valid = "
                    << (
                        node.left <
                        tlas.nodes.size()
                    )
                    << '\n';

                std::cout
                    << "  right valid = "
                    << (
                        node.right <
                        tlas.nodes.size()
                    )
                    << '\n';
            }
        }

        std::cout
            << "  RAW BYTES:\n";

        printRawBytes(
            node
        );
    }

    // =====================================================
    // TLAS INSTANCES
    // =====================================================

    std::cout
        << "\n"
        << "========================================\n"
        << "           TLAS INSTANCES\n"
        << "========================================\n";

    for (
        uint32_t i = 0;
        i < tlas.instances.size();
        ++i
    )
    {
        const TLASInstance& instance =
            tlas.instances[i];

        std::cout
            << "\nINSTANCE ["
            << i
            << "]\n";

        std::cout
            << "  address: "
            << static_cast<const void*>(&instance)
            << '\n';

        std::cout
            << "  offset in vector: "
            << i * sizeof(TLASInstance)
            << " bytes\n";

        std::cout
            << "  sizeof: "
            << sizeof(TLASInstance)
            << " bytes\n";

        std::cout
            << "  blasIndex = "
            << instance.blasIndex
            << '\n';

        std::cout
            << "  nodeOffset = "
            << instance.nodeOffset
            << '\n';

        std::cout
            << "  nodeCount = "
            << instance.nodeCount
            << '\n';

        std::cout
            << "  instanceOffset = "
            << instance.instanceOffset
            << '\n';

        std::cout
            << "  RAW BYTES:\n";

        printRawBytes(
            instance
        );
    }

    // =====================================================
    // RAW VECTOR DATA
    // =====================================================

    std::cout
        << "\n"
        << "========================================\n"
        << "          TLAS RAW VECTOR DATA\n"
        << "========================================\n";

    std::cout
        << "\n--- NODES RAW ---\n";

    printVectorRawBytes(
        tlas.nodes
    );

    std::cout
        << "\n--- INSTANCES RAW ---\n";

    printVectorRawBytes(
        tlas.instances
    );

    std::cout
        << "\n"
        << "========================================\n"
        << "             TLAS END\n"
        << "========================================\n";
}