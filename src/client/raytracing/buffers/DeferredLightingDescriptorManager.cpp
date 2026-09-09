#include "DeferredLightingDescriptorManager.hpp"

#include <array>
#include <stdexcept>

DeferredLightingDescriptorManager::
DeferredLightingDescriptorManager(
    VkDevice device,
    DeferredLightingBuffer* deferredLightingBuffer
)
    : device(device)
{
    #ifndef NDEBUG
    if (deferredLightingBuffer == nullptr)
        throw std::runtime_error("DeferredLightingDescriptorManager: Lighting is null");
    #endif

    // ------------------------------------------------------------
    // Descriptor Set Layout
    //
    // set = 0
    //
    // binding 0 -> Deferred Lighting
    // ------------------------------------------------------------

    std::array<VkDescriptorSetLayoutBinding, 1> bindings{};
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings[0].pImmutableSamplers = nullptr;

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
        throw std::runtime_error("Failed to create DeferredLighting descriptor set layout");
    }

    // ------------------------------------------------------------
    // Descriptor Pool
    // ------------------------------------------------------------

    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = 1;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = 1;

    if (vkCreateDescriptorPool(
            device,
            &poolInfo,
            nullptr,
            &descriptorPool
        ) != VK_SUCCESS)
    {
        throw std::runtime_error(
            "Failed to create DeferredLighting descriptor pool"
        );
    }

    // ------------------------------------------------------------
    // Allocate Descriptor Set
    // ------------------------------------------------------------

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descriptorSetLayout;

    if (vkAllocateDescriptorSets(
            device,
            &allocInfo,
            &descriptorSet
        ) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate DeferredLighting descriptor set");
    }

    // ------------------------------------------------------------
    // Image Info
    // ------------------------------------------------------------

    VkDescriptorImageInfo imageInfo{};
    imageInfo.sampler = deferredLightingBuffer->getSample();
    imageInfo.imageView = deferredLightingBuffer->getView();
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    // ------------------------------------------------------------
    // Descriptor Write
    // ------------------------------------------------------------

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = descriptorSet;
    write.dstBinding = 0;
    write.dstArrayElement = 0;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.descriptorCount = 1;
    write.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(
        device,
        1,
        &write,
        0,
        nullptr
    );
}

DeferredLightingDescriptorManager::
~DeferredLightingDescriptorManager()
{
    if (descriptorPool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        descriptorPool = VK_NULL_HANDLE;
    }

    if (descriptorSetLayout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
        descriptorSetLayout = VK_NULL_HANDLE;
    }
}