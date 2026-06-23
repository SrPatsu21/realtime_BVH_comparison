#include "SamplerManager.hpp"
#include <stdexcept>

const SamplerManager::SamplerDesc SamplerManager::LinearRepeat = []{
    SamplerDesc d{};
    d.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    d.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    d.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    d.compareEnable = VK_FALSE;
    return d;
}();

const SamplerManager::SamplerDesc SamplerManager::LinearClamp = []{
    SamplerDesc d{};
    d.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    d.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    d.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    d.compareEnable = VK_FALSE;
    return d;
}();
const SamplerManager::SamplerDesc SamplerManager::Shadow = []{
    SamplerDesc d{};

    d.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    d.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    d.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

    d.anisotropyEnable = VK_FALSE;

    d.compareEnable = VK_TRUE;
    d.compareOp = VK_COMPARE_OP_LESS;

    d.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

    return d;
}();

SamplerManager::SamplerManager(
    VkPhysicalDevice physicalDevice,
    VkDevice device
) :
    device(device),
    physicalDevice(physicalDevice)
{
}

SamplerManager::~SamplerManager()
{
    for (auto& pair : samplers)
        vkDestroySampler(device, pair.second, nullptr);

    samplers.clear();
}

VkSampler SamplerManager::getSampler(const SamplerDesc& desc)
{
    auto it = samplers.find(desc);
    if (it != samplers.end())
        return it->second;

    VkSampler sampler = createSampler(desc);
    samplers[desc] = sampler;

    return sampler;
}

VkSampler SamplerManager::createSampler(const SamplerDesc& desc)
{
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);

    VkSamplerCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

    info.magFilter = desc.magFilter;
    info.minFilter = desc.minFilter;

    info.addressModeU = desc.addressModeU;
    info.addressModeV = desc.addressModeV;
    info.addressModeW = desc.addressModeW;

    info.anisotropyEnable = desc.anisotropyEnable;
    info.maxAnisotropy = desc.anisotropyEnable
        ? std::min(desc.maxAnisotropy, properties.limits.maxSamplerAnisotropy)
        : 1.0f;

    info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    info.unnormalizedCoordinates = VK_FALSE;

    info.compareEnable = VK_FALSE;
    info.compareOp = VK_COMPARE_OP_ALWAYS;

    info.mipmapMode = desc.mipmapMode;
    info.mipLodBias = 0.0f;
    info.minLod = desc.minLod;
    info.maxLod = desc.maxLod;

    VkSampler sampler;
    if (vkCreateSampler(device, &info, nullptr, &sampler) != VK_SUCCESS)
        throw std::runtime_error("Failed to create sampler");

    return sampler;
}