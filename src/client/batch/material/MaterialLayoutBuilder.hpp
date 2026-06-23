#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <unordered_map>
#include <stdexcept>
#include <unordered_set>

/**
 * @brief Utility class used to build Vulkan descriptor set layout bindings for materials.
 *
 * MaterialLayoutBuilder collects descriptor bindings and tracks:
 * - Unique binding indices
 * - Reserved binding ranges for engine use
 * - Descriptor counts grouped by VkDescriptorType
 *
 * It acts as an intermediate aggregation layer before creating:
 * - VkDescriptorSetLayout
 * - VkDescriptorPool
 *
 * The builder enforces:
 * - No duplicate binding indices
 * - Protection of engine-reserved binding slots
 * - Accurate descriptor count accumulation per descriptor type
 *
 * This class does not create Vulkan objects directly. It only gathers
 * structured layout data to be consumed by higher-level systems.
 */
class MaterialLayoutBuilder
{
public:

    /**
     * @brief Describes a single descriptor binding entry.
     *
     * Each BindingInfo corresponds directly to one
     * VkDescriptorSetLayoutBinding definition.
     */
    struct BindingInfo
    {
        uint32_t binding; ///< Binding index in the descriptor set
        VkDescriptorType type; ///< Descriptor type (e.g., uniform buffer, sampler)
        uint32_t count; ///< Number of descriptors for this binding
        VkShaderStageFlags stages; ///< Shader stages that can access this binding
    };

private:
    std::vector<BindingInfo> bindings; ///< Collected binding definitions
    std::unordered_set<uint32_t> usedBindings; ///< Tracks used binding indices
    std::unordered_map<VkDescriptorType, uint32_t> descriptorCountByType; ///< Accumulated descriptor counts per type

    uint32_t reservedStart; ///< Start of reserved binding range (inclusive)
    uint32_t reservedEnd;   ///< End of reserved binding range (inclusive)

public:

    /**
     * @brief Constructs a MaterialLayoutBuilder.
     *
     * Defines a reserved binding range that cannot be used by external
     * layout contributors.
     *
     * Any binding added via addBinding() that falls inside the reserved
     * range will result in an exception.
     *
     * The reserved range is inclusive:
     *     [reservedStart, reservedEnd]
     *
     * This mechanism ensures that engine-level bindings remain protected
     * and cannot be accidentally overridden by external systems.
     *
     * @param reservedStart First reserved binding index (inclusive).
     * @param reservedEnd Last reserved binding index (inclusive).
     */
    MaterialLayoutBuilder(
        uint32_t reservedStart = 0,
        uint32_t reservedEnd = 7
    );

    /**
     * @brief Adds a non-engine descriptor binding.
     *
     * This method validates:
     * - The binding is outside the reserved range.
     * - The binding index has not already been used.
     *
     * On success, the binding is stored and descriptor counts are updated.
     *
     * @param binding Binding index.
     * @param type Descriptor type.
     * @param count Number of descriptors for this binding.
     * @param stages Shader stages that can access this binding.
     *
     * @throws std::runtime_error if:
     * - The binding falls within the reserved range.
     * - The binding index was already defined.
     */
    void addBinding(
        uint32_t binding,
        VkDescriptorType type,
        uint32_t count,
        VkShaderStageFlags stages
    );

    /**
     * @brief Adds an engine-reserved descriptor binding.
     *
     * Unlike addBinding(), this method allows bindings inside the reserved
     * range and is intended for engine-internal use only.
     *
     * This method validates:
     * - The binding index has not already been used.
     *
     * It updates the internal binding list and descriptor count tracking.
     *
     * @param binding Binding index.
     * @param type Descriptor type.
     * @param count Number of descriptors for this binding.
     * @param stages Shader stages that can access this binding.
     *
     * @throws std::runtime_error if the binding index was already defined.
     */
    void addEngineBinding(
        uint32_t binding,
        VkDescriptorType type,
        uint32_t count,
        VkShaderStageFlags stages
    );

    const std::vector<BindingInfo>& getBindings() const { return bindings; }
    const std::unordered_map<VkDescriptorType, uint32_t>& getDescriptorCounts() const { return descriptorCountByType; }
};
