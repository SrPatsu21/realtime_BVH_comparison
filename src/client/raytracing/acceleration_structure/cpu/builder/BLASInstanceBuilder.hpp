#pragma once

#include "../../utils/accelerationStructureConfig.hpp"

#include "../primitives/PrimitiveRef.hpp"
#include "../node/BLASInstance.hpp"
#include "../../../../batch/mesh/Mesh.hpp"

class BLASInstanceBuilder
{
public:

    using NodeType = DefaultBLASNode;
    using BuilderType = DefaultBLASBuilder;

    static void build(
        const Mesh& mesh,
        std::vector<NodeType>& nodes,
        std::vector<BLASInstance>& instances
    );

private:

    static void buildPrimitives(
        const Mesh& mesh,
        std::vector<PrimitiveRef>& primitives
    );

    static void buildInstances(
        const std::vector<NodeType>& nodes,
        const std::vector<PrimitiveRef>& primitives,
        std::vector<BLASInstance>& instances
    );
};