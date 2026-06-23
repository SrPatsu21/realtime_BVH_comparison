#version 450

layout(set = 1, binding = 0) uniform sampler2D albedoTex;
layout(set = 1, binding = 1) uniform sampler2D normalTex;
layout(set = 1, binding = 2) uniform sampler2D metallicRoughnessTex;

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragWorldPos;
layout(location = 3) in vec4 fragTangent;

layout(location = 0) out vec4 outColor;

void main()
{
    // Base Color

    vec4 albedo = texture(albedoTex, fragTexCoord);

    // Normal Map
    vec3 tangentNormal = texture(normalTex, fragTexCoord).xyz * 2.0 - 1.0;

    vec3 N = normalize(fragNormal);

    vec3 T = normalize(fragTangent.xyz);
    vec3 B = normalize(cross(N, T) * fragTangent.w);

    mat3 TBN = mat3(T, B, N);

    N = normalize(TBN * tangentNormal);

    // Metallic/Roughness

    vec4 mr = texture(metallicRoughnessTex, fragTexCoord);

    float metallic = mr.b;
    float roughness = mr.g;

    // Light

    vec3 lightDir = normalize(vec3(0.5,1.0,0.3));

    float diffuse = max(dot(N, lightDir), 0.0);

    // Specular
    vec3 viewDir = normalize(-fragWorldPos);

    vec3 halfDir = normalize(lightDir + viewDir);

    float spec = pow(max(dot(N, halfDir),0.0), mix(64.0,4.0,roughness));

    spec *= mix(0.04,1.0,metallic);

    //--------------------------------

    vec3 color = albedo.rgb * diffuse + vec3(spec);

    outColor = vec4(color, albedo.a);
}