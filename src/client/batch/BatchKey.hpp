#pragma once

#include "ResourceManager.hpp"
#include "instance/InstanceData.hpp"
#include "instance/RenderInstance.hpp"
#include "../graphics_pipeline/GraphicsPipelineManager.hpp"
#include "../raytracing/acceleration_structure/cpu/AS.hpp"

struct BatchKey
{
    std::shared_ptr<Mesh> mesh;
    const SubMesh* subMesh;
    uint32_t material;
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
        hash_combine(std::hash<const SubMesh*>()(key.subMesh));
        hash_combine(std::hash<uint32_t>()(key.material));

        return seed;
    }
};

inline bool BatchKey::operator==(
    const BatchKey& other
) const {
    return material == other.material && subMesh == other.subMesh && mesh == other.mesh && pipelineFlags == other.pipelineFlags;
}

inline bool BatchKey::operator<(
    const BatchKey& other
) const {
    if (pipelineFlags != other.pipelineFlags)
        return pipelineFlags < other.pipelineFlags;

    if (material != other.material)
        return material < other.material;

    if (mesh.get() != other.mesh.get())
        return mesh.get() < other.mesh.get();

    return subMesh < other.subMesh;
}