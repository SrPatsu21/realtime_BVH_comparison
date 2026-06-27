#include "BVHUtils.hpp"

int BVHUtils::selectSplitAxis(
    const AABB& centroidBounds
) {
    glm::vec3 extent = centroidBounds.max - centroidBounds.min;

    if (extent.x >= extent.y && extent.x >= extent.z)
        return 0;

    if (extent.y >= extent.z)
        return 1;

    return 2;
}