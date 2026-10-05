#pragma once

#include <cstdint>

struct BLASInstance
{
    uint32_t firstTriangle; //4
    uint32_t triangleCount; //4

    uint32_t materialOffset; //4
    uint32_t pad0; //4
}; //16