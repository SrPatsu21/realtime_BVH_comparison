#include "RenderInstanceManager.hpp"
#include "mesh/Mesh.hpp"
#include "instance/RenderInstance.hpp"
#include "instance/InstanceData.hpp"
#include "RenderBatch.hpp"

// ========================
// RenderInstanceManager
// ========================

RenderInstanceManager::RenderInstanceManager(
    ResourceManager* resourceManager
)
    : resourceManager(resourceManager)
{
}

void RenderInstanceManager::addInstance(
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

            batches_map.emplace(key, std::move(batch));

            batchPtr->addInstance(
                instance
            );

            batches_dirty = true;
        }
    }
}

bool RenderInstanceManager::removeInstance(
    RenderInstance* instance
)
{
    auto registrations = instance->getRegistrations();
    for (size_t i = 0; i < registrations.size(); i++)
    {
        RenderBatch* batch = registrations[i].renderBatch;

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

void RenderInstanceManager::rebuildSortedBatches()
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

    #ifndef NDEBUG
        std::cout << "batches map: " << batches_map.size() << " batch sorted:" << batches_sorted.size() << std::endl;
    #endif

    rebuildTLAS();

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

void RenderInstanceManager::rebuildTLAS()
{
    auto accelerationStructureManager = resourceManager->getAccelerationStructureManager();

    std::vector<BLASInstance> tlasInstances;
    tlasInstances.reserve(instances.size());

    for (const auto& instance : instances)
    {
        BLASInstance blasInstance{};

        blasInstance.blas = instance.getBLAS();
        blasInstance.transform = instance.getModelMatrix();

        const auto& localBounds = instance.getBLAS()->accelerationStructure.nodes[0].bounds;

        blasInstance.bounds =
            transformAABB(
                localBounds,
                blasInstance.transform
            );

        tlasInstances.emplace_back(std::move(blasInstance));
    }

    #ifndef NDEBUG
        std::cout << "tlas instances size " << tlasInstances.size() << '\n';

        for (const auto& i : tlasInstances)
        {
            std::cout
                << "[" << i.blas << "] "
                << "min=("
                << i.bounds.min.x << ", "
                << i.bounds.min.y << ", "
                << i.bounds.min.z << ") "
                << "max=("
                << i.bounds.max.x << ", "
                << i.bounds.max.y << ", "
                << i.bounds.max.z << ") "
                << '\n';
        }

        std::cout << "end tlas\n";
    #endif

    accelerationStructureManager->createTLAS(tlasInstances, resourceManager->getBufferManager(), tlas);

    #ifndef NDEBUG
        printBVH(tlas.accelerationStructure.nodes, 0);
    #endif
}

// ========================
// BatchKey helpers
// ========================

void RenderInstanceManager::findBatchKey(
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

BatchKey RenderInstanceManager::findBatchKey(
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
