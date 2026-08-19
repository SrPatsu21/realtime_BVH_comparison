#include "FramebufferManager.hpp"

#include <cassert>
#include <stdexcept>

FramebufferManager::FramebufferManager(
    VkDevice device,
    VkRenderPass renderPass,
    const std::vector<VkImageView>& swapchainImageViews,
    VkExtent2D swapchainExtent,
    const Config::ConfigTable& config,
    VkSampleCountFlagBits msaaSamples,
    const ForwardAttachments& forwardAttachments,
    const GBufferAttachments& gBufferAttachments
)
    : device(device)
{
    if (device == VK_NULL_HANDLE)
    {
        throw std::runtime_error(
            "FramebufferManager: invalid Vulkan device"
        );
    }

    if (renderPass == VK_NULL_HANDLE)
    {
        throw std::runtime_error(
            "FramebufferManager: invalid render pass"
        );
    }

    if (swapchainImageViews.empty())
    {
        throw std::runtime_error(
            "FramebufferManager: no swapchain image views"
        );
    }

    if (swapchainExtent.width == 0 ||
        swapchainExtent.height == 0)
    {
        throw std::runtime_error(
            "FramebufferManager: invalid framebuffer extent"
        );
    }

    framebuffers.resize(swapchainImageViews.size());

    for (size_t i = 0; i < swapchainImageViews.size(); ++i)
    {
        std::vector<VkImageView> attachments;

        switch (config.render.mode)
        {
            // ----------------------------------------------------
            // Forward
            // ----------------------------------------------------

            case Config::RenderMode::Forward:
            {
                attachments = buildForwardAttachments(
                    swapchainImageViews,
                    i,
                    forwardAttachments,
                    msaaSamples
                );

                break;
            }

            // ----------------------------------------------------
            // Geometry / GBuffer
            // ----------------------------------------------------

            case Config::RenderMode::GeometryGBuffer:
            {
                attachments = buildGBufferAttachments(
                    gBufferAttachments
                );

                break;
            }

            default:
            {
                throw std::runtime_error(
                    "FramebufferManager: unknown render mode"
                );
            }
        }

        if (attachments.empty())
        {
            throw std::runtime_error(
                "FramebufferManager: empty attachment list"
            );
        }

        VkFramebufferCreateInfo framebufferInfo{};

        framebufferInfo.sType =
            VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;

        framebufferInfo.renderPass = renderPass;

        framebufferInfo.attachmentCount =
            static_cast<uint32_t>(attachments.size());

        framebufferInfo.pAttachments =
            attachments.data();

        framebufferInfo.width =
            swapchainExtent.width;

        framebufferInfo.height =
            swapchainExtent.height;

        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(
                device,
                &framebufferInfo,
                nullptr,
                &framebuffers[i]
            ) != VK_SUCCESS)
        {
            throw std::runtime_error(
                "FramebufferManager: "
                "failed to create framebuffer"
            );
        }
    }
}


// ================================================================
// Forward
// ================================================================

std::vector<VkImageView>
FramebufferManager::buildForwardAttachments(
    const std::vector<VkImageView>& swapchainImageViews,
    size_t index,
    const ForwardAttachments& attachments,
    VkSampleCountFlagBits msaaSamples
)
{
    validateForwardAttachments(
        attachments,
        msaaSamples
    );

    if (swapchainImageViews[index] == VK_NULL_HANDLE)
    {
        throw std::runtime_error(
            "FramebufferManager: invalid swapchain image view"
        );
    }

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
        return {
            swapchainImageViews[index],
            attachments.depth
        };
    }

    return {
        attachments.color,
        attachments.depth,
        swapchainImageViews[index]
    };
}


// ================================================================
// GBuffer
// ================================================================

std::vector<VkImageView>
FramebufferManager::buildGBufferAttachments(
    const GBufferAttachments& attachments
)
{
    validateGBufferAttachments(
        attachments
    );

    /*
        GBuffer attachment order MUST match
        RenderPass::buildGeometryGBuffer():

            attachment 0 -> position
            attachment 1 -> albedo
            attachment 2 -> normal
            attachment 3 -> material
            attachment 4 -> depth
    */

    return {
        attachments.position,
        attachments.albedo,
        attachments.normal,
        attachments.material,
        attachments.depth
    };
}


// ================================================================
// Validation
// ================================================================

void FramebufferManager::validateForwardAttachments(
    const ForwardAttachments& attachments,
    VkSampleCountFlagBits msaaSamples
)
{
    if (attachments.depth == VK_NULL_HANDLE)
    {
        throw std::runtime_error(
            "FramebufferManager: "
            "Forward depth image view is invalid"
        );
    }

    /*
        The MSAA color attachment only exists when
        the selected sample count is greater than 1.
    */

    if (msaaSamples != VK_SAMPLE_COUNT_1_BIT &&
        attachments.color == VK_NULL_HANDLE)
    {
        throw std::runtime_error(
            "FramebufferManager: "
            "Forward MSAA color image view is invalid"
        );
    }
}


void FramebufferManager::validateGBufferAttachments(
    const GBufferAttachments& attachments
)
{
    if (attachments.position == VK_NULL_HANDLE)
    {
        throw std::runtime_error(
            "FramebufferManager: "
            "GBuffer position image view is invalid"
        );
    }

    if (attachments.albedo == VK_NULL_HANDLE)
    {
        throw std::runtime_error(
            "FramebufferManager: "
            "GBuffer albedo image view is invalid"
        );
    }

    if (attachments.normal == VK_NULL_HANDLE)
    {
        throw std::runtime_error(
            "FramebufferManager: "
            "GBuffer normal image view is invalid"
        );
    }

    if (attachments.material == VK_NULL_HANDLE)
    {
        throw std::runtime_error(
            "FramebufferManager: "
            "GBuffer material image view is invalid"
        );
    }

    if (attachments.depth == VK_NULL_HANDLE)
    {
        throw std::runtime_error(
            "FramebufferManager: "
            "GBuffer depth image view is invalid"
        );
    }
}


// ================================================================
// Destructor
// ================================================================

FramebufferManager::~FramebufferManager()
{
    for (VkFramebuffer& framebuffer : framebuffers)
    {
        if (framebuffer != VK_NULL_HANDLE)
        {
            vkDestroyFramebuffer(
                device,
                framebuffer,
                nullptr
            );

            framebuffer = VK_NULL_HANDLE;
        }
    }

    framebuffers.clear();
}