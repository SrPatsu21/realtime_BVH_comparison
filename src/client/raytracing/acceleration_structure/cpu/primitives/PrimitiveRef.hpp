#pragma once

#include "../../AABB.hpp"
#include <cstdint>

struct PrimitiveRef
{
    AABB bounds;

    uint32_t index;

    const AABB& getBounds() const
    {
        return bounds;
    }
};