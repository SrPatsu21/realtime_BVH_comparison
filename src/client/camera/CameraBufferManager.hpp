#pragma once
#include "../CoreVulkan.hpp"
#include "../swapchain&framebuffer/SwapchainManager.hpp"
#include "UniformBufferGlobal.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cstring>
#include "../BufferManager.hpp"

/**
 * @brief Manages per-frame camera uniform buffers.
 *
 * CameraBufferManager owns and updates a set of uniform buffers,
 * typically one per frame-in-flight, used to store camera-related
 * transformation matrices (model, view, projection).
 *
 * The buffers are:
 * - Host-visible
 * - Persistently mapped
 * - Updated once per frame
 *
 * This class deliberately separates:
 * - Buffer lifetime and memory management
 * - Camera logic and matrix generation
 */
class CameraBufferManager
{
private:
    VkDevice device;

    // One uniform buffer per frame-in-flight
    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;

    // Persistently mapped pointers for fast CPU updates
    std::vector<void*> uniformBuffersMapped;

public:

    /**
     * @brief Interface for camera data providers.
     *
     * Camera providers are responsible for filling a UniformBufferGlobal
     * with view/projection (and optionally model) matrices.
     *
     * This abstraction allows:
     * - Different camera behaviors (FPS, orbit, cinematic, debug)
     * - Tooling or mod-driven camera overrides
     * - Decoupling camera math from buffer management
     */
    struct ICameraProvider {
        virtual ~ICameraProvider() = default;

        /**
         * @param ubg Output UniformBufferGlobal to be filled.
         * @param time Current time (typically seconds since start).
         * @param extent Current swapchain extent (for aspect ratio).
         */
        virtual void fill(
            UniformBufferGlobal& ubg,
            float time,
            const VkExtent2D& extent
        ) = 0;
    };

    /**
     * @brief Default camera implementation.
     *
     * Produces:
     * - A rotating model matrix
     * - A fixed look-at view matrix
     * - A perspective projection matching the swapchain aspect ratio
     *
     * Intended as a reference implementation and sane default.
     */
    struct DefaultCameraProvider : ICameraProvider {
        void fill(
            UniformBufferGlobal& ubg,
            float time,
            const VkExtent2D& extent
        ) override;
    };

    /**
     * @brief Creates uniform buffers for all frames-in-flight.
     *
     * Allocates, binds and persistently maps one uniform buffer per frame.
     *
     * @param device Logical Vulkan device.
     * @param bufferManager BufferManager used for buffer creation/allocation.
     * @param max_frames_in_flight Number of concurrent frames.
     */
    CameraBufferManager(
        VkDevice device,
        BufferManager* bufferManager,
        int max_frames_in_flight
    );

    /**
     * @brief Releases all uniform buffers and associated memory.
     */
    ~CameraBufferManager();

    /**
     * @brief Updates the uniform buffer for the given frame.
     *
     * Copies the provided UBO into the persistently mapped buffer
     * associated with the current frame index.
     *
     * @param currentFrame Frame index (in-flight).
     * @param ubo Uniform buffer data to upload.
     */
    void update(
        uint32_t currentFrame,
        const UniformBufferGlobal& ubg
    );

    const std::vector<VkBuffer>& getUniformBuffers() const { return uniformBuffers; }
    std::vector<VkDeviceMemory> getUniformBufferMemorys() const { return uniformBuffersMemory; }
    std::vector<void*> getUniformBuffersMapped() const { return uniformBuffersMapped; }
};