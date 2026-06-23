#pragma once

#include "../CoreVulkan.hpp"
#include <vector>

class CameraBufferManager;

/**
 * @brief Manages the global descriptor set (set 0) used across all frames.
 *
 * GlobalDescriptorManager is responsible for:
 * - Creating the global descriptor set layout.
 * - Creating a descriptor pool sized per frame-in-flight.
 * - Allocating one descriptor set per frame.
 * - Binding the camera's global uniform buffer to each frame's descriptor set.
 *
 * The global descriptor set typically contains per-frame data shared by
 * all rendered objects, such as view and projection matrices.
 *
 * This manager assumes:
 * - One uniform buffer per frame.
 * - A fixed layout containing a single uniform buffer binding at binding 0.
 *
 * Descriptor sets are created during construction and remain valid
 * for the lifetime of this object.
 */
class GlobalDescriptorManager
{
private:
    VkDevice device; ///< Vulkan logical device (non-owning)

    VkDescriptorSetLayout descriptorSetLayout{VK_NULL_HANDLE}; ///< Global descriptor set layout
    VkDescriptorPool descriptorPool{VK_NULL_HANDLE}; ///< Descriptor pool for global sets
    std::vector<VkDescriptorSet> descriptorSets; ///< One descriptor set per frame-in-flight

public:

    /**
     * @brief Constructs the global descriptor manager.
     *
     * This constructor performs the following steps:
     *
     * 1. Creates a descriptor set layout containing:
     *      - Binding 0: VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
     *        Accessible in the vertex shader stage.
     *
     * 2. Creates a descriptor pool sized to allocate one descriptor set
     *    per frame-in-flight.
     *
     * 3. Allocates descriptor sets.
     *
     * 4. Updates each descriptor set with the corresponding uniform buffer
     *    obtained from CameraBufferManager.
     *
     * Each frame-in-flight receives its own descriptor set, allowing
     * safe CPU/GPU parallelism without descriptor contention.
     *
     * @param device Vulkan logical device used for descriptor operations.
     * @param cameraBufferManager Provides per-frame uniform buffers.
     * @param maxFramesInFlight Number of concurrent frames supported.
     *
     * @throws std::runtime_error if layout creation, pool creation,
     *         or descriptor allocation fails.
     */
    GlobalDescriptorManager(
        VkDevice device,
        CameraBufferManager* cameraBufferManager,
        uint32_t maxFramesInFlight
    );

    /**
     * @brief Destroys the descriptor pool and descriptor set layout.
     *
     * Descriptor sets are implicitly freed when the descriptor pool
     * is destroyed.
     *
     * The Vulkan device must remain valid during destruction.
     */
    ~GlobalDescriptorManager();

    VkDescriptorSetLayout getLayout() const { return descriptorSetLayout; }
    VkDescriptorPool getDescriptorPool() const { return descriptorPool; }
    std::vector<VkDescriptorSet> getDescriptorSets() const { return descriptorSets; }
};
