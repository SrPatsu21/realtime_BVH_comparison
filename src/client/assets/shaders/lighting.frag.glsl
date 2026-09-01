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

// ============================================================
// Shadow Ray / BVH
// ============================================================
//
// Retorna:
//   true  -> raio encontrou geometria antes da luz
//   false -> raio chegou à luz sem encontrar geometria
//
// Substitua o corpo pela implementação da sua BVH.
//

bool traceShadowRay(
    vec3 origin,
    vec3 direction,
    float maxDistance
)
{
    // TODO:
    // acessar TLAS/BLAS/BVH aqui

    return false;
}

// ============================================================
// Main
// ============================================================

void main()
{
    ivec2 pixel = ivec2(gl_FragCoord.xy);

    // --------------------------------------------------------
    // GBuffer
    // --------------------------------------------------------

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

    // --------------------------------------------------------
    // Lighting
    // --------------------------------------------------------

    vec3 lighting = vec3(0.0);

    for (uint i = 0; i < lights.length(); i++)
    {
        LightData light = lights[i];

        // ====================================================
        // 1. Direção até a luz
        // ====================================================

        vec3 toLight = light.position - position;

        // Evita sqrt neste primeiro teste.
        float distanceSq = dot(toLight, toLight);

        float rangeSq = light.range * light.range;

        // ----------------------------------------------------
        // Luz fora do alcance.
        //
        // Não precisamos calcular:
        // - sqrt
        // - direção
        // - NdotL
        // - shadow ray
        // ----------------------------------------------------

        if (distanceSq > rangeSq)
            continue;

        // Proteção contra posição exatamente igual à luz.
        if (distanceSq < 0.000001)
            continue;

        // ====================================================
        // 2. Distância real
        // ====================================================

        float distanceToLight = sqrt(distanceSq);

        // ====================================================
        // 3. Direção da luz
        // ====================================================

        vec3 lightDirection = toLight / distanceToLight;

        // ====================================================
        // 4. NdotL
        //
        // Se a superfície está virada para o lado oposto,
        // não existe contribuição.
        //
        // Portanto não precisamos gastar um shadow ray.
        // ====================================================

        float NdotL = dot(
            normal,
            lightDirection
        );

        if (NdotL <= 0.0)
            continue;

        // ====================================================
        // 5. Shadow Ray / BVH
        // ====================================================
        //
        // Pequeno offset para evitar self-intersection.
        //
        // O raio começa ligeiramente acima da superfície e
        // percorre somente a distância necessária até a luz.
        // ====================================================

        const float shadowBias = 0.001;

        vec3 shadowOrigin =
            position +
            normal * shadowBias;

        bool occluded = traceShadowRay(
            shadowOrigin,
            lightDirection,
            distanceToLight - shadowBias
        );

        // ----------------------------------------------------
        // Se encontrou geometria antes da luz:
        //
        // contribuição desta luz = 0
        // ----------------------------------------------------

        if (occluded)
            continue;

        // ====================================================
        // 6. Atenuação
        // ====================================================

        float attenuation =
            1.0 /
            max(
                distanceSq,
                0.01
            );

        // ====================================================
        // 7. Fade do range
        // ====================================================

        float rangeFade =
            1.0 -
            smoothstep(
                0.0,
                light.range,
                distanceToLight
            );

        // ====================================================
        // 8. Contribuição final desta luz
        // ====================================================

        lighting +=
            albedo *
            light.color *
            light.intensity *
            NdotL *
            attenuation *
            rangeFade;
    }

    // ========================================================
    // Output
    // ========================================================

    outColor = vec4(lighting, 1.0);
}