#pragma once

#include <glm/glm.hpp>

class AABB
{
public:
    glm::vec3 min;
    glm::vec3 max;

    AABB();

    void reset();

    void expand(const AABB& other);
    void expand(const glm::vec3& point);

    glm::vec3 getCenter() const;
    float getCenterAxis(int axis) const;

    float surfaceArea() const;
};