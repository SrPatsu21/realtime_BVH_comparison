#include "../AABB.hpp"

struct Triangle
{
    AABB bounds;

    inline const AABB& getBounds() const
    {
        return bounds;
    }
};
