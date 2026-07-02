#pragma once

#include "ResourceManager.hpp"
#include "instance/InstanceData.hpp"
#include "instance/RenderInstance.hpp"
#include "../graphics_pipeline/GraphicsPipeline.hpp"
#include "../raytracing/acceleration_structure/AccelerationStructure.hpp"
#include "../raytracing/acceleration_structure/BVHNode.hpp"

#include <list>

class RenderBatch;
class RenderBatchManager
{
public:

    struct BatchKey
    {
        std::shared_ptr<Mesh> mesh;
        const Mesh::SubMesh* subMesh;
        std::shared_ptr<Material> material;
        GraphicsPipeline::PipelineFlags pipelineFlags;
        uint32_t accelerationStructureIndex;

        bool operator==(const RenderBatchManager::BatchKey& other) const;
        bool operator<(const RenderBatchManager::BatchKey& other) const;
    };

    struct BatchKeyHasher
    {
        size_t operator()(const BatchKey& key) const
        {
            size_t seed = 0;

            auto hash_combine = [&seed](size_t value)
            {
                seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            };

            hash_combine(std::hash<GraphicsPipeline::PipelineFlags>()(key.pipelineFlags));
            hash_combine(std::hash<Mesh*>()(key.mesh.get()));
            hash_combine(std::hash<const Mesh::SubMesh*>()(key.subMesh));
            hash_combine(std::hash<Material*>()(key.material.get()));

            return seed;
        }
    };

private:

    std::unordered_map<BatchKey, std::unique_ptr<RenderBatch>, BatchKeyHasher> batches_map;
    std::vector<RenderBatch*> batches_sorted;
    bool batches_dirty = true;

    ResourceManager* resourceManager;

public:
    void addInstance(
        std::shared_ptr<Mesh> mesh,
        RenderInstance* instance
    );

    bool removeInstance(
        RenderInstance* instance
    );

    void findBatchKey(
        const std::string& meshPath,
        uint32_t subMeshIndex,
        BatchKey& key
    );

    RenderBatchManager::BatchKey findBatchKey(
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

    RenderBatchManager(ResourceManager* resourceManager);
    ~RenderBatchManager() = default;

    #ifndef NDEBUG
    void batchSize(){
        std::cout << "batches map: " << batches_map.size() << " batch sorted:" << batches_sorted.size() << std::endl;
    }
    #endif
};

class RenderBatch {
    friend class RenderBatchManager;
private:
    RenderBatchManager::BatchKey batchKey;
    std::vector<RenderInstance::BatchRegistration*> batchRegistrations;
    std::vector<InstanceData> instancesData;
public:
    explicit RenderBatch(
        RenderBatchManager::BatchKey batchKey
    );

    ~RenderBatch();

    RenderBatch(const RenderBatch& other) = delete;
    RenderBatch& operator=(const RenderBatch& other) = delete;

    RenderBatch(RenderBatch&& other) noexcept;
    RenderBatch& operator=(RenderBatch&& other) noexcept;

    void addInstance(
        RenderInstance* instance
    );

    void removeInstance(
        RenderInstance* instance,
        size_t intregistrationsIndex
    );

    bool empty();

    const RenderBatchManager::BatchKey& getKey() const { return batchKey; }

    std::vector<RenderInstance::BatchRegistration*> getRenderInstance() const{ return batchRegistrations; }
    std::vector<InstanceData>& getinstancesData() { return instancesData; }
};