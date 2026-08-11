#pragma once

#include "../BLAS.hpp"
#include "VulkanBLAS.hpp"
#include "../../../BufferManager.hpp"

template<typename NodeType>
void buildVulkanBLAS(
    BufferManager* bufferManager,
    BLAS<NodeType>& blas
)
{
    blas.gpu.nodeBuffer =
        bufferManager->createDeviceBuffer(
            blas.accelerationStructure.nodes,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            blas.gpu.nodeMemory,
            &blas.gpu.nodeAddress
        );
}