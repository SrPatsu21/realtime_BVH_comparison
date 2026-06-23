#include "AABB.hpp"

AABB::AABB()
{
    min[0] = min[1] = min[2] =  1e30f;
    max[0] = max[1] = max[2] = -1e30f;
}

void AABB::expand(const AABB& b)
{
    min[0] = std::min(min[0], b.min[0]);
    min[1] = std::min(min[1], b.min[1]);
    min[2] = std::min(min[2], b.min[2]);

    max[0] = std::max(max[0], b.max[0]);
    max[1] = std::max(max[1], b.max[1]);
    max[2] = std::max(max[2], b.max[2]);
}

void AABB::expand(const float p[3])
{
    min[0] = std::min(min[0], p[0]);
    min[1] = std::min(min[1], p[1]);
    min[2] = std::min(min[2], p[2]);

    max[0] = std::max(max[0], p[0]);
    max[1] = std::max(max[1], p[1]);
    max[2] = std::max(max[2], p[2]);
}

float AABB::surfaceArea() const
{
    float dx = max[0] - min[0];
    float dy = max[1] - min[1];
    float dz = max[2] - min[2];

    return 2.0f * (dx*dy + dy*dz + dz*dx);
}

float AABB::getCentroidAxis(int axis) const
{
    return 0.5f * (min[axis] + max[axis]);
}