#pragma once

#include "../../AABB.hpp"
#include <cstdint>

struct BVHNode
{
    AABB childBounds[2]; //128

    uint32_t children[2]; //8

    uint32_t leaf; //4
    uint32_t pad0;//4
}; // 128 + 16 = 144