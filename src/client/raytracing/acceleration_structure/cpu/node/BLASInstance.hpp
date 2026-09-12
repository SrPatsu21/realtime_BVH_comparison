#pragma once

#include <cstdint>

struct BLASInstance
{
    AABB bounds; // 64

    uint32_t firstTriangle; //4
    uint32_t triangleCount; //4

    uint32_t materialOffset; //4
    uint32_t pad0; //4
}; //16 + 64