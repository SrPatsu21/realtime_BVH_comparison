#pragma once
#include <cstdint>
#include <glm/glm.hpp>

struct LightData
{
    glm::vec3 position;
    float intensity;

    glm::vec3 color;
    float radius;

    uint32_t type;
    float range;

    float _pad0;
    float _pad1;
};