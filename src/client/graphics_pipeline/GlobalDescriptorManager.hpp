#pragma once

#include "../CoreVulkan.hpp"
#include <vector>

class CameraBufferManager;

struct AccelerationStructureGPU;

class GlobalDescriptorManager
{
private:
    VkDevice device; ///< Vulkan logical device (non-owning)

    VkDescriptorSetLayout descriptorSetLayout{VK_NULL_HANDLE}; ///< Global descriptor set layout
    VkDescriptorPool descriptorPool{VK_NULL_HANDLE}; ///< Descriptor pool for global sets
    std::vector<VkDescriptorSet> descriptorSets; ///< One descriptor set per frame-in-flight

public:

    GlobalDescriptorManager(
        VkDevice device,
        CameraBufferManager* cameraBufferManager,
        AccelerationStructureGPU* blasBuffer,
        AccelerationStructureGPU* blasInstanceBuffer,
        AccelerationStructureGPU* tlasGPU,
        AccelerationStructureGPU* tlasInstanceGPU,
        uint32_t maxFramesInFlight
    );

    ~GlobalDescriptorManager();

    VkDescriptorSetLayout getLayout() const { return descriptorSetLayout; }
    VkDescriptorPool getDescriptorPool() const { return descriptorPool; }
    std::vector<VkDescriptorSet> getDescriptorSets() const { return descriptorSets; }
};
