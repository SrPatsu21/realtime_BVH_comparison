#version 450

layout(location = 0) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform sampler2DMS gPosition;
layout(set = 1, binding = 1) uniform sampler2DMS gNormal;
layout(set = 1, binding = 2) uniform sampler2DMS gAlbedo;
layout(set = 1, binding = 3) uniform sampler2DMS gMaterial;
layout(set = 1, binding = 4) uniform sampler2DMS gDepth;

void main()
{
    ivec2 pixel = ivec2(gl_FragCoord.xy);

    vec4 albedo = texelFetch(
        gAlbedo,
        pixel,
        0
    );

    outColor = albedo;
}