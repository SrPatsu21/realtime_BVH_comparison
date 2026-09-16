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

    if (maxTextures == 0)
        throw std::invalid_argument("MaterialDescriptorManager: maxTextures must be greater than zero");
    #endif

    std::vector<VkDescriptorSetLayoutBinding> bindings(4);

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
    samplerPool.descriptorCount = maxTextures * 3;

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

    if (vkAllocateDescriptorSets(device, &allocateInfo, &descriptorSet) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate material descriptor set");

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

    std::vector<VkDescriptorImageInfo> baseColorInfos(
        maxTextures
    );
    std::vector<VkDescriptorImageInfo> normalInfos(
        maxTextures
    );
    std::vector<VkDescriptorImageInfo> metallicRoughnessInfos(
        maxTextures
    );

    const std::shared_ptr<TextureImage>& defaultWhite = textureManager->defaultWhite;
    const std::shared_ptr<TextureImage>& defaultNormal = textureManager->defaultNormal;
    const std::shared_ptr<TextureImage>& defaultMetallic = textureManager->defaultMetallic;

    for (uint32_t i = 0; i < maxTextures; i++)
    {
        baseColorInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        baseColorInfos[i].imageView = defaultWhite->getImageView();
        baseColorInfos[i].sampler = defaultWhite->getSampler();

        normalInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        normalInfos[i].imageView = defaultNormal->getImageView();
        normalInfos[i].sampler = defaultNormal->getSampler();

        metallicRoughnessInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        metallicRoughnessInfos[i].imageView = defaultMetallic->getImageView();
        metallicRoughnessInfos[i].sampler = defaultMetallic->getSampler();
    }

    uint32_t textureCount = textureManager->getTextureCount();

    if (textureCount > maxTextures)
        textureCount = maxTextures;

    for (uint32_t i = 1; i < textureCount; i++)
    {
        const std::shared_ptr<TextureImage>& texture =
            textureManager->getTexture(i);

        if (!texture)
            continue;

        baseColorInfos[i].imageView = texture->getImageView();
        baseColorInfos[i].sampler = texture->getSampler();

        normalInfos[i].imageView = texture->getImageView();
        normalInfos[i].sampler = texture->getSampler();

        metallicRoughnessInfos[i].imageView = texture->getImageView();
        metallicRoughnessInfos[i].sampler = texture->getSampler();
    }

    std::vector<VkWriteDescriptorSet> writes(3);
    writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].dstSet = descriptorSet;
    writes[0].dstBinding = 1;
    writes[0].dstArrayElement = 0;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[0].descriptorCount = maxTextures;
    writes[0].pImageInfo = baseColorInfos.data();

    writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[1].dstSet = descriptorSet;
    writes[1].dstBinding = 2;
    writes[1].dstArrayElement = 0;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[1].descriptorCount = maxTextures;
    writes[1].pImageInfo = normalInfos.data();

    writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[2].dstSet = descriptorSet;
    writes[2].dstBinding = 3;
    writes[2].dstArrayElement = 0;
    writes[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[2].descriptorCount = maxTextures;
    writes[2].pImageInfo = metallicRoughnessInfos.data();

    vkUpdateDescriptorSets(device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
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

    if (textureIndex == 0)
        return;

    const std::shared_ptr<TextureImage>& texture =
        textureManager->getTexture(textureIndex);

    if (!texture)
        return;

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = texture->getImageView();
    imageInfo.sampler = texture->getSampler();

    VkWriteDescriptorSet writes[3]{};
    writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].dstSet = descriptorSet;
    writes[0].dstBinding = 1;
    writes[0].dstArrayElement = textureIndex;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[0].descriptorCount = 1;
    writes[0].pImageInfo = &imageInfo;

    writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[1].dstSet = descriptorSet;
    writes[1].dstBinding = 2;
    writes[1].dstArrayElement = textureIndex;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[1].descriptorCount = 1;
    writes[1].pImageInfo = &imageInfo;

    writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[2].dstSet = descriptorSet;
    writes[2].dstBinding = 3;
    writes[2].dstArrayElement = textureIndex;
    writes[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[2].descriptorCount = 1;
    writes[2].pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(device, 3, writes, 0, nullptr);
}