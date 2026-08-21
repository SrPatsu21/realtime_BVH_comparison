#include "CommandManager.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "../batch/RenderBatch.hpp"
#include "../batch/instance/RenderInstance.hpp"
#include "../forward_render/ForwardRecord.hpp"
#include "../particle/ParticleRecord.hpp"
#include "../raytracing/record/GeometryRecord.hpp"
#include "../raytracing/record/LightingRecord.hpp"

CommandManager::CommandManager(
    VkDevice device,
    uint32_t graphicsQueueFamily,
    const std::vector<VkFramebuffer>& framebuffers
):
    device(device)
{
    // Create command pool
    createCommandPool(graphicsQueueFamily);

    // Allocate command buffers
    allocateCommandBuffers(framebuffers);
}

void CommandManager::createCommandPool(uint32_t graphicsQueueFamily) {
    // Create command pool
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = graphicsQueueFamily;

    if (vkCreateCommandPool(device, &poolInfo, nullptr, &this->commandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool!");
    }
}

void CommandManager::allocateCommandBuffers(
    const std::vector<VkFramebuffer>& framebuffers
){
    this->commandBuffers.resize(framebuffers.size());

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = this->commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(this->commandBuffers.size());

    if (vkAllocateCommandBuffers(device, &allocInfo, this->commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }
}

void CommandManager::beginCommandBuffer(
    VkCommandBuffer cmd
) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;
    beginInfo.pInheritanceInfo = nullptr;

    if (vkBeginCommandBuffer(cmd, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }
}

void CommandManager::buildClearValues(
    const std::vector<IClearValueProvider*>& providers,
    std::vector<VkClearValue>& clearValues
) {
    for (auto* p : providers) {
        p->contribute(clearValues);
    }

    if (clearValues.empty()) {
        clearValues.reserve(2);
        VkClearValue color{};
        color.color = {{0.4f, 1.0f, 1.0f, 1.0f}};
        clearValues.emplace_back(color);

        VkClearValue depth{};
        depth.depthStencil = {1.0f, 0};
        clearValues.emplace_back(depth);
    }
}

void CommandManager::beginRenderPass(
    VkCommandBuffer cmd,
    VkRenderPass renderPass,
    VkFramebuffer framebuffer,
    VkExtent2D extent,
    const std::vector<VkClearValue>& clearValues
) {
    VkRenderPassBeginInfo info{};
    info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    info.renderPass = renderPass;
    info.framebuffer = framebuffer;
    info.renderArea.extent = extent;
    info.renderArea.offset = {0, 0};
    info.clearValueCount = static_cast<uint32_t>(clearValues.size());
    info.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(cmd, &info, VK_SUBPASS_CONTENTS_INLINE);
}

void CommandManager::setViewportAndScissor(
    VkCommandBuffer cmd,
    GraphicsPipelineManager* graphicsPipeline,
    const std::vector<IViewportProvider*>& viewportProviders,
    const std::vector<IScissorProvider*>& scissorProviders
) {
    // Viewport
    VkViewport viewport = graphicsPipeline->getViewport();
    for (auto* p : viewportProviders) {
        if (p->overrideViewport(viewport)) {
            break;
        }
    }
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    // Scissor
    VkRect2D scissor = graphicsPipeline->getScissor();
    for (auto* p : scissorProviders) {
        if (p->overrideScissor(scissor)) {
            break;
        }
    }
    vkCmdSetScissor(cmd, 0, 1, &scissor);
}

// TODO change to multi RenderPass
void CommandManager::recordCommandBuffer(
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
    const std::vector<ParticleData>& particles,
    const std::vector<IClearValueProvider*>& clearProviders,
    const std::vector<IViewportProvider*>& viewportProviders,
    const std::vector<IScissorProvider*>& scissorProviders,
    const std::vector<ICommandBufferRecorder*>& extraRecorders,
    const Config::ConfigTable& config
) {
#ifndef NDEBUG
    assert(imageIndex < commandBuffers.size());
    assert(imageIndex < framebuffers.size());
#endif

    VkCommandBuffer cmd = commandBuffers[imageIndex];
    beginCommandBuffer(cmd);

    std::vector<VkClearValue> clearValues;
    buildClearValues(
        clearProviders,
        clearValues
    );

    beginRenderPass(
        cmd,
        renderPass,
        framebuffers[imageIndex],
        extent,
        clearValues
    );

    setViewportAndScissor(
        cmd,
        graphicsPipeline,
        viewportProviders,
        scissorProviders
    );

    VkDescriptorSet globalSet = globalDescriptorManager->getDescriptorSets()[currentFrame];

    if (config.render.mode == Config::RenderMode::Forward)
    {
        ForwardRecord::record(
            cmd,
            currentFrame,
            graphicsPipeline,
            globalSet,
            instanceDescriptorManager,
            renderInstanceManager
        );
    }else if (config.render.mode == Config::RenderMode::GeometryGBuffer)
    {
        GeometryRecord::record(
            cmd,
            currentFrame,
            graphicsPipeline,
            globalSet,
            instanceDescriptorManager,
            renderInstanceManager
        );
        vkCmdEndRenderPass(cmd);

        beginRenderPass(
            cmd,
            lightRenderPass,
            lightingFramebuffers[imageIndex],
            extent,
            clearValues
        );
        VkDescriptorSet gBufferSet = gBufferDescriptorManager->getDescriptorSet();
        LightingRecord::record(
            cmd,
            graphicsPipeline,
            globalSet,
            gBufferSet,
            config
        );
    }

//* === TEST PARTICLE ===

    // ParticleRecord::record(
    //     cmd,
    //     currentFrame,
    //     graphicsPipeline,
    //     globalSet,
    //     particleInstanceDescriptorManager,
    //     particles
    // );

//* Extra recorders (ImGui, debug, etc)
    for (auto* r : extraRecorders) {
        r->record(cmd);
    }

    vkCmdEndRenderPass(cmd);

    if (vkEndCommandBuffer(cmd) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
}

CommandManager::~CommandManager() {
    vkDestroyCommandPool(device, this->commandPool, nullptr);
    commandPool = VK_NULL_HANDLE;
}