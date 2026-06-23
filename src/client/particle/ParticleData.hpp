#pragma once
#include <glm/glm.hpp>

struct ParticleData {
    glm::vec4 positionSize; // xyz = pos, w = size
    glm::vec4 color;
};