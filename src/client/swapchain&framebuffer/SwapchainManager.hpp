#pragma once

#include "../CoreVulkan.hpp"
#include "SwapchainSupportDetails.hpp"

/**
 * @brief Manages Vulkan swapchain creation, recreation, and image views.
 *
 * SwapchainManager encapsulates all logic related to swapchain setup,
 * including format selection, present mode selection, extent calculation,
 * image retrieval, and image view creation.
 *
 * This class is intended to be used internally by the rendering engine.
 * It exposes swapchain state through accessors while keeping Vulkan
 * creation details isolated.
 *
 * Vulkan does not have the concept of a "default framebuffer," hence it
 * requires an infrastructure that will own the buffers we will render to
 * before we visualize them on the screen. This infrastructure is known as
 * the swap chain and must be created explicitly in Vulkan. The swap chain
 * is essentially a queue of images that are waiting to be presented
 * to the screen. Our application will acquire such an image to draw to it,
 * and then return it to the queue. How exactly the queue works and the
 * conditions for presenting an image from the queue depend on how the
 * swap chain is set up. However, the general purpose of the swap chain
 * is to synchronize the presentation of images with the refresh rate
 * of the screen.
 */
class SwapchainManager
{
private:
    VkDevice device;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkExtent2D swapchainExtent;
    std::vector<VkImage> swapchainImages;
    VkFormat swapchainImageFormat;
    std::vector<VkImageView> swapchainImageViews;

    /**
     * @brief Chooses the most appropriate surface format.
     *
     * Prefers SRGB color space when available, falling back to the first
     * supported format otherwise.
     */
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(
        const std::vector<VkSurfaceFormatKHR>& availableFormats
    );

    /**
     * @brief Chooses the swapchain present mode.
     *
     * Currently defaults to FIFO for guaranteed availability and
     * predictable synchronization behavior.
     */
    VkPresentModeKHR chooseSwapPresentMode(
        const std::vector<VkPresentModeKHR>& availablePresentModes
    );

    /**
     * @brief Determines the swapchain extent based on surface capabilities.
     *
     * If the surface defines a fixed extent, it is used directly.
     * Otherwise, the extent is derived from the current framebuffer size
     * and clamped to supported limits.
     */
    VkExtent2D chooseSwapExtent(
        const VkSurfaceCapabilitiesKHR&,
        GLFWwindow*
    );

    /**
     * @brief Creates image views for all swapchain images.
     */
    void createImageViews();

public:

        /**
     * @brief Interface for contributing to swapchain creation parameters.
     *
     * Implementations may modify the VkSwapchainCreateInfoKHR structure
     * before the swapchain is created, allowing engine subsystems to
     * customize swapchain behavior.
     */
    struct ISwapchainConfigProvider {
        virtual ~ISwapchainConfigProvider() = default;

        /**
         * @param info Swapchain creation info to be modified.
         * @param support Queried swapchain support details.
         */
        virtual void contribute(
            VkSwapchainCreateInfoKHR& info,
            const SwapchainSupportDetails& support
        ) = 0;
    };

private:
    /**
     * @brief Creates or recreates the Vulkan swapchain.
     *
     * This function performs swapchain creation using the provided parameters,
     * applies modifications from configuration providers, validates values
     * against surface capabilities, and retrieves swapchain images.
     *
     * If an old swapchain is provided, it is destroyed after the new one
     * is successfully created.
     */
    void createSwapchainInternal(
        VkSurfaceKHR surface,
        const QueueFamilyIndices& queueFamilies,
        VkSurfaceFormatKHR surfaceFormat,
        VkPresentModeKHR presentMode,
        VkExtent2D extent,
        const SwapchainSupportDetails& swapChainSupport,
        const std::vector<ISwapchainConfigProvider*>& swapchainProviders,
        VkSwapchainKHR oldSwapchain
    );

public:
    /**
     * @brief Constructs and initializes the swapchain.
     *
     * This constructor selects swapchain parameters, creates the swapchain,
     * retrieves swapchain images, and creates image views.
     *
     * @param device Logical Vulkan device.
     * @param queueFamilies Queue family indices used for sharing mode decisions.
     * @param swapchainSupportDetails Queried surface capabilities and formats.
     * @param surface Vulkan surface associated with the window.
     * @param window GLFW window used to determine framebuffer size.
     * @param swapchainProviders Optional contributors to swapchain configuration.
     */
    SwapchainManager(
        VkDevice device,
        const QueueFamilyIndices& queueFamilies,
        const SwapchainSupportDetails& swapchainSupportDetails,
        VkSurfaceKHR surface,
        GLFWwindow* window,
        const std::vector<ISwapchainConfigProvider*>& swapchainProviders
    );

    /**
     * @brief Destroys the swapchain and all associated image views.
     */
    ~SwapchainManager();

    /**
     * @brief Recreates the swapchain.
     *
     * Typically called in response to window resize events or
     * surface invalidation.
     *
     * This function destroys existing image views, recreates the swapchain,
     * and rebuilds image views using updated surface capabilities.
     */
    void recreate(
        const QueueFamilyIndices& queueFamilies,
        const SwapchainSupportDetails& swapchainSupportDetails,
        VkSurfaceKHR surface,
        GLFWwindow* window,
        const std::vector<ISwapchainConfigProvider*>& swapchainProviders
    );

    const VkSwapchainKHR getSwapchain() const { return this->swapchain; }
    const VkFormat getImageFormat() const { return this->swapchainImageFormat; }
    const VkExtent2D getExtent() const { return this->swapchainExtent; }
    const std::vector<VkImage>& getImages() const { return this->swapchainImages; }
    const std::vector<VkImageView>& getImageViews() const { return this->swapchainImageViews; }
};