#include "BatchKey.hpp"

bool BatchKey::operator==(
    const BatchKey& other
) const {
    return material == other.material && subMesh == other.subMesh && mesh == other.mesh && pipelineFlags == other.pipelineFlags;
}

bool BatchKey::operator<(
    const BatchKey& other
) const {
    if (pipelineFlags != other.pipelineFlags)
        return pipelineFlags < other.pipelineFlags;

    if (material.get() != other.material.get())
        return material.get() < other.material.get();

    if (mesh.get() != other.mesh.get())
        return mesh.get() < other.mesh.get();

    return subMesh < other.subMesh;
}
