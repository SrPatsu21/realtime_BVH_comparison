#pragma once

#include "ResourceManager.hpp"
#include "instance/InstanceData.hpp"
#include "instance/RenderInstance.hpp"
#include "../graphics_pipeline/GraphicsPipelineManager.hpp"
#include "../raytracing/acceleration_structure/AccelerationStructure.hpp"

struct BatchKey
{
    std::shared_ptr<Mesh> mesh;
    const Mesh::SubMesh* subMesh;
    std::shared_ptr<Material> material;
    GraphicsPipelineManager::PipelineFlags pipelineFlags;

    bool operator==(const BatchKey& other) const;
    bool operator<(const BatchKey& other) const;
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

        hash_combine(std::hash<GraphicsPipelineManager::PipelineFlags>()(key.pipelineFlags));
        hash_combine(std::hash<Mesh*>()(key.mesh.get()));
        hash_combine(std::hash<const Mesh::SubMesh*>()(key.subMesh));
        hash_combine(std::hash<Material*>()(key.material.get()));

        return seed;
    }
};
