#pragma once

#include "../../AABB.hpp"
#include <cstdint>

struct TLASInstance
{
    AABB bounds;

    glm::mat4 inverseTransform;

    uint32_t blasIndex;
    uint32_t nodeOffset;
    uint32_t nodeCount;
    uint32_t instanceOffset;
};