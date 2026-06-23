#pragma once

#include "../CoreVulkan.hpp"

/**
 * @brief High-level manager for Vulkan render pass creation.
 *
 * RenderPass encapsulates the construction of a VkRenderPass using
 * a declarative description of attachments, subpasses and dependencies.
 *
 * The class provides:
 * - A validated default render pass layout (color + depth + resolve).
 * - Extension points via IRenderPassProvider for engine subsystems.
 * - Defensive validation against invalid attachment/subpass combinations.
 *
 * This design separates render pass topology (what exists)
 * from policy decisions (how subsystems extend it).
 *
 * In Vulkan, all of the rendering happens inside a VkRenderPass.
 * It is not possible to do rendering commands outside of a renderpass,
 * but it is possible to do Compute commands without them.
 *
 * A VkRenderPass is a Vulkan object that encapsulates the state needed
 * to setup the "target" for rendering, and the state of the images you will be rendering to.
 */
class RenderPass {
private:
    VkDevice device;
    VkRenderPass renderPass{VK_NULL_HANDLE};

public:
    /**
     * @brief High-level description of a render pass attachment.
     *
     * This struct is an engine-friendly representation that is later
     * converted into VkAttachmentDescription.
     *
     * It intentionally omits Vulkan-only noise (stencil ops, flags)
     * to keep render pass construction readable and composable.
     */
    struct AttachmentDesc {
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

    /**
     * @brief High-level description of a subpass.
     *
     * Attachments are referenced by index into the attachments array.
     *
     * Invariants enforced by RenderPass:
     * - All referenced indices must be valid.
     * - Resolve attachments must correspond to MSAA color attachments.
     * - Resolve attachments must be SAMPLE_COUNT_1.
     */
    struct SubpassDesc {
        std::vector<uint32_t> colorAttachments;
        std::optional<uint32_t> depthAttachment;
        std::vector<uint32_t> resolveAttachments;
    };

    /**
     * @brief Interface for extending render pass construction.
     *
     * Implementations may:
     * - Add new attachments.
     * - Add or modify subpasses.
     * - Add subpass dependencies.
     *
     * Providers are applied before validation and Vulkan conversion,
     * allowing complex render graphs to be composed incrementally.
     */
    struct IRenderPassProvider {
        virtual ~IRenderPassProvider() = default;

        /**
         * @param attachments Mutable list of attachment descriptions.
         * @param subpasses Mutable list of subpass descriptions.
         * @param dependencies Mutable list of subpass dependencies.
         */
        virtual void contribute(
            std::vector<AttachmentDesc>& attachments,
            std::vector<SubpassDesc>& subpasses,
            std::vector<VkSubpassDependency>& dependencies
        ) = 0;
    };

    /**
     * @brief Compares two subpass dependencies for structural equality.
     *
     * Used internally to detect redundant dependencies contributed
     * by multiple providers.
     */
    static bool equalDependency(
        const VkSubpassDependency& a,
        const VkSubpassDependency& b
    );

    /**
     * @brief Removes duplicated subpass dependencies.
     *
     * Dependencies are considered equal if all structural fields match.
     */
    static void deduplicateDependencies(
        std::vector<VkSubpassDependency>& deps
    );

    /**
     * @brief Constructs a render pass with optional provider extensions.
     *
     * The base render pass includes:
     * - MSAA color attachment
     * - Depth attachment
     * - Resolve attachment to swapchain image
     *
     * Providers are applied before validation and Vulkan object creation.
     *
     * @param device Logical Vulkan device.
     * @param swapchainImageFormat Format of the swapchain images.
     * @param msaaSamples Sample count used for MSAA color/depth attachments.
     * @param depthFormat Format of the depth attachment.
     * @param providers List of render pass extension providers.
     */
    RenderPass(
        VkDevice device,
        VkFormat swapchainImageFormat,
        VkSampleCountFlagBits msaaSamples,
        VkFormat depthFormat,
        const std::vector<IRenderPassProvider*> providers
    );

    /**
     * @brief Destroys the Vulkan render pass.
     */
    ~RenderPass();

    /**
     * @return Underlying VkRenderPass handle.
     */
    VkRenderPass get() const { return renderPass; }
};