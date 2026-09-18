#version 450

#extension GL_EXT_buffer_reference_uvec2 : require
#extension GL_EXT_buffer_reference : require

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

// =========================================================
// BVH Nodes
// =========================================================

struct BVHNode
{
    vec4 min;
    vec4 max;

    uint left;
    uint right;

    uint leaf;
    uint pad0;
};

struct BVH8Node
{
    vec4 min;
    vec4 max;

    uint children[8];

    uint childCount;
    uint leaf;

    uint pad0;
    uint pad1;
};


// =========================================================
// TLAS
// =========================================================

struct TLASInstance
{
    vec4 boundsMin;
    vec4 boundsMax;

    mat4 inverseTransform;

    uvec2 vertexAddress;
    uvec2 indexAddress;

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
    vec4 boundsMin;
    vec4 boundsMax;

    uint firstTriangle;
    uint triangleCount;

    uint materialOffset;
    uint pad0;
};


// =========================================================
// Buffer references
// =========================================================

struct Vertex
{
    vec3 pos;
    vec3 normal;
    vec4 tangent;
    vec2 texCoord;
};

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

vec3 loadVertexPosition(
    VertexBuffer vertices,
    uint vertexIndex
)
{
    const uint base = vertexIndex * 12u;

    return vec3(
        vertices.data[base + 0u],
        vertices.data[base + 1u],
        vertices.data[base + 2u]
    );
}

vec2 loadVertexTexCoord(
    VertexBuffer vertices,
    uint vertexIndex
)
{
    const uint base = vertexIndex * 12u;

    return vec2(
        vertices.data[base + 10u],
        vertices.data[base + 11u]
    );
}


// =========================================================
// Acceleration structure buffers
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
// Ray
// =========================================================

struct RayHit
{
    float distance;
    uint materialIndex;
    vec2 uv;
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
// Transparent GBuffer
// =========================================================

layout(set = 3, binding = 0)
uniform sampler2DMS tgPosition;

layout(set = 3, binding = 1)
uniform sampler2DMS tgNormal;

layout(set = 3, binding = 2)
uniform sampler2DMS tgAlbedo;

layout(set = 3, binding = 3)
uniform sampler2DMS tgMaterial;


// =========================================================
// Material
// =========================================================

struct MaterialGPU
{
    vec4 baseColorFactor;

    float metallicFactor;
    float roughnessFactor;

    uint alphaMode;
    float alphaCutoff;

    uint baseColorTexture;
    uint normalTexture;
    uint metallicRoughnessTexture;

    uint _padding0;
};

layout(set = 4, binding = 0) readonly buffer MaterialBuffer
{
    MaterialGPU materials[];
};

layout(set = 4, binding = 1) uniform sampler2D textures[1024];


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
    float maxDistance,
    out float hitDistance,
    out float hitU,
    out float hitV
)
{
    const float epsilon = 0.00001;

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

    if (distance <= epsilon || distance >= maxDistance)
        return false;

    hitDistance = distance;
    hitU = u;
    hitV = v;

    return true;
}


// =========================================================
// BLAS
// =========================================================

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

    if (tlasInstance.nodeCount == 0)
        return false;

    bool foundHit = false;

    uint stack[128];
    uint stackSize = 0u;

    stack[stackSize++] = 0u;

    while (stackSize > 0u)
    {
        uint localNodeIndex =
            stack[--stackSize];

        if (localNodeIndex >= tlasInstance.nodeCount)
            continue;

        uint nodeIndex =
            tlasInstance.nodeOffset +
            localNodeIndex;


        // =====================================================
        // BVH8
        // =====================================================

#if defined(USE_BLAS_BVH8)

        BVH8Node node = blasNodes[nodeIndex];

        if (!intersectAABB(
                origin,
                direction,
                node.min.xyz,
                node.max.xyz,
                closestHit.distance
            ))
        {
            continue;
        }

        // -----------------------------------------------------
        // Leaf
        // -----------------------------------------------------

        if (node.leaf != 0u)
        {
            VertexBuffer vertices =
                VertexBuffer(tlasInstance.vertexAddress);

            IndexBuffer indices =
                IndexBuffer(tlasInstance.indexAddress);

            for (uint child = 0u; child < node.childCount; ++child)
            {
                uint instanceIndex =
                    tlasInstance.instanceOffset +
                    node.children[child];

                BLASInstance instance =
                    blasInstances[
                        instanceIndex
                    ];

                if (!intersectAABB(
                        origin,
                        direction,
                        instance.boundsMin.xyz,
                        instance.boundsMax.xyz,
                        closestHit.distance
                    ))
                {
                    continue;
                }

                for (
                    uint triangle = 0u;
                    triangle < instance.triangleCount;
                    ++triangle
                )
                {
                    uint triangleIndex =
                        instance.firstTriangle +
                        triangle;

                    uint indexOffset =
                        triangleIndex * 3u;

                    uint index0 =
                        indices.indices[
                            indexOffset + 0u
                        ];

                    uint index1 =
                        indices.indices[
                            indexOffset + 1u
                        ];

                    uint index2 =
                        indices.indices[
                            indexOffset + 2u
                        ];

                    vec3 v0 =
                        loadVertexPosition(
                            vertices,
                            index0
                        );

                    vec3 v1 =
                        loadVertexPosition(
                            vertices,
                            index1
                        );

                    vec3 v2 =
                        loadVertexPosition(
                            vertices,
                            index2
                        );

                    float hitDistance;
                    float hitU;
                    float hitV;

                    if (intersectTriangle(
                            origin,
                            direction,
                            v0,
                            v1,
                            v2,
                            closestHit.distance,
                            hitDistance,
                            hitU,
                            hitV
                        ))
                    {
                        vec2 uv0 =
                            loadVertexTexCoord(
                                vertices,
                                index0
                            );

                        vec2 uv1 =
                            loadVertexTexCoord(
                                vertices,
                                index1
                            );

                        vec2 uv2 =
                            loadVertexTexCoord(
                                vertices,
                                index2
                            );

                        float hitW =
                            1.0 -
                            hitU -
                            hitV;

                        closestHit.distance =
                            hitDistance;

                        closestHit.materialIndex =
                            instance.materialOffset;

                        closestHit.uv =
                            uv0 * hitW +
                            uv1 * hitU +
                            uv2 * hitV;

                        foundHit = true;
                    }
                }
            }

            continue;
        }

        for (uint child = 0u; child < node.childCount; ++child)
        {
            if (stackSize >= 128u)
                break;

            stack[stackSize++] = node.children[child];
        }

        continue;


// =====================================================
// Binary BVH
// =====================================================
#else

        BVHNode node = blasNodes[nodeIndex];

        if (!intersectAABB(
                origin,
                direction,
                node.min.xyz,
                node.max.xyz,
                closestHit.distance
            ))
        {
            continue;
        }

        // -----------------------------------------------------
        // Leaf
        // -----------------------------------------------------

        if (node.leaf != 0u)
        {
            VertexBuffer vertices =
                VertexBuffer(
                    tlasInstance.vertexAddress
                );

            IndexBuffer indices =
                IndexBuffer(
                    tlasInstance.indexAddress
                );

            uint instanceIndex =
                tlasInstance.instanceOffset +
                node.left;

            for (uint side = 0u; side < 2u; ++side)
            {
                BLASInstance instance =
                    blasInstances[instanceIndex];

                if (intersectAABB(
                        origin,
                        direction,
                        instance.boundsMin.xyz,
                        instance.boundsMax.xyz,
                        closestHit.distance
                    ))
                {
                    for (
                        uint triangle = 0u;
                        triangle < instance.triangleCount;
                        ++triangle
                    )
                    {
                        uint triangleIndex =
                            instance.firstTriangle +
                            triangle;

                        uint indexOffset =
                            triangleIndex * 3u;

                        uint index0 =
                            indices.indices[indexOffset + 0u];

                        uint index1 =
                            indices.indices[indexOffset + 1u];

                        uint index2 =
                            indices.indices[indexOffset + 2u];

                        vec3 v0 =
                            loadVertexPosition(
                                vertices,
                                index0
                            );

                        vec3 v1 =
                            loadVertexPosition(
                                vertices,
                                index1
                            );

                        vec3 v2 =
                            loadVertexPosition(
                                vertices,
                                index2
                            );

                        float hitDistance;
                        float hitU;
                        float hitV;

                        if (intersectTriangle(
                                origin,
                                direction,
                                v0,
                                v1,
                                v2,
                                closestHit.distance,
                                hitDistance,
                                hitU,
                                hitV
                            ))
                        {
                            vec2 uv0 =
                                loadVertexTexCoord(
                                    vertices,
                                    index0
                                );

                            vec2 uv1 =
                                loadVertexTexCoord(
                                    vertices,
                                    index1
                                );

                            vec2 uv2 =
                                loadVertexTexCoord(
                                    vertices,
                                    index2
                                );

                            float hitW =
                                1.0 -
                                hitU -
                                hitV;

                            closestHit.distance =
                                hitDistance;

                            closestHit.materialIndex =
                                instance.materialOffset;

                            closestHit.uv =
                                uv0 * hitW +
                                uv1 * hitU +
                                uv2 * hitV;

                            foundHit = true;
                        }
                    }
                }

                if (side == 0u)
                {
                    instanceIndex =
                        tlasInstance.instanceOffset +
                        node.right;
                }
            }

            continue;
        }

        // -----------------------------------------------------
        // Internal binary node
        // -----------------------------------------------------

        if (stackSize + 2u > 128u)
            continue;

        stack[stackSize++] = node.right;
        stack[stackSize++] = node.left;
#endif
    }

    return foundHit;
}


// =========================================================
// TLAS
// =========================================================

bool traceTLAS(
    vec3 origin,
    vec3 direction,
    float maxDistance,
    out RayHit closestHit
)
{
    closestHit.distance =
        maxDistance;

    closestHit.materialIndex =
        0u;

    closestHit.uv =
        vec2(0.0);

    bool foundHit =
        false;

    uint stack[128];
    uint stackSize =
        0u;

    stack[
        stackSize++
    ] = 0u;

    while (stackSize > 0u)
    {
        uint nodeIndex =
            stack[
                --stackSize
            ];


        // =====================================================
        // BVH8
        // =====================================================

#if defined(USE_TLAS_BVH8)

        BVH8Node node =
            tlasNodes[nodeIndex];

        if (!intersectAABB(
                origin,
                direction,
                node.min.xyz,
                node.max.xyz,
                closestHit.distance
            ))
        {
            continue;
        }

        // -----------------------------------------------------
        // Leaf
        // -----------------------------------------------------

        if (node.leaf != 0u)
        {
            for (
                uint child = 0u;
                child < node.childCount;
                ++child
            )
            {
                uint instanceIndex =
                    node.children[child];

                TLASInstance instance =
                    tlasInstances[
                        instanceIndex
                    ];

                vec3 localOrigin =
                    (
                        instance.inverseTransform *
                        vec4(
                            origin,
                            1.0
                        )
                    ).xyz;

                vec3 localDirectionRaw =
                    (
                        instance.inverseTransform *
                        vec4(
                            direction,
                            0.0
                        )
                    ).xyz;

                float directionScale =
                    length(
                        localDirectionRaw
                    );

                if (
                    directionScale <
                    0.000001
                )
                {
                    continue;
                }

                vec3 localDirection =
                    localDirectionRaw /
                    directionScale;

                float localMaxDistance =
                    closestHit.distance *
                    directionScale;

                RayHit localHit;

                if (traceBLAS(
                        instance,
                        localOrigin,
                        localDirection,
                        localMaxDistance,
                        localHit
                    ))
                {
                    float worldDistance =
                        localHit.distance /
                        directionScale;

                    if (
                        worldDistance <
                        closestHit.distance
                    )
                    {
                        closestHit.distance =
                            worldDistance;

                        closestHit.materialIndex =
                            localHit.materialIndex;

                        closestHit.uv =
                            localHit.uv;

                        foundHit =
                            true;
                    }
                }
            }

            continue;
        }

        // -----------------------------------------------------
        // Internal BVH8 node
        // -----------------------------------------------------

        for (
            uint child = 0u;
            child < node.childCount;
            ++child
        )
        {
            if (stackSize >= 128u)
                break;

            stack[
                stackSize++
            ] =
                node.children[child];
        }

        continue;

        // =====================================================
        // Binary BVH
        // =====================================================

#else

        BVHNode node =
            tlasNodes[nodeIndex];

        if (!intersectAABB(
                origin,
                direction,
                node.min.xyz,
                node.max.xyz,
                closestHit.distance
            ))
        {
            continue;
        }

        // -----------------------------------------------------
        // Leaf
        // -----------------------------------------------------

        if (node.leaf != 0u)
        {
            for (
                uint side = 0u;
                side < 2u;
                ++side
            )
            {
                uint instanceIndex =
                    side == 0u ?
                    node.left :
                    node.right;

                TLASInstance instance =
                    tlasInstances[instanceIndex];

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
                    closestHit.distance *
                    directionScale;

                RayHit localHit;

                if (traceBLAS(
                        instance,
                        localOrigin,
                        localDirection,
                        localMaxDistance,
                        localHit
                    ))
                {
                    float worldDistance = localHit.distance / directionScale;

                    if (worldDistance < closestHit.distance)
                    {
                        closestHit.distance =
                            worldDistance;

                        closestHit.materialIndex =
                            localHit.materialIndex;

                        closestHit.uv =
                            localHit.uv;

                        foundHit = true;
                    }
                }
            }

            continue;
        }

        // -----------------------------------------------------
        // Internal binary node
        // -----------------------------------------------------

        if (stackSize + 2u > 128u)
            continue;

        stack[stackSize++] = node.right;
        stack[stackSize++] = node.left;
#endif
    }

    return foundHit;
}


// =========================================================
// Shadow ray
// =========================================================

vec4 traceShadowRay(
    vec3 origin,
    vec3 direction,
    float maxDistance,
    vec4 light
)
{
    const float rayEpsilon = 0.001;

    const uint maxHits = 16u;

    vec4 transmittedLight = light;

    for (uint hitIndex = 0u; hitIndex < maxHits; ++hitIndex)
    {
        RayHit hit;

        if (!traceTLAS(
                origin,
                direction,
                maxDistance,
                hit
            ))
        {
            return transmittedLight;
        }

        MaterialGPU material = materials[hit.materialIndex];

        float alpha =
            texture(
                textures[material.baseColorTexture],
                hit.uv
            ).a *
            material.baseColorFactor.a;

        if (material.alphaMode == 1u)
        {
            return vec4(0.0);
        }

        if (material.alphaMode == 2u)
        {
            if (alpha >= material.alphaCutoff)
                return vec4(0.0);
        }

        if (material.alphaMode == 4u)
        {
            transmittedLight *= 1.0 - alpha;
        }

        origin += direction * (hit.distance + rayEpsilon);

        maxDistance -= hit.distance + rayEpsilon;

        if (maxDistance <= 0.0)
            return transmittedLight;

        if (transmittedLight.a <= 0.00001)
            return vec4(0.0);
    }

    return transmittedLight;
}


// =========================================================
// Lighting
// =========================================================

vec3 calculateLighting(
    vec3 position,
    vec3 normal,
    vec3 albedo
)
{
    vec3 lighting = vec3(0.0);

    for (uint i = 0u; i < lights.length(); ++i)
    {
        LightData light = lights[i];

        vec3 toLight = light.position - position;

        float distanceSq =
            dot(
                toLight,
                toLight
            );

        float rangeSq =
            light.range *
            light.range;

        if (distanceSq > rangeSq)
            continue;

        if (distanceSq < 0.000001)
            continue;

        float distanceToLight =
            sqrt(distanceSq);

        vec3 lightDirection =
            toLight /
            distanceToLight;

        float NdotL =
            dot(
                normal,
                lightDirection
            );

        if (NdotL <= 0.0)
            continue;

        const float shadowBias = 0.01;

        vec3 shadowOrigin =
            position +
            normal *
            shadowBias;

        vec4 transmittedLight =
            traceShadowRay(
                shadowOrigin,
                lightDirection,
                distanceToLight -
                    shadowBias,
                vec4(1.0)
            );

        if (transmittedLight.a <= 0.00001)
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
            rangeFade *
            transmittedLight.rgb;
    }

    return lighting;
}

// =========================================================
// Main
// =========================================================

void main()
{
    ivec2 pixel = ivec2(gl_FragCoord.xy);

    int this_sample = gl_SampleID;

    // =========================================================
    // GBuffer
    // =========================================================

    vec3 position =
        texelFetch(
            gPosition,
            pixel,
            this_sample
        ).xyz;

    vec3 normal =
        normalize(
            texelFetch(
                gNormal,
                pixel,
                this_sample
            ).xyz
        );

    vec3 albedo =
        texelFetch(
            gAlbedo,
            pixel,
            this_sample
        ).rgb;

    vec3 lighting =
        calculateLighting(
            position,
            normal,
            albedo
        );

    // =========================================================
    // Transparent GBuffer
    // =========================================================

    vec4 transparentAlbedo =
        texelFetch(
            tgAlbedo,
            pixel,
            this_sample
        );

    bool hasTransparent =
        transparentAlbedo.a > 0.0;

    if (hasTransparent)
    {
        vec3 transparentPosition =
            texelFetch(
                tgPosition,
                pixel,
                this_sample
            ).xyz;

        vec3 transparentNormal =
            normalize(
                texelFetch(
                    tgNormal,
                    pixel,
                    this_sample
                ).xyz
            );

        vec3 transparentLighting =
            calculateLighting(
                transparentPosition,
                transparentNormal,
                transparentAlbedo.rgb
            );

        float transparentAlpha = transparentAlbedo.a;

        lighting =
            mix(
                lighting,
                transparentLighting,
                transparentAlpha
            );
    }

    // =========================================================
    // Output
    // =========================================================
    outColor =
        vec4(
            lighting,
            1.0
        );
}