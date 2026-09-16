#include "MaterialDescriptorManager.hpp"
#include "../texture/TextureManager.hpp"

#include <stdexcept>

MaterialDescriptorManager::MaterialDescriptorManager(
    VkDevice device,
    VkBuffer materialBuffer,
    TextureManager* textureManager,
    uint32_t maxTextures
) :
    device(device),
    textureManager(textureManager),
    maxTextures(maxTextures)
{
    #ifndef NDEBUG
    if (!textureManager)
        throw std::invalid_argument("MaterialDescriptorManager: textureManager is null");

    if (materialBuffer == VK_NULL_HANDLE)
        throw std::invalid_argument("MaterialDescriptorManager: materialBuffer is null");

    if (maxTextures < 3)
        throw std::invalid_argument("MaterialDescriptorManager: maxTextures must be at least 3");
    #endif

    std::vector<VkDescriptorSetLayoutBinding> bindings(2);

    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags =
        VK_SHADER_STAGE_VERTEX_BIT |
        VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings[0].pImmutableSamplers = nullptr;

    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[1].descriptorCount = maxTextures;
    bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings[1].pImmutableSamplers = nullptr;

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
        throw std::runtime_error("Failed to create material descriptor set layout");
    }

    std::vector<VkDescriptorPoolSize> poolSizes;

    VkDescriptorPoolSize storageBufferPool{};
    storageBufferPool.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    storageBufferPool.descriptorCount = 1;

    poolSizes.push_back(storageBufferPool);

    VkDescriptorPoolSize samplerPool{};
    samplerPool.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerPool.descriptorCount = maxTextures;

    poolSizes.push_back(samplerPool);

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = 1;

    if (vkCreateDescriptorPool(
            device,
            &poolInfo,
            nullptr,
            &descriptorPool
        ) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create material descriptor pool");
    }

    VkDescriptorSetAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocateInfo.descriptorPool = descriptorPool;
    allocateInfo.descriptorSetCount = 1;
    allocateInfo.pSetLayouts = &descriptorSetLayout;

    if (vkAllocateDescriptorSets(
            device,
            &allocateInfo,
            &descriptorSet
        ) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate material descriptor set");
    }

    VkDescriptorBufferInfo materialBufferInfo{};
    materialBufferInfo.buffer = materialBuffer;
    materialBufferInfo.offset = 0;
    materialBufferInfo.range = VK_WHOLE_SIZE;

    VkWriteDescriptorSet materialWrite{};
    materialWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    materialWrite.dstSet = descriptorSet;
    materialWrite.dstBinding = 0;
    materialWrite.dstArrayElement = 0;
    materialWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    materialWrite.descriptorCount = 1;
    materialWrite.pBufferInfo = &materialBufferInfo;

    vkUpdateDescriptorSets(
        device,
        1,
        &materialWrite,
        0,
        nullptr
    );

    updateTextures();
}

MaterialDescriptorManager::~MaterialDescriptorManager()
{
    if (descriptorPool)
    {
        vkDestroyDescriptorPool(
            device,
            descriptorPool,
            nullptr
        );
    }

    if (descriptorSetLayout)
    {
        vkDestroyDescriptorSetLayout(
            device,
            descriptorSetLayout,
            nullptr
        );
    }
}

void MaterialDescriptorManager::updateTextures()
{
    #ifndef NDEBUG
    if (!textureManager)
        throw std::runtime_error(
            "MaterialDescriptorManager: textureManager is null"
        );
    #endif

    uint32_t textureCount =
        textureManager->getTextureCount();

    if (textureCount > maxTextures)
        textureCount = maxTextures;

    for (uint32_t i = 0; i < textureCount; i++)
    {
        const std::shared_ptr<TextureImage>& texture =
            textureManager->getTexture(i);

        if (!texture)
            continue;

        updateTexture(i);
    }
}

void MaterialDescriptorManager::updateTexture(
    uint32_t textureIndex
)
{
    #ifndef NDEBUG
    if (!textureManager)
        throw std::runtime_error("MaterialDescriptorManager: textureManager is null");

    if (textureIndex >= maxTextures)
        throw std::out_of_range("MaterialDescriptorManager: texture index out of range");
    #endif

    const std::shared_ptr<TextureImage>& texture =
        textureManager->getTexture(textureIndex);

    if (!texture)
        return;

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = texture->getImageView();
    imageInfo.sampler = texture->getSampler();

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = descriptorSet;
    write.dstBinding = 1;
    write.dstArrayElement = textureIndex;
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