#pragma once

#include "../CoreVulkan.hpp"
#include "../graphics_pipeline/GraphicsPipeline.hpp"
#include "../graphics_pipeline/GlobalDescriptorManager.hpp"
#include "../particle/ParticleData.hpp"
#include "../particle/ParticleInstanceDescriptorManager.hpp"

// TODO improve

class ParticlePass
{
public:

    static void record(
        VkCommandBuffer cmd,
        uint32_t currentFrame,
        GraphicsPipeline* graphicsPipeline,
        VkDescriptorSet globalSet,
        ParticleInstanceDescriptorManager* particleInstanceDescriptorManager,
        const std::vector<ParticleData>& particles
    );
};