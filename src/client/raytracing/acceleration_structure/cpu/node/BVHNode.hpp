#pragma once

#include "../../AABB.hpp"
#include <cstdint>

struct BVHNode
{
    AABB bounds; // 64

    uint32_t left; // 4
    uint32_t right; // 4

    uint32_t leaf; //4
    uint32_t pad0;//4

    AABB& getBounds() { return bounds; }
    const AABB& getBounds() const { return bounds; }
}; // 80