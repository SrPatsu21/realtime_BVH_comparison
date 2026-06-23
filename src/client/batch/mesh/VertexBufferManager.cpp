#include "VertexBufferManager.hpp"

VertexBufferManager::VertexBufferManager(
        VkDevice device,
        BufferManager* bufferManager,
        const std::vector<Vertex>& vertices
) :
    device(device)
{
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    bufferManager->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, stagingBuffer);
    bufferManager->allocateBufferMemory(stagingBuffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBufferMemory);
    vkBindBufferMemory(device, stagingBuffer, stagingBufferMemory, 0);

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(device, stagingBufferMemory);

    bufferManager->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, this->vertexBuffer);
    bufferManager->allocateBufferMemory(this->vertexBuffer, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, this->vertexBufferMemory);
    vkBindBufferMemory(device, this->vertexBuffer, this->vertexBufferMemory, 0);

    bufferManager->copyBuffer(stagingBuffer, this->vertexBuffer, bufferSize);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
};

VertexBufferManager::~VertexBufferManager()
{
    vkDestroyBuffer(device, this->vertexBuffer, nullptr);
    vkFreeMemory(device, this->vertexBufferMemory, nullptr);
}