#pragma once

#include "../CoreVulkan.hpp"
#include "../BufferManager.hpp"
#include "ParticleData.hpp"

class ParticleInstanceDescriptorManager {
private:
    VkDevice device;
    VkDeviceSize nonCoherentAtomSize;
    uint32_t maxParticles;

    std::vector<VkBuffer> buffers;


    std::vector<BufferManager::AllocatedMemoryINFO> memoryInfo;
    std::vector<void*> mapped;

    VkDescriptorPool descriptorPool{};
    VkDescriptorSetLayout descriptorSetLayout{};
    std::vector<VkDescriptorSet> descriptorSets;
public:
    ParticleInstanceDescriptorManager(
        VkDevice device,
        BufferManager* bufferManager,
        VkDeviceSize nonCoherentAtomSize,
        uint32_t maxFramesInFlight,
        uint32_t maxParticlesPerFrame
    );

    ~ParticleInstanceDescriptorManager();

    void update(
        uint32_t frameIndex,
        uint32_t baseParticle,
        const std::vector<ParticleData>& particles
    );

    const std::vector<VkDescriptorSet>& getDescriptorSets() const {
        return descriptorSets;
    }

    VkDescriptorSetLayout getLayout() const {
        return descriptorSetLayout;
    }
};