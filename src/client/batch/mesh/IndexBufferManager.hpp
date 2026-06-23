#pragma once

#include "../../CoreVulkan.hpp"
#include "../../BufferManager.hpp"

#include <cstring>

class IndexBufferManager
{
private:
    VkDevice device;

    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;
public:
    IndexBufferManager(
        VkDevice device,
        BufferManager* bufferManager,
        const std::vector<uint32_t>& indices
    );

    IndexBufferManager(const IndexBufferManager&) = delete;
    IndexBufferManager& operator=(const IndexBufferManager&) = delete;

    IndexBufferManager(IndexBufferManager&&) noexcept = delete;
    IndexBufferManager& operator=(IndexBufferManager&&) noexcept = delete;

    ~IndexBufferManager();

    VkBuffer getIndexBuffer() const {return indexBuffer;}
    VkDeviceMemory getIndexBufferMemory() const {return indexBufferMemory;}
};