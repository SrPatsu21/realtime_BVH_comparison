#pragma once

#include <vector>
#include <memory>

#include "VertexBufferManager.hpp"
#include "IndexBufferManager.hpp"
#include "SubMesh.hpp"

class Mesh
{
protected:
    std::unique_ptr<VertexBufferManager> vertexBufferManager;
    std::unique_ptr<IndexBufferManager> indexBufferManager;

    uint64_t vertexAddress;
    uint64_t indexAddress;

    std::vector<SubMesh> subMeshes;

public:

    Mesh(
        VkDevice device,
        BufferManager* bufferManager,
        const std::vector<Vertex>& vertices,
        const std::vector<uint32_t>& indices,
        const std::vector<SubMesh>& subMeshes
    );

    ~Mesh() = default;

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    Mesh(Mesh&&) noexcept = delete;
    Mesh& operator=(Mesh&&) noexcept = delete;

    void setSubMeshes(
        const std::vector<SubMesh>& subMeshes
    )
    {
        this->subMeshes = subMeshes;
    }

    VkBuffer getIndexBuffer() const { return indexBufferManager->getIndexBuffer(); }
    VkBuffer getVertexBuffer() const { return vertexBufferManager->getVertexBuffer(); }
    const std::vector<SubMesh>& getSubMeshes() const { return subMeshes; }
    uint64_t getIndexDeviceAddress() const { return indexAddress; }
    uint64_t getVertexDeviceAddress() const { return vertexAddress; }
};