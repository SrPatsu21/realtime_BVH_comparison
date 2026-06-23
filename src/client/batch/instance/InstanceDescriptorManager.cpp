#include "InstanceDescriptorManager.hpp"
#include <stdexcept>
#include <vector>
#include <cstring>

InstanceDescriptorManager::InstanceDescriptorManager(
    VkDevice device,
    BufferManager* bufferManager,
    VkDeviceSize nonCoherentAtomSize,
    uint32_t maxFramesInFlight,
    uint32_t maxInstancesPerFrame
) :
    device(device),
    nonCoherentAtomSize(nonCoherentAtomSize),
    maxInstances(maxInstancesPerFrame)
{
    VkDeviceSize bufferSize = sizeof(InstanceData) * maxInstances;

    buffers.resize(maxFramesInFlight);
    memoryInfo.resize(maxFramesInFlight);
    mapped.resize(maxFramesInFlight);

    for (uint32_t i = 0; i < maxFramesInFlight; i++)
    {
        bufferManager->createBuffer(
            bufferSize,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            buffers[i]
        );

        bufferManager->allocateBufferMemory(
            buffers[i],
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, // required
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, // preferred
            memoryInfo[i]
        );

        vkBindBufferMemory(device, buffers[i], memoryInfo[i].memory, 0);

        vkMapMemory(
            device,
            memoryInfo[i].memory,
            0,
            bufferSize,
            0,
            &mapped[i]
        );
    }

    VkDescriptorSetLayoutBinding binding{};
    binding.binding = 0;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    binding.descriptorCount = 1;
    binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    binding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &binding;

    if (vkCreateDescriptorSetLayout(
            device,
            &layoutInfo,
            nullptr,
            &descriptorSetLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create instance descriptor set layout");
    }

    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSize.descriptorCount = maxFramesInFlight;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = maxFramesInFlight;

    if (vkCreateDescriptorPool(
            device,
            &poolInfo,
            nullptr,
            &descriptorPool) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create instance descriptor pool");
    }

    std::vector<VkDescriptorSetLayout> layouts(
        maxFramesInFlight,
        descriptorSetLayout
    );

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = maxFramesInFlight;
    allocInfo.pSetLayouts = layouts.data();

    descriptorSets.resize(maxFramesInFlight);

    if (vkAllocateDescriptorSets(
            device,
            &allocInfo,
            descriptorSets.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate instance descriptor sets");
    }

    for (uint32_t i = 0; i < maxFramesInFlight; i++)
    {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = buffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = bufferSize;

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = descriptorSets[i];
        write.dstBinding = 0;
        write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        write.descriptorCount = 1;
        write.pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
    }
}

void InstanceDescriptorManager::update(
    uint32_t frameIndex,
    uint32_t baseInstance,
    const std::vector<InstanceData>& models
) {
    if (baseInstance + models.size() > maxInstances)
        throw std::runtime_error("Instance buffer overflow");

    VkDeviceSize offset = baseInstance * sizeof(InstanceData);
    VkDeviceSize size = models.size() * sizeof(InstanceData);

    std::memcpy(
        static_cast<char*>(mapped[frameIndex]) + offset,
        models.data(),
        size
    );

    if (!memoryInfo[frameIndex].isCoherent)
    {
        VkDeviceSize atomSize = nonCoherentAtomSize;

        VkDeviceSize alignedOffset = offset & ~(atomSize - 1);
        VkDeviceSize alignedSize = ((offset + size + atomSize - 1) & ~(atomSize - 1)) - alignedOffset;

        VkMappedMemoryRange range{};
        range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        range.memory = memoryInfo[frameIndex].memory;
        range.offset = alignedOffset;
        range.size = alignedSize;

        vkFlushMappedMemoryRanges(device, 1, &range);
    }
}

InstanceDescriptorManager::~InstanceDescriptorManager()
{
    for (size_t i = 0; i < buffers.size(); i++)
    {
        if (mapped[i])
            vkUnmapMemory(device, memoryInfo[i].memory);

        if (buffers[i])
            vkDestroyBuffer(device, buffers[i], nullptr);

        if (memoryInfo[i].memory)
            vkFreeMemory(device, memoryInfo[i].memory, nullptr);
    }

    if (descriptorPool)
        vkDestroyDescriptorPool(device, descriptorPool, nullptr);

    if (descriptorSetLayout)
        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
}
