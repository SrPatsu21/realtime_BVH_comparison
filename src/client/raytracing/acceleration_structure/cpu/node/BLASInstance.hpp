#pragma once

#include <cstdint>

struct BLASInstance
{
    uint64_t vertexAddress; //8
    uint64_t indexAddress; //8

    uint32_t nodeOffset; //4
    uint32_t nodeCount; //4

    uint32_t triangleCount; //4
    uint32_t pad0; //4
};