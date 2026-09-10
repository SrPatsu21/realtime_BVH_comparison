#pragma once

#include <memory>
#include <vulkan/vulkan.h>
#include <glm/vec4.hpp>

class TextureImage;
class MaterialDescriptorManager;
class BufferManager;

class Material
{
public:

    enum class AlphaMode : uint32_t
    {
        OPAQUE = 1,
        MASK   = 2,
        BLEND  = 4
    };

    struct Properties {
        glm::vec4 baseColorFactor{1.0f};

        float metallicFactor = 1.0f;
        float roughnessFactor = 1.0f;

        AlphaMode alphaMode = AlphaMode::OPAQUE;
        float alphaCutoff = 0.5f;
    };

    Material(
        VkDevice device,
        BufferManager* bufferManager,
        MaterialDescriptorManager* descriptorManager,
        std::shared_ptr<TextureImage> baseColorHandle,
        std::shared_ptr<TextureImage> normalHandle,
        std::shared_ptr<TextureImage> metallicRoughnessHandle,
        const Properties& properties
    );

    ~Material();

    VkDescriptorSet getDescriptorSet() const
    {
        return descriptorSet;
    }

    const Properties& getProperties() const
    {
        return properties;
    }

    AlphaMode getAlphaMode() const
    {
        return properties.alphaMode;
    }

    float getAlphaCutoff() const
    {
        return properties.alphaCutoff;
    }

private:

    VkDevice device{};

    VkDescriptorSet descriptorSet{};

    std::shared_ptr<TextureImage> baseColorHandle;
    std::shared_ptr<TextureImage> normalHandle;
    std::shared_ptr<TextureImage> metallicRoughnessHandle;

    Properties properties;

    VkBuffer materialBuffer{};
    VkDeviceMemory materialMemory{};
};