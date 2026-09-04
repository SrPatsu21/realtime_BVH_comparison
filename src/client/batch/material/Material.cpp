#include "Material.hpp"
#include "../../Render.hpp"
#include "../../BufferManager.hpp"

#include <array>
#include <vector>
#include <stdexcept>

Material::Material(
    VkDevice device,
    BufferManager* bufferManager,
    MaterialDescriptorManager* descriptorManager,
    std::shared_ptr<TextureImage> baseColorHandle,
    std::shared_ptr<TextureImage> normalHandle,
    std::shared_ptr<TextureImage> metallicRoughnessHandle,
    AlphaMode alphaMode,
    float alphaCutoff
) :
    device(device),
    baseColorHandle(baseColorHandle),
    normalHandle(normalHandle),
    metallicRoughnessHandle(metallicRoughnessHandle)
{
    materialAlphaData = {
        .alphaMode = alphaMode,
        .alphaCutoff = alphaCutoff
    };

    std::vector<MaterialAlphaData> materialData = {
        materialAlphaData
    };

    materialAlphaBuffer = bufferManager->createDeviceBuffer(
        materialData,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        materialAlphaMemory,
        nullptr
    );

    auto albedo =
        baseColorHandle
            ? baseColorHandle
            : Render::defaultTextures.white;

    auto normal =
        normalHandle
            ? normalHandle
            : Render::defaultTextures.normal;

    auto metallic =
        metallicRoughnessHandle
            ? metallicRoughnessHandle
            : Render::defaultTextures.metallic;

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorManager->getDescriptorPool();
    allocInfo.descriptorSetCount = 1;

    VkDescriptorSetLayout layout = descriptorManager->getLayout();
    allocInfo.pSetLayouts = &layout;

    if (vkAllocateDescriptorSets(
        device,
        &allocInfo,
        &descriptorSet
    ) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate material descriptor set");
    }

    VkDescriptorImageInfo albedoInfo{};
    albedoInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    albedoInfo.imageView = albedo->getImageView();
    albedoInfo.sampler = albedo->getSampler();

    VkDescriptorImageInfo normalInfo{};
    normalInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    normalInfo.imageView = normal->getImageView();
    normalInfo.sampler = normal->getSampler();

    VkDescriptorImageInfo metallicInfo{};
    metallicInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    metallicInfo.imageView = metallic->getImageView();
    metallicInfo.sampler = metallic->getSampler();

    VkDescriptorBufferInfo materialInfo{};
    materialInfo.buffer = materialAlphaBuffer;
    materialInfo.offset = 0;
    materialInfo.range = sizeof(MaterialAlphaData);

    std::array<VkWriteDescriptorSet, 4> writes{};

    writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].dstSet = descriptorSet;
    writes[0].dstBinding = 0;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[0].descriptorCount = 1;
    writes[0].pImageInfo = &albedoInfo;

    writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[1].dstSet = descriptorSet;
    writes[1].dstBinding = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[1].descriptorCount = 1;
    writes[1].pImageInfo = &normalInfo;

    writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[2].dstSet = descriptorSet;
    writes[2].dstBinding = 2;
    writes[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[2].descriptorCount = 1;
    writes[2].pImageInfo = &metallicInfo;

    writes[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[3].dstSet = descriptorSet;
    writes[3].dstBinding = 3;
    writes[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[3].descriptorCount = 1;
    writes[3].pBufferInfo = &materialInfo;

    vkUpdateDescriptorSets(
        device,
        static_cast<uint32_t>(writes.size()),
        writes.data(),
        0,
        nullptr
    );
}

Material::~Material()
{
    if (materialAlphaBuffer != VK_NULL_HANDLE)
        vkDestroyBuffer(device, materialAlphaBuffer, nullptr);

    if (materialAlphaMemory != VK_NULL_HANDLE)
        vkFreeMemory(device, materialAlphaMemory, nullptr);
}