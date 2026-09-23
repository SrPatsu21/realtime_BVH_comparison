#pragma once

#include <memory>
#include <cstdint>
#include "../texture/TextureImage.hpp"

#include <vulkan/vulkan.h>
#include <glm/vec4.hpp>

struct MaterialData
{
    enum class AlphaMode : uint32_t
    {
        OPAQUE = 1,
        MASK   = 2,
        BLEND  = 4
    };

    glm::vec4 baseColorFactor{1.0f};

    float metallicFactor = 1.0f;
    float roughnessFactor = 1.0f;

    AlphaMode alphaMode = AlphaMode::OPAQUE;
    float alphaCutoff = 0.5f;

    bool doubleSided = false;

    uint32_t baseColor;
    uint32_t normal;
    uint32_t metallicRoughness;
};

struct MaterialCPU
{
    uint32_t materialOffset;

    MaterialData::AlphaMode alphaMode = MaterialData::AlphaMode::OPAQUE;

    uint32_t baseColor;
    uint32_t normal;
    uint32_t metallicRoughness;
};

struct MaterialGPU
{
    glm::vec4 baseColorFactor; // 16

    float metallicFactor; // 4
    float roughnessFactor; // 4

    uint32_t alphaMode; // 4
    float alphaCutoff; // 4

    uint32_t baseColorTexture; // 4
    uint32_t normalTexture; // 4
    uint32_t metallicRoughnessTexture; // 4

    uint32_t doubleSided; // 4
}; // 48