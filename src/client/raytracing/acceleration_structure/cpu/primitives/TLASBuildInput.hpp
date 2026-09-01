#pragma once

#include "../../AABB.hpp"
#include <glm/glm.hpp>
#include <cstdint>

struct TLASBuildInput
{
    AABB bounds;

    glm::mat4 inverseTransform;

    uint32_t blasIndex;
};