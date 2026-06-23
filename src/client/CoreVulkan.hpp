#pragma once

#ifndef GLFW_INCLUDE_VULKAN
    #define GLFW_INCLUDE_VULKAN
#endif

#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <cstring>
#include <map>
#include <optional>
#include "./swapchain&framebuffer/SwapchainSupportDetails.hpp"
#ifndef NDEBUG
    #include <cassert>

    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT severity,
        VkDebugUtilsMessageTypeFlagsEXT type,
        const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
        void* userData)
    {
        const char* severityStr = "UNKNOWN";
        if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)   severityStr = "VERBOSE";
        if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)      severityStr = "INFO";
        if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)   severityStr = "WARNING";
        if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)     severityStr = "ERROR";

        const char* typeStr = "UNKNOWN";
        if (type & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)       typeStr = "GENERAL";
        if (type & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)    typeStr = "VALIDATION";
        if (type & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)   typeStr = "PERFORMANCE";

        std::cerr << "\n[VK " << severityStr << "][" << typeStr << "] " << callbackData->pMessage << "\n";

        if (callbackData->objectCount > 0) {
            std::cerr << "  Objects: ";
            for (uint32_t i = 0; i < callbackData->objectCount; ++i) {
                std::cerr << callbackData->pObjects[i].objectHandle << " ";
            }
            std::cerr << "\n";
        }

        std::cerr << std::endl;

        return VK_FALSE;
    }

#endif

/// Vulkan validation layers enabled in debug builds.
const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

/**
 * @brief Holds queue family indices required by the engine.
 *
 * This struct represents the queue families needed for rendering and presentation.
 * A Vulkan device is considered usable only if all required queue families are present.
 *
 * This structure is part of the public API and may be used by external systems
 * (e.g. render modules or engine extensions) to query queue compatibility.
 */
struct QueueFamilyIndices {
    /// Queue family supporting graphics commands.
    std::optional<uint32_t> graphicsFamily;

    /// Queue family supporting presentation to a surface.
    std::optional<uint32_t> presentFamily;

    /**
     * @brief Checks whether all required queue families were found.
     * @return True if both graphics and present queue families are available.
     */
    bool isComplete() const {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

/**
 * @brief Core Vulkan initialization and device management class.
 *
 * This class owns and manages the Vulkan instance, physical device selection,
 * logical device creation, queues, surface, and related capabilities.
 *
 * It is designed to be extensible through provider and selector interfaces,
 * allowing engine subsystems and mods to influence Vulkan configuration
 * without tightly coupling to the core.
 */
class CoreVulkan
{

    #ifndef NDEBUG
        void CreateDebugCallback();
        void DestroyDebugCallback();
        VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    #endif

//* vars
protected:
    VkInstance instance;
    VkSurfaceKHR surface;
    QueueFamilyIndices graphicsQueueFamilyIndices;
    VkPhysicalDevice physicalDevice;
    SwapchainSupportDetails swapchainSupportDetails;
    VkSampleCountFlagBits msaaSamples;
    VkDevice device;
    VkQueue presentQueue;
    VkQueue graphicsQueue;
    VkFormat depthFormat;
    VkDeviceSize atomSize;
    VkPhysicalDeviceVulkan12Features supportedFeatures12{};
    /// Device extensions required by the engine.
    const std::vector<const char*> DEVICE_EXTENSIONS = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,  // * Enables swapchain functionality for presenting images to the screen
        VK_EXT_MEMORY_BUDGET_EXTENSION_NAME, // * Provides information about memory budgets, allowing applications to make better memory usage decisions
        // VK_KHR_PERFORMANCE_QUERY_EXTENSION_NAME, // * Allows querying performance-related metrics to help optimize graphics performance
        // VK_EXT_CALIBRATED_TIMESTAMPS_EXTENSION_NAME, // * Provides a mechanism for high-precision timestamps for better timing and synchronization in applications
        // VK_KHR_MULTIVIEW_EXTENSION_NAME, // * Enables rendering to multiple views within a single pass, useful for VR and stereoscopic rendering
    };
public:
    /**
     * @brief Configuration data used during Vulkan instance creation.
     *
     * Providers may append extensions, layers, or override the API version.
     * This struct is intentionally simple and additive.
     */
    struct InstanceConfig {
        uint32_t apiVersion = VK_API_VERSION_1_3;
        std::vector<const char*> extensions;
        std::vector<const char*> layers;
    };

    /**
     * @brief Interface for systems that want to contribute to instance creation.
     *
     * Implementations may add extensions, layers, or modify the instance config.
     * Called before the Vulkan instance is created.
     */
    struct IInstanceConfigProvider {
        virtual ~IInstanceConfigProvider() = default;

        /**
         * @param config Mutable instance configuration.
         */
        virtual void contribute(
            InstanceConfig& config
        ) = 0;
    };

    /**
     * @brief Required capabilities for a physical device.
     *
     * Used during device selection to ensure mandatory extensions and features
     * are supported before a device is considered usable.
     */
    struct PhysicalDeviceRequirements {
        std::vector<const char*> requiredExtensions;
        VkPhysicalDeviceFeatures requiredFeatures{};
    };

    /**
     * @brief Interface used to validate and score physical devices.
     *
     * Selectors allow engine modules or mods to influence GPU selection logic.
     */
    struct IPhysicalDeviceSelector {
        virtual ~IPhysicalDeviceSelector() = default;

        /**
         * @brief Checks whether a physical device is compatible.
         */
        virtual bool isDeviceCompatible(
            VkPhysicalDevice device,
            const PhysicalDeviceRequirements& requirements
        ) = 0;

        /**
         * @brief Adds a score contribution to a compatible device.
         */
        virtual void scoreDevice(
            VkPhysicalDevice device,
            int& score
        ) = 0;
    };

    /**
     * @brief Configuration for logical device creation.
     *
     * Required features must be supported by the device.
     * Optional features are enabled when available.
     */
    struct DeviceConfig {
        std::vector<const char*> extensions;
        VkPhysicalDeviceFeatures requiredFeatures{};
        VkPhysicalDeviceFeatures optionalFeatures{};
    };

    /**
     * @brief Interface for contributing to logical device configuration.
     */
    struct IDeviceConfigProvider {
        virtual ~IDeviceConfigProvider() = default;
        virtual void contribute(DeviceConfig& config) = 0;
    };

    /**
     * @brief Requirements used when selecting a depth buffer format.
     */
    struct DepthFormatRequirements {
        bool requireStencil = false;
        bool preferHighPrecision = false;
    };

    /**
     * @brief Interface for systems that influence depth format selection.
     */
    struct IDepthFormatProvider {
        virtual void contribute(DepthFormatRequirements&) = 0;
    };

private:
    /// Checks whether requested validation layers are supported by the system.
    bool checkValidationLayerSupport();

    /// Creates the Vulkan instance using contributions from providers.
    void createInstance(
        const std::vector<IInstanceConfigProvider*>& providers
    );

    /// Creates the Vulkan surface from a GLFW window.
    void createSurface(
        GLFWwindow* window
    );

    /// Finds queue families supported by a physical device.
    QueueFamilyIndices findQueueFamilies(
        VkPhysicalDevice physicalDevice
    );

    /// Queries swapchain capabilities for a physical device.
    SwapchainSupportDetails querySwapchainSupport(
        VkPhysicalDevice physicalDevice
    );

    /// Validates whether a physical device meets all requirements.
    bool isDeviceSuitable(
        VkPhysicalDevice physicalDevice,
        const PhysicalDeviceRequirements& reqs,
        const std::vector<IPhysicalDeviceSelector*>& selectors
    );

    /// Assigns a suitability score to a physical device.
    int rateDeviceSuitability(
        VkPhysicalDevice physicalDevice,
        const PhysicalDeviceRequirements& reqs,
        const std::vector<IPhysicalDeviceSelector*>& selectors
    );

    /// Determines the maximum usable MSAA sample count.
    VkSampleCountFlagBits findMaxLimitedUsableSampleCount(
        VkSampleCountFlagBits maxDesiredSamples,
        VkPhysicalDevice physicalDevice
    );

    /// Selects the best physical device available.
    void pickPhysicalDevice(
        const std::vector<IPhysicalDeviceSelector*>& selectors
    );

    /// Creates the Vulkan logical device.
    void createLogicalDevice(
        const std::vector<IDeviceConfigProvider*>& providers
    );

    /// Releases all Vulkan resources owned by this instance.
    void cleanup();

public:
    /**
     * @brief Finds a supported image format from a list of candidates.
     *
     * @param candidates List of acceptable formats.
     * @param tiling Required image tiling mode.
     * @param features Required format feature flags.
     *
     * @return A format that satisfies all requirements.
     *
     * @throws std::runtime_error if no suitable format is found.
     */
    VkFormat findSupportedFormat(
        const std::vector<VkFormat>& candidates,
        VkImageTiling tiling,
        VkFormatFeatureFlags features
    );

    /// Re-queries and updates swapchain support details.
    void updateSwapchainDetails();

    /**
     * @brief Constructs and fully initializes the Vulkan core.
     *
     * This constructor performs:
     * - Instance creation
     * - Surface creation
     * - Physical device selection
     * - Logical device creation
     * - Queue retrieval
     * - Depth format selection
     *
     * @param window GLFW window used to create the Vulkan surface.
     * @param instanceProviders Contributors to instance configuration.
     * @param PhysicalDeviceSelector Device compatibility and scoring logic.
     * @param DeviceProviders Contributors to logical device configuration.
     * @param depthProviders Contributors to depth format selection.
     */
    explicit CoreVulkan(
        GLFWwindow* window,
        const std::vector<IInstanceConfigProvider*>& instanceProviders,
        const std::vector<IPhysicalDeviceSelector*>& PhysicalDeviceSelector,
        const std::vector<IDeviceConfigProvider*>& DeviceProviders,
        const std::vector<IDepthFormatProvider*>& depthProviders
    );

    /// Destroys all Vulkan resources owned by this instance.
    ~CoreVulkan();

    // Deleting the copy constructor and copy assignment to prevent copies

    CoreVulkan(const CoreVulkan& obj) = delete;
    CoreVulkan& operator=(const CoreVulkan& other) = delete;

    // move constructor and move assignment

    CoreVulkan(CoreVulkan&& other) noexcept;
    CoreVulkan& operator=(CoreVulkan&& other) noexcept;

    /**
     * @brief Finds a suitable memory type for buffer or image allocation.
     *
     * @param physicalDevice Physical device used for memory queries.
     * @param typeFilter Bitmask of allowed memory types.
     * @param required Required memory property flags.
     * @param preferred Preferred (but optional) memory property flags.
     *
     * @return Index of a compatible memory type.
     *
     * @throws std::runtime_error if no suitable memory type is found.
     */
    static uint32_t findMemoryType(
        VkPhysicalDevice physicalDevice,
        uint32_t typeFilter,
        VkMemoryPropertyFlags required,
        VkMemoryPropertyFlags preferred
    );

    /**
     * @brief Checks whether a depth format includes a stencil component.
     *
     * Used to automatically extend the image aspect mask when creating
     * the depth image view.
     *
     * @param format Depth image format.
     * @return true if the format includes a stencil component.
     */
    static bool hasStencilComponent(VkFormat format);

    /**
     * @brief Retrieves the non-coherent atom size of the physical device.
     *
     * The nonCoherentAtomSize defines the minimum alignment requirement
     * when flushing or invalidating mapped memory ranges that are not
     * host-coherent.
     *
     * When working with non-coherent memory, any vkFlushMappedMemoryRanges
     * or vkInvalidateMappedMemoryRanges operation must:
     *
     * - Use offsets aligned to nonCoherentAtomSize
     * - Use sizes that are multiples of nonCoherentAtomSize
     *
     * Failing to respect this alignment leads to undefined behavior.
     *
     * This value is device-specific and retrieved from
     * VkPhysicalDeviceProperties::limits.
     *
     * @param physicalDevice The Vulkan physical device.
     * @return The non-coherent atom size in bytes.
     */
    static VkDeviceSize takeAtomSize(
        VkPhysicalDevice physicalDevice
    );

    /**
     * @brief Checks whether a memory type is host-coherent.
     *
     * Host-coherent memory guarantees that writes performed by the CPU
     * are automatically visible to the GPU without requiring explicit
     * flush operations, and vice versa.
     *
     * If the memory type is not host-coherent, explicit synchronization
     * using vkFlushMappedMemoryRanges and/or vkInvalidateMappedMemoryRanges
     * is required.
     *
     * This function inspects the memory type flags obtained from
     * vkGetPhysicalDeviceMemoryProperties.
     *
     * @param physicalDevice The Vulkan physical device.
     * @param memoryTypeIndex Index of the memory type to inspect.
     *
     * @return true if the memory type includes VK_MEMORY_PROPERTY_HOST_COHERENT_BIT.
     *
     * @throws std::runtime_error if memoryTypeIndex is out of range.
     */
    static bool isMemoryCoherent(
        VkPhysicalDevice physicalDevice,
        uint32_t memoryTypeIndex
    );

    /**
     * @brief Returns whether a Vulkan format contains a depth component.
     *
     * This helper is typically used to determine the correct
     * VkImageAspectFlags when creating image views or performing
     * layout transitions.
     *
     * Depth formats detected:
     * - VK_FORMAT_D32_SFLOAT
     * - VK_FORMAT_D32_SFLOAT_S8_UINT
     * - VK_FORMAT_D24_UNORM_S8_UINT
     *
     * @param format Vulkan image format.
     * @return True if the format includes a depth component.
     */
    static bool hasDepthComponent(
        VkFormat format
    );
//* get
    const VkInstance& getInstance() const { return instance; }
    const VkSurfaceKHR& getSurface() const { return surface; }
    const QueueFamilyIndices& getGraphicsQueueFamilyIndices() const { return graphicsQueueFamilyIndices; }
    const VkPhysicalDevice& getPhysicalDevice() const { return physicalDevice; }
    const SwapchainSupportDetails& getSwapchainSupportDetails() const { return swapchainSupportDetails; }
    const VkSampleCountFlagBits& getMsaaSamples() const { return msaaSamples; }
    const VkDevice& getDevice() const { return device; }
    const VkQueue& getGraphicsQueue() const { return graphicsQueue; }
    const VkQueue& getPresentQueue() const { return presentQueue; }
    const VkFormat& getDepthFormat() const { return depthFormat; }
    const std::vector<const char*>& getDeviceExtensions() const { return DEVICE_EXTENSIONS; }
    const VkDeviceSize getAtomSize() const { return atomSize; }
    const VkPhysicalDeviceVulkan12Features getSupportedFeatures12() const { return supportedFeatures12; }
};
