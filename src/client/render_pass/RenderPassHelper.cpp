#include "RenderPassHelper.hpp"

#include <algorithm>
#include <stdexcept>

// ================================================================
// Attachments
// ================================================================

uint32_t RenderPassHelper::addColorAttachment(
    RenderPassManager::Description& description,
    VkFormat format,
    VkSampleCountFlagBits samples,
    VkAttachmentStoreOp storeOp
)
{
    const uint32_t index =
        static_cast<uint32_t>(
            description.attachments.size()
        );

    description.attachments.emplace_back(
        format,
        samples,
        VK_ATTACHMENT_LOAD_OP_CLEAR,
        storeOp,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    );

    return index;
}


uint32_t RenderPassHelper::addDepthAttachment(
    RenderPassManager::Description& description,
    VkFormat format,
    VkSampleCountFlagBits samples,
    VkAttachmentStoreOp storeOp
)
{
    const uint32_t index =
        static_cast<uint32_t>(
            description.attachments.size()
        );

    description.attachments.emplace_back(
        format,
        samples,
        VK_ATTACHMENT_LOAD_OP_CLEAR,
        storeOp,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    );

    return index;
}


uint32_t RenderPassHelper::addResolveAttachment(
    RenderPassManager::Description& description,
    VkFormat format
)
{
    const uint32_t index =
        static_cast<uint32_t>(
            description.attachments.size()
        );

    description.attachments.emplace_back(
        format,
        VK_SAMPLE_COUNT_1_BIT,
        VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        VK_ATTACHMENT_STORE_OP_STORE,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    );

    return index;
}


// ================================================================
// Subpass
// ================================================================

uint32_t RenderPassHelper::addSubpass(
    RenderPassManager::Description& description
)
{
    const uint32_t index =
        static_cast<uint32_t>(
            description.subpasses.size()
        );

    description.subpasses.emplace_back();

    return index;
}


void RenderPassHelper::addColorAttachment(
    RenderPassManager::Description& description,
    uint32_t subpassIndex,
    uint32_t attachmentIndex
)
{
    if (subpassIndex >= description.subpasses.size())
    {
        throw std::runtime_error(
            "Invalid subpass index while adding "
            "color attachment"
        );
    }

    if (attachmentIndex >= description.attachments.size())
    {
        throw std::runtime_error(
            "Invalid attachment index while adding "
            "color attachment"
        );
    }

    description.subpasses[subpassIndex]
        .colorAttachments
        .push_back(attachmentIndex);
}


void RenderPassHelper::setDepthAttachment(
    RenderPassManager::Description& description,
    uint32_t subpassIndex,
    uint32_t attachmentIndex
)
{
    if (subpassIndex >= description.subpasses.size())
    {
        throw std::runtime_error(
            "Invalid subpass index while setting "
            "depth attachment"
        );
    }

    if (attachmentIndex >= description.attachments.size())
    {
        throw std::runtime_error(
            "Invalid attachment index while setting "
            "depth attachment"
        );
    }

    description.subpasses[subpassIndex]
        .depthAttachment = attachmentIndex;
}


void RenderPassHelper::addResolveAttachment(
    RenderPassManager::Description& description,
    uint32_t subpassIndex,
    uint32_t attachmentIndex
)
{
    if (subpassIndex >= description.subpasses.size())
    {
        throw std::runtime_error(
            "Invalid subpass index while adding "
            "resolve attachment"
        );
    }

    if (attachmentIndex >= description.attachments.size())
    {
        throw std::runtime_error(
            "Invalid attachment index while adding "
            "resolve attachment"
        );
    }

    description.subpasses[subpassIndex]
        .resolveAttachments
        .push_back(attachmentIndex);
}


// ================================================================
// Dependencies
// ================================================================

void RenderPassHelper::addDependency(
    RenderPassManager::Description& description,
    const VkSubpassDependency& dependency
)
{
    description.dependencies.push_back(
        dependency
    );
}


void RenderPassHelper::addExternalDependency(
    RenderPassManager::Description& description,
    uint32_t dstSubpass
)
{
    VkSubpassDependency dependency{};

    dependency.srcSubpass =
        VK_SUBPASS_EXTERNAL;

    dependency.dstSubpass =
        dstSubpass;

    dependency.srcStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
        VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;

    dependency.srcAccessMask =
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    dependency.dstStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
        VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;

    dependency.dstAccessMask =
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    dependency.dependencyFlags = 0;

    addDependency(
        description,
        dependency
    );
}


// ================================================================
// Validation
// ================================================================

void RenderPassHelper::validate(
    const RenderPassManager::Description& description
)
{
    if (description.attachments.empty())
    {
        throw std::runtime_error(
            "Render pass contains no attachments"
        );
    }

    if (description.subpasses.empty())
    {
        throw std::runtime_error(
            "Render pass contains no subpasses"
        );
    }

    const size_t attachmentCount =
        description.attachments.size();

    for (const auto& subpass : description.subpasses)
    {
        // --------------------------------------------------------
        // Color attachments
        // --------------------------------------------------------

        for (uint32_t index : subpass.colorAttachments)
        {
            if (index >= attachmentCount)
            {
                throw std::runtime_error(
                    "Invalid color attachment index "
                    "in subpass"
                );
            }
        }

        // --------------------------------------------------------
        // Depth attachment
        // --------------------------------------------------------

        if (subpass.depthAttachment.has_value())
        {
            if (*subpass.depthAttachment >= attachmentCount)
            {
                throw std::runtime_error(
                    "Invalid depth attachment index "
                    "in subpass"
                );
            }
        }

        // --------------------------------------------------------
        // Resolve attachments
        // --------------------------------------------------------

        for (uint32_t index : subpass.resolveAttachments)
        {
            if (index >= attachmentCount)
            {
                throw std::runtime_error(
                    "Invalid resolve attachment index "
                    "in subpass"
                );
            }
        }

        // --------------------------------------------------------
        // Resolve count
        // --------------------------------------------------------

        if (!subpass.resolveAttachments.empty())
        {
            if (
                subpass.resolveAttachments.size() !=
                subpass.colorAttachments.size()
            )
            {
                throw std::runtime_error(
                    "Resolve attachment count must match "
                    "color attachment count"
                );
            }

            for (size_t i = 0;
                 i < subpass.colorAttachments.size();
                 ++i)
            {
                const uint32_t colorIndex =
                    subpass.colorAttachments[i];

                const uint32_t resolveIndex =
                    subpass.resolveAttachments[i];

                const auto& color =
                    description.attachments[colorIndex];

                const auto& resolve =
                    description.attachments[resolveIndex];

                // Resolve must be single sampled.
                if (
                    resolve.samples !=
                    VK_SAMPLE_COUNT_1_BIT
                )
                {
                    throw std::runtime_error(
                        "Resolve attachment must be "
                        "SAMPLE_COUNT_1"
                    );
                }

                // Source must be multisampled.
                if (
                    color.samples ==
                    VK_SAMPLE_COUNT_1_BIT
                )
                {
                    throw std::runtime_error(
                        "Resolve source must be "
                        "multiSampled"
                    );
                }
            }
        }

        // --------------------------------------------------------
        // Subpass must contain something
        // --------------------------------------------------------

        if (
            subpass.colorAttachments.empty() &&
            !subpass.depthAttachment.has_value()
        )
        {
            throw std::runtime_error(
                "Subpass contains no color or depth attachment"
            );
        }
    }
}


// ================================================================
// Vulkan attachment conversion
// ================================================================

std::vector<VkAttachmentDescription>
RenderPassHelper::createAttachments(
    const RenderPassManager::Description& description
)
{
    std::vector<VkAttachmentDescription>
        vkAttachments;

    vkAttachments.reserve(
        description.attachments.size()
    );

    for (
        const auto& attachment :
        description.attachments
    )
    {
        VkAttachmentDescription vkAttachment{};

        vkAttachment.flags = 0;
        vkAttachment.format = attachment.format;
        vkAttachment.samples = attachment.samples;
        vkAttachment.loadOp = attachment.loadOp;
        vkAttachment.storeOp = attachment.storeOp;

        // This abstraction currently does not expose
        // stencil-specific load/store operations.
        vkAttachment.stencilLoadOp =
            VK_ATTACHMENT_LOAD_OP_DONT_CARE;

        vkAttachment.stencilStoreOp =
            VK_ATTACHMENT_STORE_OP_DONT_CARE;

        vkAttachment.initialLayout =
            attachment.initialLayout;

        vkAttachment.finalLayout =
            attachment.finalLayout;

        vkAttachments.push_back(
            vkAttachment
        );
    }

    return vkAttachments;
}


// ================================================================
// Vulkan subpass conversion
// ================================================================

std::vector<VkSubpassDescription>
RenderPassHelper::createSubpasses(
    const RenderPassManager::Description& description,
    std::vector<std::vector<VkAttachmentReference>>& colorRefs,
    std::vector<std::vector<VkAttachmentReference>>& resolveRefs,
    std::vector<VkAttachmentReference>& depthRefs
)
{
    std::vector<VkSubpassDescription>
        vkSubpasses;

    const size_t subpassCount =
        description.subpasses.size();

    colorRefs.reserve(
        subpassCount
    );

    resolveRefs.reserve(
        subpassCount
    );

    depthRefs.reserve(
        subpassCount
    );

    vkSubpasses.reserve(
        subpassCount
    );

    for (const auto& subpass : description.subpasses)
    {
        VkSubpassDescription vkSubpass{};

        vkSubpass.flags = 0;

        vkSubpass.pipelineBindPoint =
            VK_PIPELINE_BIND_POINT_GRAPHICS;

        // --------------------------------------------------------
        // Color attachments
        // --------------------------------------------------------

        colorRefs.emplace_back();

        auto& currentColorRefs =
            colorRefs.back();

        currentColorRefs.reserve(
            subpass.colorAttachments.size()
        );

        for (uint32_t index :
             subpass.colorAttachments)
        {
            currentColorRefs.push_back({
                index,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
            });
        }

        vkSubpass.colorAttachmentCount =
            static_cast<uint32_t>(
                currentColorRefs.size()
            );

        vkSubpass.pColorAttachments =
            currentColorRefs.empty()
                ? nullptr
                : currentColorRefs.data();

        // --------------------------------------------------------
        // Depth attachment
        // --------------------------------------------------------

        if (subpass.depthAttachment.has_value())
        {
            depthRefs.push_back({
                *subpass.depthAttachment,
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
            });

            vkSubpass.pDepthStencilAttachment =
                &depthRefs.back();
        }
        else
        {
            vkSubpass.pDepthStencilAttachment =
                nullptr;
        }

        // --------------------------------------------------------
        // Resolve attachments
        // --------------------------------------------------------

        if (!subpass.resolveAttachments.empty())
        {
            resolveRefs.emplace_back();

            auto& currentResolveRefs =
                resolveRefs.back();

            currentResolveRefs.reserve(
                subpass.resolveAttachments.size()
            );

            for (uint32_t index :
                 subpass.resolveAttachments)
            {
                currentResolveRefs.push_back({
                    index,
                    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
                });
            }

            vkSubpass.pResolveAttachments =
                currentResolveRefs.data();
        }
        else
        {
            vkSubpass.pResolveAttachments =
                nullptr;
        }

        // --------------------------------------------------------
        // Other optional attachment arrays
        // --------------------------------------------------------

        vkSubpass.inputAttachmentCount = 0;
        vkSubpass.pInputAttachments = nullptr;

        vkSubpass.preserveAttachmentCount = 0;
        vkSubpass.pPreserveAttachments = nullptr;

        vkSubpasses.push_back(
            vkSubpass
        );
    }

    return vkSubpasses;
}


// ================================================================
// Vulkan RenderPass creation
// ================================================================

VkRenderPass RenderPassHelper::create(
    VkDevice device,
    const RenderPassManager::Description& description
)
{
    // ------------------------------------------------------------
    // Attachments
    // ------------------------------------------------------------

    std::vector<VkAttachmentDescription>
        vkAttachments =
            createAttachments(description);

    // ------------------------------------------------------------
    // Subpasses
    //
    // These vectors must remain alive until
    // vkCreateRenderPass() returns because
    // VkSubpassDescription contains pointers into them.
    // ------------------------------------------------------------

    std::vector<std::vector<VkAttachmentReference>>
        colorRefs;

    std::vector<std::vector<VkAttachmentReference>>
        resolveRefs;

    std::vector<VkAttachmentReference>
        depthRefs;

    std::vector<VkSubpassDescription>
        vkSubpasses =
            createSubpasses(
                description,
                colorRefs,
                resolveRefs,
                depthRefs
            );

    // ------------------------------------------------------------
    // CreateInfo
    // ------------------------------------------------------------

    VkRenderPassCreateInfo info{};

    info.sType =
        VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

    info.pNext = nullptr;
    info.flags = 0;

    info.attachmentCount =
        static_cast<uint32_t>(
            vkAttachments.size()
        );

    info.pAttachments =
        vkAttachments.empty()
            ? nullptr
            : vkAttachments.data();

    info.subpassCount =
        static_cast<uint32_t>(
            vkSubpasses.size()
        );

    info.pSubpasses =
        vkSubpasses.empty()
            ? nullptr
            : vkSubpasses.data();

    info.dependencyCount =
        static_cast<uint32_t>(
            description.dependencies.size()
        );

    info.pDependencies =
        description.dependencies.empty()
            ? nullptr
            : description.dependencies.data();

    // ------------------------------------------------------------
    // Vulkan object
    // ------------------------------------------------------------

    VkRenderPass renderPass =
        VK_NULL_HANDLE;

    const VkResult result =
        vkCreateRenderPass(
            device,
            &info,
            nullptr,
            &renderPass
        );

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error(
            "Failed to create VkRenderPass"
        );
    }

    return renderPass;
}