#pragma once

#include "../../AABB.hpp"
#include <cstdint>

struct BVH8Node
{
    AABB bounds;

    uint32_t children[8];

    uint32_t childCount;

    uint32_t leaf;

    uint32_t pad0;
    uint32_t pad1;

    AABB& getBounds()
    {
        return bounds;
    }

    const AABB& getBounds() const
    {
        return bounds;
    }
};