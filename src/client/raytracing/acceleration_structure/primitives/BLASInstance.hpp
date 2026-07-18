#include "../AABB.hpp"
#include <cstdint>
#include "../BLAS.hpp"
#include "../accelerationStructureConfig.hpp"
struct BLASInstance
{
    AABB bounds;

    const BLAS<DefaultBLASNode>* blas;

    glm::mat4 transform;

    inline const AABB& getBounds() const
    {
        return bounds;
    }
};
