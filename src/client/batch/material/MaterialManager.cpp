#include "MaterialManager.hpp"
#include "../texture/TextureManager.hpp"

#include <stdexcept>

MaterialManager::MaterialManager(
    VkDevice device,
    BufferManager* bufferManager,
    TextureManager* textureManager,
    uint32_t maxMaterials,
    uint32_t maxTextures
) :
    device(device),
    bufferManager(bufferManager),
    maxMaterials(maxMaterials)
{
    #ifndef NDEBUG
    if (!bufferManager)
        throw std::invalid_argument(
            "MaterialManager: bufferManager is null"
        );

    if (!textureManager)
        throw std::invalid_argument(
            "MaterialManager: textureManager is null"
        );

    if (maxMaterials == 0)
        throw std::invalid_argument(
            "MaterialManager: maxMaterials must be greater than zero"
        );

    if (maxTextures == 0)
        throw std::invalid_argument(
            "MaterialManager: maxTextures must be greater than zero"
        );
    #endif

    materials.reserve(maxMaterials);

    materialBuffer =
        bufferManager->createDeviceBuffer(
            sizeof(MaterialGPU) * maxMaterials,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            materialMemory,
            nullptr
        );

    descriptorManager =
        std::make_unique<MaterialDescriptorManager>(
            device,
            materialBuffer,
            textureManager,
            maxTextures
        );
}

MaterialManager::~MaterialManager()
{
    descriptorManager.reset();

    if (materialBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(
            device,
            materialBuffer,
            nullptr
        );

        materialBuffer = VK_NULL_HANDLE;
    }

    if (materialMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(
            device,
            materialMemory,
            nullptr
        );

        materialMemory = VK_NULL_HANDLE;
    }
}

uint32_t MaterialManager::createMaterial(
    Mesh* mesh,
    uint32_t aiMaterialIndex,
    const MaterialData& material
)
{
    MaterialKey key{
        mesh,
        aiMaterialIndex
    };

    auto found = materialIndices.find(key);

    if (found != materialIndices.end())
    {
        return found->second;
    }

    if (materials.size() >= maxMaterials)
        throw std::runtime_error("MaterialManager: maximum number of materials reached");

    uint32_t materialIndex = static_cast<uint32_t>(materials.size());

    MaterialGPU gpuMaterial{};
    gpuMaterial.baseColorFactor = material.baseColorFactor;
    gpuMaterial.metallicFactor = material.metallicFactor;
    gpuMaterial.roughnessFactor = material.roughnessFactor;
    gpuMaterial.alphaMode = static_cast<uint32_t>(material.alphaMode);
    gpuMaterial.alphaCutoff = material.alphaCutoff;
    gpuMaterial.doubleSided = material.doubleSided ? 1u : 0u;

    gpuMaterial.baseColorTexture =
        material.baseColor
        ? material.baseColor
        : 0;

    gpuMaterial.normalTexture =
        material.normal
        ? material.normal
        : 1;

    gpuMaterial.metallicRoughnessTexture =
        material.metallicRoughness
        ? material.metallicRoughness
        : 2;

    VkDeviceSize offset =
        static_cast<VkDeviceSize>(
            materialIndex
        ) * sizeof(MaterialGPU);

    VkDeviceSize size = sizeof(MaterialGPU);

    bufferManager->uploadBuffer(
        materialBuffer,
        &gpuMaterial,
        size,
        offset
    );

    MaterialCPU cpuMaterial{};

    cpuMaterial.materialOffset = materialIndex;
    cpuMaterial.alphaMode = material.alphaMode;
    cpuMaterial.baseColor = material.baseColor;
    cpuMaterial.normal = material.normal;
    cpuMaterial.metallicRoughness = material.metallicRoughness;

    materials.push_back(
        std::move(cpuMaterial)
    );

    materialIndices.emplace(
        std::move(key),
        materialIndex
    );

    descriptorManager->updateTexture(
        material.baseColor
    );
    descriptorManager->updateTexture(
        material.normal
    );
    descriptorManager->updateTexture(
        material.metallicRoughness
    );

    return materialIndex;
}