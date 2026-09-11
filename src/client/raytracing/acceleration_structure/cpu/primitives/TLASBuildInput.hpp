#pragma once

#include "../../AABB.hpp"
#include "../AS.hpp"
#include "../node/BLASInstance.hpp"
#include "../../utils/accelerationStructureConfig.hpp"

struct TLASBuildInput
{
    AABB bounds;
    glm::mat4 inverseTransform;

    uint64_t vertexAddress;
    uint64_t indexAddress;

    AS<DefaultBLASNode, BLASInstance>* blas;
};