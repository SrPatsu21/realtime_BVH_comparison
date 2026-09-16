#include "Mesh.hpp"

#include "Mesh.hpp"

Mesh::Mesh(
    VkDevice device,
    BufferManager* bufferManager,
    const std::vector<Vertex>& vertices,
    const std::vector<uint32_t>& indices,
    const std::vector<SubMesh>& subMeshes
) :
    subMeshes(subMeshes)
{
    vertexBufferManager =
        std::make_unique<VertexBufferManager>(
            device,
            bufferManager,
            vertices
        );

    indexBufferManager =
        std::make_unique<IndexBufferManager>(
            device,
            bufferManager,
            indices
        );

    // Address
    VkBufferDeviceAddressInfo info{
        VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO
    };
    info.buffer = vertexBufferManager->getVertexBuffer();

    vertexAddress = static_cast<uint64_t>(vkGetBufferDeviceAddress(device, &info));

    info.buffer = indexBufferManager->getIndexBuffer();
    indexAddress = static_cast<uint64_t>(vkGetBufferDeviceAddress(device, &info));
}