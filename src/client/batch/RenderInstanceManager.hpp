#pragma once

#include "ResourceManager.hpp"
#include "instance/InstanceData.hpp"
#include "instance/RenderInstance.hpp"
#include "RenderBatch.hpp"
#include "../graphics_pipeline/GraphicsPipeline.hpp"
#include "../raytracing/acceleration_structure/AccelerationStructure.hpp"
#include "../raytracing/acceleration_structure/accelerationStructureConfig.hpp"
#include "../raytracing/acceleration_structure/BVHNode.hpp"
#include "BatchKey.hpp"
#include "instance/RenderInstanceRegistration.hpp"

#include <list>

class RenderBatch;

class RenderInstanceManager
{
private:

    std::unordered_map<BatchKey, std::unique_ptr<RenderBatch>, BatchKeyHasher> batches_map;
    std::vector<RenderBatch*> batches_sorted;
    std::vector<RenderInstance> instances;
    TLAS<DefaultTLASNode> tlas;
    bool batches_dirty = true;

    ResourceManager* resourceManager;

    void addInstance(
        std::shared_ptr<Mesh> mesh,
        RenderInstance* instance
    );

    bool removeInstance(
        RenderInstance* instance
    );

public:
    RenderInstanceRegistration* createRenderInstance(
        std::shared_ptr<Mesh> mesh
    );

    bool removeRenderInstance(
        RenderInstanceRegistration* registration
    );

    void findBatchKey(
        const std::string& meshPath,
        uint32_t subMeshIndex,
        BatchKey& key
    );

    BatchKey findBatchKey(
        const std::string& meshPath,
        uint32_t subMeshIndex
    );

    template<typename Func> void forEachBatch(Func&& func)
    {
        rebuildSortedBatches();

        for (RenderBatch* batch : batches_sorted)
        {
            func(*batch);
        }
    }


    void rebuildSortedBatches();
    void rebuildTLAS();

    std::vector<RenderInstance> const getRenderInstances(){
        return instances;
    }
    RenderInstance* const getRenderInstances(size_t index){
        return &instances[index];
    }

    RenderInstanceManager(ResourceManager* resourceManager);
    ~RenderInstanceManager() = default;
};