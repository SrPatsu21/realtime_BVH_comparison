#pragma once
#include <cstdint>

struct SubMesh
{
    uint32_t firstIndex;
    uint32_t indexCount;
    int32_t vertexOffset;
    uint32_t materialIndex;
};