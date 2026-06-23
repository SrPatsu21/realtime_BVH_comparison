#pragma once

#include <unordered_map>
#include <memory>
#include <string>

#include "mesh/Mesh.hpp"
#include "material/Material.hpp"
#include "texture/SamplerManager.hpp"
#include "texture/TextureImage.hpp"

class ResourceManager
{
private:
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    BufferManager* bufferManager;
    SamplerManager samplerManager;
    MaterialDescriptorManager* descriptorManager;

    std::unordered_map<std::string, std::weak_ptr<Mesh>> meshes;
    std::unordered_map<std::string, std::weak_ptr<TextureImage>> textures;
    std::unordered_map<std::string, std::weak_ptr<Material>> materials;
public:
    ResourceManager(
        VkPhysicalDevice physicalDevice,
        VkDevice device,
        BufferManager* bufferManager,
        MaterialDescriptorManager* descriptorManager
    );
    ~ResourceManager() = default;

    std::shared_ptr<Mesh> getMesh(
        const std::string& meshPath
    );

    std::vector<std::shared_ptr<Material>> getMaterialsForMesh(
        const Mesh& mesh
    );

    std::shared_ptr<Material> getMaterialForSubMesh(
        const Mesh& mesh,
        const Mesh::SubMesh& subMesh
    );

    std::shared_ptr<TextureImage> getTexture(
        const std::string& path
    );
};
