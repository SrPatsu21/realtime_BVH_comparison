#pragma once

#include "../CoreVulkan.hpp"
#include "ShaderLoader.hpp"
#include "../batch/instance/InstanceData.hpp"
#include "../particle/ParticleData.hpp"
#include "../batch/mesh/Vertex.hpp"
#include <unordered_map>

class GraphicsPipelineManager {
public:
    using PipelineFlags = uint16_t;
    enum : PipelineFlags
    {
        // bits 0-1: topology (2 bits)
        PIPE_TOPO_TRIANGLES = 0 << 0,
        PIPE_TOPO_LINES = 1 << 0,
        PIPE_TOPO_POINTS = 2 << 0,

        // bits 2-3: cull mode (2 bits)
        PIPE_CULL_NONE = 0 << 2,
        PIPE_CULL_BACK = 1 << 2,
        PIPE_CULL_FRONT = 2 << 2,

        // individual
        PIPE_DEPTH_TEST = 1 << 4,
        PIPE_DEPTH_WRITE = 1 << 5,
        PIPE_BLEND = 1 << 6,

        // type
        PIPE_GEOMETRY = 1 << 7,
        PIPE_LIGHTING = 1 << 8
        //next 9-15
    };

private:
    VkDevice device;

    std::unordered_map<PipelineFlags, VkPipeline> graphicsPipelines;
    std::unordered_map<PipelineFlags, VkPipelineLayout> pipelineLayouts;
    VkViewport viewport{};
    VkRect2D scissor{};

public:

    GraphicsPipelineManager(
        VkDevice device,
        VkExtent2D swapchainExtent,
        VkRenderPass renderPass,
        VkDescriptorSetLayout globalLayout,
        VkDescriptorSetLayout materialLayout,
        VkDescriptorSetLayout instanceLayout,
        VkDescriptorSetLayout particleLayout,
        VkSampleCountFlagBits msaaSamples,
        VkPhysicalDeviceVulkan12Features supportedFeatures12
    );

    ~GraphicsPipelineManager();

    bool createPipeline(
        PipelineFlags flags,
        VkPipeline pipeline
    );

    bool createLayout(
        PipelineFlags flags,
        VkPipelineLayout layout
    );

    VkPipeline getPipeline(PipelineFlags flags) const { return graphicsPipelines.at(flags); }
    VkPipelineLayout getLayout(PipelineFlags flags) const { return pipelineLayouts.at(flags); }

    const VkViewport& getViewport() const  { return viewport; }
    const VkRect2D& getScissor() const { return scissor; }
};
