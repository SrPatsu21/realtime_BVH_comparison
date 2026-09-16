#pragma once

#include "../../CoreVulkan.hpp"

#include <cstdint>
#include <vector>

class TextureManager;

class MaterialDescriptorManager
{
private:

    VkDevice device{};

    TextureManager* textureManager{};

    VkDescriptorSetLayout descriptorSetLayout{};
    VkDescriptorPool descriptorPool{};
    VkDescriptorSet descriptorSet{};

    uint32_t maxTextures{};

public:

    MaterialDescriptorManager(
        VkDevice device,
        VkBuffer materialBuffer,
        TextureManager* textureManager,
        uint32_t maxTextures
    );

    ~MaterialDescriptorManager();

    void updateTextures();

    void updateTexture(
        uint32_t textureIndex
    );

    VkDescriptorSetLayout getLayout() const { return descriptorSetLayout; }
    VkDescriptorPool getDescriptorPool() const { return descriptorPool; }
    VkDescriptorSet getDescriptorSet() const { return descriptorSet; }
};