#pragma once

#include "../CoreVulkan.hpp"
#include "ShaderLoader.hpp"
#include "../batch/instance/InstanceData.hpp"
#include "../particle/ParticleData.hpp"
#include "../batch/mesh/Vertex.hpp"
#include <array>
#include <unordered_map>

class GraphicsPipeline {
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
        //next 7-15
    };

private:
    VkDevice device;

    std::unordered_map<PipelineFlags, VkPipeline> graphicsPipelines;
    std::unordered_map<PipelineFlags, VkPipelineLayout> pipelineLayouts;
    VkViewport viewport{};
    VkRect2D scissor{};
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};

    VkPipelineLayout createPipelineLayout(
        uint32_t pushConstantRangeSize,
        std::vector<VkDescriptorSetLayout> descriptorSetLayouts
    );

    VkPipelineVertexInputStateCreateInfo createVertexInputState(
        VkVertexInputBindingDescription& bindingDescription,
        std::array<VkVertexInputAttributeDescription, 4>& attributeDescriptions
    );
    VkPipelineInputAssemblyStateCreateInfo createInputAssemblyState(
        VkPrimitiveTopology topology
    );
    VkPipelineViewportStateCreateInfo createViewportState(
        VkViewport& viewport,
        VkRect2D& scissor
    );
    VkPipelineRasterizationStateCreateInfo createRasterizerState(
        VkCullModeFlags cullMode,
        VkPolygonMode polygonMode
    );
    VkPipelineMultisampleStateCreateInfo createMultisampleState(
        VkSampleCountFlagBits msaaSamples
    );
    VkPipelineDynamicStateCreateInfo createDynamicState(
        const std::vector<VkDynamicState>& dynamicState
    );
    VkPipelineDepthStencilStateCreateInfo createDepthStencilState();
    VkPipelineColorBlendStateCreateInfo createColorBlendState(
        VkPipelineColorBlendAttachmentState& colorBlendAttachment
    );

    VkPipeline createPipeline(
        const VkRenderPass renderPass,
        const VkPipelineLayout& pipelineLayout,
        const VkPipelineShaderStageCreateInfo* shaderStages,
        const VkPipelineVertexInputStateCreateInfo& vertexInput,
        const VkPipelineInputAssemblyStateCreateInfo& inputAssembly,
        const VkPipelineViewportStateCreateInfo& viewportState,
        const VkPipelineRasterizationStateCreateInfo& rasterizer,
        const VkPipelineMultisampleStateCreateInfo& multisampling,
        const VkPipelineDepthStencilStateCreateInfo& depthStencil,
        const VkPipelineColorBlendStateCreateInfo& colorBlend,
        const VkPipelineDynamicStateCreateInfo& dynamicState
    );

public:

    GraphicsPipeline(
        VkDevice device,
        VkExtent2D swapchainExtent,
        VkRenderPass renderPass,
        VkDescriptorSetLayout globalLayout,
        VkDescriptorSetLayout materialLayout,
        VkDescriptorSetLayout instanceLayout,
        VkDescriptorSetLayout particleLayout,
        VkSampleCountFlagBits msaaSamples,
        VkPhysicalDeviceVulkan12Features SupportedFeatures12
    );

    ~GraphicsPipeline();

    VkPipeline getPipeline(PipelineFlags flags) const { return graphicsPipelines.at(flags); }
    VkPipelineLayout getLayout(PipelineFlags flags) const { return pipelineLayouts.at(flags); }
    const VkViewport& getViewport() const  { return viewport; }
    const VkRect2D& getScissor() const { return scissor; }
    const VkPipelineColorBlendAttachmentState& getColorBlendAttachment() const { return colorBlendAttachment; }
};
