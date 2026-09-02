#version 450

#extension GL_EXT_buffer_reference_uvec2 : require
#extension GL_EXT_buffer_reference : require

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;


// =========================================================
// BVH
// =========================================================

struct BVHNode
{
    vec4 min;
    vec4 max;

    uint left;
    uint right;

    uint leaf;
    uint firstPrimitive;
    uint primitiveCount;
    uint pad0;
};


// =========================================================
// TLAS
// =========================================================

struct TLASInstance
{
    vec4 boundsMin;
    vec4 boundsMax;

    mat4 inverseTransform;

    uint blasIndex;
    uint nodeOffset;
    uint nodeCount;
    uint instanceOffset;
};


// =========================================================
// BLAS leaf
// =========================================================

struct BLASInstance
{
    uvec2 vertexAddress;
    uvec2 indexAddress;

    uint firstTriangle;
    uint triangleCount;

    uint materialOffset;
    uint pad0;
};


// =========================================================
// Buffer references
// =========================================================

layout(buffer_reference, std430)
readonly buffer VertexBuffer
{
    vec3 positions[];
};

layout(buffer_reference, std430)
readonly buffer IndexBuffer
{
    uint indices[];
};


// =========================================================
// Acceleration structure buffers
// =========================================================

layout(set = 0, binding = 1, std430)
readonly buffer BLASNodeBuffer
{
    BVHNode blasNodes[];
};

layout(set = 0, binding = 2, std430)
readonly buffer BLASInstanceBuffer
{
    BLASInstance blasInstances[];
};

layout(set = 0, binding = 3, std430)
readonly buffer TLASNodeBuffer
{
    BVHNode tlasNodes[];
};

layout(set = 0, binding = 4, std430)
readonly buffer TLASInstanceBuffer
{
    TLASInstance tlasInstances[];
};


// =========================================================
// GBuffer
// =========================================================

layout(set = 1, binding = 0)
uniform sampler2DMS gPosition;

layout(set = 1, binding = 1)
uniform sampler2DMS gNormal;

layout(set = 1, binding = 2)
uniform sampler2DMS gAlbedo;

layout(set = 1, binding = 3)
uniform sampler2DMS gMaterial;

layout(set = 1, binding = 4)
uniform sampler2DMS gDepth;


// =========================================================
// Lights
// =========================================================

struct LightData
{
    vec3 position;
    float intensity;

    vec3 color;
    float radius;

    int type;
    float range;

    float pad0;
    float pad1;
};

layout(set = 2, binding = 0, std430)
readonly buffer LightBuffer
{
    LightData lights[];
};


// =========================================================
// AABB
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

    float nearDistance =
        max(
            max(tMin.x, tMin.y),
            tMin.z
        );

    float farDistance =
        min(
            min(tMax.x, tMax.y),
            tMax.z
        );

    return
        farDistance >= 0.0 &&
        nearDistance <= farDistance &&
        nearDistance <= maxDistance;
}


// =========================================================
// Triangle
// =========================================================

bool intersectTriangle(
    vec3 origin,
    vec3 direction,
    vec3 v0,
    vec3 v1,
    vec3 v2,
    float maxDistance
)
{
    const float epsilon = 0.000001;

    vec3 edge1 = v1 - v0;
    vec3 edge2 = v2 - v0;

    vec3 p =
        cross(
            direction,
            edge2
        );

    float determinant =
        dot(
            edge1,
            p
        );

    if (abs(determinant) < epsilon)
        return false;

    float inverseDeterminant = 1.0 / determinant;

    vec3 s = origin - v0;

    float u =
        dot(
            s,
            p
        ) *
        inverseDeterminant;

    if (u < 0.0 || u > 1.0)
        return false;

    vec3 q =
        cross(
            s,
            edge1
        );

    float v =
        dot(
            direction,
            q
        ) *
        inverseDeterminant;

    if (v < 0.0 || u + v > 1.0)
        return false;

    float distance =
        dot(
            edge2,
            q
        ) *
        inverseDeterminant;

    return
        distance > epsilon &&
        distance < maxDistance;
}


// =========================================================
// BLAS
// =========================================================

bool traceBLAS(
    TLASInstance tlasInstance,
    vec3 origin,
    vec3 direction,
    float maxDistance
)
{
    if (tlasInstance.nodeCount == 0)
        return false;

    uint stack[64];
    uint stackSize = 0;

    stack[stackSize++] =
        tlasInstance.nodeOffset;

    while (stackSize > 0)
    {
        uint nodeIndex =
            stack[--stackSize];

        if (
            nodeIndex <
            tlasInstance.nodeOffset ||
            nodeIndex >=
            tlasInstance.nodeOffset +
            tlasInstance.nodeCount
        )
        {
            continue;
        }

        BVHNode node = blasNodes[nodeIndex];

        if (!intersectAABB(
                origin,
                direction,
                node.min.xyz,
                node.max.xyz,
                maxDistance
            ))
        {
            continue;
        }

        if (node.leaf != 0)
        {
            uint first = node.firstPrimitive;

            uint count = node.primitiveCount;

            for (uint i = 0; i < count; ++i)
            {
                uint instanceIndex =
                    tlasInstance.instanceOffset +
                    first +
                    i;

                BLASInstance instance = blasInstances[instanceIndex];

                VertexBuffer vertices =
                    VertexBuffer(
                        instance.vertexAddress
                    );

                IndexBuffer indices =
                    IndexBuffer(
                        instance.indexAddress
                    );

                uint firstTriangle =
                    instance.firstTriangle;

                for (
                    uint triangle = 0;
                    triangle < instance.triangleCount;
                    ++triangle
                )
                {
                    uint triangleIndex = firstTriangle + triangle;

                    uint index0 = indices.indices[triangleIndex * 3 + 0];

                    uint index1 = indices.indices[triangleIndex * 3 + 1];

                    uint index2 = indices.indices[triangleIndex * 3 + 2];

                    vec3 v0 = vertices.positions[index0];
                    vec3 v1 = vertices.positions[index1];
                    vec3 v2 = vertices.positions[index2];

                    if (intersectTriangle(
                            origin,
                            direction,
                            v0,
                            v1,
                            v2,
                            maxDistance
                        ))
                    {
                        return true;
                    }
                }
            }

            continue;
        }

        if (stackSize + 2 > 64)
            return false;

        stack[stackSize++] =
            node.left;

        stack[stackSize++] =
            node.right;
    }

    return false;
}


// =========================================================
// TLAS
// =========================================================

bool traceTLAS(
    vec3 origin,
    vec3 direction,
    float maxDistance
)
{
    uint stack[64];
    uint stackSize = 0;

    stack[stackSize++] = 0;

    while (stackSize > 0)
    {
        uint nodeIndex = stack[--stackSize];

        BVHNode node = tlasNodes[nodeIndex];

        if (!intersectAABB(
                origin,
                direction,
                node.min.xyz,
                node.max.xyz,
                maxDistance
            ))
        {
            continue;
        }

        if (node.leaf != 0)
        {
            uint first =
                node.firstPrimitive;

            uint count =
                node.primitiveCount;

            for (uint i = 0; i < count; ++i)
            {
                TLASInstance instance =
                    tlasInstances[first + i];

                vec3 localOrigin =
                    (
                        instance.inverseTransform *
                        vec4(origin, 1.0)
                    ).xyz;

                vec3 localDirectionRaw =
                    (
                        instance.inverseTransform *
                        vec4(direction, 0.0)
                    ).xyz;

                float directionScale =
                    length(localDirectionRaw);

                if (directionScale < 0.000001)
                    continue;

                vec3 localDirection =
                    localDirectionRaw /
                    directionScale;

                float localMaxDistance =
                    maxDistance *
                    directionScale;

                if (traceBLAS(
                        instance,
                        localOrigin,
                        localDirection,
                        localMaxDistance
                    ))
                {
                    return true;
                }
            }

            continue;
        }

        if (stackSize + 2 > 64)
            return false;

        stack[stackSize++] =
            node.left;

        stack[stackSize++] =
            node.right;
    }

    return false;
}


// =========================================================
// Shadow ray
// =========================================================

bool traceShadowRay(
    vec3 origin,
    vec3 direction,
    float maxDistance
)
{
    return traceTLAS(
        origin,
        direction,
        maxDistance
    );
}


// =========================================================
// Main
// =========================================================

void main()
{
    ivec2 pixel = ivec2(gl_FragCoord.xy);

    vec3 position =
        texelFetch(
            gPosition,
            pixel,
            0
        ).xyz;

    vec3 normal =
        normalize(
            texelFetch(
                gNormal,
                pixel,
                0
            ).xyz
        );

    vec3 albedo =
        texelFetch(
            gAlbedo,
            pixel,
            0
        ).rgb;

    vec3 lighting = vec3(0.0);

    for (uint i = 0; i < lights.length(); ++i)
    {
        LightData light = lights[i];

        vec3 toLight = light.position - position;

        float distanceSq =
            dot(
                toLight,
                toLight
            );

        float rangeSq = light.range * light.range;

        if (distanceSq > rangeSq)
            continue;

        if (distanceSq < 0.000001)
            continue;

        float distanceToLight = sqrt(distanceSq);

        vec3 lightDirection = toLight / distanceToLight;

        float NdotL =
            dot(
                normal,
                lightDirection
            );

        if (NdotL <= 0.0)
            continue;

        const float shadowBias = 0.001;

        vec3 shadowOrigin = position + normal * shadowBias;

        // bool occluded = false;
        bool occluded =
            traceShadowRay(
                shadowOrigin,
                lightDirection,
                distanceToLight -
                shadowBias
            );

        if (occluded)
            continue;

        float attenuation =
            1.0 /
            max(
                distanceSq,
                0.01
            );

        float rangeFade =
            1.0 -
            smoothstep(
                0.0,
                light.range,
                distanceToLight
            );

        lighting +=
            albedo *
            light.color *
            light.intensity *
            NdotL *
            attenuation *
            rangeFade;
    }

    outColor =
        vec4(
            lighting,
            1.0
        );
}