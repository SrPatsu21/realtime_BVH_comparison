#version 450

layout(location = 0) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

// ============================================================
// GBuffer
// ============================================================

layout(set = 1, binding = 0) uniform sampler2DMS gPosition;
layout(set = 1, binding = 1) uniform sampler2DMS gNormal;
layout(set = 1, binding = 2) uniform sampler2DMS gAlbedo;
layout(set = 1, binding = 3) uniform sampler2DMS gMaterial;
layout(set = 1, binding = 4) uniform sampler2DMS gDepth;

// ============================================================
// Lights
// ============================================================

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

layout(set = 2, binding = 0, std430) readonly buffer LightBuffer
{
    LightData lights[];
};

void main()
{
    ivec2 pixel = ivec2(gl_FragCoord.xy);

    vec3 position = texelFetch(
        gPosition,
        pixel,
        0
    ).xyz;

    vec3 normal = normalize(
        texelFetch(
            gNormal,
            pixel,
            0
        ).xyz
    );

    vec3 albedo = texelFetch(
        gAlbedo,
        pixel,
        0
    ).rgb;

    vec3 lighting = vec3(0.0);

    for (uint i = 0; i < lights.length(); i++)
    {
        LightData light = lights[i];

        vec3 toLight = light.position - position;

        float distanceToLight = length(toLight);

        if (distanceToLight > light.range)
            continue;

        vec3 lightDirection = toLight / max(distanceToLight, 0.0001);

        float NdotL = max(
            dot(normal, lightDirection),
            0.0
        );

        float attenuation =
            1.0 /
            max(
                distanceToLight * distanceToLight,
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

    outColor = vec4(lighting, 1.0);
}