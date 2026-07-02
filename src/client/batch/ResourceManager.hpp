#pragma once

#include <unordered_map>
#include <memory>
#include <string>

#include "mesh/Mesh.hpp"
#include "material/Material.hpp"
#include "texture/SamplerManager.hpp"
#include "texture/TextureImage.hpp"
#include "../raytracing/acceleration_structure/AccelerationStructure.hpp"
#include "../raytracing/acceleration_structure/BVHNode.hpp"
#include "../raytracing/acceleration_structure/AccelerationStructureManager.hpp"
#include "../raytracing/acceleration_structure/builder/BVHBuilder.hpp"
#include "../raytracing/acceleration_structure/primitives/PrimitiveRef.hpp"

class ResourceManager
{
private:
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    BufferManager* bufferManager;
    SamplerManager samplerManager;
    MaterialDescriptorManager* descriptorManager;
    AccelerationStructureManager<BVHBuilder<BVHNode>, BVHBuilder<BVHNode>>* accelerationStructureManager;

    std::unordered_map<std::string, std::weak_ptr<Mesh>> meshes;
    std::unordered_map<std::string, std::weak_ptr<TextureImage>> textures;
    std::unordered_map<std::string, std::weak_ptr<Material>> materials;
    std::unordered_map<const Mesh*, uint32_t> accelerationStructuresIndex;

    static void buildPrimitiveRefs(
        const Mesh& mesh,
        std::vector<PrimitiveRef>& primitives
    );
public:
    ResourceManager(
        VkPhysicalDevice physicalDevice,
        VkDevice device,
        BufferManager* bufferManager,
        MaterialDescriptorManager* descriptorManager
    );
    ~ResourceManager();

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

    AccelerationStructureManager<BVHBuilder<BVHNode>, BVHBuilder<BVHNode>>* getAccelerationStructureManager(){
        return accelerationStructureManager;
    }

    uint32_t
    getAccelerationStructureIndex(
        const Mesh* mash
    );
};
