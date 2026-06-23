#include "SwapchainManager.hpp"
#include <limits> // Necessary for std::numeric_limits
#include <algorithm> // Necessary for std::clamp
#include "../image/VulkanImageUtils.hpp"

SwapchainManager::SwapchainManager(
        VkDevice device,
        const QueueFamilyIndices& queueFamilies,
        const SwapchainSupportDetails& swapchainSupportDetails,
        VkSurfaceKHR surface,
        GLFWwindow* window,
        const std::vector<ISwapchainConfigProvider*>& swapchainProviders
) :
    device(device)
{
    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapchainSupportDetails.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapchainSupportDetails.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapchainSupportDetails.capabilities, window);

    this->swapchainImageFormat = surfaceFormat.format;
    this->swapchainExtent = extent;

    createSwapchainInternal(surface, queueFamilies, surfaceFormat, presentMode, extent, swapchainSupportDetails, swapchainProviders, VK_NULL_HANDLE);
    createImageViews();
}

SwapchainManager::~SwapchainManager() {
    if (!this->swapchainImageViews.empty()) {
        for (auto view : swapchainImageViews) {
            vkDestroyImageView(device, view, nullptr);
        }
        swapchainImageViews.clear();
    }
    if (this->swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(device, this->swapchain, nullptr);
        this->swapchain = VK_NULL_HANDLE; // reset
    }
}

VkSurfaceFormatKHR SwapchainManager::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR SwapchainManager::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    //TODO create a limiter to enable other modes
    // for (const auto& availablePresentMode : availablePresentModes) {
    //     if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
    //         return availablePresentMode;
    //     }
    // }
    // for (const auto& availablePresentMode : availablePresentModes) {
    //     if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
    //         return availablePresentMode;
    //     }
    // }
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D SwapchainManager::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

void SwapchainManager::createSwapchainInternal(
        VkSurfaceKHR surface,
        const QueueFamilyIndices& queueFamilies,
        VkSurfaceFormatKHR surfaceFormat,
        VkPresentModeKHR presentMode,
        VkExtent2D extent,
        const SwapchainSupportDetails& swapChainSupport,
        const std::vector<ISwapchainConfigProvider*>& swapchainProviders,
        VkSwapchainKHR oldSwapchain
) {
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = swapChainSupport.capabilities.minImageCount + 1;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    // swapChainSupport
    uint32_t queueFamilyIndices[] = {queueFamilies.graphicsFamily.value(), queueFamilies.presentFamily.value()};
    if (queueFamilies.graphicsFamily != queueFamilies.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0; // Optional
        createInfo.pQueueFamilyIndices = nullptr; // Optional
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = oldSwapchain;

    // mods
    for (auto* p : swapchainProviders)
        p->contribute(createInfo, swapChainSupport);

    // fix invalid minImageCount
    uint32_t minImgCount = swapChainSupport.capabilities.minImageCount;
    uint32_t maxImgCount = swapChainSupport.capabilities.maxImageCount;

    if (createInfo.minImageCount < minImgCount) {
        createInfo.minImageCount = minImgCount;
    }
    if (maxImgCount > 0 && createInfo.minImageCount > maxImgCount) {
        createInfo.minImageCount = maxImgCount;
    }

    // fix invalid imageUsage
    createInfo.imageUsage &= swapChainSupport.capabilities.supportedUsageFlags;

    // invalid compositeAlpha
    auto supported = swapChainSupport.capabilities.supportedCompositeAlpha;
    if (!(supported & createInfo.compositeAlpha)) {
        createInfo.compositeAlpha = static_cast<VkCompositeAlphaFlagBitsKHR>(supported & -supported);
    }

    // create
    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &this->swapchain) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }

    // if old delete
    if (oldSwapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(device, oldSwapchain, nullptr);
    }

    // resize and get swapchain image
    uint32_t count;
    vkGetSwapchainImagesKHR(device, this->swapchain, &count, nullptr);
    this->swapchainImages.resize(count);
    vkGetSwapchainImagesKHR(device, this->swapchain, &count, this->swapchainImages.data());
}

void SwapchainManager::createImageViews() {
    this->swapchainImageViews.resize(this->swapchainImages.size());

    for (size_t i = 0; i < this->swapchainImages.size(); i++) {
        swapchainImageViews[i] = createImageView(device, swapchainImages[i], swapchainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }
}
void SwapchainManager::recreate(
    const QueueFamilyIndices& queueFamilies,
    const SwapchainSupportDetails& swapchainSupportDetails,
    VkSurfaceKHR surface,
    GLFWwindow* window,
    const  std::vector<ISwapchainConfigProvider*>& swapchainProviders
) {
    // destroy ImageViews
    for (auto view : swapchainImageViews) {
        vkDestroyImageView(device, view, nullptr);
    }
    swapchainImageViews.clear();

    // details
    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapchainSupportDetails.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapchainSupportDetails.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapchainSupportDetails.capabilities, window);

    this->swapchainExtent = extent;
    this->swapchainImageFormat = surfaceFormat.format;

    // create new
    createSwapchainInternal(surface, queueFamilies, surfaceFormat, presentMode, extent, swapchainSupportDetails, swapchainProviders, swapchain);


    // recreate imageViews
    createImageViews();
}