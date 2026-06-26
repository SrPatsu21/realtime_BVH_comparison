#pragma once

#include <vector>
#include <string>
#include <stdexcept>
#include <memory>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "VertexBufferManager.hpp"
#include "IndexBufferManager.hpp"

/**
 * @class Mesh
 * @brief Loads a 3D model using Assimp and builds GPU vertex/index buffers.
 *
 * Mesh is responsible for:
 * - Loading model data from disk via Assimp
 * - Extracting materials and texture paths
 * - Extracting vertex attributes (position, normal, tangent, UV)
 * - Building submesh ranges
 * - Creating GPU vertex and index buffers
 *
 * Supports multiple submeshes and materials per model.
 */
class Mesh {
public:

    /**
     * @struct MaterialData
     * @brief Stores texture paths for a material.
     *
     * These paths are extracted from Assimp materials and can be used
     * to create TextureAsset / TextureImage instances.
     */
    struct MaterialData {
        std::string baseColorPath;
        std::string normalPath;
        std::string metallicRoughnessPath;
    };

        /**
     * @struct SubMesh
     * @brief Represents a drawable submesh range.
     *
     * A submesh defines:
     * - An index range inside the global index buffer
     * - A material index
     *
     * Rendering typically binds the material corresponding to
     * materialIndex before drawing this submesh.
     */
    struct SubMesh {
        uint32_t firstIndex;
        uint32_t indexCount;
        int32_t  vertexOffset;
        uint32_t materialIndex;
    };

protected:
    std::unique_ptr<VertexBufferManager> vertexBufferManager;
    std::unique_ptr<IndexBufferManager> indexBufferManager;

    std::vector<SubMesh> subMeshes;
    std::vector<MaterialData> materials;

    std::vector<Vertex> cpuVertices;
    std::vector<uint32_t> cpuIndices;

    uint32_t indexCount = 0;

    /**
     * @brief Loads model data from file using Assimp.
     *
     * Performs:
     * - Triangulation
     * - UV flipping
     * - Normal generation
     * - Tangent space calculation
     * - Vertex deduplication
     *
     * Extracts:
     * - Vertex attributes
     * - Index data
     * - Submesh ranges
     * - Material texture paths
     *
     * @param path Path to model file.
     * @param vertices Output vertex array.
     * @param indices Output index array.
     * @throws std::runtime_error on load failure.
     */
    void load(
        const std::string& path,
        std::vector<Vertex>& vertices,
        std::vector<uint32_t>& indices
    );

public:
    /**
     * @brief Constructs a Mesh from a model file.
     *
     * Steps:
     * - Loads model using Assimp
     * - Creates GPU vertex buffer
     * - Creates GPU index buffer
     *
     * @param path Path to model file.
     * @param device Vulkan logical device.
     * @param bufferManager Buffer manager used for staging and uploads.
     */
    explicit Mesh(
        const std::string& path,
        VkDevice device,
        BufferManager* bufferManager
    );

    ~Mesh() = default;

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    Mesh(Mesh&&) noexcept = delete;
    Mesh& operator=(Mesh&&) noexcept = delete;


    VkBuffer getIndexBuffer() const { return indexBufferManager->getIndexBuffer(); }
    VkBuffer getVertexBuffer() const { return vertexBufferManager->getVertexBuffer(); }

    const std::vector<Mesh::SubMesh>& getSubMeshes() const { return subMeshes; }
    const std::vector<Mesh::MaterialData>& getMaterials() const { return materials; }

    const std::vector<Vertex>& getVertices() const { return cpuVertices; }
    const std::vector<uint32_t>& getIndices() const { return cpuIndices; }

    uint32_t getIndexCount() const { return indexCount; }
};
