#pragma once

#include "AABB.hpp"
#include <cstdint>

struct BVHNode
{
    AABB bounds;

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
    };

    bool leaf;

    inline AABB& getBounds()
    {
        return bounds;
    }
};