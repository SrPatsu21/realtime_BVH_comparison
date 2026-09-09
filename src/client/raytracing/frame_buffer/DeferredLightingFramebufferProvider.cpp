#include "DeferredLightingFramebufferProvider.hpp"

#include <stdexcept>

void DeferredLightingFramebufferProvider::build(
    VkImageView lightingView,
    std::size_t framebufferCount,
    std::vector<std::vector<VkImageView>>& attachmentsVector
)
{
#ifndef NDEBUG
    if (lightingView == VK_NULL_HANDLE)
        throw std::runtime_error("DeferredLightingFramebufferProvider: invalid lighting image view");
#endif

    attachmentsVector.reserve(
        framebufferCount
    );

    for (std::size_t i = 0; i < framebufferCount; i++)
    {
        attachmentsVector.push_back({lightingView});
    }
}