#pragma once

#include "../../CoreVulkan.hpp"
#include <unordered_map>
#include <functional>

/**
 * @class SamplerManager
 * @brief Caches and manages VkSampler objects based on sampler descriptions.
 *
 * SamplerManager prevents redundant VkSampler creation by caching
 * samplers using a SamplerDesc key. If a sampler with identical
 * parameters is requested multiple times, the same VkSampler handle
 * is reused.
 *
 * This improves performance and reduces driver overhead.
 *
 * Samplers are destroyed automatically when the manager is destroyed.
 */
class SamplerManager
{
public:

    /**
     * @struct SamplerDesc
     * @brief Describes a Vulkan sampler configuration.
     *
     * This structure mirrors VkSamplerCreateInfo parameters and is used
     * as a key for sampler caching.
     *
     * Two SamplerDesc instances are considered equal if all fields match.
     */
    struct SamplerDesc {
        VkFilter magFilter = VK_FILTER_LINEAR;
        VkFilter minFilter = VK_FILTER_LINEAR;

        VkSamplerAddressMode addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        VkSamplerAddressMode addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        VkSamplerAddressMode addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

        VkBool32 anisotropyEnable = VK_TRUE;
        float maxAnisotropy = 4.0f;

        VkSamplerMipmapMode mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

        float minLod = 0.0f;
        float maxLod = VK_LOD_CLAMP_NONE;

        VkBorderColor borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        VkBool32 compareEnable = VK_FALSE;
        VkCompareOp compareOp = VK_COMPARE_OP_ALWAYS;

        bool operator==(const SamplerDesc& other) const {
            return
                magFilter == other.magFilter &&
                minFilter == other.minFilter &&
                addressModeU == other.addressModeU &&
                addressModeV == other.addressModeV &&
                addressModeW == other.addressModeW &&
                anisotropyEnable == other.anisotropyEnable &&
                maxAnisotropy == other.maxAnisotropy &&
                mipmapMode == other.mipmapMode &&
                minLod == other.minLod &&
                maxLod == other.maxLod &&
                borderColor == other.borderColor &&
                compareEnable == other.compareEnable &&
                compareOp == other.compareOp;
        }
    };

    /**
     * @brief Default linear filtering with repeat addressing.
     */
    static const SamplerDesc LinearRepeat;

    /**
     * @brief Linear filtering with clamp-to-edge addressing.
     */
    static const SamplerDesc LinearClamp;

    /**
     * @brief Depth comparison sampler for shadow mapping.
     *
     * Uses:
     * - Clamp-to-edge addressing
     * - Comparison enabled
     * - VK_COMPARE_OP_LESS
     */
    static const SamplerDesc Shadow;

private:

    struct SamplerDescHash {
        size_t operator()(const SamplerDesc& d) const {
            size_t h = 0;

            auto hashCombine = [&h](auto v) {
                std::hash<std::decay_t<decltype(v)>> hasher;
                h ^= hasher(v) + 0x9e3779b9 + (h << 6) + (h >> 2);
            };

            hashCombine(d.magFilter);
            hashCombine(d.minFilter);
            hashCombine(d.addressModeU);
            hashCombine(d.addressModeV);
            hashCombine(d.addressModeW);
            hashCombine(d.anisotropyEnable);
            hashCombine(d.maxAnisotropy);
            hashCombine(d.mipmapMode);
            hashCombine(d.minLod);
            hashCombine(d.maxLod);
            hashCombine(d.borderColor);
            hashCombine(d.compareEnable);
            hashCombine(d.compareOp);

            return h;
        }
    };

    VkDevice device;
    VkPhysicalDevice physicalDevice;

    std::unordered_map<SamplerDesc, VkSampler, SamplerDescHash> samplers;

    /**
     * @brief Creates a Vulkan sampler from a SamplerDesc.
     *
     * - Clamps anisotropy to device limits
     * - Populates VkSamplerCreateInfo
     *
     * @param desc Sampler description
     * @return Created VkSampler handle
     * @throws std::runtime_error on failure
     */
    VkSampler createSampler(const SamplerDesc& desc);

public:

    /**
     * @brief Constructs the sampler manager.
     *
     * @param physicalDevice Vulkan physical device (used for limits)
     * @param device         Vulkan logical device
     */
    SamplerManager(
        VkPhysicalDevice physicalDevice,
        VkDevice device
    );

    /**
     * @brief Destroys all cached samplers.
     */
    ~SamplerManager();

    /**
     * @brief Retrieves a sampler matching the description.
     *
     * If a sampler with identical parameters already exists,
     * it is returned from the cache. Otherwise, a new one is created
     * and stored.
     *
     * @param desc Sampler description
     * @return VkSampler handle
     */
    VkSampler getSampler(const SamplerDesc& desc);
};