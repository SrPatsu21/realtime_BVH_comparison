#pragma once

#include <vector>
#include <cstdint>

#include "../node/BLASInstance.hpp"
#include "../node/BVHNode.hpp"
#include "../primitives/PrimitiveRef.hpp"

#include "../../../../batch/mesh/Mesh.hpp"

class Mesh;

class BLASInstanceBuilder
{
public:

    static void buildPrimitives(
        const Mesh& mesh,
        std::vector<PrimitiveRef>& primitives
    );

    static void buildInstances(
        const std::vector<BVHNode>& nodes,
        const std::vector<PrimitiveRef>& primitives,
        std::vector<BLASInstance>& instances
    );
};