#pragma once

#include "../CoreVulkan.hpp"
#include "RenderPassManager.hpp"

class RenderPassHelper
{
public:

    // ============================================================
    // Attachments
    // ============================================================

    static uint32_t addColorAttachment(
        RenderPassManager::Description& description,
        VkFormat format,
        VkSampleCountFlagBits samples,
        VkAttachmentStoreOp storeOp =
            VK_ATTACHMENT_STORE_OP_STORE
    );


    static uint32_t addDepthAttachment(
        RenderPassManager::Description& description,
        VkFormat format,
        VkSampleCountFlagBits samples,
        VkAttachmentStoreOp storeOp =
            VK_ATTACHMENT_STORE_OP_DONT_CARE
    );


    static uint32_t addResolveAttachment(
        RenderPassManager::Description& description,
        VkFormat format
    );


    // ============================================================
    // Subpass
    // ============================================================

    static uint32_t addSubpass(
        RenderPassManager::Description& description
    );


    static void addColorAttachment(
        RenderPassManager::Description& description,
        uint32_t subpassIndex,
        uint32_t attachmentIndex
    );


    static void setDepthAttachment(
        RenderPassManager::Description& description,
        uint32_t subpassIndex,
        uint32_t attachmentIndex
    );


    static void addResolveAttachment(
        RenderPassManager::Description& description,
        uint32_t subpassIndex,
        uint32_t attachmentIndex
    );


    // ============================================================
    // Dependencies
    // ============================================================

    static void addDependency(
        RenderPassManager::Description& description,
        const VkSubpassDependency& dependency
    );


    static void addExternalDependency(
        RenderPassManager::Description& description,
        uint32_t dstSubpass
    );


    // ============================================================
    // Validation
    // ============================================================

    static void validate(
        const RenderPassManager::Description& description
    );


    // ============================================================
    // Vulkan conversion
    // ============================================================

    static std::vector<VkAttachmentDescription>
    createAttachments(
        const RenderPassManager::Description& description
    );


    static std::vector<VkSubpassDescription>
    createSubpasses(
        const RenderPassManager::Description& description,
        std::vector<std::vector<VkAttachmentReference>>& colorRefs,
        std::vector<std::vector<VkAttachmentReference>>& resolveRefs,
        std::vector<VkAttachmentReference>& depthRefs
    );


    static VkRenderPass create(
        VkDevice device,
        const RenderPassManager::Description& description
    );
};
