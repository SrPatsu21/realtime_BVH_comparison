#version 450

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

layout(set = 1, binding = 0) readonly buffer MaterialBuffer
{
    MaterialGPU materials[];
};

layout(set = 1, binding = 1) uniform sampler2D baseColorTextures[1024];
layout(set = 1, binding = 2) uniform sampler2D normalTextures[1024];
layout(set = 1, binding = 3) uniform sampler2D metallicRoughnessTextures[1024];

layout(push_constant) uniform MaterialPushConstant
{
    uint materialIndex;
} materialPush;

// --------------------------------------------------
// Vertex inputs
// --------------------------------------------------

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragWorldPos;
layout(location = 3) in vec4 fragTangent;

// --------------------------------------------------
// GBuffer attachments
// --------------------------------------------------

layout(location = 0) out vec4 outPosition;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outAlbedo;
layout(location = 3) out vec4 outMaterial;

void main()
{
    MaterialGPU material = materials[materialPush.materialIndex];

    // --------------------------------------------------
    // Albedo
    // --------------------------------------------------

    vec4 albedo =
        texture(
            baseColorTextures[material.baseColorTexture],
            fragTexCoord
        ) *
        material.baseColorFactor;

    // --------------------------------------------------
    // Normal map
    // --------------------------------------------------

    vec3 tangentNormal =
        texture(
            normalTextures[material.normalTexture],
            fragTexCoord
        ).xyz * 2.0 - 1.0;

    vec3 N = normalize(fragNormal);

    vec3 T = normalize(fragTangent.xyz);

    T = normalize(T - N * dot(N, T));

    vec3 B = normalize(cross(N, T) * fragTangent.w);

    mat3 TBN = mat3(T, B, N);

    N = normalize(TBN * tangentNormal);

    // --------------------------------------------------
    // Metallic / Roughness
    // --------------------------------------------------

    vec4 mr =
        texture(
            metallicRoughnessTextures[
                material.metallicRoughnessTexture
            ],
            fragTexCoord
        );

    float metallic =
        mr.b *
        material.metallicFactor;

    float roughness =
        mr.g *
        material.roughnessFactor;

    // --------------------------------------------------
    // GBuffer
    // --------------------------------------------------

    outPosition = vec4(fragWorldPos, 1.0);

    outNormal = vec4(N, 1.0);

    outAlbedo = albedo;

    outMaterial =
        vec4(
            metallic,
            roughness,
            0.0,
            1.0
        );
}