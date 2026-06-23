#include "Material.hpp"
#include "../../Render.hpp"

Material::Material(
    VkDevice device,
    MaterialDescriptorManager* descriptorManager,
    std::shared_ptr<TextureImage> baseColorHandle,
    std::shared_ptr<TextureImage> normalHandle,
    std::shared_ptr<TextureImage> metallicRoughnessHandle
) :
    baseColorHandle(baseColorHandle),
    normalHandle(normalHandle),
    metallicRoughnessHandle(metallicRoughnessHandle)
{

    auto albedo = baseColorHandle ? baseColorHandle : Render::defaultTextures.white;
    auto normal = normalHandle ? normalHandle : Render::defaultTextures.normal;
    auto metallic = metallicRoughnessHandle ? metallicRoughnessHandle : Render::defaultTextures.metallic;

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorManager->getDescriptorPool();
    allocInfo.descriptorSetCount = 1;

    VkDescriptorSetLayout layout = descriptorManager->getLayout();
    allocInfo.pSetLayouts = &layout;

    vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet);

    //image info
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

    std::array<VkWriteDescriptorSet, 3> writes{};

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

    vkUpdateDescriptorSets(
        device,
        static_cast<uint32_t>(writes.size()),
        writes.data(),
        0,
        nullptr
    );
}