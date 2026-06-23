#pragma once

#include <glm/glm.hpp>

struct UniformBufferGlobal {
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};