#pragma once

#include <memory>
#include <vulkan/vulkan.h>

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

    struct MaterialAlphaData
    {
        AlphaMode alphaMode;
        float alphaCutoff;
    };

    Material(
        VkDevice device,
        BufferManager* bufferManager,
        MaterialDescriptorManager* descriptorManager,
        std::shared_ptr<TextureImage> baseColorHandle,
        std::shared_ptr<TextureImage> normalHandle,
        std::shared_ptr<TextureImage> metallicRoughnessHandle,
        AlphaMode alphaMode = AlphaMode::OPAQUE,
        float alphaCutoff = 0.5f
    );

    ~Material();

    VkDescriptorSet getDescriptorSet() const { return descriptorSet; }
    AlphaMode getAlphaMode() const { return materialAlphaData.alphaMode; }
    float getAlphaCutoff() const { return materialAlphaData.alphaCutoff; }

private:

    VkDevice device{};

    VkDescriptorSet descriptorSet{};

    std::shared_ptr<TextureImage> baseColorHandle;
    std::shared_ptr<TextureImage> normalHandle;
    std::shared_ptr<TextureImage> metallicRoughnessHandle;

    MaterialAlphaData materialAlphaData;

    VkBuffer materialAlphaBuffer{};
    VkDeviceMemory materialAlphaMemory{};
};