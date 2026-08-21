#include "LightingFramebufferProvider.hpp"

void LightingFramebufferProvider::build(
    const std::vector<VkImageView>& swapchainImageViews,
    std::size_t swapchainImageViewsSize,
    std::vector<std::vector<VkImageView>>& attachmentsVector
)
{
    attachmentsVector.reserve(
        swapchainImageViewsSize
    );

    for (std::size_t i = 0; i < swapchainImageViewsSize; ++i)
    {
        #ifndef NDEBUG
        if (swapchainImageViews[i] == VK_NULL_HANDLE)
        {
            throw std::runtime_error(
                "LightingFramebufferProvider: invalid swapchain image view"
            );
        }
        #endif
        attachmentsVector.push_back(
            {
                swapchainImageViews[i]
            }
        );
    }
}