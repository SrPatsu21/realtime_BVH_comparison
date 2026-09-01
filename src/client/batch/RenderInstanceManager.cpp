#include "RenderInstanceManager.hpp"
#include "mesh/Mesh.hpp"
#include "instance/RenderInstance.hpp"
#include "instance/InstanceData.hpp"
#include "RenderBatch.hpp"
#include <utility>

// ========================
// RenderInstanceManager
// ========================

RenderInstanceManager::RenderInstanceManager(
    ResourceManager* resourceManager
)
    : resourceManager(resourceManager)
{
    batches_map.clear();
    batches_sorted.clear();
    instances.clear();
}

RenderInstanceRegistration* RenderInstanceManager::createRenderInstance(
    std::shared_ptr<Mesh> mesh
){
    instances.emplace_back();

    RenderInstance& instance = instances.back();

    RenderInstanceRegistration* registration = new RenderInstanceRegistration{
        instances.size() - 1
    };

    instance.renderInstanceRegistration = registration;

    instance.blas = resourceManager->getAccelerationStructureManager()->createBLAS(mesh.get());

    addInstance(
        mesh,
        &instance
    );

    return registration;
};

bool RenderInstanceManager::removeRenderInstance(
    RenderInstanceRegistration* registration
){
    RenderInstance& instance = instances[registration->indexInVector];
    if(removeInstance(&instance)){
        size_t index = registration->indexInVector;
        size_t last = instances.size() - 1;

        //* swap
        if (index != last)
        {
            instances[index] = std::move(instances[last]);
            instances[index].renderInstanceRegistration->indexInVector = index;
        }

        instances.pop_back();
        return true;
    };
    return false;
};

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
            GraphicsPipelineManager::PIPE_TOPO_TRIANGLES |
            GraphicsPipelineManager::PIPE_CULL_NONE |
            GraphicsPipelineManager::PIPE_DEPTH_TEST |
            GraphicsPipelineManager::PIPE_DEPTH_WRITE |
            GraphicsPipelineManager::PIPE_BLEND |
            GraphicsPipelineManager::PIPE_GEOMETRY |
            GraphicsPipelineManager::PIPE_LIGHTING,
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

    const glm::vec4& min = localBounds.min;
    const glm::vec4& max = localBounds.max;

    const glm::vec4 corners[8] =
    {
        { min.x, min.y, min.z, 1.0f },
        { max.x, min.y, min.z, 1.0f },
        { min.x, max.y, min.z, 1.0f },
        { max.x, max.y, min.z, 1.0f },

        { min.x, min.y, max.z, 1.0f },
        { max.x, min.y, max.z, 1.0f },
        { min.x, max.y, max.z, 1.0f },
        { max.x, max.y, max.z, 1.0f }
    };

    for (const glm::vec4& corner : corners)
    {
        const glm::vec4 world = transform * corner;

        result.expand(
            glm::vec3(world)
        );
    }

    return result;
}
void RenderInstanceManager::rebuildTLAS()
{
    auto* accelerationStructureManager = resourceManager->getAccelerationStructureManager();

    std::vector<TLASBuildInput> inputs;
    inputs.reserve(instances.size());

    for (const auto& instance : instances)
    {
        AS<DefaultBLASNode, BLASInstance>* blas = instance.getBLAS();

        if (!blas)
            continue;

        TLASBuildInput input{};

        input.blas = blas;

        input.bounds =
            transformAABB(
                blas->getBounds(),
                instance.getModelMatrix()
            );

        input.inverseTransform =
            glm::inverse(
                instance.getModelMatrix()
            );

        inputs.emplace_back(
            input
        );
    }

    accelerationStructureManager->createTLAS(
        inputs
    );

    tlas = accelerationStructureManager->getTLAS();

    #ifndef NDEBUG

        std::cout
            << "tlas instances size "
            << tlas.instances.size()
            << '\n';

        for (const auto& instance : tlas.instances)
        {
            std::cout
                << "[blasIndex="
                << instance.blasIndex
                << "] "
                << "min=("
                << instance.bounds.min.x
                << ", "
                << instance.bounds.min.y
                << ", "
                << instance.bounds.min.z
                << ") "
                << "max=("
                << instance.bounds.max.x
                << ", "
                << instance.bounds.max.y
                << ", "
                << instance.bounds.max.z
                << ") "
                << '\n';
        }

        std::cout
            << "end tlas\n";

        printBVH(
            tlas.nodes,
            0
        );

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
        GraphicsPipelineManager::PIPE_TOPO_TRIANGLES |
        GraphicsPipelineManager::PIPE_CULL_NONE |
        GraphicsPipelineManager::PIPE_DEPTH_TEST |
        GraphicsPipelineManager::PIPE_DEPTH_WRITE |
        GraphicsPipelineManager::PIPE_BLEND;
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
        GraphicsPipelineManager::PIPE_TOPO_TRIANGLES |
        GraphicsPipelineManager::PIPE_CULL_NONE |
        GraphicsPipelineManager::PIPE_DEPTH_TEST |
        GraphicsPipelineManager::PIPE_DEPTH_WRITE |
        GraphicsPipelineManager::PIPE_BLEND;
    return key;
}

RenderInstanceManager::~RenderInstanceManager()
{
    VkDevice device = resourceManager->getBufferManager()->getDevice();

    batches_map.clear();
    batches_sorted.clear();
    instances.clear();
}