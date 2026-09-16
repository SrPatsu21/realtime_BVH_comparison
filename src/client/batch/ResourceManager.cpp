#include "ResourceManager.hpp"

#include <stdexcept>

ResourceManager::ResourceManager(
    VkPhysicalDevice physicalDevice,
    VkDevice device,
    BufferManager* bufferManager
) :
    physicalDevice(physicalDevice),
    device(device),
    bufferManager(bufferManager),
    samplerManager(physicalDevice, device),
    textureManager(
        physicalDevice,
        device,
        bufferManager,
        &samplerManager,
        1024
    ),
    materialManager(
        device,
        bufferManager,
        &textureManager,
        1024,
        1024
    )
{
    accelerationStructureManager =
        new AccelerationStructureManager<
            DefaultTLASBuilder,
            DefaultBLASBuilder
        >(bufferManager);
}

std::shared_ptr<Mesh> ResourceManager::getMesh(
    const std::string& meshPath
)
{
    auto [it, inserted] = meshes.try_emplace(meshPath);

    if (!inserted)
    {
        if (auto mesh = it->second.lock())
            return mesh;
    }

    MeshImportData importData =
        AssimpModelLoader::load(meshPath);

    std::shared_ptr<Mesh> mesh =
        std::make_shared<Mesh>(
            device,
            bufferManager,
            importData.vertices,
            importData.indices,
            importData.subMeshes
        );

    std::vector<uint32_t> materialIndices;
    materialIndices.resize(
        importData.materials.size()
    );

    for (uint32_t i = 0; i < importData.materials.size(); i++)
    {
        const auto& materialData =
            importData.materials[i];

        MaterialData material{};

        material.baseColorFactor = materialData.baseColorFactor;
        material.metallicFactor = materialData.metallicFactor;
        material.roughnessFactor = materialData.roughnessFactor;

        material.alphaMode = static_cast<MaterialData::AlphaMode>(materialData.alphaMode);
        material.alphaCutoff = materialData.alphaCutoff;

        if (!materialData.baseColorPath.empty())
            material.baseColor = textureManager.createTexture(materialData.baseColorPath);

        if (!materialData.normalPath.empty())
            material.normal = textureManager.createTexture(materialData.normalPath);

        if (!materialData.metallicRoughnessPath.empty())
            material.metallicRoughness = textureManager.createTexture(materialData.metallicRoughnessPath);

        materialIndices[i] =
            materialManager.createMaterial(
                mesh.get(),
                i,
                material
            );
    }

    for (uint32_t i = 0; i < importData.subMeshes.size(); i++)
    {
        uint32_t assimpMaterialIndex = importData.subMeshes[i].materialIndex;

        if (assimpMaterialIndex >= materialIndices.size())
            throw std::runtime_error("Invalid material index in imported SubMesh");

        importData.subMeshes[i].materialIndex = materialIndices[assimpMaterialIndex];
    }


    mesh->setSubMeshes(importData.subMeshes);

    accelerationStructureManager->createBLAS(
        mesh.get(),
        importData.vertices,
        importData.indices,
        mesh.get()->getSubMeshes()
    );
    meshes[meshPath] = mesh;

    return mesh;
}

ResourceManager::~ResourceManager(){
    delete accelerationStructureManager;
}