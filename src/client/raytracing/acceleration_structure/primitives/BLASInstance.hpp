#include "../AABB.hpp"
#include <cstdint>

struct BLASInstance
{
    AABB bounds;

    uint32_t blasIndex;

    float transform[16];

    inline const AABB& getBounds() const
    {
        return bounds;
    }
};
