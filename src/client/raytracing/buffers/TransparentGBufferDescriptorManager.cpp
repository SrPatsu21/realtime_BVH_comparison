#include "TransparentGBufferDescriptorManager.hpp"

#include <array>
#include <stdexcept>

TransparentGBufferDescriptorManager::TransparentGBufferDescriptorManager(
    VkDevice device,
    TransparentGBuffer* transparentGBuffer
)
    : device(device)
{
    #ifndef NDEBUG
    if (transparentGBuffer == nullptr)
        throw std::runtime_error(
            "TransparentGBufferDescriptorManager: TransparentGBuffer is null"
        );
    #endif

    // ------------------------------------------------------------
    // Descriptor Set Layout
    //
    // set = 1
    //
    // binding 0 -> Position
    // binding 1 -> Normal
    // binding 2 -> Albedo
    // binding 3 -> Material
    // ------------------------------------------------------------

    std::array<VkDescriptorSetLayoutBinding, 4> bindings{};

    for (uint32_t i = 0; i < bindings.size(); ++i)
    {
        bindings[i].binding = i;
        bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[i].descriptorCount = 1;
        bindings[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings[i].pImmutableSamplers = nullptr;
    }

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
            "Failed to create TransparentGBuffer descriptor set layout"
        );
    }

    // ------------------------------------------------------------
    // Descriptor Pool
    // ------------------------------------------------------------

    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = 4;

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
            "Failed to create TransparentGBuffer descriptor pool"
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
        throw std::runtime_error(
            "Failed to allocate TransparentGBuffer descriptor set"
        );
    }

    // ------------------------------------------------------------
    // Image infos
    // ------------------------------------------------------------

    std::array<VkDescriptorImageInfo, 4> imageInfos{};
    const std::array<TransparentGBuffer::Attachment, 4> attachments = {
        TransparentGBuffer::Attachment::Position,
        TransparentGBuffer::Attachment::Normal,
        TransparentGBuffer::Attachment::Albedo,
        TransparentGBuffer::Attachment::Material,
    };

    for (size_t i = 0; i < attachments.size(); ++i)
    {
        imageInfos[i].sampler = transparentGBuffer->getSample(attachments[i]);
        imageInfos[i].imageView = transparentGBuffer->getView(attachments[i]);
        imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    // ------------------------------------------------------------
    // Descriptor writes
    // ------------------------------------------------------------

    std::array<VkWriteDescriptorSet, 4> writes{};

    for (uint32_t i = 0; i < writes.size(); ++i)
    {
        writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[i].dstSet = descriptorSet;
        writes[i].dstBinding = i;
        writes[i].dstArrayElement = 0;
        writes[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[i].descriptorCount = 1;
        writes[i].pImageInfo = &imageInfos[i];
    }

    vkUpdateDescriptorSets(
        device,
        static_cast<uint32_t>(writes.size()),
        writes.data(),
        0,
        nullptr
    );
}

TransparentGBufferDescriptorManager::~TransparentGBufferDescriptorManager()
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