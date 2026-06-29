#include "../AABB.hpp"

struct PrimitiveRef
{
    uint32_t triangleIndex;

    AABB bounds;
    inline const AABB& getBounds() const
    {
        return bounds;
    }
};