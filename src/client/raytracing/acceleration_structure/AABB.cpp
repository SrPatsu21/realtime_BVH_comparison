#include "AABB.hpp"
#include <algorithm>

AABB::AABB()
{
    reset();
}

void AABB::reset()
{
    min = glm::vec4( 1e30f, 1e30f, 1e30f, 0.0f);
    max = glm::vec4(-1e30f,-1e30f,-1e30f, 0.0f);
}

void AABB::expand(const AABB& other)
{
    min = glm::min(min, other.min);
    max = glm::max(max, other.max);
}

void AABB::expand(const glm::vec3& point)
{
    min = glm::min(
        min,
        glm::vec4(point, 0.0f)
    );

    max = glm::max(
        max,
        glm::vec4(point, 0.0f)
    );
}

glm::vec3 AABB::getCenter() const
{
    return glm::vec3(
        (min + max) * 0.5f
    );
}

float AABB::getCenterAxis(int axis) const
{
    return getCenter()[axis];
}

float AABB::surfaceArea() const
{
    glm::vec3 size =
        glm::vec3(max - min);

    return 2.0f * (
        size.x * size.y +
        size.y * size.z +
        size.z * size.x
    );
}