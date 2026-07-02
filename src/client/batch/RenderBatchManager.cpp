#include "RenderBatchManager.hpp"
#include "mesh/Mesh.hpp"
#include "instance/RenderInstance.hpp"
#include "instance/InstanceData.hpp"

// ========================
// BatchKey
// ========================

bool RenderBatchManager::BatchKey::operator==(
    const RenderBatchManager::BatchKey& other
) const {
    return material == other.material && subMesh == other.subMesh && mesh == other.mesh && pipelineFlags == other.pipelineFlags;
}

bool RenderBatchManager::BatchKey::operator<(
    const RenderBatchManager::BatchKey& other
) const {
    if (pipelineFlags != other.pipelineFlags)
        return pipelineFlags < other.pipelineFlags;

    if (material.get() != other.material.get())
        return material.get() < other.material.get();

    if (mesh.get() != other.mesh.get())
        return mesh.get() < other.mesh.get();

    return subMesh < other.subMesh;
}

// ========================
// RenderBatch
// ========================

RenderBatch::RenderBatch(
    RenderBatchManager::BatchKey batchKey
)
    : batchKey(batchKey)
{}

RenderBatch::RenderBatch(
    RenderBatch&& other
) noexcept :
    batchKey(std::move(other.batchKey)),
    batchRegistrations(std::move(other.batchRegistrations)),
    instancesData(std::move(other.instancesData))
{}

RenderBatch&
RenderBatch::operator=(
    RenderBatch&& other
) noexcept
{
    if (this != &other)
    {
        batchKey = std::move(other.batchKey);
        batchRegistrations = std::move(other.batchRegistrations);
        instancesData = std::move(other.instancesData);
    }
    return *this;
}

RenderBatch::~RenderBatch() = default;

void RenderBatch::addInstance(
    RenderInstance* instance
)
{
    size_t index = instancesData.size();
    instancesData.emplace_back();
    instance->addRegistration(this, index);

    batchRegistrations.push_back(&instance->registrations.back());
}

void RenderBatch::removeInstance(
    RenderInstance* instance,
    size_t intregistrationsIndex
) {
    auto& reg = instance->registrations[intregistrationsIndex];

    size_t index = reg.indexInBatch;
    size_t lastIndex = batchRegistrations.size() - 1;

    if (index != lastIndex)
    {
        batchRegistrations[index] = batchRegistrations[lastIndex];
        batchRegistrations[index]->indexInBatch = index;

        instancesData[index] = instancesData[lastIndex];
    }

    batchRegistrations.pop_back();
    instancesData.pop_back();

    reg.batch = nullptr;
}

bool RenderBatch::empty()
{
    return batchRegistrations.empty();
}

// ========================
// RenderBatchManager
// ========================

RenderBatchManager::RenderBatchManager(
    ResourceManager* resourceManager
)
    : resourceManager(resourceManager)
{
}

void RenderBatchManager::addInstance(
    std::shared_ptr<Mesh> mesh,
    RenderInstance* instance
) {
    instance->getRegistrations().reserve(mesh->getSubMeshes().size());
    const std::vector<Mesh::SubMesh>& meshes = mesh->getSubMeshes();

    for (size_t i = 0; i < meshes.size(); i++)
    {
        BatchKey key = {
            mesh,
            &meshes[i],
            resourceManager->getMaterialForSubMesh(*mesh.get(), meshes[i]),
            GraphicsPipeline::PIPE_TOPO_TRIANGLES | GraphicsPipeline::PIPE_CULL_NONE | GraphicsPipeline::PIPE_DEPTH_TEST | GraphicsPipeline::PIPE_DEPTH_WRITE | GraphicsPipeline::PIPE_BLEND,
            resourceManager->getAccelerationStructureIndex(mesh.get())
        };

        auto it = batches_map.find(key);
        if (it != batches_map.end())
        {
            it->second->addInstance(
                instance
            );
        }
        else{
            auto batch = std::make_unique<RenderBatch>(key);
            auto* batchPtr = batch.get();

            batches_map.emplace(key, std::move(batch));

            batchPtr->addInstance(
                instance
            );

            batches_dirty = true;
        }
    }
}

bool RenderBatchManager::removeInstance(
    RenderInstance* instance
)
{
    auto registrations = instance->getRegistrations();
    for (size_t i = 0; i < registrations.size(); i++)
    {
        auto* batch = registrations[i].batch;

        if (!batch)
            return false;

        BatchKey key = batch->getKey();

        batch->removeInstance(
            instance,
            registrations[i].indexInBatch
        );

        if (batch->empty())
        {
            batches_map.erase(key);
            batches_dirty = true;
        }
    }
    return true;

}

void RenderBatchManager::rebuildSortedBatches()
{
    if (!batches_dirty)
        return;

    batches_sorted.clear();
    batches_sorted.reserve(batches_map.size());

    for (auto& [key, batch] : batches_map)
        batches_sorted.push_back(batch.get());

    std::sort(
        batches_sorted.begin(),
        batches_sorted.end(),
        [](RenderBatch* a, RenderBatch* b)
        {
            return a->getKey() < b->getKey();
        }
    );

    batches_dirty = false;
}

inline AABB transformAABB(
    const AABB& localBounds,
    const glm::mat4& transform
)
{
    AABB result;
    result.reset();

    const glm::vec3 min = localBounds.min;
    const glm::vec3 max = localBounds.max;

    const glm::vec3 corners[8] =
    {
        { min.x, min.y, min.z },
        { max.x, min.y, min.z },
        { min.x, max.y, min.z },
        { max.x, max.y, min.z },

        { min.x, min.y, max.z },
        { max.x, min.y, max.z },
        { min.x, max.y, max.z },
        { max.x, max.y, max.z }
    };

    for (const glm::vec3& corner : corners)
    {
        glm::vec3 world =
            glm::vec3(transform * glm::vec4(corner, 1.0f));

        result.expand(world);
    }

    return result;
}

void RenderBatchManager::rebuildTLAS()
{
    std::vector<BLASInstance> tlasInstances;

    size_t totalInstances = 0;
    for (RenderBatch* batch : batches_sorted)
        totalInstances += batch->getinstancesData().size();

    tlasInstances.reserve(totalInstances);

    auto accelerationStructureManager = resourceManager->getAccelerationStructureManager();

    for (RenderBatch* batch : batches_sorted)
    {
        const uint32_t blasIndex =
            batch->getKey().accelerationStructureIndex;

        const auto& instanceData = batch->getinstancesData();

        for (const InstanceData& instance : instanceData)
        {
            BLASInstance blasInstance{};

            blasInstance.blasIndex = blasIndex;
            blasInstance.transform = instance.model;

            const auto& localBounds = accelerationStructureManager->getBLAS(blasIndex).accelerationStructure.nodes[0].bounds;

            blasInstance.bounds =
                transformAABB(
                    localBounds,
                    instance.model
                );

            tlasInstances.emplace_back(std::move(blasInstance));
        }
    }

    accelerationStructureManager->buildTLAS(tlasInstances);
}

// ========================
// BatchKey helpers
// ========================

void RenderBatchManager::findBatchKey(
    const std::string& meshPath,
    uint32_t subMeshIndex,
    BatchKey& key
)
{
    key.mesh = resourceManager->getMesh(meshPath);
    key.subMesh = &key.mesh->getSubMeshes()[subMeshIndex];
    key.material = resourceManager->getMaterialForSubMesh(*key.mesh.get(), *key.subMesh);
    key.pipelineFlags =
        GraphicsPipeline::PIPE_TOPO_TRIANGLES |
        GraphicsPipeline::PIPE_CULL_NONE |
        GraphicsPipeline::PIPE_DEPTH_TEST |
        GraphicsPipeline::PIPE_DEPTH_WRITE |
        GraphicsPipeline::PIPE_BLEND;
}

RenderBatchManager::BatchKey
RenderBatchManager::findBatchKey(
    const std::string& meshPath,
    uint32_t subMeshIndex
)
{
    BatchKey key;
    key.mesh = resourceManager->getMesh(meshPath);
    key.subMesh = &key.mesh->getSubMeshes()[subMeshIndex];
    key.material = resourceManager->getMaterialForSubMesh(*key.mesh.get(), *key.subMesh);
    key.pipelineFlags =
        GraphicsPipeline::PIPE_TOPO_TRIANGLES |
        GraphicsPipeline::PIPE_CULL_NONE |
        GraphicsPipeline::PIPE_DEPTH_TEST |
        GraphicsPipeline::PIPE_DEPTH_WRITE |
        GraphicsPipeline::PIPE_BLEND;
    return key;
}
