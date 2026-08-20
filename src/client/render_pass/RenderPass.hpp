#pragma once

#include "../CoreVulkan.hpp"
#include "../ConfigTable.hpp"

class RenderPass {
private:
    VkDevice device;
    VkRenderPass renderPass{VK_NULL_HANDLE};

public:
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

    struct SubpassDesc {
        std::vector<uint32_t> colorAttachments;
        std::optional<uint32_t> depthAttachment;
        std::vector<uint32_t> resolveAttachments;
    };

    struct Description {
        std::vector<AttachmentDesc> attachments;
        std::vector<SubpassDesc> subpasses;
        std::vector<VkSubpassDependency> dependencies;
    };

    struct IRenderPassProvider {
        virtual ~IRenderPassProvider() = default;

        virtual void contribute(
            Description& description
        ) = 0;
    };

public:
    RenderPass(
        VkDevice device,
        VkFormat swapchainImageFormat,
        VkSampleCountFlagBits msaaSamples,
        VkFormat depthFormat,
        const Config::ConfigTable& config,
        const std::vector<IRenderPassProvider*>& providers
    );

    ~RenderPass();

    VkRenderPass get() const noexcept { return renderPass; }

private:
    // ------------------------------------------------------------
    // Render pass types
    // ------------------------------------------------------------

    static Description buildForward(
        VkFormat swapchainImageFormat,
        VkSampleCountFlagBits msaaSamples,
        VkFormat depthFormat
    );

    static Description buildGeometryGBuffer(
        VkSampleCountFlagBits msaaSamples,
        VkFormat depthFormat
    );


    // ------------------------------------------------------------
    // Description construction
    // ------------------------------------------------------------

    static Description buildDescription(
        VkFormat swapchainImageFormat,
        VkSampleCountFlagBits msaaSamples,
        VkFormat depthFormat,
        Config::RenderMode renderMode
    );

    static inline void applyProviders(
        Description& description,
        const std::vector<IRenderPassProvider*>& providers
    );

    static inline void addDefaultDependency(
        Description& description
    );

    // ------------------------------------------------------------
    // Attachment helpers
    // ------------------------------------------------------------

    static inline uint32_t addColorAttachment(
        Description& description,
        VkFormat format,
        VkSampleCountFlagBits samples,
        VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_STORE
    );

    static inline uint32_t addDepthAttachment(
        Description& description,
        VkFormat format,
        VkSampleCountFlagBits samples,
        VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE
    );

    static inline uint32_t addResolveAttachment(
        Description& description,
        VkFormat format
    );

    // ------------------------------------------------------------
    // Subpass helpers
    // ------------------------------------------------------------

    static inline uint32_t addSubpass(
        Description& description
    );

    static inline void addColorAttachment(
        Description& description,
        uint32_t subpassIndex,
        uint32_t attachmentIndex
    );

    static void setDepthAttachment(
        Description& description,
        uint32_t subpassIndex,
        uint32_t attachmentIndex
    );

    static void addResolveAttachment(
        Description& description,
        uint32_t subpassIndex,
        uint32_t attachmentIndex
    );

    // ------------------------------------------------------------
    // Validation
    // ------------------------------------------------------------

    static void validate(
        const Description& description
    );

    static void validateAttachmentReferences(
        const Description& description
    );

    static void validateResolveAttachments(
        const Description& description
    );

    static void validateSubpasses(
        const Description& description
    );

    // ------------------------------------------------------------
    // Dependency helpers
    // ------------------------------------------------------------

    static inline bool equalDependency(
        const VkSubpassDependency& lhs,
        const VkSubpassDependency& rhs
    );

    static void deduplicateDependencies(
        std::vector<VkSubpassDependency>& dependencies
    );

    // ------------------------------------------------------------
    // Vulkan conversion
    // ------------------------------------------------------------

    static std::vector<VkAttachmentDescription> createVkAttachments(
        const Description& description
    );

    static std::vector<VkSubpassDescription> createVkSubpasses(
        const Description& description,
        std::vector<std::vector<VkAttachmentReference>>& colorRefs,
        std::vector<std::vector<VkAttachmentReference>>& resolveRefs,
        std::vector<VkAttachmentReference>& depthRefs
    );

    static VkRenderPassCreateInfo createCreateInfo(
        const std::vector<VkAttachmentDescription>& attachments,
        const std::vector<VkSubpassDescription>& subpasses,
        const std::vector<VkSubpassDependency>& dependencies
    );

    static VkRenderPass createVkRenderPass(
        VkDevice device,
        const Description& description
    );
};