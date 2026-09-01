#pragma once

#include "../../AABB.hpp"
#include <cstdint>

struct PrimitiveRef
{
    uint32_t triangleIndex;

    AABB bounds;

    const AABB& getBounds() const
    {
        return bounds;
    }
};