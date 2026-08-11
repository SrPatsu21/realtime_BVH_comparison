#pragma once

#include <cstddef>

class RenderBatch;

struct BatchRegistration
{
    RenderBatch* renderBatch = nullptr;
    size_t indexInBatch = 0;
};