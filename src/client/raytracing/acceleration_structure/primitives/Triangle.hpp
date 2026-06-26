#include "../AABB.hpp"

struct Triangle
{
    AABB bounds;

    uint32_t i0;
    uint32_t i1;
    uint32_t i2;

    inline const AABB& getBounds() const
    {
        return bounds;
    }
};
