#pragma once

#include "../../AABB.hpp"
#include <cstdint>

struct PrimitiveRef
{
    AABB bounds;

    uint32_t index;
    uint32_t count;

    const AABB& getBounds() const
    {
        return bounds;
    }
};