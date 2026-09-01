#pragma once

#include "../../AABB.hpp"
#include <cstdint>

struct TLASInstance
{
    AABB bounds;

    glm::mat4 inverseTransform;

    uint32_t blasIndex;
    uint32_t pad0;
    uint32_t pad1;
    uint32_t pad2;
};