#pragma once

#include <vector>
#include <string>

#include "Vertex.hpp"
#include "SubMesh.hpp"

#include <glm/vec4.hpp>

struct MaterialImportData
{
    std::string baseColorPath;
    std::string normalPath;
    std::string metallicRoughnessPath;

    glm::vec4 baseColorFactor{1.0f};

    float metallicFactor = 1.0f;
    float roughnessFactor = 1.0f;

    uint32_t alphaMode = 1;
    float alphaCutoff = 0.5f;

    bool doubleSided = false;
};

struct MeshImportData
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    std::vector<SubMesh> subMeshes;

    std::vector<MaterialImportData> materials;
};