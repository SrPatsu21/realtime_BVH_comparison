#include "CommandManager.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "../batch/RenderBatch.hpp"
#include "../batch/instance/RenderInstance.hpp"
#include "../particle/ParticleRecord.hpp"
#include "../raytracing/record/GeometryRecord.hpp"
#include "../raytracing/record/LightingRecord.hpp"
#include "../raytracing/record/CompositeRecord.hpp"

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
    color.color = {{0.0f, 0.0f, 0.0f, 0.0f}};

    clearValues[0] = color; // position
    clearValues[1] = color; // normal
    clearValues[2] = color; // albedo
    clearValues[3] = color; // material

    VkClearValue depth{};
    depth.depthStencil = {1.0f, 0};

    clearValues[4] = depth;
}

void CommandManager::buildTransparentGBufferClearValues(
    std::vector<VkClearValue>& clearValues
) {
    clearValues.resize(4);

    VkClearValue color{};
    color.color = {{0.0f, 0.0f, 0.0f, 0.0f}};

    clearValues[0] = color; // position
    clearValues[1] = color; // normal
    clearValues[2] = color; // albedo
    clearValues[3] = color; // material
}


void CommandManager::buildLightingClearValues(
    std::vector<VkClearValue>& clearValues
) {
    clearValues.resize(1);

    VkClearValue color{};
    color.color = {{0.4f, 1.0f, 1.0f, 1.0f}};

    clearValues[0] = color;
}

void CommandManager::buildCompositeClearValues(
    std::vector<VkClearValue>& clearValues
)
{
    VkClearValue color{};
    color.color = {
        0.0f,
        0.0f,
        0.0f,
        1.0f
    };

    clearValues.push_back(color);
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
    VkRenderPass GBufferRenderPass,
    VkRenderPass GBufferTransparentRenderPass,
    VkRenderPass deferredLightRenderPass,
    VkRenderPass compositeRenderPass,
    GraphicsPipelineManager* graphicsPipeline,
    const std::vector<VkFramebuffer>& framebuffers,
    const std::vector<VkFramebuffer>& transparentFramebuffers,
    const std::vector<VkFramebuffer>& deferredLightingFramebuffers,
    const std::vector<VkFramebuffer>& compositeFramebuffers,
    VkExtent2D extent,
    GlobalDescriptorManager* globalDescriptorManager,
    InstanceDescriptorManager* instanceDescriptorManager,
    ParticleInstanceDescriptorManager* particleInstanceDescriptorManager,
    RenderInstanceManager* renderInstanceManager,
    GBufferDescriptorManager* gBufferDescriptorManager,
    TransparentGBufferDescriptorManager* transparentGBufferDescriptorManager,
    MaterialManager* materialManager,
    LightInstanceManager* lightInstanceManager,
    DeferredLightingDescriptorManager* deferredLightingDescriptorManager,
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

    // ----------------------------------
    // GBuffer
    // ----------------------------------
    {
        buildGBufferClearValues(
            clearValues
        );
        uint32_t currentOffset = 0;

        beginRenderPass(
            cmd,
            GBufferRenderPass,
            framebuffers[imageIndex],
            extent,
            clearValues,
            VK_SUBPASS_CONTENTS_INLINE
        );

        VkDescriptorSet instanceSet = instanceDescriptorManager->getDescriptorSets()[currentFrame];

        setViewportAndScissor(
            cmd,
            graphicsPipeline,
            viewportProviders,
            scissorProviders
        );

        GeometryRecord::record(
            cmd,
            graphicsPipeline,
            globalSet,
            materialManager->getDescriptorSet(),
            instanceSet,
            renderInstanceManager,
            0,
            renderInstanceManager->getBatchRanges().blendStart,
            currentOffset
        );
        vkCmdEndRenderPass(cmd);

        clearValues.clear();

        // ----------------------------------
        // Transparent GBuffer
        // ----------------------------------
        buildTransparentGBufferClearValues(clearValues);

        beginRenderPass(
            cmd,
            GBufferTransparentRenderPass,
            transparentFramebuffers[imageIndex],
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

        GeometryRecord::record(
            cmd,
            graphicsPipeline,
            globalSet,
            materialManager->getDescriptorSet(),
            instanceSet,
            renderInstanceManager,
            renderInstanceManager->getBatchRanges().blendStart,
            renderInstanceManager->getBatchRanges().end,
            currentOffset
        );

        vkCmdEndRenderPass(cmd);
    }

    clearValues.clear();

    // ----------------------------------
    // Lighting
    // ----------------------------------
    {
        buildLightingClearValues(clearValues);

        beginRenderPass(
            cmd,
            deferredLightRenderPass,
            deferredLightingFramebuffers[imageIndex],
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
        VkDescriptorSet transparentGBufferSet = transparentGBufferDescriptorManager->getDescriptorSet();
        VkDescriptorSet lightSet = lightInstanceManager->getDescriptorSet(currentFrame);

        LightingRecord::record(
            cmd,
            graphicsPipeline,
            globalSet,
            gBufferSet,
            lightSet,
            transparentGBufferSet,
            materialManager->getDescriptorSet(),
            config
        );
        vkCmdEndRenderPass(cmd);
    }

    clearValues.clear();

    // ----------------------------------
    // Composite
    // ----------------------------------
    {
        buildCompositeClearValues(clearValues);

        beginRenderPass(
            cmd,
            compositeRenderPass,
            compositeFramebuffers[imageIndex],
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

        VkDescriptorSet deferredLightingSet = deferredLightingDescriptorManager->getDescriptorSet();

        CompositeRecord::record(
            cmd,
            graphicsPipeline,
            deferredLightingSet
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

// void CommandManager::recordGeometrySecondaryCommandBuffer(
//     VkCommandBuffer secondaryCommandBuffer,

//     VkRenderPass renderPass,

//     uint32_t currentFrame,

//     GraphicsPipelineManager* graphicsPipeline,

//     VkDescriptorSet globalSet,
//     VkDescriptorSet instanceSet,

//     RenderInstanceManager* renderInstanceManager,

//     uint32_t firstBatch,
//     uint32_t lastBatch,
//     uint32_t firstInstanceOffset,

//     const std::vector<IViewportProvider*>& viewportProviders,
//     const std::vector<IScissorProvider*>& scissorProviders
// )
// {
//     beginSecondaryCommandBuffer(
//         secondaryCommandBuffer,
//         renderPass,
//         VK_NULL_HANDLE,
//         0
//     );

//     setViewportAndScissor(
//         secondaryCommandBuffer,
//         graphicsPipeline,
//         viewportProviders,
//         scissorProviders
//     );

//     GeometryRecord::record(
//         secondaryCommandBuffer,
//         graphicsPipeline,
//         globalSet,
//         instanceSet,
//         renderInstanceManager,
//         firstBatch,
//         lastBatch,
//         firstInstanceOffset
//     );

//     if (vkEndCommandBuffer(secondaryCommandBuffer) != VK_SUCCESS)
//     {
//         throw std::runtime_error(
//             "failed to record geometry secondary command buffer!"
//         );
//     }
// }