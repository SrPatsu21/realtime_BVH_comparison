#pragma once

#include "../../AABB.hpp"
#include <cstdint>

struct TLASInstance
{
    AABB bounds; // 64

    glm::mat4 inverseTransform; // 4*4*32 512

    uint64_t vertexAddress; //8
    uint64_t indexAddress; //8

    uint32_t blasIndex; // 4
    uint32_t nodeOffset; // 4
    uint32_t nodeCount; // 4
    uint32_t instanceOffset; // 4
};