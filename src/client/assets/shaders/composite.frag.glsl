#version 450

layout(constant_id = 0) const int SAMPLE_COUNT = 1;

layout(set = 0, binding = 0)
uniform sampler2DMS deferredLighting;

layout(location = 0) out vec4 outColor;

void main()
{
    ivec2 pixel = ivec2(gl_FragCoord.xy);

    vec4 color = vec4(0.0);

    for (int i = 0; i < SAMPLE_COUNT; ++i)
    {
        color += texelFetch(
            deferredLighting,
            pixel,
            i
        );
    }

    color /= float(SAMPLE_COUNT);

    outColor = color;
}