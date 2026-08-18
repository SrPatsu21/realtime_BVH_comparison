#include "ForwardRecord.hpp"

void ForwardRecord::record(
    VkCommandBuffer cmd,
    uint32_t currentFrame,
    GraphicsPipelineManager* graphicsPipeline,
    VkDescriptorSet globalSet,
    InstanceDescriptorManager* instanceDescriptorManager,
    RenderInstanceManager* renderInstanceManager
){
    VkPipelineLayout layout = VK_NULL_HANDLE;
    VkDescriptorSet instanceSet = instanceDescriptorManager->getDescriptorSets()[currentFrame];

    Mesh* lastMesh = nullptr;
    Material* lastMaterial = nullptr;
    GraphicsPipelineManager::PipelineFlags lastPipeline = 0;
    uint32_t currentOffset = 0;

    renderInstanceManager->forEachBatch(
        [&](RenderBatch& batch)
        {
            const BatchKey& key = batch.getKey();
            const std::shared_ptr<Mesh>&  mesh = key.mesh;
            const Mesh::SubMesh* submesh = key.subMesh;
            const std::shared_ptr<Material> material = key.material;
            const GraphicsPipelineManager::PipelineFlags pipelineFlags = key.pipelineFlags;
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
};