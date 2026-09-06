#include "GeometryRecord.hpp"

void GeometryRecord::record(
    VkCommandBuffer cmd,

    GraphicsPipelineManager* graphicsPipeline,

    VkDescriptorSet globalSet,
    VkDescriptorSet instanceSet,

    RenderInstanceManager* renderInstanceManager,

    uint32_t firstBatch,
    uint32_t lastBatch,
    uint32_t& currentOffset
)
{
    VkPipelineLayout layout = VK_NULL_HANDLE;

    Mesh* lastMesh = nullptr;
    Material* lastMaterial = nullptr;

    GraphicsPipelineManager::PipelineFlags lastPipeline = 0;

    for (uint32_t i = firstBatch; i < lastBatch; ++i)
    {
        const RenderBatch& batch = renderInstanceManager->getBatch(i);

        const BatchKey& key = batch.getKey();

        const std::shared_ptr<Mesh>& mesh = key.mesh;
        const Mesh::SubMesh* subMesh = key.subMesh;
        const std::shared_ptr<Material>& material = key.material;

        const auto pipelineFlags = key.pipelineFlags | GraphicsPipelineManager::PIPE_GEOMETRY;

        const uint32_t instanceCount = static_cast<uint32_t>(batch.getInstancesData().size());

        if (lastPipeline != pipelineFlags)
        {
            lastPipeline = pipelineFlags;

            layout = graphicsPipeline->getLayout( pipelineFlags & 0x3 );

            vkCmdBindPipeline(
                cmd,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                graphicsPipeline->getPipeline(
                    pipelineFlags
                )
            );
        }

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

        if (material.get() != lastMaterial)
        {
            lastMaterial = material.get();

            VkDescriptorSet descriptorSets[] =
            {
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

        vkCmdBindDescriptorSets(
            cmd,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            layout,
            2,
            1,
            &instanceSet,
            0,
            nullptr
        );

        vkCmdDrawIndexed(
            cmd,
            subMesh->indexCount,
            instanceCount,
            subMesh->firstIndex,
            subMesh->vertexOffset,
            currentOffset
        );

        currentOffset += instanceCount;
    }
}