#include "../../CoreVulkan.hpp"

class GBufferFramebufferProvider
{
public:

    struct GBufferAttachments
    {
        VkImageView position = VK_NULL_HANDLE;
        VkImageView albedo = VK_NULL_HANDLE;
        VkImageView normal = VK_NULL_HANDLE;
        VkImageView material = VK_NULL_HANDLE;
        VkImageView depth = VK_NULL_HANDLE;
    };

    static void build(
        const GBufferAttachments& attachments,
        std::size_t swapchainImageViewsSize,
        std::vector<std::vector<VkImageView>>& attachmentsVector
    );

    static void validateAttachments(
        const GBufferAttachments& attachments
    );
};
