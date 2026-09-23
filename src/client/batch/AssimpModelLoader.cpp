#include "AssimpModelLoader.hpp"

#include <filesystem>
#include <stdexcept>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/GltfMaterial.h>

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/geometric.hpp>

MeshImportData AssimpModelLoader::load(
    const std::string& path
)
{
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(
        path,
        aiProcess_Triangulate |
        aiProcess_FlipUVs |
        aiProcess_GenNormals |
        aiProcess_CalcTangentSpace |
        aiProcess_JoinIdenticalVertices |
        aiProcess_PreTransformVertices
    );

    if (!scene || !scene->mRootNode || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE))
    {
        throw std::runtime_error(
            importer.GetErrorString()
        );
    }

    MeshImportData data{};

    std::filesystem::path modelPath(path);
    std::filesystem::path directory = modelPath.parent_path();

    // MATERIALS
    data.materials.resize(scene->mNumMaterials);

    for (unsigned int i = 0; i < scene->mNumMaterials; i++)
    {
        aiMaterial* mat = scene->mMaterials[i];

        MaterialImportData material{};

        // ---------------------------------------------------------
        // BASE COLOR FACTOR
        // ---------------------------------------------------------

        aiColor4D baseColor;

        if (mat->Get(AI_MATKEY_BASE_COLOR, baseColor) == AI_SUCCESS)
        {
            material.baseColorFactor = {
                baseColor.r,
                baseColor.g,
                baseColor.b,
                baseColor.a
            };
        }

        // ---------------------------------------------------------
        // METALLIC FACTOR
        // ---------------------------------------------------------

        float metallicFactor = 1.0f;

        if (mat->Get(AI_MATKEY_METALLIC_FACTOR, metallicFactor) == AI_SUCCESS)
        {
            material.metallicFactor = metallicFactor;
        }

        // ---------------------------------------------------------
        // ROUGHNESS FACTOR
        // ---------------------------------------------------------

        float roughnessFactor = 1.0f;

        if (mat->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughnessFactor) == AI_SUCCESS)
        {
            material.roughnessFactor = roughnessFactor;
        }

        // ---------------------------------------------------------
        // TEXTURES
        // ---------------------------------------------------------

        aiString pathStr;

        if (mat->GetTextureCount(aiTextureType_BASE_COLOR) > 0)
        {
            if (mat->GetTexture(aiTextureType_BASE_COLOR, 0, &pathStr) == AI_SUCCESS)
            {
                material.baseColorPath = (directory / pathStr.C_Str()).string();
            }
        }

        if (mat->GetTextureCount(aiTextureType_NORMALS) > 0)
        {
            if (mat->GetTexture(aiTextureType_NORMALS, 0, &pathStr) == AI_SUCCESS)
            {
                material.normalPath = (directory / pathStr.C_Str()).string();
            }
        }

        if (mat->GetTextureCount(aiTextureType_METALNESS) > 0)
        {
            if (mat->GetTexture(aiTextureType_METALNESS, 0, &pathStr) == AI_SUCCESS)
            {
                material.metallicRoughnessPath = (directory / pathStr.C_Str()).string();
            }
        }

        // ---------------------------------------------------------
        // ALPHA MODE
        // ---------------------------------------------------------

        aiString alphaMode;

        if (mat->Get(AI_MATKEY_GLTF_ALPHAMODE, alphaMode) == AI_SUCCESS)
        {
            std::string mode = alphaMode.C_Str();

            if (mode == "MASK")
            {
                material.alphaMode = 2;
            }
            else if (mode == "BLEND")
            {
                material.alphaMode = 4;
            }
            else
            {
                material.alphaMode = 1;
            }
        }

        // ---------------------------------------------------------
        // ALPHA CUTOFF
        // ---------------------------------------------------------

        float alphaCutoff = 0.5f;

        if (mat->Get(AI_MATKEY_GLTF_ALPHACUTOFF, alphaCutoff) == AI_SUCCESS)
        {
            material.alphaCutoff = alphaCutoff;
        }

        // ---------------------------------------------------------
        // DOUBLE SIDED
        // ---------------------------------------------------------
        int doubleSided = 0;

        if (mat->Get(AI_MATKEY_TWOSIDED, doubleSided) == AI_SUCCESS)
        {
            material.doubleSided = doubleSided != 0;
        }

        data.materials[i] = material;
    }

    // MESHES
    for (unsigned int m = 0; m < scene->mNumMeshes; m++)
    {
        aiMesh* mesh = scene->mMeshes[m];

        uint32_t baseVertex =
            static_cast<uint32_t>(data.vertices.size());

        uint32_t indexOffset =
            static_cast<uint32_t>(data.indices.size());

        // VERTICES
        for (unsigned int v = 0; v < mesh->mNumVertices; v++)
        {
            glm::vec3 position{
                mesh->mVertices[v].x,
                mesh->mVertices[v].y,
                mesh->mVertices[v].z
            };

            glm::vec3 normal{0.0f};

            if (mesh->HasNormals())
            {
                normal = {
                    mesh->mNormals[v].x,
                    mesh->mNormals[v].y,
                    mesh->mNormals[v].z
                };
            }

            glm::vec4 tangent{0.0f};

            if (mesh->HasTangentsAndBitangents())
            {
                glm::vec3 t{
                    mesh->mTangents[v].x,
                    mesh->mTangents[v].y,
                    mesh->mTangents[v].z
                };

                glm::vec3 b{
                    mesh->mBitangents[v].x,
                    mesh->mBitangents[v].y,
                    mesh->mBitangents[v].z
                };

                float handedness = (glm::dot(glm::cross(normal, t), b) < 0.0f) ? -1.0f : 1.0f;

                tangent = glm::vec4(
                    t,
                    handedness
                );
            }

            glm::vec2 texCoord{0.0f};

            if (mesh->HasTextureCoords(0))
            {
                texCoord = {mesh->mTextureCoords[0][v].x, mesh->mTextureCoords[0][v].y};
            }

            data.vertices.emplace_back(
                position,
                normal,
                tangent,
                texCoord
            );
        }

        // INDICES
        for (unsigned int f = 0; f < mesh->mNumFaces; f++)
        {
            const aiFace& face = mesh->mFaces[f];
            data.indices.emplace_back(baseVertex + face.mIndices[0]);
            data.indices.emplace_back(baseVertex + face.mIndices[1]);
            data.indices.emplace_back(baseVertex + face.mIndices[2]);
        }

        // SUBMESH
        SubMesh sub{};
        sub.firstIndex = indexOffset;
        sub.indexCount = mesh->mNumFaces * 3;
        sub.vertexOffset = 0;
        sub.materialIndex = mesh->mMaterialIndex;
        data.subMeshes.push_back(sub);
    }

    return data;
}