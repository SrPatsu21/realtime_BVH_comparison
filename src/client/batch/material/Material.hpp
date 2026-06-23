#pragma once

#include "../texture/TextureImage.hpp"
#include "MaterialDescriptorManager.hpp"
#include <memory>

/**
 * @brief CPU-side representation of a material for bindless rendering.
 *
 * A Material stores indices into global bindless descriptor arrays.
 *
 * These indices are used in the shader to fetch textures.
 */
class Material
{
private:
    std::shared_ptr<TextureImage> baseColorHandle;
    std::shared_ptr<TextureImage> normalHandle;
    std::shared_ptr<TextureImage> metallicRoughnessHandle;
    VkDescriptorSet descriptorSet;

public:
    Material(
        VkDevice device,
        MaterialDescriptorManager* descriptorManager,
        std::shared_ptr<TextureImage> baseColorHandle,
        std::shared_ptr<TextureImage> normalHandle,
        std::shared_ptr<TextureImage> metallicRoughnessHandle
    );

    Material(const Material& other) :
        baseColorHandle(other.baseColorHandle),
        normalHandle(other.normalHandle),
        metallicRoughnessHandle(other.metallicRoughnessHandle),
        descriptorSet(other.descriptorSet)
    {}
    Material& operator=(const Material& other)
    {
        if (this != &other)
        {
            baseColorHandle = other.baseColorHandle;
            normalHandle = other.normalHandle;
            metallicRoughnessHandle = other.metallicRoughnessHandle;
            descriptorSet = other.descriptorSet;
        }
        return *this;
    }

    Material(Material&& other) noexcept :
        baseColorHandle(std::move(other.baseColorHandle)),
        normalHandle(std::move(other.normalHandle)),
        metallicRoughnessHandle(std::move(other.metallicRoughnessHandle)),
        descriptorSet(other.descriptorSet)
    {
        other.descriptorSet = VK_NULL_HANDLE;
    }
    Material& operator=(Material&& other) noexcept
    {
        if (this != &other)
        {
            baseColorHandle = std::move(other.baseColorHandle);
            normalHandle = std::move(other.normalHandle);
            metallicRoughnessHandle = std::move(other.metallicRoughnessHandle);
            descriptorSet = other.descriptorSet;

            other.descriptorSet = VK_NULL_HANDLE;
        }
        return *this;
    }


    std::shared_ptr<TextureImage> getBaseColorHandle() const { return baseColorHandle; }
    std::shared_ptr<TextureImage> getNnormalHandle() const { return normalHandle; }
    std::shared_ptr<TextureImage> getMetallicRoughnessHandle() const { return metallicRoughnessHandle; }
    const VkDescriptorSet getDescriptorSet() const { return descriptorSet; }
};