#include "MaterialDescriptorManager.hpp"

#include <stdexcept>

MaterialDescriptorManager::MaterialDescriptorManager(
    VkDevice device,
    uint32_t maxMaterials,
    std::vector<MaterialDescriptorManager::IMaterialLayoutProvider*> providers
)
    : device(device)
{
    MaterialLayoutBuilder builder(0, 7);

    // engine provider
    EngineMaterialProvider engineProvider;
    engineProvider.contribute(builder);

    // mods providers
    for (auto* p : providers)
        p->contribute(builder);

    // create layouts
    std::vector<VkDescriptorSetLayoutBinding> vkBindings;

    for (const auto& b : builder.getBindings())
    {
        VkDescriptorSetLayoutBinding vk{};
        vk.binding = b.binding;
        vk.descriptorType = b.type;
        vk.descriptorCount = b.count;
        vk.stageFlags = b.stages;
        vk.pImmutableSamplers = nullptr;

        vkBindings.push_back(vk);
    }

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(vkBindings.size());
    layoutInfo.pBindings = vkBindings.data();

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
        throw std::runtime_error("Failed to create material descriptor layout");

    // create pools
    std::vector<VkDescriptorPoolSize> poolSizes;

    for (const auto& [type, count] : builder.getDescriptorCounts())
    {
        VkDescriptorPoolSize ps{};
        ps.type = type;
        ps.descriptorCount = count * maxMaterials;
        poolSizes.push_back(ps);
    }

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = maxMaterials;

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
        throw std::runtime_error("Failed to create material descriptor pool");
}

MaterialDescriptorManager::~MaterialDescriptorManager()
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