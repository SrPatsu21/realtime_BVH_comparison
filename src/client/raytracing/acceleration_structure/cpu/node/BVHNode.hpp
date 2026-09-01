#pragma once

#include "../../AABB.hpp"
#include <cstdint>

struct BVHNode
{
    AABB bounds; //32

    union
    {
        struct
        {
            uint32_t left;
            uint32_t right;
        };

        struct
        {
            uint32_t firstPrimitive;
            uint32_t primitiveCount;
        };
    }; //8

    uint32_t leaf; //4
    uint32_t pad0;//4

    AABB& getBounds() { return bounds; }
    const AABB& getBounds() const { return bounds; }
}; //48