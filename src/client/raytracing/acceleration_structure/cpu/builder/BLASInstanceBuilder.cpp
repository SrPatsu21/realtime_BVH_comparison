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
    const auto& vertices =
        mesh.getVertices();

    const auto& indices =
        mesh.getIndices();

    const uint32_t triangleCount =
        static_cast<uint32_t>(
            indices.size() / 3
        );

    primitives.clear();

    if (triangleCount == 0)
        return;

    const uint32_t primitiveCount =
        (
            triangleCount +
            TRIANGLES_PER_PRIMITIVE -
            1
        ) /
        TRIANGLES_PER_PRIMITIVE;

    primitives.reserve(
        primitiveCount
    );

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
            const uint32_t triangleIndex = triangleStart + triangle;

            const uint32_t indexOffset = triangleIndex * 3;

            const uint32_t i0 = indices[indexOffset + 0];
            const uint32_t i1 = indices[indexOffset + 1];
            const uint32_t i2 = indices[indexOffset + 2];

            bounds.expand(vertices[i0].pos);
            bounds.expand(vertices[i1].pos);
            bounds.expand(vertices[i2].pos);
        }

        PrimitiveRef primitive{};
        primitive.bounds = bounds;
        primitive.index = triangleStart;
        primitive.count = trianglesInPrimitive;

        primitives.emplace_back(
            primitive
        );
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
        instance.materialOffset = 0;
        instance.pad0 = 0;

        instances.emplace_back(
            instance
        );
    }
}