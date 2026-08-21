#include "GBufferFramebufferProvider.hpp"

void GBufferFramebufferProvider::build(
    const GBufferAttachments& attachments,
    std::size_t swapchainImageViewsSize,
    std::vector<std::vector<VkImageView>>& attachmentsVector
)
{
    #ifndef NDEBUG
        validateAttachments(
            attachments
        );
    #endif

    attachmentsVector.reserve(swapchainImageViewsSize);

    for (size_t i = 0; i < swapchainImageViewsSize; i++)
    {
        attachmentsVector.push_back(
            {
                attachments.position,
                attachments.normal,
                attachments.albedo,
                attachments.material,
                attachments.depth
            }
        );
    }
    return;
}

void GBufferFramebufferProvider::validateAttachments(
    const GBufferAttachments& attachments
)
{
    if (attachments.position == VK_NULL_HANDLE)
    {
        throw std::runtime_error(
            "GBuffer position image view is invalid"
        );
    }

    if (attachments.albedo == VK_NULL_HANDLE)
    {
        throw std::runtime_error(
            "GBuffer albedo image view is invalid"
        );
    }

    if (attachments.normal == VK_NULL_HANDLE)
    {
        throw std::runtime_error(
            "GBuffer normal image view is invalid"
        );
    }

    if (attachments.material == VK_NULL_HANDLE)
    {
        throw std::runtime_error(
            "GBuffer material image view is invalid"
        );
    }

    if (attachments.depth == VK_NULL_HANDLE)
    {
        throw std::runtime_error(
            "GBuffer depth image view is invalid"
        );
    }
}
