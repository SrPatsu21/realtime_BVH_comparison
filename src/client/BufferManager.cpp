#include "BufferManager.hpp"

BufferManager::BufferManager(
    VkPhysicalDevice physicalDevice,
    VkDevice device,
    VkQueue graphicsQueue,
    uint32_t graphicsQueueFamily
) :
    physicalDevice(physicalDevice),
    device(device),
    graphicsQueue(graphicsQueue)
{
    initImmediateContext(graphicsQueueFamily);
}

void BufferManager::createBuffer(
    VkDeviceSize bufferSize,
    VkBufferUsageFlags usage,
    VkBuffer& buffer
) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create vertex buffer!");
    }
}

void BufferManager::allocateBufferMemory(
    VkBuffer buffer,
    VkMemoryPropertyFlags properties,
    VkDeviceMemory& bufferMemory,
    bool deviceAddress
)
{
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(
        device,
        buffer,
        &memRequirements
    );

    VkMemoryAllocateFlagsInfo flagsInfo{};
    flagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
    flagsInfo.flags =
        deviceAddress
            ? VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT
            : 0;

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.pNext =
        deviceAddress
            ? &flagsInfo
            : nullptr;

    allocInfo.allocationSize = memRequirements.size;

    allocInfo.memoryTypeIndex =
        CoreVulkan::findMemoryType(
            physicalDevice,
            memRequirements.memoryTypeBits,
            properties,
            0
        );

    if (vkAllocateMemory(
            device,
            &allocInfo,
            nullptr,
            &bufferMemory
        ) != VK_SUCCESS)
    {
        throw std::runtime_error(
            "failed to allocate buffer memory!"
        );
    }
}

void BufferManager::allocateBufferMemory(
    VkBuffer buffer,
    VkMemoryPropertyFlags properties,
    VkDeviceMemory& bufferMemory
) {
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = CoreVulkan::findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties, 0);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate vertex buffer memory!");
    }
};

void BufferManager::allocateBufferMemory(
    VkBuffer buffer,
    VkMemoryPropertyFlags required,
    VkMemoryPropertyFlags preferred,
    BufferManager::AllocatedMemoryINFO& info
) {
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    uint32_t memoryTypeIndex = CoreVulkan::findMemoryType(
        physicalDevice,
        memRequirements.memoryTypeBits,
        required,
        preferred
    );

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memoryTypeIndex;

    if (vkAllocateMemory(device, &allocInfo, nullptr, &info.memory) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate buffer memory!");
    }

    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    VkMemoryPropertyFlags flags = memProperties.memoryTypes[memoryTypeIndex].propertyFlags;

    info.memoryTypeIndex = memoryTypeIndex;
    info.isCoherent = (flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != 0;
}

void BufferManager::copyBuffer(
    VkBuffer srcBuffer,
    VkBuffer dstBuffer,
    VkDeviceSize size
) {
    VkCommandBuffer commandBuffer = beginImmediate();

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0; // Optional
    copyRegion.dstOffset = 0; // Optional
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    endImmediate();
}

void BufferManager::initImmediateContext(
    uint32_t graphicsQueueFamily
) {
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = graphicsQueueFamily;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    vkCreateCommandPool(device, &poolInfo, nullptr, &immediate.pool);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = immediate.pool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    vkAllocateCommandBuffers(device, &allocInfo, &immediate.cmd);

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    vkCreateFence(device, &fenceInfo, nullptr, &immediate.fence);
}

void BufferManager::destroyImmediateContext()
{
    if (immediate.pool == VK_NULL_HANDLE) return;

    vkDeviceWaitIdle(device);

    if (immediate.fence != VK_NULL_HANDLE) {
        vkDestroyFence(device, immediate.fence, nullptr);
        immediate.fence = VK_NULL_HANDLE;
    }

    vkDestroyCommandPool(device, immediate.pool, nullptr);
    immediate.pool = VK_NULL_HANDLE;
    immediate.cmd = VK_NULL_HANDLE;
}

VkCommandBuffer BufferManager::beginImmediate() {
    vkWaitForFences(device, 1, &immediate.fence, VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &immediate.fence);

    vkResetCommandPool(device, immediate.pool, 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(immediate.cmd, &beginInfo);
    return immediate.cmd;
}

void BufferManager::endImmediate() {
    vkEndCommandBuffer(immediate.cmd);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &immediate.cmd;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, immediate.fence);
    vkWaitForFences(device, 1, &immediate.fence, VK_TRUE, UINT64_MAX);
}

void BufferManager::copyBufferToImage(
    VkBuffer buffer,
    VkImage image,
    uint32_t width,
    uint32_t height
) {
    VkCommandBuffer commandBuffer = beginImmediate();

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = {0, 0, 0};
    region.imageExtent = {
        width,
        height,
        1
    };

    vkCmdCopyBufferToImage(
        commandBuffer,
        buffer,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region
    );

    endImmediate();
}

void BufferManager::uploadToImageMipLevel(
    const void* data,
    VkDeviceSize size,
    VkImage image,
    uint32_t width,
    uint32_t height,
    uint32_t mipLevel,
    uint32_t baseArrayLayer,
    uint32_t layerCount
)
{
    // Create staging buffer
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

    vkBindBufferMemory(device, stagingBuffer, stagingMemory, 0);

    // Copy data CPU → staging
    void* mapped;
    vkMapMemory(device, stagingMemory, 0, size, 0, &mapped);
    memcpy(mapped, data, (size_t)size);
    vkUnmapMemory(device, stagingMemory);

    // Copy staging → image
    VkCommandBuffer cmd = beginImmediate();

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = mipLevel;
    region.imageSubresource.baseArrayLayer = baseArrayLayer;
    region.imageSubresource.layerCount = layerCount;

    region.imageOffset = {0, 0, 0};
    region.imageExtent = { width, height, 1 };

    vkCmdCopyBufferToImage(
        cmd,
        stagingBuffer,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region
    );

    endImmediate();

    // Cleanup
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingMemory, nullptr);
}

BufferManager::~BufferManager() {
    destroyImmediateContext();
}
