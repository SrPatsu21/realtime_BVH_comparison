#pragma once

#include "../../CoreVulkan.hpp"
#include "../../BufferManager.hpp"
#include "MaterialDescriptorManager.hpp"
#include "Material.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class Mesh;
class TextureManager;

class MaterialManager
{
public:

    MaterialManager(
        VkDevice device,
        BufferManager* bufferManager,
        TextureManager* textureManager,
        uint32_t maxMaterials,
        uint32_t maxTextures
    );

    ~MaterialManager();

    uint32_t createMaterial(
        Mesh* mesh,
        uint32_t aiMaterialIndex,
        const MaterialData& material
    );

    const MaterialCPU* getMaterial(uint32_t materialIndex) const{ return &materials[materialIndex]; }

    VkBuffer getBuffer() const{return materialBuffer;}

    VkDescriptorSetLayout getDescriptorSetLayout() const
    {
        return descriptorManager->getLayout();
    }

    VkDescriptorPool getDescriptorPool() const
    {
        return descriptorManager->getDescriptorPool();
    }

    VkDescriptorSet getDescriptorSet() const
    {
        return descriptorManager->getDescriptorSet();
    }

    uint32_t getMaterialCount() const {return static_cast<uint32_t>(materials.size());}

private:

    struct MaterialKey
    {
        Mesh* mesh;
        uint32_t aiMaterialIndex;

        bool operator==(const MaterialKey& other) const
        {
            return mesh == other.mesh && aiMaterialIndex == other.aiMaterialIndex;
        }
    };

    struct MaterialKeyHash
    {
        std::size_t operator()(const MaterialKey& key) const
        {
            std::size_t h1 = std::hash<Mesh*>{}(key.mesh);
            std::size_t h2 = std::hash<uint32_t>{}(key.aiMaterialIndex);

            return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
        }
    };

private:

    VkDevice device{};

    BufferManager* bufferManager{};
    std::unique_ptr<MaterialDescriptorManager> descriptorManager;

    std::unordered_map<
        MaterialKey,
        uint32_t,
        MaterialKeyHash
    > materialIndices;

    std::vector<MaterialCPU> materials;

    VkBuffer materialBuffer{};
    VkDeviceMemory materialMemory{};

    uint32_t maxMaterials{};
};