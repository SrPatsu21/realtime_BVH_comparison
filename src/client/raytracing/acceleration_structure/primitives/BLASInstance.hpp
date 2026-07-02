#include "../AABB.hpp"
#include <cstdint>

struct BLASInstance
{
    AABB bounds;

    uint32_t blasIndex;

    glm::mat4 transform;

    inline const AABB& getBounds() const
    {
        return bounds;
    }
};
