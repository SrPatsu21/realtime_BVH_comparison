#pragma once

#include <cstdint>
#include <vector>

#include "AABB.hpp"

class BVHUtils
{
public:
    template<typename PrimitiveType>
    static AABB ComputeBounds(
        const std::vector<PrimitiveType>& primitives,
        uint32_t begin,
        uint32_t end
    );

    template<typename PrimitiveType>
    static AABB ComputeCentroidBounds(
        const std::vector<PrimitiveType>& primitives,
        uint32_t begin,
        uint32_t end
    );

    static int SelectSplitAxis(
        const AABB& centroidBounds
    );
};