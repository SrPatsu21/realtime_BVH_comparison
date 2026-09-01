#include "BLASInstanceBuilder.hpp"

void BLASInstanceBuilder::buildPrimitives(
    const Mesh& mesh,
    std::vector<PrimitiveRef>& primitives
)
{
    const auto& vertices = mesh.getVertices();
    const auto& indices = mesh.getIndices();

    const uint32_t triangleCount = static_cast<uint32_t>(indices.size() / 3);

    primitives.clear();
    primitives.reserve(triangleCount);

    for (uint32_t triangle = 0; triangle < triangleCount; ++triangle)
    {
        const uint32_t i0 = indices[triangle * 3 + 0];
        const uint32_t i1 = indices[triangle * 3 + 1];
        const uint32_t i2 = indices[triangle * 3 + 2];

        const glm::vec3& v0 = vertices[i0].pos;
        const glm::vec3& v1 = vertices[i1].pos;
        const glm::vec3& v2 = vertices[i2].pos;

        AABB bounds;
        bounds.reset();

        bounds.expand(v0);
        bounds.expand(v1);
        bounds.expand(v2);

        PrimitiveRef primitive{};

        primitive.bounds = bounds;
        primitive.index = triangle;

        primitives.emplace_back(primitive);
    }
}

void BLASInstanceBuilder::buildInstances(
    const std::vector<BVHNode>& nodes,
    const std::vector<PrimitiveRef>& primitives,
    std::vector<BLASInstance>& instances
)
{
    instances.clear();

    if (nodes.empty())
        return;

    for (const BVHNode& node : nodes)
    {
        if (!node.leaf)
            continue;

        if (node.primitiveCount == 0)
            continue;

        const uint32_t primitiveIndex = node.firstPrimitive;

        const PrimitiveRef& firstPrimitive = primitives[primitiveIndex];

        BLASInstance instance{};

        instance.vertexAddress = 0;
        instance.indexAddress = 0;

        instance.firstTriangle = firstPrimitive.index;

        instance.triangleCount = node.primitiveCount;

        instance.materialOffset = 0;

        instance.pad0 = 0;

        instances.emplace_back(instance);
    }
}