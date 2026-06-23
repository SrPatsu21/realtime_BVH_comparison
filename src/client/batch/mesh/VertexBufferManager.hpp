#pragma once

#include "../../CoreVulkan.hpp"
#include "Vertex.hpp"
#include "../../BufferManager.hpp"

#include <cstring>

class VertexBufferManager
{
private:
    VkDevice device;

    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
public:
    VertexBufferManager(
        VkDevice device,
        BufferManager* bufferManager,
        const std::vector<Vertex>& vertices
    );

    VertexBufferManager(const VertexBufferManager&) = delete;
    VertexBufferManager& operator=(const VertexBufferManager&) = delete;

    VertexBufferManager(VertexBufferManager&&) noexcept = delete;
    VertexBufferManager& operator=(VertexBufferManager&&) noexcept = delete;

    ~VertexBufferManager();

    VkBuffer getVertexBuffer() const {return vertexBuffer;}
    VkDeviceMemory getVertexBufferMemory() const {return vertexBufferMemory;}
};