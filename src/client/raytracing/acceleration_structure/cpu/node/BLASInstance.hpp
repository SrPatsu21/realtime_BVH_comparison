#pragma once

#include <cstdint>

struct BLASInstance
{
    uint64_t vertexAddress;
    uint64_t indexAddress;

    uint32_t firstTriangle;
    uint32_t triangleCount;

    uint32_t materialOffset;
    uint32_t pad0;
}; //32