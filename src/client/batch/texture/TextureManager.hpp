#pragma once

#include "../../CoreVulkan.hpp"
#include "../../BufferManager.hpp"
#include "TextureImage.hpp"
#include "TextureAsset.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class TextureManager
{
private:

    VkPhysicalDevice physicalDevice{};
    VkDevice device{};

    BufferManager* bufferManager{};
    SamplerManager* samplerManager{};

    std::unordered_map<
        std::string,
        uint32_t
    > textureIndices;

    std::vector<
        std::shared_ptr<TextureImage>
    > textures;

    uint32_t maxTextures{};

public:

    std::shared_ptr<TextureImage> defaultWhite;
    std::shared_ptr<TextureImage> defaultNormal;
    std::shared_ptr<TextureImage> defaultMetallic;

    TextureManager(
        VkPhysicalDevice physicalDevice,
        VkDevice device,
        BufferManager* bufferManager,
        SamplerManager* samplerManager,
        uint32_t maxTextures
    );

    ~TextureManager();

    uint32_t createTexture(
        const std::string& path
    );

    const std::shared_ptr<TextureImage>& getTexture(
        uint32_t textureIndex
    ) const;

    VkImageView getImageView(
        uint32_t textureIndex
    ) const;

    uint32_t getTextureCount() const { return static_cast<uint32_t>(textures.size()); }
};