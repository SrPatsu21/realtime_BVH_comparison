#version 450

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_buffer_reference_uvec2 : require
#extension GL_EXT_buffer_reference : require

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

layout(std140, set = 0, binding = 0) uniform UniformBufferGlobal
{
    mat4 view;
    mat4 proj;
    vec4 cameraPosition;
} ubo;

// BVH / TLAS / BLAS traversal
#include "bvh.glsl"

// =========================================================
// GBuffer
// =========================================================

layout(set = 1, binding = 0) uniform sampler2D gPosition;
layout(set = 1, binding = 1) uniform sampler2D gNormal;
layout(set = 1, binding = 2) uniform sampler2D gAlbedo;
layout(set = 1, binding = 3) uniform sampler2D gMaterial;
layout(set = 1, binding = 4) uniform sampler2D gDepth;

// Transparent GBuffer
layout(set = 3, binding = 0) uniform sampler2D tgPosition;
layout(set = 3, binding = 1) uniform sampler2D tgNormal;
layout(set = 3, binding = 2) uniform sampler2D tgAlbedo;
layout(set = 3, binding = 3) uniform sampler2D tgMaterial;

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

    uint doubleSided;
};

layout(set = 4, binding = 0) readonly buffer MaterialBuffer
{
    MaterialGPU materials[];
};

layout(set = 4, binding = 1) uniform sampler2D textures[1024];

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

        if (!traceTLAS(origin, direction, maxDistance, hit))
            return transmittedLight;

        MaterialGPU material = materials[hit.materialIndex];

        if (material.alphaMode == 1u)
            return vec4(0.0);

        vec4 baseColor =
            texture(textures[material.baseColorTexture], hit.uv) *
            material.baseColorFactor;

        float alpha = baseColor.a;

        if (material.alphaMode == 2u)
        {
            if (alpha >= material.alphaCutoff)
                return vec4(0.0);
        }

        if (material.alphaMode == 4u)
        {
            transmittedLight.rgb *= mix(vec3(1.0), baseColor.rgb, alpha);
            transmittedLight.a *= 1.0 - alpha;
        }

        float advance = hit.distance + rayEpsilon;

        origin += direction * advance;
        maxDistance -= advance;

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
    vec3 albedo,
    bool twoSided
)
{
    vec3 lighting = vec3(0.0);

    for (uint i = 0u; i < lights.length(); ++i)
    {
        LightData light = lights[i];

        vec3 toLight = light.position - position;
        float distanceSq = dot(toLight, toLight);
        float rangeSq = light.range * light.range;

        if (distanceSq > rangeSq)
            continue;

        if (distanceSq < 0.000001)
            continue;

        float distanceToLight = sqrt(distanceSq);
        vec3 lightDirection = toLight / distanceToLight;

        // Two-sided lighting: flip the normal when facing away from the light
        vec3 shadingNormal = normal;
        float NdotL = dot(shadingNormal, lightDirection);

        if (twoSided)
        {
            if (NdotL < 0.0)
            {
                shadingNormal = -shadingNormal;
                NdotL = -NdotL;
            }
        }
        else
        {
            if (NdotL <= 0.0)
                continue;
        }

        const float shadowBias = 0.01;

        vec3 shadowOrigin = position + shadingNormal * shadowBias;

        vec4 transmittedLight = traceShadowRay(
            shadowOrigin,
            lightDirection,
            distanceToLight - shadowBias,
            vec4(1.0)
        );

        if (transmittedLight.a <= 0.00001)
            continue;

        float attenuation = 1.0 / max(distanceSq, 0.01);
        float rangeFade = 1.0 - smoothstep(0.0, light.range, distanceToLight);

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

    vec3 lighting = vec3(0.0);

    // Opaque GBuffer
    vec4 positionSample = texelFetch(gPosition, pixel, this_sample);

    if (positionSample.a > 0.0)
    {
        vec3 normal = normalize(texelFetch(gNormal, pixel, this_sample).xyz);
        vec3 albedo = texelFetch(gAlbedo, pixel, this_sample).rgb;

        vec3 position =
            positionSample.xyz +
            ubo.cameraPosition.xyz;

        lighting = calculateLighting(position, normal, albedo, false);
    }

    // Transparent GBuffer
    vec4 transparentAlbedo = texelFetch(tgAlbedo, pixel, this_sample);

    if (transparentAlbedo.a > 0.0)
    {
        vec3 transparentPosition = texelFetch(tgPosition, pixel, this_sample).xyz + ubo.cameraPosition.xyz;
        vec3 transparentNormal   = normalize(texelFetch(tgNormal, pixel, this_sample).xyz);

        vec3 transparentLighting = calculateLighting(
            transparentPosition,
            transparentNormal,
            transparentAlbedo.rgb,
            true
        );

        lighting = mix(lighting, transparentLighting, transparentAlbedo.a);
    }

    outColor = vec4(lighting, 1.0);
}