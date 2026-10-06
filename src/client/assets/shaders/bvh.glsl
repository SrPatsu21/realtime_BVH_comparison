#ifndef BVH_GLSL
#define BVH_GLSL

// Requires GL_EXT_buffer_reference and GL_EXT_buffer_reference_uvec2
// to be enabled by the including shader.
// Define USE_BLAS_BVH8 / USE_TLAS_BVH8 at compile time (-D) to select BVH8.

// =========================================================
// Types
// =========================================================

struct AABB
{
    vec4 boundsMin;
    vec4 boundsMax;
};

struct BVHNode
{
    AABB childBounds[2];
    uint children[2];
    uint leaf;
    uint pad0;
};

struct BVH8Node
{
    AABB childBounds[8];
    uint children[8];
    uint childCount;
    uint leaf;
    uint pad0;
    uint pad1;
};

struct TLASInstance
{
    mat4 inverseTransform;

    uvec2 vertexAddress;
    uvec2 indexAddress;

    uint blasIndex;
    uint nodeOffset;
    uint nodeCount;
    uint instanceOffset;
};

struct BLASInstance
{
    uint firstTriangle;
    uint triangleCount;

    uint materialOffset;
    uint pad0;
};

struct RayHit
{
    float distance;
    uint materialIndex;
    vec2 uv;
};

// =========================================================
// Buffer references
// =========================================================

// Vertex layout in memory: 12 floats per vertex
// pos(3) + normal(3) + tangent(4) + texCoord(2)
layout(buffer_reference, std430)
readonly buffer VertexBuffer
{
    float data[];
};

layout(buffer_reference, std430)
readonly buffer IndexBuffer
{
    uint indices[];
};

vec3 loadVertexPosition(VertexBuffer vertices, uint vertexIndex)
{
    const uint base = vertexIndex * 12u;

    return vec3(
        vertices.data[base + 0u],
        vertices.data[base + 1u],
        vertices.data[base + 2u]
    );
}

vec2 loadVertexTexCoord(VertexBuffer vertices, uint vertexIndex)
{
    const uint base = vertexIndex * 12u;

    return vec2(
        vertices.data[base + 10u],
        vertices.data[base + 11u]
    );
}

// =========================================================
// Acceleration structure buffers (set 0)
// =========================================================

#if defined(USE_BLAS_BVH8)
layout(set = 0, binding = 1, std430)
readonly buffer BLASNodeBuffer
{
    BVH8Node blasNodes[];
};
#else
layout(set = 0, binding = 1, std430)
readonly buffer BLASNodeBuffer
{
    BVHNode blasNodes[];
};
#endif

layout(set = 0, binding = 2, std430)
readonly buffer BLASInstanceBuffer
{
    BLASInstance blasInstances[];
};

#if defined(USE_TLAS_BVH8)
layout(set = 0, binding = 3, std430)
readonly buffer TLASNodeBuffer
{
    BVH8Node tlasNodes[];
};
#else
layout(set = 0, binding = 3, std430)
readonly buffer TLASNodeBuffer
{
    BVHNode tlasNodes[];
};
#endif

layout(set = 0, binding = 4, std430)
readonly buffer TLASInstanceBuffer
{
    TLASInstance tlasInstances[];
};

// =========================================================
// Intersection helpers
// =========================================================

bool intersectAABB(
    vec3 origin,
    vec3 direction,
    vec3 boundsMin,
    vec3 boundsMax,
    float maxDistance
)
{
    vec3 invDirection = 1.0 / direction;

    vec3 t0 = (boundsMin - origin) * invDirection;
    vec3 t1 = (boundsMax - origin) * invDirection;

    vec3 tMin = min(t0, t1);
    vec3 tMax = max(t0, t1);

    float nearDistance = max(max(tMin.x, tMin.y), tMin.z);
    float farDistance  = min(min(tMax.x, tMax.y), tMax.z);

    return farDistance >= 0.0 &&
           nearDistance <= farDistance &&
           nearDistance <= maxDistance;
}

// Moller-Trumbore ray/triangle intersection.
bool intersectTriangle(
    vec3 origin,
    vec3 direction,
    vec3 v0,
    vec3 v1,
    vec3 v2,
    float maxDistance,
    out float hitDistance,
    out float hitU,
    out float hitV
)
{
    const float epsilon = 0.00001;

    vec3 edge1 = v1 - v0;
    vec3 edge2 = v2 - v0;

    vec3 p = cross(direction, edge2);
    float determinant = dot(edge1, p);

    if (abs(determinant) < epsilon)
        return false;

    float inverseDeterminant = 1.0 / determinant;

    vec3 s = origin - v0;
    float u = dot(s, p) * inverseDeterminant;

    if (u < 0.0 || u > 1.0)
        return false;

    vec3 q = cross(s, edge1);
    float v = dot(direction, q) * inverseDeterminant;

    if (v < 0.0 || u + v > 1.0)
        return false;

    float distance = dot(edge2, q) * inverseDeterminant;

    if (distance <= epsilon || distance >= maxDistance)
        return false;

    hitDistance = distance;
    hitU = u;
    hitV = v;

    return true;
}

// =========================================================
// Traversal
// =========================================================

const uint BVH_STACK_SIZE = 128u;

// Number of children actually used by a node.
#if defined(USE_BLAS_BVH8)
    #define BLAS_CHILD_COUNT(node) ((node).childCount)
#else
    #define BLAS_CHILD_COUNT(node) 2u
#endif

#if defined(USE_TLAS_BVH8)
    #define TLAS_CHILD_COUNT(node) ((node).childCount)
#else
    #define TLAS_CHILD_COUNT(node) 2u
#endif

// Tests every triangle of one BLAS leaf entry and updates closestHit.
bool intersectBLASLeafEntry(
    TLASInstance tlasInstance,
    VertexBuffer vertices,
    IndexBuffer indices,
    uint leafChildValue,
    vec3 origin,
    vec3 direction,
    inout RayHit closestHit
)
{
    bool foundHit = false;

    BLASInstance instance =
        blasInstances[tlasInstance.instanceOffset + leafChildValue];

    for (uint triangle = 0u; triangle < instance.triangleCount; ++triangle)
    {
        uint indexOffset = (instance.firstTriangle + triangle) * 3u;

        uint index0 = indices.indices[indexOffset + 0u];
        uint index1 = indices.indices[indexOffset + 1u];
        uint index2 = indices.indices[indexOffset + 2u];

        vec3 v0 = loadVertexPosition(vertices, index0);
        vec3 v1 = loadVertexPosition(vertices, index1);
        vec3 v2 = loadVertexPosition(vertices, index2);

        float hitDistance;
        float hitU;
        float hitV;

        if (intersectTriangle(
                origin, direction,
                v0, v1, v2,
                closestHit.distance,
                hitDistance, hitU, hitV))
        {
            vec2 uv0 = loadVertexTexCoord(vertices, index0);
            vec2 uv1 = loadVertexTexCoord(vertices, index1);
            vec2 uv2 = loadVertexTexCoord(vertices, index2);

            float hitW = 1.0 - hitU - hitV;

            closestHit.distance      = hitDistance;
            closestHit.materialIndex = instance.materialOffset;
            closestHit.uv            = uv0 * hitW + uv1 * hitU + uv2 * hitV;

            foundHit = true;
        }
    }

    return foundHit;
}

// BLAS traversal (object space).
bool traceBLAS(
    TLASInstance tlasInstance,
    vec3 origin,
    vec3 direction,
    float maxDistance,
    out RayHit closestHit
)
{
    closestHit.distance = maxDistance;
    closestHit.materialIndex = 0u;
    closestHit.uv = vec2(0.0);

    if (tlasInstance.nodeCount == 0u)
        return false;

    bool foundHit = false;

    uint stack[BVH_STACK_SIZE];
    uint stackSize = 0u;

    stack[stackSize++] = 0u;

    while (stackSize > 0u)
    {
        uint localNodeIndex = stack[--stackSize];

        if (localNodeIndex >= tlasInstance.nodeCount)
            continue;

#if defined(USE_BLAS_BVH8)
        BVH8Node node = blasNodes[tlasInstance.nodeOffset + localNodeIndex];
#else
        BVHNode node = blasNodes[tlasInstance.nodeOffset + localNodeIndex];
#endif

        const uint childCount = BLAS_CHILD_COUNT(node);

        // Leaf
        if (node.leaf != 0u)
        {
            VertexBuffer vertices = VertexBuffer(tlasInstance.vertexAddress);
            IndexBuffer indices   = IndexBuffer(tlasInstance.indexAddress);

            for (uint child = 0u; child < childCount; ++child)
            {
                if (!intersectAABB(
                        origin, direction,
                        node.childBounds[child].boundsMin.xyz,
                        node.childBounds[child].boundsMax.xyz,
                        closestHit.distance))
                {
                    continue;
                }

                if (intersectBLASLeafEntry(
                        tlasInstance, vertices, indices,
                        node.children[child],
                        origin, direction, closestHit))
                {
                    foundHit = true;
                }
            }

            continue;
        }

        // Internal node
        for (uint child = 0u; child < childCount; ++child)
        {
            if (!intersectAABB(
                    origin, direction,
                    node.childBounds[child].boundsMin.xyz,
                    node.childBounds[child].boundsMax.xyz,
                    closestHit.distance))
            {
                continue;
            }

            if (stackSize >= BVH_STACK_SIZE)
                break;

            stack[stackSize++] = node.children[child];
        }
    }

    return foundHit;
}

// Transforms the ray into instance space, traces the BLAS and
// updates closestHit with the world-space distance.
bool intersectTLASInstance(
    uint instanceIndex,
    vec3 origin,
    vec3 direction,
    inout RayHit closestHit
)
{
    TLASInstance instance = tlasInstances[instanceIndex];

    vec3 localOrigin =
        (instance.inverseTransform * vec4(origin, 1.0)).xyz;

    vec3 localDirectionRaw =
        (instance.inverseTransform * vec4(direction, 0.0)).xyz;

    float directionScale = length(localDirectionRaw);

    if (directionScale < 0.000001)
        return false;

    vec3 localDirection = localDirectionRaw / directionScale;
    float localMaxDistance = closestHit.distance * directionScale;

    RayHit localHit;

    if (!traceBLAS(instance, localOrigin, localDirection, localMaxDistance, localHit))
        return false;

    float worldDistance = localHit.distance / directionScale;

    if (worldDistance >= closestHit.distance)
        return false;

    closestHit.distance      = worldDistance;
    closestHit.materialIndex = localHit.materialIndex;
    closestHit.uv            = localHit.uv;

    return true;
}

// TLAS traversal (world space).
bool traceTLAS(
    vec3 origin,
    vec3 direction,
    float maxDistance,
    out RayHit closestHit
)
{
    closestHit.distance = maxDistance;
    closestHit.materialIndex = 0u;
    closestHit.uv = vec2(0.0);

    bool foundHit = false;

    uint stack[BVH_STACK_SIZE];
    uint stackSize = 0u;

    stack[stackSize++] = 0u;

    while (stackSize > 0u)
    {
        uint nodeIndex = stack[--stackSize];

#if defined(USE_TLAS_BVH8)
        BVH8Node node = tlasNodes[nodeIndex];
#else
        BVHNode node = tlasNodes[nodeIndex];
#endif

        const uint childCount = TLAS_CHILD_COUNT(node);

        // Leaf
        if (node.leaf != 0u)
        {
            for (uint child = 0u; child < childCount; ++child)
            {
                if (!intersectAABB(
                        origin, direction,
                        node.childBounds[child].boundsMin.xyz,
                        node.childBounds[child].boundsMax.xyz,
                        closestHit.distance))
                {
                    continue;
                }

                if (intersectTLASInstance(
                        node.children[child],
                        origin, direction, closestHit))
                {
                    foundHit = true;
                }
            }

            continue;
        }

        // Internal node
        for (uint child = 0u; child < childCount; ++child)
        {
            if (!intersectAABB(
                    origin, direction,
                    node.childBounds[child].boundsMin.xyz,
                    node.childBounds[child].boundsMax.xyz,
                    closestHit.distance))
            {
                continue;
            }

            if (stackSize >= BVH_STACK_SIZE)
                break;

            stack[stackSize++] = node.children[child];
        }
    }

    return foundHit;
}

#endif