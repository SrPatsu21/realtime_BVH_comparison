#pragma once

#include <unordered_map>
#include <memory>
#include <string>

#include "mesh/Mesh.hpp"
#include "AssimpModelLoader.hpp"

#include "material/MaterialManager.hpp"

#include "texture/TextureManager.hpp"
#include "texture/SamplerManager.hpp"

#include "../raytracing/acceleration_structure/AccelerationStructureManager.hpp"
#include "../raytracing/acceleration_structure/utils/accelerationStructureConfig.hpp"

class ResourceManager
{
private:
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    BufferManager* bufferManager;

    SamplerManager samplerManager;

    TextureManager textureManager;
    MaterialManager materialManager;

    AccelerationStructureManager<DefaultTLASBuilder, DefaultBLASBuilder>* accelerationStructureManager;

    std::unordered_map<std::string, std::weak_ptr<Mesh>> meshes;

    static void buildPrimitiveRefs(
        const Mesh& mesh,
        std::vector<PrimitiveRef>& primitives
    );

public:

    ResourceManager(
        VkPhysicalDevice physicalDevice,
        VkDevice device,
        BufferManager* bufferManager
    );

    ~ResourceManager();

    BufferManager* getBufferManager(){
            return bufferManager;
    };

    std::shared_ptr<Mesh> getMesh(
        const std::string& meshPath
    );

    TextureManager* getTextureManager() { return &textureManager; }

    MaterialManager* getMaterialManager(){ return &materialManager; }

    AccelerationStructureManager<DefaultTLASBuilder, DefaultBLASBuilder>* getAccelerationStructureManager(){
        return accelerationStructureManager;
    }

    template<typename Map>
    void CleanupMap(Map& map)
    {
        std::erase_if(map, [](const auto& item) {
            return item.second.expired();
        });
    }

    void CleanupMaps();
};