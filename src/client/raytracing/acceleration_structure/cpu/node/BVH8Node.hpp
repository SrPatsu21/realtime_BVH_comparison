#pragma once

#include "../../AABB.hpp"
#include <cstdint>

struct BVH8Node
{

    AABB childBounds[8]; //512
    uint32_t children[8]; //32

    uint32_t childCount; //4

    uint32_t leaf; //4

    uint32_t pad0; //4
    uint32_t pad1; //4
}; //512 + 64