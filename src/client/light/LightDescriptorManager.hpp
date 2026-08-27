#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <cstdint>

#include "LightData.hpp"
#include "../BufferManager.hpp"

class LightDescriptorManager
{
public:

    LightDescriptorManager(
        VkDevice device,
        BufferManager* bufferManager,
        VkDeviceSize nonCoherentAtomSize,
        uint32_t maxFramesInFlight,
        uint32_t maxLights
    );

    ~LightDescriptorManager();

    void update(
        uint32_t frameIndex,
        const std::vector<LightData>& lights
    );

    const std::vector<VkDescriptorSet>& getDescriptorSets() const { return descriptorSets; }
    VkDescriptorSetLayout getDescriptorSetLayout() const { return descriptorSetLayout; }
    uint32_t getMaxLights() const { return maxLights; }

private:

    VkDevice device;

    VkDeviceSize nonCoherentAtomSize;

    uint32_t maxLights;

    std::vector<VkBuffer> buffers;
    std::vector<BufferManager::AllocatedMemoryINFO> memoryInfo;
    std::vector<void*> mapped;

    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> descriptorSets;
};