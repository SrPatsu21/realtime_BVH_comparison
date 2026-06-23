#include "IndexBufferManager.hpp"

IndexBufferManager::IndexBufferManager(
    VkDevice device,
    BufferManager* bufferManager,
    const std::vector<uint32_t>& indices
) :
    device(device)
{
    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    bufferManager->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, stagingBuffer);
    bufferManager->allocateBufferMemory(stagingBuffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBufferMemory);
    vkBindBufferMemory(device, stagingBuffer, stagingBufferMemory, 0);

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, indices.data(), (size_t) bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);

    bufferManager->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, this->indexBuffer);
    bufferManager->allocateBufferMemory(this->indexBuffer, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, this->indexBufferMemory);
    vkBindBufferMemory(device, this->indexBuffer, this->indexBufferMemory, 0);

    bufferManager->copyBuffer(stagingBuffer, this->indexBuffer, bufferSize);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

IndexBufferManager::~IndexBufferManager(){
    vkDestroyBuffer(device, this->indexBuffer, nullptr);
    vkFreeMemory(device, this->indexBufferMemory, nullptr);
}