#pragma once

#include <vector>
#include <string>
#include <stdexcept>
#include <algorithm>

#include <vulkan/vulkan.h>
#include <ktx.h>
#include <ktxvulkan.h>

#include "SamplerManager.hpp"

/**
 * @class TextureAsset
 * @brief Loads and interprets KTX2 textures for Vulkan usage.
 *
 * TextureAsset is responsible for:
 * - Loading KTX2 textures from disk
 * - Transcoding Basis-compressed textures when required
 * - Extracting all mip levels, layers, and cubemap faces
 * - Providing CPU-side access to subresource data
 * - Recommending sampler configuration based on texture characteristics
 *
 * The class keeps ownership of the underlying ktxTexture2 object
 * and exposes structured mip/subresource data for GPU upload.
 *
 * Supports:
 * - 2D textures
 * - Texture arrays
 * - Cubemaps
 * - Mipmapped textures
 */
class TextureAsset
{
public:

    /**
     * @struct SubresourceData
     * @brief Represents a single image subresource.
     *
     * A subresource corresponds to one combination of:
     * - mip level
     * - array layer
     * - cubemap face
     *
     * Data is a pointer into the internal KTX texture memory.
     */
    struct SubresourceData {
        uint8_t* data = nullptr;
        size_t size = 0;
    };

    /**
     * @struct MipLevelData
     * @brief Contains all subresources for a single mip level.
     *
     * For each mip level:
     * - width and height are stored
     * - subresources are stored in layer-major order:
     *   index = layer * faces + face
     */
    struct MipLevelData {
        uint32_t width = 0;
        uint32_t height = 0;
        std::vector<SubresourceData> subresources; // layers * faces
    };

private:

    ktxTexture2* texture = nullptr;

    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t depth = 1;
    uint32_t mipLevels = 0;
    uint32_t layers = 1;
    uint32_t faces = 1;

    VkFormat vkFormat = VK_FORMAT_UNDEFINED;

    bool cubemap = false;
    bool arrayTexture = false;

    std::vector<MipLevelData> mipData;

    /**
     * @brief Loads the KTX2 file from disk.
     *
     * Populates metadata such as:
     * - dimensions
     * - mip count
     * - layers
     * - faces
     * - Vulkan format
     *
     * @param path Path to the KTX2 file.
     * @throws std::runtime_error on failure.
     */
    void loadFromFile(const std::string& path);

    /**
     * @brief Extracts all mip levels, layers, and faces into structured data.
     *
     * Populates mipData with CPU-accessible pointers and sizes.
     * Data pointers reference the internal KTX memory buffer.
     *
     * @throws std::runtime_error if subresource offsets cannot be retrieved.
     */
    void extractAllSubresources();

    /**
     * @brief Transcodes Basis-compressed textures if necessary.
     *
     * If the texture uses Basis (KTX_SS_BASIS_LZ) supercompression,
     * it is transcoded to a GPU-friendly format (currently BC7).
     *
     * @param physicalDevice Vulkan physical device used to query features.
     * @throws std::runtime_error if transcoding fails.
     */
    void transcodeIfNeeded(VkPhysicalDevice physicalDevice);

public:

    /**
     * @brief Constructs a TextureAsset from a KTX2 file.
     *
     * Steps performed:
     * - Load KTX2 file
     * - Transcode Basis textures if required
     * - Extract all mip/subresource data
     *
     * @param path Path to KTX2 file.
     * @param physicalDevice Vulkan physical device for format decisions.
     */
    TextureAsset(
        const std::string& path,
        VkPhysicalDevice physicalDevice
    );

    /**
     * @brief Destroys the underlying KTX texture object.
     */
    ~TextureAsset();

    // Getters
    uint32_t getWidth() const { return width; }
    uint32_t getHeight() const { return height; }
    uint32_t getDepth() const { return depth; }
    uint32_t getMipLevels() const { return mipLevels; }
    uint32_t getLayers() const { return layers; }
    uint32_t getFaces() const { return faces; }

    bool isCubemap() const { return cubemap; }
    bool isArray() const { return arrayTexture; }

    VkFormat getFormat() const { return vkFormat; }

    /**
     * @brief Returns all mip level data for GPU upload.
     *
     * Data layout:
     * mipData[level].subresources[layer * faces + face]
     */
    const std::vector<MipLevelData>& getMipData() const { return mipData; }

    /**
     * @brief Provides a recommended sampler configuration.
     *
     * Heuristics:
     * - Cubemaps clamp to edge
     * - Array textures clamp to edge
     * - Single-mip textures disable mip filtering
     *
     * This does not create a sampler — it only provides a description.
     *
     * @return Recommended SamplerDesc configuration.
     */
    SamplerManager::SamplerDesc getRecommendedSamplerDesc() const;
};