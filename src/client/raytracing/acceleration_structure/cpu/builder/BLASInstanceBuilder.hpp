#pragma once

#include <vector>

#include "../../utils/accelerationStructureConfig.hpp"

#include "../node/BLASInstance.hpp"
#include "../primitives/PrimitiveRef.hpp"
#include "../builder/BVHBuilder.hpp"
#include "../../../../batch/mesh/Mesh.hpp"

class BLASInstanceBuilder
{
public:

    static constexpr uint32_t TRIANGLES_PER_PRIMITIVE = 4;

    using NodeType = DefaultBLASNode;
    using BuilderType = DefaultBLASBuilder;


    static void build(
        const std::vector<Vertex>& vertices,
        const std::vector<uint32_t>& indices,
        const std::vector<SubMesh>& subMeshes,
        std::vector<NodeType>& nodes,
        std::vector<BLASInstance>& instances
    );

private:

    static void buildPrimitives(
        const std::vector<Vertex>& vertices,
        const std::vector<uint32_t>& indices,
        const std::vector<SubMesh>& subMeshes,
        std::vector<PrimitiveRef>& primitives
    );

    static void buildInstances(
        const std::vector<PrimitiveRef>& primitives,
        std::vector<BLASInstance>& instances
    );
};