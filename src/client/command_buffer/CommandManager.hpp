#pragma once

#include <bits/stdc++.h>
#include "../CoreVulkan.hpp"
#include "../graphics_pipeline/GraphicsPipelineManager.hpp"
#include "../batch/RenderInstanceManager.hpp"
#include "../batch/instance/InstanceDescriptorManager.hpp"
#include "../batch/material/MaterialDescriptorManager.hpp"
#include "../graphics_pipeline/GlobalDescriptorManager.hpp"
#include "../particle/ParticleInstanceDescriptorManager.hpp"
#include "../raytracing/buffers/GBufferDescriptorManager.hpp"
#include "../light/LightInstanceManager.hpp"
#include <thread>

class CommandManager {
public:
    struct ICommandBufferRecorder {
        virtual ~ICommandBufferRecorder() = default;
        virtual void record(VkCommandBuffer cmd) = 0;
    };

    struct IClearValueProvider {
        virtual ~IClearValueProvider() = default;
        virtual void contribute(std::vector<VkClearValue>& clearValues) = 0;
    };

    struct IViewportProvider {
        virtual ~IViewportProvider() = default;
        virtual bool overrideViewport(VkViewport& viewport) = 0;
    };

    struct IScissorProvider {
        virtual ~IScissorProvider() = default;
        virtual bool overrideScissor(VkRect2D& scissor) = 0;
    };

private:
    VkDevice device;

    uint32_t workerThreadCount;

    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;

    std::vector<VkCommandPool> secondaryCommandPools;
    std::vector<std::vector<VkCommandBuffer>> secondaryCommandBuffers;

    void createCommandPool(
        uint32_t graphicsQueueFamily
    );
    void beginCommandBuffer(
        VkCommandBuffer cmd
    );

    void buildClearValues(
        const std::vector<IClearValueProvider*>& providers,
        std::vector<VkClearValue>& clearValues
    );

    void buildGBufferClearValues(
        std::vector<VkClearValue>& clearValues
    );

    void buildLightingClearValues(
        std::vector<VkClearValue>& clearValues
    );

    void beginRenderPass(
        VkCommandBuffer cmd,
        VkRenderPass renderPass,
        VkFramebuffer framebuffer,
        VkExtent2D extent,
        const std::vector<VkClearValue>& clearValues,
        VkSubpassContents contents
    );

    void beginSecondaryCommandBuffer(
        VkCommandBuffer cmd,
        VkRenderPass renderPass,
        VkFramebuffer framebuffer,
        uint32_t subpass
    );

    void setViewportAndScissor(
        VkCommandBuffer cmd,
        GraphicsPipelineManager* graphicsPipeline,
        const std::vector<IViewportProvider*>& viewportProviders,
        const std::vector<IScissorProvider*>& scissorProviders
    );

    void createSecondaryCommandPools(
        uint32_t graphicsQueueFamily
    );

    void createSecondaryCommandBuffers(
        uint32_t imageCount
    );

    void beginSecondaryCommandBuffer(
        VkCommandBuffer cmd,
        VkRenderPass renderPass,
        uint32_t subpass
    );

    void recordGeometrySecondaryCommandBuffer(
        VkCommandBuffer secondaryCommandBuffer,

        VkRenderPass renderPass,

        uint32_t currentFrame,

        GraphicsPipelineManager* graphicsPipeline,

        VkDescriptorSet globalSet,
        VkDescriptorSet instanceSet,

        RenderInstanceManager* renderInstanceManager,

        uint32_t firstBatch,
        uint32_t lastBatch,
        uint32_t firstInstanceOffset,

        const std::vector<IViewportProvider*>& viewportProviders,
        const std::vector<IScissorProvider*>& scissorProviders
    );

public:
    void allocateCommandBuffers(
        const std::vector<VkFramebuffer>& framebuffers
    );

    CommandManager(
        VkDevice device,
        uint32_t graphicsQueueFamily,
        const std::vector<VkFramebuffer>& framebuffers,
        uint32_t maxFramesInFlight,
        uint32_t workerThreadCount
    );

    ~CommandManager();

    void recordCommandBuffer(
        uint32_t imageIndex,
        uint32_t currentFrame,
        VkRenderPass renderPass,
        VkRenderPass lightRenderPass,
        GraphicsPipelineManager* graphicsPipeline,
        const std::vector<VkFramebuffer>& framebuffers,
        const std::vector<VkFramebuffer>&  lightingFramebuffers,
        VkExtent2D extent,
        GlobalDescriptorManager* globalDescriptorManager,
        InstanceDescriptorManager* instanceDescriptorManager,
        ParticleInstanceDescriptorManager* particleInstanceDescriptorManager,
        RenderInstanceManager* renderInstanceManager,
        GBufferDescriptorManager* gBufferDescriptorManager,
        LightInstanceManager* lightInstanceManager,
        const std::vector<ParticleData>& particlesData,
        const std::vector<IClearValueProvider*>& clearProviders,
        const std::vector<IViewportProvider*>& viewportProviders,
        const std::vector<IScissorProvider*>& scissorProviders,
        const std::vector<ICommandBufferRecorder*>& extraRecorders,
        const Config::ConfigTable& config
    );

    VkCommandPool getCommandPool() const { return commandPool; }
    const std::vector<VkCommandBuffer>& getCommandBuffers() const { return commandBuffers; }
};
