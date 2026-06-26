#pragma once

#include <cstdint>
#include <vector>

#include "AABB.hpp"

class BVHUtils
{
public:
    template<typename PrimitiveType>
    static AABB computeBounds(
        const std::vector<PrimitiveType>& primitives,
        uint32_t begin,
        uint32_t end
    );

    template<typename PrimitiveType>
    static AABB computeCentroidBounds(
        const std::vector<PrimitiveType>& primitives,
        uint32_t begin,
        uint32_t end
    );

    static int selectSplitAxis(
        const AABB& centroidBounds
    );
};