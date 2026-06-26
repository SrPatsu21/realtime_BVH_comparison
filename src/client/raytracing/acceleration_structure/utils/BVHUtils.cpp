#include "BVHUtils.hpp"

template<typename PrimitiveType>
AABB BVHUtils::computeBounds(
    const std::vector<PrimitiveType>& primitives,
    uint32_t begin,
    uint32_t end
) {
    AABB bounds;

    for (uint32_t i = begin; i < end; ++i)
    {
        bounds.expand(primitives[i].getBounds());
    }

    return bounds;
}

template<typename PrimitiveType>
AABB BVHUtils::computeCentroidBounds(
    const std::vector<PrimitiveType>& primitives,
    uint32_t begin,
    uint32_t end
) {
    AABB bounds;

    for (uint32_t i = begin; i < end; ++i)
    {
        float centroid[3]
        {
            primitives[i].getBounds().getCentroidAxis(AABB::Axis::X),
            primitives[i].getBounds().getCentroidAxis(AABB::Axis::Y),
            primitives[i].getBounds().getCentroidAxis(AABB::Axis::Z)
        };

        bounds.expand(centroid);
    }

    return bounds;
}

int BVHUtils::selectSplitAxis(
    const AABB& centroidBounds
) {
    float dx =
        centroidBounds.max[0] -
        centroidBounds.min[0];

    float dy =
        centroidBounds.max[1] -
        centroidBounds.min[1];

    float dz =
        centroidBounds.max[2] -
        centroidBounds.min[2];

    if (dx >= dy && dx >= dz)
        return AABB::Axis::X;

    if (dy >= dz)
        return AABB::Axis::Y;

    return AABB::Axis::Z;
}
