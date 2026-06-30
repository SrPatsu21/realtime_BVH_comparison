#pragma once

#include "CoreVulkan.hpp"

/**
 * @brief Utility class for Vulkan buffer creation and immediate GPU transfers.
 *
 * BufferManager encapsulates common low-level Vulkan operations related to
 * buffer creation, memory allocation, and short-lived transfer commands.
 *
 * This class is intended to be used internally by the engine as a helper
 * for resource uploads (e.g. vertex buffers, staging buffers, texture uploads).
 * It operates directly on Vulkan objects and assumes correct usage by the caller.
 */
class BufferManager
{
private:
    VkDevice device;
    VkPhysicalDevice physicalDevice;
    VkQueue graphicsQueue;

    /**
     * @brief Internal context used for immediate command submission.
     *
     * This context owns a dedicated command pool, a single command buffer,
     * and a fence used to synchronously submit short GPU operations.
     *
     * It is optimized for infrequent, blocking transfers such as resource uploads
     * during initialization or loading phases.
     */
    struct ImmediateSubmitContext {
        VkCommandPool pool;
        VkCommandBuffer cmd;
        VkFence fence;
    };

    ImmediateSubmitContext immediate;

    /**
     * @brief Initializes the immediate submission context.
     *
     * Creates a command pool, allocates a single primary command buffer,
     * and initializes a fence used to synchronize submissions.
     *
     * @param graphicsQueueFamily Queue family index used for graphics and transfer commands.
     */
    void initImmediateContext(
        uint32_t graphicsQueueFamily
    );

    /**
     * @brief Destroys all resources associated with the immediate submission context.
     *
     * Ensures the device is idle before releasing Vulkan objects.
     */
    void destroyImmediateContext();

public:    /**
     * @brief Describes memory allocated for a Vulkan buffer.
     *
     * This structure stores the result of a buffer memory allocation,
     * including the allocated VkDeviceMemory handle, the selected memory
     * type index, and whether the memory is host-coherent.
     *
     * The isCoherent flag allows higher-level systems to determine whether
     * explicit flushing (vkFlushMappedMemoryRanges) is required when writing
     * to mapped memory.
     *
     * Instances of this struct are filled by allocateBufferMemory().
     */
    struct AllocatedMemoryINFO {
        VkDeviceMemory memory; ///< Allocated device memory handle
        uint32_t memoryTypeIndex; ///< Index of the selected memory type
        bool isCoherent; ///< True if memory is host-coherent
    };
    /**
     * @brief Constructs a BufferManager and initializes its internal transfer context.
     *
     * @param physicalDevice Vulkan physical device used for memory queries.
     * @param device Logical Vulkan device.
     * @param graphicsQueue Queue used to submit transfer commands.
     * @param graphicsQueueFamily Queue family index associated with the graphics queue.
     */
    BufferManager(
        VkPhysicalDevice physicalDevice,
        VkDevice device,
        VkQueue graphicsQueue,
        uint32_t graphicsQueueFamily
    );

    /**
     * @brief Creates a Vulkan buffer object.
     *
     * This function only creates the buffer handle.
     * Memory allocation and binding must be performed separately.
     *
     * @param bufferSize Size of the buffer in bytes.
     * @param usage Vulkan buffer usage flags.
     * @param buffer Output buffer handle.
     *
     * @throws std::runtime_error if buffer creation fails.
     */
    void createBuffer(
        VkDeviceSize bufferSize,
        VkBufferUsageFlags usage,
        VkBuffer& buffer
    );

    void allocateBufferMemory(
        VkBuffer buffer,
        VkMemoryPropertyFlags properties,
        VkDeviceMemory& bufferMemory,
        bool deviceAddress
    );

    /**
     * @brief Allocates device memory for a buffer.
     *
     * Selects an appropriate memory type based on the buffer requirements
     * and requested memory properties.
     *
     * @param buffer Buffer requiring memory allocation.
     * @param properties Required Vulkan memory property flags.
     * @param bufferMemory Output memory handle.
     *
     * @throws std::runtime_error if no suitable memory type is found or allocation fails.
     */
    void allocateBufferMemory(
        VkBuffer buffer,
        VkMemoryPropertyFlags properties,
        VkDeviceMemory& bufferMemory
    );

    /**
     * @brief Allocates memory for a Vulkan buffer.
     *
     * This function:
     *
     * 1. Queries the buffer's memory requirements.
     * 2. Selects a suitable memory type using required and preferred flags.
     * 3. Allocates VkDeviceMemory.
     * 4. Determines whether the selected memory type is host-coherent.
     * 5. Fills the provided AllocatedMemoryINFO structure.
     *
     * Memory selection follows this logic:
     * - All required flags must be satisfied.
     * - Preferred flags are used to choose the best candidate when possible.
     *
     * The allocated memory is not automatically bound to the buffer.
     * Binding must be performed separately using vkBindBufferMemory.
     *
     * @param buffer The Vulkan buffer requiring memory.
     * @param required Memory property flags that must be present.
     * @param preferred Memory property flags that are desirable but optional.
     * @param info Output structure receiving allocation details.
     *
     * @throws std::runtime_error if memory allocation fails.
     */
    void allocateBufferMemory(
        VkBuffer buffer,
        VkMemoryPropertyFlags required,
        VkMemoryPropertyFlags preferred,
        BufferManager::AllocatedMemoryINFO& info
    );

    /**
     * @brief Begins recording an immediate-use command buffer.
     *
     * This command buffer is intended for short, synchronous GPU operations
     * such as buffer or image transfers.
     *
     * @return A command buffer ready for recording.
     */
    VkCommandBuffer beginImmediate();

    /**
     * @brief Ends recording and submits the immediate command buffer.
     *
     * The submission is synchronous and blocks until execution completes.
     */
    void endImmediate();

    /**
     * @brief Copies data from one buffer to another.
     *
     * The copy operation is executed immediately and blocks until completion.
     *
     * @param srcBuffer Source buffer.
     * @param dstBuffer Destination buffer.
     * @param size Number of bytes to copy.
     */
    void copyBuffer(
        VkBuffer srcBuffer,
        VkBuffer dstBuffer,
        VkDeviceSize size
    );

    /**
     * @brief Copies buffer data into an image.
     *
     * Assumes the destination image is already in
     * VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL layout.
     *
     * The operation is executed immediately and blocks until completion.
     *
     * @param buffer Source buffer containing image data.
     * @param image Destination image.
     * @param width Image width in pixels.
     * @param height Image height in pixels.
     */
    void copyBufferToImage(
        VkBuffer buffer,
        VkImage image,
        uint32_t width,
        uint32_t height
    );

    /**
     * @brief Uploads data to a specific mip level and array layer of a VkImage.
     *
     * This function:
     * - Creates a temporary host-visible staging buffer
     * - Copies CPU memory into the staging buffer
     * - Records a vkCmdCopyBufferToImage command
     * - Submits the command immediately
     * - Destroys the staging resources
     *
     * The destination image must already be in
     * VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL layout.
     *
     * @param data Pointer to source CPU data.
     * @param size Size of the data in bytes.
     * @param image Destination Vulkan image.
     * @param width Width of the mip level.
     * @param height Height of the mip level.
     * @param mipLevel Target mip level.
     * @param baseArrayLayer First array layer to write to.
     * @param layerCount Number of layers to update.
     */
    void uploadToImageMipLevel(
        const void* data,
        VkDeviceSize size,
        VkImage image,
        uint32_t width,
        uint32_t height,
        uint32_t mipLevel,
        uint32_t baseArrayLayer,
        uint32_t layerCount
    );

    template<typename T>
    VkBuffer
    createDeviceBuffer(
        const std::vector<T>& data,
        VkBufferUsageFlags usage,
        VkDeviceMemory& memory,
        VkDeviceAddress* address
    );

    /**
     * @brief Destroys the BufferManager and releases internal resources.
     */
    ~BufferManager();
};

template<typename T>
VkBuffer BufferManager::createDeviceBuffer(
    const std::vector<T>& data,
    VkBufferUsageFlags usage,
    VkDeviceMemory& memory,
    VkDeviceAddress* address
)
{
    const VkDeviceSize size = sizeof(T) * data.size();

    VkBuffer buffer;

    // staging

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;

    createBuffer(
        size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        stagingBuffer
    );

    allocateBufferMemory(
        stagingBuffer,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingMemory
    );

    vkBindBufferMemory(
        device,
        stagingBuffer,
        stagingMemory,
        0
    );

    void* mapped;

    vkMapMemory(
        device,
        stagingMemory,
        0,
        size,
        0,
        &mapped
    );

    memcpy(
        mapped,
        data.data(),
        size
    );

    vkUnmapMemory(
        device,
        stagingMemory
    );

    // device buffer

    createBuffer(
        size,
        usage |
        VK_BUFFER_USAGE_TRANSFER_DST_BIT |
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        buffer
    );

    allocateBufferMemory(
        buffer,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        memory,
        true
    );

    vkBindBufferMemory(
        device,
        buffer,
        memory,
        0
    );

    copyBuffer(
        stagingBuffer,
        buffer,
        size
    );

    vkDestroyBuffer(
        device,
        stagingBuffer,
        nullptr
    );

    vkFreeMemory(
        device,
        stagingMemory,
        nullptr
    );

    if (address)
    {
        VkBufferDeviceAddressInfo info{};
        info.sType =
            VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        info.buffer = buffer;

        *address =
            vkGetBufferDeviceAddress(
                device,
                &info
            );
    }

    return buffer;
}