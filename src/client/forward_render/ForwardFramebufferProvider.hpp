#include "../CoreVulkan.hpp"

class ForwardFramebufferProvider
{
public:

    struct ForwardAttachments
    {
        // MSAA color attachment.
        //
        // Required when msaaSamples != VK_SAMPLE_COUNT_1_BIT.
        // Ignored when msaaSamples == VK_SAMPLE_COUNT_1_BIT.
        VkImageView color = VK_NULL_HANDLE;

        // Depth attachment.
        VkImageView depth = VK_NULL_HANDLE;
    };

    static void build(
        const ForwardAttachments& attachments,
        const std::vector<VkImageView>& swapchainImageViews,
        std::size_t swapchainImageViewsSize,
        VkSampleCountFlagBits msaaSamples,
        std::vector<std::vector<VkImageView>>& attachmentsVector
    );

    static void validateAttachments(
        const ForwardAttachments& attachments,
        VkSampleCountFlagBits msaaSamples
    );
};
