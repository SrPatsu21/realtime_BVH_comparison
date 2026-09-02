#include "GlobalDescriptorManager.hpp"

#include "../camera/CameraBufferManager.hpp"
#include "../camera/UniformBufferGlobal.hpp"
#include "../raytracing/acceleration_structure/gpu/AccelerationStructureGPU.hpp"

#include <array>
#include <stdexcept>

GlobalDescriptorManager::GlobalDescriptorManager(
    VkDevice device,
    CameraBufferManager* cameraBufferManager,
    AccelerationStructureGPU* blasBuffer,
    AccelerationStructureGPU* blasInstanceBuffer,
    AccelerationStructureGPU* tlasGPU,
    AccelerationStructureGPU* tlasInstanceGPU,
    uint32_t maxFramesInFlight
)
    : device(device)
{
    // ============================================================
    // Layout (set 0)
    // ============================================================

    std::array<VkDescriptorSetLayoutBinding, 5> bindings{};

    // ------------------------------------------------------------
    // binding 0 - Global camera UBO
    // ------------------------------------------------------------

    bindings[0].binding = 0;
    bindings[0].descriptorCount = 1;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[0].stageFlags =
        VK_SHADER_STAGE_VERTEX_BIT |
        VK_SHADER_STAGE_FRAGMENT_BIT;

    // ------------------------------------------------------------
    // binding 1 - BLAS nodes
    // ------------------------------------------------------------

    bindings[1].binding = 1;
    bindings[1].descriptorCount = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    // ------------------------------------------------------------
    // binding 2 - BLAS instances
    // ------------------------------------------------------------

    bindings[2].binding = 2;
    bindings[2].descriptorCount = 1;
    bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    // ------------------------------------------------------------
    // binding 3 - TLAS nodes
    // ------------------------------------------------------------

    bindings[3].binding = 3;
    bindings[3].descriptorCount = 1;
    bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[3].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    // ------------------------------------------------------------
    // binding 4 - TLAS instances
    // ------------------------------------------------------------

    bindings[4].binding = 4;
    bindings[4].descriptorCount = 1;
    bindings[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[4].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(
            device,
            &layoutInfo,
            nullptr,
            &descriptorSetLayout
        ) != VK_SUCCESS)
    {
        throw std::runtime_error(
            "Failed to create global descriptor set layout"
        );
    }

    // ============================================================
    // Pool
    // ============================================================

    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    // UBO
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = maxFramesInFlight;

    // SSBO
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[1].descriptorCount = maxFramesInFlight * 4;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = maxFramesInFlight;

    if (vkCreateDescriptorPool(
            device,
            &poolInfo,
            nullptr,
            &descriptorPool
        ) != VK_SUCCESS)
    {
        throw std::runtime_error(
            "Failed to create global descriptor pool"
        );
    }

    // ============================================================
    // Allocate
    // ============================================================

    std::vector<VkDescriptorSetLayout> layouts(
        maxFramesInFlight,
        descriptorSetLayout
    );

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = maxFramesInFlight;
    allocInfo.pSetLayouts = layouts.data();

    descriptorSets.resize(
        maxFramesInFlight
    );

    if (vkAllocateDescriptorSets(
            device,
            &allocInfo,
            descriptorSets.data()
        ) != VK_SUCCESS)
    {
        throw std::runtime_error(
            "Failed to allocate global descriptor sets"
        );
    }

    // ============================================================
    // Camera buffers
    // ============================================================

    auto buffers = cameraBufferManager->getUniformBuffers();

    // ============================================================
    // Acceleration structure buffers
    // ============================================================

    if (!blasBuffer ||
        !blasInstanceBuffer ||
        !tlasGPU ||
        !tlasInstanceGPU)
    {
        throw std::runtime_error("GlobalDescriptorManager: invalid acceleration structure buffer");
    }

    VkDescriptorBufferInfo blasNodeInfo{};
    blasNodeInfo.buffer = blasBuffer->buffer;
    blasNodeInfo.offset = 0;
    blasNodeInfo.range = VK_WHOLE_SIZE;

    VkDescriptorBufferInfo blasInstanceInfo{};
    blasInstanceInfo.buffer = blasInstanceBuffer->buffer;
    blasInstanceInfo.offset = 0;
    blasInstanceInfo.range = VK_WHOLE_SIZE;

    VkDescriptorBufferInfo tlasNodeInfo{};
    tlasNodeInfo.buffer = tlasGPU->buffer;
    tlasNodeInfo.offset = 0;
    tlasNodeInfo.range = VK_WHOLE_SIZE;

    VkDescriptorBufferInfo tlasInstanceInfo{};
    tlasInstanceInfo.buffer = tlasInstanceGPU->buffer;
    tlasInstanceInfo.offset = 0;
    tlasInstanceInfo.range = VK_WHOLE_SIZE;

    // ============================================================
    // Populate
    // ============================================================

    for (uint32_t i = 0; i < maxFramesInFlight; i++)
    {
        VkDescriptorBufferInfo cameraInfo{};

        cameraInfo.buffer = buffers[i];
        cameraInfo.offset = 0;
        cameraInfo.range = sizeof(UniformBufferGlobal);

        std::array<VkWriteDescriptorSet, 5> writes{};

        // --------------------------------------------------------
        // binding 0
        // --------------------------------------------------------

        writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[0].dstSet = descriptorSets[i];
        writes[0].dstBinding = 0;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writes[0].descriptorCount = 1;
        writes[0].pBufferInfo = &cameraInfo;

        // --------------------------------------------------------
        // binding 1
        // --------------------------------------------------------

        writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[1].dstSet = descriptorSets[i];
        writes[1].dstBinding = 1;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[1].descriptorCount = 1;
        writes[1].pBufferInfo = &blasNodeInfo;

        // --------------------------------------------------------
        // binding 2
        // --------------------------------------------------------

        writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[2].dstSet = descriptorSets[i];
        writes[2].dstBinding = 2;
        writes[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[2].descriptorCount = 1;
        writes[2].pBufferInfo = &blasInstanceInfo;

        // --------------------------------------------------------
        // binding 3
        // --------------------------------------------------------

        writes[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[3].dstSet = descriptorSets[i];
        writes[3].dstBinding = 3;
        writes[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[3].descriptorCount = 1;
        writes[3].pBufferInfo = &tlasNodeInfo;

        // --------------------------------------------------------
        // binding 4
        // --------------------------------------------------------

        writes[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[4].dstSet =descriptorSets[i];
        writes[4].dstBinding = 4;
        writes[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[4].descriptorCount = 1;
        writes[4].pBufferInfo = &tlasInstanceInfo;

        vkUpdateDescriptorSets(
            device,
            static_cast<uint32_t>(writes.size()),
            writes.data(),
            0,
            nullptr
        );
    }
}

GlobalDescriptorManager::~GlobalDescriptorManager()
{
    if (descriptorPool)
    {
        vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    }
    if (descriptorSetLayout)
    {
        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
    }
}