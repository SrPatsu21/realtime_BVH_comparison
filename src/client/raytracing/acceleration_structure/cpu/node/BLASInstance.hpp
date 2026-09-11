#pragma once

#include <cstdint>

struct BLASInstance
{
    uint32_t firstTriangle;
    uint32_t triangleCount;

    uint32_t materialOffset;
    uint32_t pad0;
}; //16