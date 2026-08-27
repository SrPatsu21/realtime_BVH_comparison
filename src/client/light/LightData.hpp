#pragma once
#include <cstdint>
#include <glm/glm.hpp>
#include "../ConfigTable.hpp"
struct LightData
{
    glm::vec3 position;
    float intensity;

    glm::vec3 color;
    float radius;

    Config::LightType type;
    float range;

    float _pad0;
    float _pad1;
};