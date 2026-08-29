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
    const std::vector<VkFramebuffer>& framebuffers,
    uint32_t swapchainImageViewsSize,
    uint32_t workerThreadCount
)
    :
    device(device),
    workerThreadCount(workerThreadCount)
{
    if (this->workerThreadCount == 0)
        throw std::runtime_error("CommandManager requires at least one worker thread");

    // Primary command pool
    createCommandPool(graphicsQueueFamily);
    // Primary command buffers
    allocateCommandBuffers(framebuffers);

    // Secondary command pools
    createSecondaryCommandPools(graphicsQueueFamily);
    // Secondary command buffers
    createSecondaryCommandBuffers(swapchainImageViewsSize);
}

CommandManager::~CommandManager()
{
    for (VkCommandPool pool : secondaryCommandPools)
    {
        if (pool != VK_NULL_HANDLE)
        {
            vkDestroyCommandPool(
                device,
                pool,
                nullptr
            );
        }
    }

    secondaryCommandPools.clear();
    secondaryCommandBuffers.clear();

    if (commandPool != VK_NULL_HANDLE)
    {
        vkDestroyCommandPool(
            device,
            commandPool,
            nullptr
        );

        commandPool = VK_NULL_HANDLE;
    }
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

void CommandManager::createSecondaryCommandPools(
    uint32_t graphicsQueueFamily
)
{
    secondaryCommandPools.resize(workerThreadCount);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = graphicsQueueFamily;

    for (uint32_t i = 0; i < workerThreadCount; ++i)
    {
        if (vkCreateCommandPool(device, &poolInfo, nullptr, &secondaryCommandPools[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create secondary command pool!");
        }
    }
}

void CommandManager::createSecondaryCommandBuffers(
    uint32_t imageCount
)
{
    if (imageCount == 0)
        return;

    secondaryCommandBuffers.resize(workerThreadCount);

    for (uint32_t worker = 0; worker < workerThreadCount; ++worker)
    {
        auto& buffers = secondaryCommandBuffers[worker];
        buffers.resize(imageCount);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = secondaryCommandPools[worker];
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
        allocInfo.commandBufferCount = imageCount;

        if (vkAllocateCommandBuffers(device, &allocInfo, buffers.data()) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to allocate secondary command buffers!");
        }
    }
}

//* Render

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

void CommandManager::beginSecondaryCommandBuffer(
    VkCommandBuffer cmd,
    VkRenderPass renderPass,
    VkFramebuffer framebuffer,
    uint32_t subpass
)
{
    VkCommandBufferInheritanceInfo inheritance{};

    inheritance.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
    inheritance.renderPass = renderPass;
    inheritance.subpass = subpass;
    inheritance.framebuffer = framebuffer;

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags =
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT |
        VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;

    beginInfo.pInheritanceInfo = &inheritance;

    if (vkBeginCommandBuffer(cmd, &beginInfo) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to begin secondary command buffer!");
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

void CommandManager::buildGBufferClearValues(
    std::vector<VkClearValue>& clearValues
) {
    clearValues.resize(5);

    VkClearValue color{};
    color.color = {{1.0f, 1.0f, 1.0f, 1.0f}};

    clearValues[0] = color; // position
    clearValues[1] = color; // normal
    clearValues[2] = color; // albedo
    clearValues[3] = color; // material

    VkClearValue depth{};
    depth.depthStencil = {1.0f, 0};

    clearValues[4] = depth;
}

void CommandManager::buildLightingClearValues(
    std::vector<VkClearValue>& clearValues
) {
    clearValues.resize(1);

    VkClearValue color{};
    color.color = {{0.4f, 1.0f, 1.0f, 1.0f}};

    clearValues[0] = color;
}

void CommandManager::beginRenderPass(
    VkCommandBuffer cmd,
    VkRenderPass renderPass,
    VkFramebuffer framebuffer,
    VkExtent2D extent,
    const std::vector<VkClearValue>& clearValues,
    VkSubpassContents contents
)
{
    VkRenderPassBeginInfo info{};
    info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    info.renderPass = renderPass;
    info.framebuffer = framebuffer;
    info.renderArea.extent = extent;
    info.renderArea.offset = {0, 0};
    info.clearValueCount = static_cast<uint32_t>(clearValues.size());
    info.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(cmd, &info, contents);
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
    const std::vector<ParticleData>& particlesData,
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

    VkDescriptorSet globalSet = globalDescriptorManager->getDescriptorSets()[currentFrame];
    std::vector<VkClearValue> clearValues;


    if (config.render.mode == Config::RenderMode::Forward)
    {
        buildClearValues(
            clearProviders,
            clearValues
        );
        beginRenderPass(
            cmd,
            renderPass,
            framebuffers[imageIndex],
            extent,
            clearValues,
            VK_SUBPASS_CONTENTS_INLINE
        );
        setViewportAndScissor(
            cmd,
            graphicsPipeline,
            viewportProviders,
            scissorProviders
        );

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
        // ----------------------------------
        // GBuffer
        // ----------------------------------
        {
            buildGBufferClearValues(
                clearValues
            );

            beginRenderPass(
                cmd,
                renderPass,
                framebuffers[imageIndex],
                extent,
                clearValues,
                VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS
            );

            VkDescriptorSet instanceSet = instanceDescriptorManager->getDescriptorSets()[currentFrame];

            const uint32_t batchCount = static_cast<uint32_t>(renderInstanceManager->getBatches().size());
            const uint32_t batchesPerWorker = (batchCount + workerThreadCount - 1) / workerThreadCount;
            std::vector<std::thread> workers;

            workers.reserve(workerThreadCount);

            for (uint32_t worker = 0; worker < workerThreadCount; ++worker)
            {
                const uint32_t firstBatch = std::min(worker * batchesPerWorker, batchCount);
                const uint32_t lastBatch = std::min(firstBatch + batchesPerWorker, batchCount);
                if (firstBatch >= lastBatch)
                    continue;

                uint32_t firstInstanceOffset = 0;

                for (uint32_t i = 0; i < firstBatch; ++i)
                {
                    firstInstanceOffset += static_cast<uint32_t>(renderInstanceManager->getBatch(i).getInstancesData().size());
                }

                workers.emplace_back(
                    [&, worker, firstBatch, lastBatch, firstInstanceOffset]()
                    {
                        VkCommandBuffer secondaryCommandBuffer = secondaryCommandBuffers[worker][imageIndex];
                        beginSecondaryCommandBuffer(
                            secondaryCommandBuffer,
                            renderPass,
                            VK_NULL_HANDLE,
                            0
                        );

                        setViewportAndScissor(
                            secondaryCommandBuffer,
                            graphicsPipeline,
                            viewportProviders,
                            scissorProviders
                        );

                        GeometryRecord::record(
                            secondaryCommandBuffer,
                            graphicsPipeline,
                            globalSet,
                            instanceSet,
                            renderInstanceManager,
                            firstBatch,
                            lastBatch,
                            firstInstanceOffset
                        );

                        if (vkEndCommandBuffer(secondaryCommandBuffer) != VK_SUCCESS)
                        {
                            throw std::runtime_error(
                                "failed to record geometry secondary command buffer!"
                            );
                        }
                    }
                );
            }

            for (auto& worker : workers)
                worker.join();

            std::vector<VkCommandBuffer> secondaryCommands;

            secondaryCommands.reserve(workerThreadCount);

            for (uint32_t worker = 0; worker < workerThreadCount; ++worker)
            {
                const uint32_t firstBatch =
                    std::min(
                        worker * batchesPerWorker,
                        batchCount
                    );

                const uint32_t lastBatch =
                    std::min(
                        firstBatch + batchesPerWorker,
                        batchCount
                    );

                if (firstBatch >= lastBatch)
                    continue;

                secondaryCommands.push_back(
                    secondaryCommandBuffers[worker][imageIndex]
                );
            }

            vkCmdExecuteCommands(
                cmd,
                static_cast<uint32_t>(secondaryCommands.size()),
                secondaryCommands.data()
            );

            vkCmdEndRenderPass(cmd);
        }

        clearValues.clear();

        // ----------------------------------
        // Lighting
        // ----------------------------------

        buildLightingClearValues(clearValues);

        beginRenderPass(
            cmd,
            lightRenderPass,
            lightingFramebuffers[imageIndex],
            extent,
            clearValues,
            VK_SUBPASS_CONTENTS_INLINE
        );

        setViewportAndScissor(
            cmd,
            graphicsPipeline,
            viewportProviders,
            scissorProviders
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
    //     particlesData
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

void CommandManager::recordGeometrySecondaryCommandBuffer(
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
)
{
    beginSecondaryCommandBuffer(
        secondaryCommandBuffer,
        renderPass,
        VK_NULL_HANDLE,
        0
    );

    setViewportAndScissor(
        secondaryCommandBuffer,
        graphicsPipeline,
        viewportProviders,
        scissorProviders
    );

    GeometryRecord::record(
        secondaryCommandBuffer,
        graphicsPipeline,
        globalSet,
        instanceSet,
        renderInstanceManager,
        firstBatch,
        lastBatch,
        firstInstanceOffset
    );

    if (vkEndCommandBuffer(secondaryCommandBuffer) != VK_SUCCESS)
    {
        throw std::runtime_error(
            "failed to record geometry secondary command buffer!"
        );
    }
}