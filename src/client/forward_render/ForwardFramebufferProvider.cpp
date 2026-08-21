#include "ForwardFramebufferProvider.hpp"

void ForwardFramebufferProvider::build(
    const ForwardAttachments& attachments,
    const std::vector<VkImageView>& swapchainImageViews,
    std::size_t swapchainImageViewsSize,
    VkSampleCountFlagBits msaaSamples,
    std::vector<std::vector<VkImageView>>& attachmentsVector
)
{
    #ifndef NDEBUG
        validateAttachments(
            attachments,
            msaaSamples
        );
    #endif

    attachmentsVector.reserve(swapchainImageViewsSize);

    for (size_t i = 0; i < swapchainImageViewsSize; i++)
    {
        #ifndef NDEBUG
        if (swapchainImageViews[i] == VK_NULL_HANDLE)
        {
            throw std::runtime_error(
                "ForwardFramebufferProvider: invalid swapchain image view"
            );
        }
        #endif

            /*
                Forward without MSAA:

                    attachment 0 -> swapchain
                    attachment 1 -> depth

                Forward with MSAA:

                    attachment 0 -> MSAA color
                    attachment 1 -> MSAA depth
                    attachment 2 -> swapchain resolve
            */

            if (msaaSamples == VK_SAMPLE_COUNT_1_BIT)
            {
                attachmentsVector.push_back(
                    {
                        swapchainImageViews[i],
                        attachments.depth
                    }
                );
            }else
            {
                attachmentsVector.push_back(
                    {
                        attachments.color,
                        attachments.depth,
                        swapchainImageViews[i]
                    }
                );
            }
    }
    return;
}

void ForwardFramebufferProvider::validateAttachments(
    const ForwardAttachments& attachments,
    VkSampleCountFlagBits msaaSamples
)
{
    if (attachments.depth == VK_NULL_HANDLE)
    {
        throw std::runtime_error(
            "Forward depth image view is invalid"
        );
    }

    if (msaaSamples != VK_SAMPLE_COUNT_1_BIT &&
        attachments.color == VK_NULL_HANDLE)
    {
        throw std::runtime_error(
            "Forward MSAA color image view is invalid"
        );
    }
}
