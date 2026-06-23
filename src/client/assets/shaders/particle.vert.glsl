#version 450

layout(location = 0) out vec4 fragColor;

layout(std140, set = 0, binding = 0) uniform UniformBufferGlobal {
    mat4 view;
    mat4 proj;
} ubo;

struct Particle {
    vec3 position;
    float size;
    vec4 color;
};

layout(std430, set = 1, binding = 0) readonly buffer ParticleBuffer {
    Particle particles[];
};

void main() {
    Particle p = particles[gl_InstanceIndex];

    gl_Position = ubo.proj * ubo.view * vec4(p.position, 1.0);
    gl_PointSize = p.size;

    fragColor = p.color;
}