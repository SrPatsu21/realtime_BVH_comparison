#pragma once

#include "../TLAS.hpp"
#include "VulkanTLAS.hpp"
#include "../../../BufferManager.hpp"

template<typename NodeType>
void buildVulkanTLAS(
    BufferManager* bufferManager,
    TLAS<NodeType>& tlas
)
{
    tlas.gpu.nodeBuffer =
        bufferManager->createDeviceBuffer(
            tlas.accelerationStructure.nodes,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            tlas.gpu.nodeMemory,
            &tlas.gpu.nodeAddress
        );
}