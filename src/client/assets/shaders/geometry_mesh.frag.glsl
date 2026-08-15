#version 450

layout(set = 1, binding = 0) uniform sampler2D albedoTex;
layout(set = 1, binding = 1) uniform sampler2D normalTex;
layout(set = 1, binding = 2) uniform sampler2D metallicRoughnessTex;

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragWorldPos;
layout(location = 3) in vec4 fragTangent;


// GBuffer attachments

layout(location = 0) out vec4 outPosition;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outAlbedo;
layout(location = 3) out vec4 outMaterial;


void main()
{
    // --------------------------------------------------
    // Albedo
    // --------------------------------------------------

    vec4 albedo =
        texture(albedoTex, fragTexCoord);


    // --------------------------------------------------
    // Normal map
    // --------------------------------------------------

    vec3 tangentNormal = texture(normalTex, fragTexCoord).xyz * 2.0 - 1.0;

    vec3 N = normalize(fragNormal);

    vec3 T = normalize(fragTangent.xyz);

    // Re-orthogonalize T against N.
    T = normalize(T - N * dot(N, T));

    vec3 B =normalize(cross(N, T) * fragTangent.w);

    mat3 TBN = mat3(T, B, N);

    N = normalize(TBN * tangentNormal);

    // --------------------------------------------------
    // Metallic / Roughness
    // --------------------------------------------------

    vec4 mr = texture(metallicRoughnessTex, fragTexCoord);

    float metallic = mr.b;
    float roughness = mr.g;


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