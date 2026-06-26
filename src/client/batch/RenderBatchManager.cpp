#include "RenderBatchManager.hpp"
#include "mesh/Mesh.hpp"
#include "instance/RenderInstance.hpp"
#include "instance/InstanceData.hpp"

#include <algorithm>

// ========================
// BatchKey
// ========================

bool RenderBatchManager::BatchKey::operator==(
    const RenderBatchManager::BatchKey& other
) const {
    return material == other.material && submesh == other.submesh && mesh == other.mesh && pipelineFlags == other.pipelineFlags;
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

    return submesh < other.submesh;
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
    const std::vector<Mesh::SubMesh>& meshs = mesh->getSubMeshes();

    for (size_t i = 0; i < meshs.size(); i++)
    {
        BatchKey key = {
            mesh,
            &meshs[i],
            resourceManager->getMaterialForSubMesh(*mesh.get(), meshs[i]),
            GraphicsPipeline::PIPE_TOPO_TRIANGLES | GraphicsPipeline::PIPE_CULL_NONE | GraphicsPipeline::PIPE_DEPTH_TEST | GraphicsPipeline::PIPE_DEPTH_WRITE | GraphicsPipeline::PIPE_BLEND
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

            auto triangles = buildTriangles(
                *mesh,
                meshs[i]
            );

            batchPtr->batchKey.blasIndex = accelerationStructureManager->createBLAS(triangles);

            batches_map.emplace(key, std::move(batch));

            batchPtr->addInstance(
                instance
            );

            batches_dirty = true;
        }
    }
}

std::vector<Triangle> RenderBatchManager::buildTriangles(
    const Mesh& mesh,
    const Mesh::SubMesh& submesh
) const
{
    std::vector<Triangle> triangles;

    const auto& vertices = mesh.getVertices();
    const auto& indices  = mesh.getIndices();

    triangles.reserve(submesh.indexCount / 3);

    for (uint32_t i = 0; i < submesh.indexCount; i += 3)
    {
        uint32_t i0 = indices[submesh.firstIndex + i + 0];
        uint32_t i1 = indices[submesh.firstIndex + i + 1];
        uint32_t i2 = indices[submesh.firstIndex + i + 2];

        triangles.emplace_back(
            vertices[i0 + submesh.vertexOffset],
            vertices[i1 + submesh.vertexOffset],
            vertices[i2 + submesh.vertexOffset]
        );
    }

    return triangles;
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

// ========================
// BatchKey helpers
// ========================

void RenderBatchManager::findBatchKey(
    const std::string& meshPath,
    uint32_t submeshIndex,
    BatchKey& key
)
{
    key.mesh = resourceManager->getMesh(meshPath);
    key.submesh = &key.mesh->getSubMeshes()[submeshIndex];
    key.material = resourceManager->getMaterialForSubMesh(*key.mesh.get(), *key.submesh);
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
    uint32_t submeshIndex
)
{
    BatchKey key;
    key.mesh = resourceManager->getMesh(meshPath);
    key.submesh = &key.mesh->getSubMeshes()[submeshIndex];
    key.material = resourceManager->getMaterialForSubMesh(*key.mesh.get(), *key.submesh);
    key.pipelineFlags =
        GraphicsPipeline::PIPE_TOPO_TRIANGLES |
        GraphicsPipeline::PIPE_CULL_NONE |
        GraphicsPipeline::PIPE_DEPTH_TEST |
        GraphicsPipeline::PIPE_DEPTH_WRITE |
        GraphicsPipeline::PIPE_BLEND;
    return key;
}
