#pragma once

#include "../CoreVulkan.hpp"

#include <optional>
#include <vector>

class RenderPassManager
{
public:

    // ============================================================
    // Attachment
    // ============================================================

    struct AttachmentDesc
    {
        VkFormat format;
        VkSampleCountFlagBits samples;
        VkAttachmentLoadOp loadOp;
        VkAttachmentStoreOp storeOp;
        VkImageLayout initialLayout;
        VkImageLayout finalLayout;

        AttachmentDesc(
            VkFormat format,
            VkSampleCountFlagBits samples,
            VkAttachmentLoadOp loadOp,
            VkAttachmentStoreOp storeOp,
            VkImageLayout initialLayout,
            VkImageLayout finalLayout
        );
    };


    // ============================================================
    // Subpass
    // ============================================================

    struct SubpassDesc
    {
        std::vector<uint32_t> colorAttachments;

        std::optional<uint32_t> depthAttachment;

        std::vector<uint32_t> resolveAttachments;
    };


    // ============================================================
    // Description
    // ============================================================

    struct Description
    {
        std::vector<AttachmentDesc> attachments;

        std::vector<SubpassDesc> subpasses;

        std::vector<VkSubpassDependency> dependencies;
    };


public:

    // ============================================================
    // Construction
    // ============================================================

    RenderPassManager(
        VkDevice device,
        Description description
    );


    ~RenderPassManager();


    RenderPassManager(
        const RenderPassManager&
    ) = delete;


    RenderPassManager& operator=(
        const RenderPassManager&
    ) = delete;


    RenderPassManager(
        RenderPassManager&&
    ) = delete;


    RenderPassManager& operator=(
        RenderPassManager&&
    ) = delete;


    // ============================================================
    // Access
    // ============================================================

    VkRenderPass get() const noexcept
    {
        return renderPass;
    }


private:

    VkDevice device;

    VkRenderPass renderPass{
        VK_NULL_HANDLE
    };
};