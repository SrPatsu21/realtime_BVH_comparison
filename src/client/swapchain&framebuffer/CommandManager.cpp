#include "CommandManager.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "../batch/instance/RenderInstance.hpp"

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
    GraphicsPipeline* graphicsPipeline,
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
    GraphicsPipeline* graphicsPipeline,
    const std::vector<VkFramebuffer>& framebuffers,
    VkExtent2D extent,
    GlobalDescriptorManager* globalDescriptorManager,
    InstanceDescriptorManager* instanceDescriptorManager,
    ParticleInstanceDescriptorManager* particleInstanceDescriptorManager,
    RenderBatchManager* renderBatchManager,
    const std::vector<ParticleData>& particles,
    const std::vector<IClearValueProvider*>& clearProviders,
    const std::vector<IViewportProvider*>& viewportProviders,
    const std::vector<IScissorProvider*>& scissorProviders,
    const std::vector<ICommandBufferRecorder*>& extraRecorders
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

    // browse batches
    VkPipelineLayout layout = VK_NULL_HANDLE;
    VkDescriptorSet globalSet = globalDescriptorManager->getDescriptorSets()[currentFrame];
    VkDescriptorSet instanceSet = instanceDescriptorManager->getDescriptorSets()[currentFrame];
    Mesh* lastMesh = nullptr;
    Material* lastMaterial = nullptr;
    GraphicsPipeline::PipelineFlags lastPipeline = 0;
    uint32_t currentOffset = 0;
    renderBatchManager->forEachBatch(
        [&](RenderBatch& batch)
        {
            const RenderBatchManager::BatchKey& key = batch.getKey();
            const std::shared_ptr<Mesh>&  mesh = key.mesh;
            const Mesh::SubMesh* submesh = key.submesh;
            const std::shared_ptr<Material> material = key.material;
            const GraphicsPipeline::PipelineFlags pipelineFlags = key.pipelineFlags;
            const std::vector<InstanceData>& instancesData = batch.getinstancesData();

            uint32_t instanceCount = static_cast<uint32_t>(instancesData.size());


            // Bind pipeline
            if (lastPipeline != pipelineFlags)
            {
                lastPipeline = pipelineFlags;
                layout = graphicsPipeline->getLayout(pipelineFlags & 0x3);
                vkCmdBindPipeline(
                    cmd,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    graphicsPipeline->getPipeline(pipelineFlags)
                );
            }

            // Bind mesh
            if (mesh.get() != lastMesh)
            {
                lastMesh = mesh.get();

                VkBuffer vertexBuffer = mesh->getVertexBuffer();
                VkDeviceSize offsets[] = { 0 };
                vkCmdBindVertexBuffers(
                    cmd,
                    0,
                    1,
                    &vertexBuffer,
                    offsets
                );

                vkCmdBindIndexBuffer(
                    cmd,
                    mesh->getIndexBuffer(),
                    0,
                    VK_INDEX_TYPE_UINT32
                );
            }

            // Bind descriptor sets (set 0 & 1)
            if (material.get() != lastMaterial)
            {
                lastMaterial = material.get();
                VkDescriptorSet descriptorSets[] = {
                    globalSet,
                    material->getDescriptorSet()
                };

                vkCmdBindDescriptorSets(
                    cmd,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    layout,
                    0,
                    2,
                    descriptorSets,
                    0,
                    nullptr
                );
            }

            // Update storage buffer of the current frame.
            instanceDescriptorManager->update(
                currentFrame,
                currentOffset,
                instancesData
            );

            // Bind descriptor set 2 (instances)
            vkCmdBindDescriptorSets(
                cmd,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                layout,
                2, // set index
                1,
                &instanceSet,
                0,
                nullptr
            );

            // Draw instanciado
            vkCmdDrawIndexed(
                cmd,
                submesh->indexCount,
                instanceCount,
                submesh->firstIndex,
                submesh->vertexOffset,
                currentOffset
            );

            currentOffset += instanceCount;
        }
    );

//* === TEST PARTICLE ===
    currentOffset = 0;
    layout = graphicsPipeline->getLayout(GraphicsPipeline::PIPE_TOPO_POINTS);

    // Bind particle pipeline
    vkCmdBindPipeline(
        cmd,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        graphicsPipeline->getPipeline(
            GraphicsPipeline::PIPE_TOPO_POINTS |
            GraphicsPipeline::PIPE_CULL_NONE |
            GraphicsPipeline::PIPE_DEPTH_TEST |
            GraphicsPipeline::PIPE_BLEND
        )
    );

    // replicate viewport/scissor
    setViewportAndScissor(
        cmd,
        graphicsPipeline,
        viewportProviders,
        scissorProviders
    );

    particleInstanceDescriptorManager->update(
        currentFrame,
        currentOffset,
        particles
    );

    // set 0 = global UBO
    vkCmdBindDescriptorSets(
        cmd,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        layout,
        0,
        1,
        &globalSet,
        0,
        nullptr
    );

    VkDescriptorSet particleSet = particleInstanceDescriptorManager->getDescriptorSets()[currentFrame];

    vkCmdBindDescriptorSets(
        cmd,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        layout,
        1,
        1,
        &particleSet,
        0,
        nullptr
    );

    // Draw 1 vertex, 1 instance
    vkCmdDraw(cmd, 1, particles.size(), 0, 0);

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