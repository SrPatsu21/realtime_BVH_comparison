#include "BLASInstanceBuilder.hpp"

#include "../builder/BVHBuilder.hpp"

void BLASInstanceBuilder::build(
    const Mesh& mesh,
    std::vector<NodeType>& nodes,
    std::vector<BLASInstance>& instances
)
{
    nodes.clear();
    instances.clear();

    std::vector<PrimitiveRef> primitives;

    buildPrimitives(
        mesh,
        primitives
    );

    if (primitives.empty())
    {
        primitives.clear();
        return;
    }

    BuilderType::build(
        nodes,
        primitives
    );

    buildInstances(
        primitives,
        instances
    );

    primitives.clear();
}

void BLASInstanceBuilder::buildPrimitives(
    const Mesh& mesh,
    std::vector<PrimitiveRef>& primitives
)
{
    const auto& vertices = mesh.getVertices();
    const auto& indices = mesh.getIndices();
    const auto& subMeshes = mesh.getSubMeshes();

    primitives.clear();

    for (const Mesh::SubMesh& subMesh : subMeshes)
    {
        const uint32_t triangleCount = subMesh.indexCount / 3;

        if (triangleCount == 0)
            continue;

        /*
         * Every primitive generated from this SubMesh inherits
         * exactly one materialIndex from the SubMesh.
         *
         * A primitive is never allowed to cross the SubMesh boundary,
         * therefore it can never contain triangles with different materials.
         */
        for (
            uint32_t triangleStart = 0;
            triangleStart < triangleCount;
            triangleStart += TRIANGLES_PER_PRIMITIVE
        )
        {
            const uint32_t trianglesInPrimitive =
                std::min(
                    TRIANGLES_PER_PRIMITIVE,
                    triangleCount - triangleStart
                );

            AABB bounds;

            bounds.reset();

            for (
                uint32_t triangle = 0;
                triangle < trianglesInPrimitive;
                ++triangle
            )
            {
                const uint32_t triangleIndex =
                    triangleStart + triangle;

                const uint32_t indexOffset =
                    subMesh.firstIndex + triangleIndex * 3;

                const uint32_t i0 = indices[indexOffset + 0];
                const uint32_t i1 = indices[indexOffset + 1];
                const uint32_t i2 = indices[indexOffset + 2];

                bounds.expand(vertices[i0].pos);
                bounds.expand(vertices[i1].pos);
                bounds.expand(vertices[i2].pos);
            }

            PrimitiveRef primitive{};

            primitive.bounds = bounds;
            primitive.index = subMesh.firstIndex / 3 + triangleStart;
            primitive.count = trianglesInPrimitive;
            primitive.materialIndex = subMesh.materialIndex;

            primitives.emplace_back(
                primitive
            );
        }
    }
}

void BLASInstanceBuilder::buildInstances(
    const std::vector<PrimitiveRef>& primitives,
    std::vector<BLASInstance>& instances
)
{
    instances.clear();

    if (primitives.empty())
        return;

    instances.reserve(
        primitives.size()
    );

    for (
        const PrimitiveRef& primitive :
        primitives
    )
    {
        BLASInstance instance{};
        instance.bounds = primitive.bounds;
        instance.firstTriangle = primitive.index;
        instance.triangleCount = primitive.count;
        instance.materialOffset = primitive.materialIndex;
        instance.pad0 = 0;

        instances.emplace_back(
            instance
        );
    }
}