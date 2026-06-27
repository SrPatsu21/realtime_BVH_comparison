#include "AABB.hpp"
#include <algorithm>

AABB::AABB()
{
    reset();
}

void AABB::reset()
{
    min = glm::vec3( 1e30f);
    max = glm::vec3(-1e30f);
}

void AABB::expand(const AABB& other)
{
    min = glm::min(min, other.min);
    max = glm::max(max, other.max);
}

void AABB::expand(const glm::vec3& point)
{
    min = glm::min(min, point);
    max = glm::max(max, point);
}

glm::vec3 AABB::getCenter() const
{
    return (min + max) * 0.5f;
}

float AABB::getCenterAxis(int axis) const
{
    return getCenter()[axis];
}

float AABB::surfaceArea() const
{
    glm::vec3 size = max - min;
    return 2.0f * (size.x * size.y +
                   size.y * size.z +
                   size.z * size.x);
}