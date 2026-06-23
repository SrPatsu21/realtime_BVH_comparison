#pragma once

#include "../../CoreVulkan.hpp"
#include "MaterialLayoutBuilder.hpp"

/**
 * @brief Manages the Vulkan descriptor set layout and descriptor pool used by materials.
 *
 * MaterialDescriptorManager is responsible for:
 * - Building a unified descriptor set layout shared by all materials.
 * - Allowing both engine-defined and external providers to contribute bindings.
 * - Creating a descriptor pool sized to support a maximum number of materials.
 *
 * The class centralizes descriptor configuration so that material systems,
 * engine modules, and optional extensions can safely contribute bindings
 * without directly managing Vulkan layout or pool creation.
 *
 * The final layout is immutable after construction.
 */
class MaterialDescriptorManager
{
private:
    VkDevice device;
    VkDescriptorSetLayout descriptorSetLayout{};
    VkDescriptorPool descriptorPool{};
public:

    /**
     * @brief Interface for contributing descriptor bindings to the material layout.
     *
     * Implementations of this interface can inject descriptor bindings into the
     * material layout during construction of the manager.
     *
     * This allows modular systems (engine core, plugins, render features, etc.)
     * to extend the material descriptor set without tightly coupling layout logic.
     *
     * The order of contribution determines final binding organization.
     */
    class IMaterialLayoutProvider
    {
    public:
        virtual ~IMaterialLayoutProvider() = default;

        /**
         * @brief Contributes descriptor bindings to the provided builder.
         *
         * Implementations should register one or more bindings using the
         * MaterialLayoutBuilder interface.
         *
         * @param builder Builder used to collect descriptor bindings.
         */
        virtual void contribute(MaterialLayoutBuilder& builder) = 0;
    };

    /**
     * @brief Default engine-level material layout provider.
     *
     * This provider defines bindings that are always present in the engine's
     * material system.
     *
     * Current reserved bindings:
     * - Binding 0: Albedo texture (combined image sampler, fragment stage)
     * - Binding 1: Normal texture (combined image sampler, fragment stage)
     *
     * These bindings are considered engine-reserved and should not be reused
     * by external providers.
     */
    class EngineMaterialProvider : public IMaterialLayoutProvider
    {
    public:
        void contribute(MaterialLayoutBuilder& builder) override
        {
            // Binding 0 reserved for albedo
            builder.addEngineBinding(
                0,
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                1,
                VK_SHADER_STAGE_FRAGMENT_BIT
            );

            // Binding 1 reserved for normal
            builder.addEngineBinding(
                1,
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                1,
                VK_SHADER_STAGE_FRAGMENT_BIT
            );

            // Binding 1 reserved for metallicRoughness
            builder.addEngineBinding(
                2,
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                1,
                VK_SHADER_STAGE_FRAGMENT_BIT
            );
        }
    };

    /**
     * @brief Constructs the material descriptor manager.
     *
     * This constructor performs the full descriptor setup process:
     *
     * 1. Instantiates the engine's default provider.
     * 2. Invokes all external IMaterialLayoutProvider instances.
     * 3. Builds the final VkDescriptorSetLayout from collected bindings.
     * 4. Creates a VkDescriptorPool sized for the specified maximum number of materials.
     *
     * The descriptor pool is sized proportionally to:
     *     (descriptor count per layout binding) * maxMaterials
     *
     * @param device Vulkan logical device used for descriptor creation.
     * @param maxMaterials Maximum number of material descriptor sets that can be allocated.
     *                     This directly determines pool capacity.
     * @param providers External layout providers that contribute additional bindings.
     *
     * @throws std::runtime_error if layout or pool creation fails.
     */
    MaterialDescriptorManager(
        VkDevice device,
        uint32_t maxMaterials,
        std::vector<MaterialDescriptorManager::IMaterialLayoutProvider*> providers
    );

    /**
     * @brief Destroys the descriptor pool and descriptor set layout.
     *
     * Resources are destroyed in reverse order of usage safety:
     * - Descriptor pool
     * - Descriptor set layout
     *
     * The Vulkan device must remain valid during destruction.
     */
    ~MaterialDescriptorManager();

    VkDescriptorSetLayout getLayout() const { return descriptorSetLayout; }
    VkDescriptorPool getDescriptorPool() const { return descriptorPool; }
};