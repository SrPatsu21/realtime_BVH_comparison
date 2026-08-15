#include "RenderPass.hpp"

#include <algorithm>
#include <stdexcept>

// ================================================================
// AttachmentDesc
// ================================================================

RenderPass::AttachmentDesc::AttachmentDesc(
    VkFormat format,
    VkSampleCountFlagBits samples,
    VkAttachmentLoadOp loadOp,
    VkAttachmentStoreOp storeOp,
    VkImageLayout initialLayout,
    VkImageLayout finalLayout
):
    format(format),
    samples(samples),
    loadOp(loadOp),
    storeOp(storeOp),
    initialLayout(initialLayout),
    finalLayout(finalLayout)
{
}

// ================================================================
// RenderPass
// ================================================================

RenderPass::RenderPass(
    VkDevice device,
    VkFormat swapchainImageFormat,
    VkSampleCountFlagBits msaaSamples,
    VkFormat depthFormat,
    const Config::ConfigTable& config,
    const std::vector<IRenderPassProvider*>& providers
)
    : device(device)
{
    Description description = buildDescription(
        swapchainImageFormat,
        msaaSamples,
        depthFormat,
        config.render.mode
    );

    addDefaultDependency(description);

    applyProviders(
        description,
        providers
    );

    deduplicateDependencies(
        description.dependencies
    );

    validate(description);

    renderPass = createVkRenderPass(
        device,
        description
    );
}

RenderPass::~RenderPass()
{
    if (renderPass != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(
            device,
            renderPass,
            nullptr
        );

        renderPass = VK_NULL_HANDLE;
    }
}

// ================================================================
// Render mode builders
// ================================================================

RenderPass::Description RenderPass::buildDescription(
    VkFormat swapchainImageFormat,
    VkSampleCountFlagBits msaaSamples,
    VkFormat depthFormat,
    Config::RenderMode renderMode
)
{
    switch (renderMode)
    {
        case Config::RenderMode::Forward:
            return buildForward(
                swapchainImageFormat,
                msaaSamples,
                depthFormat
            );

        case Config::RenderMode::GeometryGbuffer:
            return buildGeometryGbuffer(
                msaaSamples,
                depthFormat
            );

        case Config::RenderMode::RayTracing:
            return buildRayTracing(
                swapchainImageFormat,
                msaaSamples,
                depthFormat
            );

        default:
            throw std::runtime_error(
                "Unknown render mode"
            );
    }
}

// ================================================================
// Forward
// ================================================================

RenderPass::Description RenderPass::buildForward(
    VkFormat swapchainImageFormat,
    VkSampleCountFlagBits msaaSamples,
    VkFormat depthFormat
)
{
    Description description;

    const bool useMSAA = msaaSamples != VK_SAMPLE_COUNT_1_BIT;

    // ------------------------------------------------------------
    // Color
    // ------------------------------------------------------------

    const uint32_t colorAttachment =
        addColorAttachment(
            description,
            swapchainImageFormat,
            useMSAA
                ? msaaSamples
                : VK_SAMPLE_COUNT_1_BIT
        );

    // ------------------------------------------------------------
    // Depth
    // ------------------------------------------------------------

    const uint32_t depthAttachment =
        addDepthAttachment(
            description,
            depthFormat,
            msaaSamples
        );

    // ------------------------------------------------------------
    // Resolve
    // ------------------------------------------------------------

    std::optional<uint32_t> resolveAttachment;

    if (useMSAA)
    {
        resolveAttachment =
            addResolveAttachment(
                description,
                swapchainImageFormat
            );
    }

    // ------------------------------------------------------------
    // Subpass
    // ------------------------------------------------------------

    const uint32_t subpass =
        addSubpass(description);

    addColorAttachment(
        description,
        subpass,
        colorAttachment
    );

    setDepthAttachment(
        description,
        subpass,
        depthAttachment
    );

    if (resolveAttachment.has_value())
    {
        addResolveAttachment(
            description,
            subpass,
            *resolveAttachment
        );
    }

    return description;
}

// ================================================================
// Geometry / GBuffer
// ================================================================

RenderPass::Description RenderPass::buildGeometryGbuffer(
    VkSampleCountFlagBits msaaSamples,
    VkFormat depthFormat
)
{
    Description description;

    // ------------------------------------------------------------
    // GBuffer attachments
    // ------------------------------------------------------------

    const uint32_t position =
        addColorAttachment(
            description,
            VK_FORMAT_R16G16B16A16_SFLOAT,
            msaaSamples
        );

    const uint32_t normal =
        addColorAttachment(
            description,
            VK_FORMAT_R16G16B16A16_SFLOAT,
            msaaSamples
        );

    const uint32_t albedo =
        addColorAttachment(
            description,
            VK_FORMAT_R8G8B8A8_SRGB,
            msaaSamples
        );

    const uint32_t material =
        addColorAttachment(
            description,
            VK_FORMAT_R8G8B8A8_UNORM,
            msaaSamples
        );

    // ------------------------------------------------------------
    // Depth
    // ------------------------------------------------------------

    const uint32_t depth =
        addDepthAttachment(
            description,
            depthFormat,
            msaaSamples,
            VK_ATTACHMENT_STORE_OP_STORE
        );

    // ------------------------------------------------------------
    // Subpass
    // ------------------------------------------------------------

    const uint32_t subpass =
        addSubpass(description);

    addColorAttachment(
        description,
        subpass,
        position
    );

    addColorAttachment(
        description,
        subpass,
        normal
    );

    addColorAttachment(
        description,
        subpass,
        albedo
    );

    addColorAttachment(
        description,
        subpass,
        material
    );

    setDepthAttachment(
        description,
        subpass,
        depth
    );

    return description;
}

// ================================================================
// Ray Tracing
// ================================================================

RenderPass::Description RenderPass::buildRayTracing(
    VkFormat,
    VkSampleCountFlagBits,
    VkFormat
)
{
    /*
     * Ray tracing does not require a traditional VkRenderPass.
     *
     * Keep this explicit instead of silently creating an invalid
     * or meaningless render pass.
     */

    throw std::runtime_error(
        "RayTracing does not use a traditional VkRenderPass"
    );
}

// ================================================================
// Providers
// ================================================================

void RenderPass::applyProviders(
    Description& description,
    const std::vector<IRenderPassProvider*>& providers
)
{
    for (IRenderPassProvider* provider : providers)
    {
        if (provider == nullptr)
        {
            continue;
        }

        provider->contribute(description);
    }
}

// ================================================================
// Default dependency
// ================================================================

void RenderPass::addDefaultDependency(
    Description& description
)
{
    VkSubpassDependency dependency{};

    dependency.srcSubpass =
        VK_SUBPASS_EXTERNAL;

    dependency.dstSubpass =
        0;

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

    description.dependencies.push_back(
        dependency
    );
}

// ================================================================
// Attachment helpers
// ================================================================

uint32_t RenderPass::addColorAttachment(
    Description& description,
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

uint32_t RenderPass::addDepthAttachment(
    Description& description,
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

uint32_t RenderPass::addResolveAttachment(
    Description& description,
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
// Subpass helpers
// ================================================================

uint32_t RenderPass::addSubpass(
    Description& description
)
{
    const uint32_t index =
        static_cast<uint32_t>(
            description.subpasses.size()
        );

    description.subpasses.emplace_back();

    return index;
}

void RenderPass::addColorAttachment(
    Description& description,
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

void RenderPass::setDepthAttachment(
    Description& description,
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

    description.subpasses[subpassIndex].depthAttachment = attachmentIndex;
}

void RenderPass::addResolveAttachment(
    Description& description,
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
// Validation
// ================================================================

void RenderPass::validate(
    const Description& description
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

    validateAttachmentReferences(
        description
    );

    validateResolveAttachments(
        description
    );

    validateSubpasses(
        description
    );
}

void RenderPass::validateAttachmentReferences(
    const Description& description
)
{
    const size_t attachmentCount = description.attachments.size();

    for (const SubpassDesc& subpass : description.subpasses)
    {
        // --------------------------------------------------------
        // Color
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
        // Depth
        // --------------------------------------------------------

        if (subpass.depthAttachment.has_value())
        {
            if (*subpass.depthAttachment >=
                attachmentCount)
            {
                throw std::runtime_error(
                    "Invalid depth attachment index "
                    "in subpass"
                );
            }
        }

        // --------------------------------------------------------
        // Resolve
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
    }
}

void RenderPass::validateResolveAttachments(
    const Description& description
)
{
    for (const SubpassDesc& subpass : description.subpasses)
    {
        if (subpass.resolveAttachments.empty())
        {
            continue;
        }

        if (subpass.resolveAttachments.size() !=
            subpass.colorAttachments.size())
        {
            throw std::runtime_error(
                "Resolve attachment count must match "
                "color attachment count"
            );
        }

        for (size_t i = 0;
            i < subpass.colorAttachments.size();
            ++i
        )
        {
            const uint32_t colorIndex =
                subpass.colorAttachments[i];

            const uint32_t resolveIndex =
                subpass.resolveAttachments[i];

            const AttachmentDesc& color =
                description.attachments[colorIndex];

            const AttachmentDesc& resolve =
                description.attachments[resolveIndex];

            // ----------------------------------------------------
            // Resolve must be single sampled.
            // ----------------------------------------------------

            if (resolve.samples !=
                VK_SAMPLE_COUNT_1_BIT)
            {
                throw std::runtime_error(
                    "Resolve attachment must be "
                    "SAMPLE_COUNT_1"
                );
            }

            // ----------------------------------------------------
            // Source must be multisampled.
            // ----------------------------------------------------

            if (color.samples ==
                VK_SAMPLE_COUNT_1_BIT)
            {
                throw std::runtime_error(
                    "Resolve source must be multisampled"
                );
            }
        }
    }
}

void RenderPass::validateSubpasses(
    const Description& description
)
{
    for (const SubpassDesc& subpass : description.subpasses)
    {
        /*
         * Vulkan requires one resolve attachment for each
         * color attachment when resolve attachments are used.
         */
        if (!subpass.resolveAttachments.empty() &&
            subpass.resolveAttachments.size() !=
                subpass.colorAttachments.size())
        {
            throw std::runtime_error(
                "Subpass has an invalid number of "
                "resolve attachments"
            );
        }

        /*
         * A subpass without any rendering attachment is
         * probably a configuration error for this abstraction.
         */
        if (subpass.colorAttachments.empty() &&
            !subpass.depthAttachment.has_value())
        {
            throw std::runtime_error(
                "Subpass contains no color or depth attachment"
            );
        }
    }
}

// ================================================================
// Dependency helpers
// ================================================================

bool RenderPass::equalDependency(
    const VkSubpassDependency& lhs,
    const VkSubpassDependency& rhs
)
{
    return
        lhs.srcSubpass == rhs.srcSubpass &&
        lhs.dstSubpass == rhs.dstSubpass &&
        lhs.srcStageMask == rhs.srcStageMask &&
        lhs.dstStageMask == rhs.dstStageMask &&
        lhs.srcAccessMask == rhs.srcAccessMask &&
        lhs.dstAccessMask == rhs.dstAccessMask &&
        lhs.dependencyFlags == rhs.dependencyFlags;
}

void RenderPass::deduplicateDependencies(
    std::vector<VkSubpassDependency>& dependencies
)
{
    std::vector<VkSubpassDependency> unique;

    unique.reserve(dependencies.size());

    for (const VkSubpassDependency& dependency : dependencies)
    {
        const bool alreadyExists =
            std::any_of(
                unique.begin(),
                unique.end(),
                [&](const VkSubpassDependency& existing)
                {
                    return equalDependency(
                        existing,
                        dependency
                    );
                }
            );

        if (!alreadyExists)
        {
            unique.push_back(
                dependency
            );
        }
    }

    dependencies = std::move(unique);
}

// ================================================================
// Vulkan attachment conversion
// ================================================================

std::vector<VkAttachmentDescription>
RenderPass::createVkAttachments(
    const Description& description
)
{
    std::vector<VkAttachmentDescription> vkAttachments;

    vkAttachments.reserve(description.attachments.size());

    for (const AttachmentDesc& attachment : description.attachments)
    {
        VkAttachmentDescription vkAttachment{};

        vkAttachment.flags = 0;
        vkAttachment.format = attachment.format;
        vkAttachment.samples = attachment.samples;
        vkAttachment.loadOp = attachment.loadOp;
        vkAttachment.storeOp = attachment.storeOp;

        /*
         * This abstraction currently does not expose
         * stencil-specific load/store operations.
         */
        vkAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        vkAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        vkAttachment.initialLayout = attachment.initialLayout;
        vkAttachment.finalLayout = attachment.finalLayout;
        vkAttachments.push_back(vkAttachment);
    }

    return vkAttachments;
}

// ================================================================
// Vulkan subpass conversion
// ================================================================

std::vector<VkSubpassDescription>
RenderPass::createVkSubpasses(
    const Description& description,
    std::vector<std::vector<VkAttachmentReference>>& colorRefs,
    std::vector<std::vector<VkAttachmentReference>>& resolveRefs,
    std::vector<VkAttachmentReference>& depthRefs
)
{
    std::vector<VkSubpassDescription> vkSubpasses;

    const size_t subpassCount = description.subpasses.size();

    colorRefs.reserve(subpassCount);
    resolveRefs.reserve(subpassCount);
    depthRefs.reserve(subpassCount);

    vkSubpasses.reserve(subpassCount);

    for (const SubpassDesc& subpass : description.subpasses)
    {
        VkSubpassDescription vkSubpass{};
        vkSubpass.flags = 0;
        vkSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

        // --------------------------------------------------------
        // Color attachments
        // --------------------------------------------------------

        colorRefs.emplace_back();

        auto& currentColorRefs = colorRefs.back();

        currentColorRefs.reserve(subpass.colorAttachments.size());

        for (uint32_t index : subpass.colorAttachments)
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
            currentColorRefs.empty() ?
                nullptr :
                currentColorRefs.data();

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

            for (uint32_t index : subpass.resolveAttachments)
            {
                currentResolveRefs.push_back({
                    index,
                    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
                });
            }

            vkSubpass.pResolveAttachments = currentResolveRefs.data();
        }
        else
        {
            vkSubpass.pResolveAttachments = nullptr;
        }

        // --------------------------------------------------------
        // Other optional attachment arrays.
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
// VkRenderPassCreateInfo
// ================================================================

VkRenderPassCreateInfo RenderPass::createCreateInfo(
    const std::vector<VkAttachmentDescription>& attachments,
    const std::vector<VkSubpassDescription>& subpasses,
    const std::vector<VkSubpassDependency>& dependencies
)
{
    VkRenderPassCreateInfo info{};

    info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

    info.pNext = nullptr;

    info.flags = 0;

    info.attachmentCount =
        static_cast<uint32_t>(
            attachments.size()
        );

    info.pAttachments =
        attachments.empty()
            ? nullptr
            : attachments.data();

    info.subpassCount =
        static_cast<uint32_t>(
            subpasses.size()
        );

    info.pSubpasses =
        subpasses.empty()
            ? nullptr
            : subpasses.data();

    info.dependencyCount =
        static_cast<uint32_t>(
            dependencies.size()
        );

    info.pDependencies =
        dependencies.empty()
            ? nullptr
            : dependencies.data();

    return info;
}

// ================================================================
// Vulkan RenderPass creation
// ================================================================

VkRenderPass RenderPass::createVkRenderPass(
    VkDevice device,
    const Description& description
)
{
    // ------------------------------------------------------------
    // Attachments
    // ------------------------------------------------------------

    std::vector<VkAttachmentDescription>
        vkAttachments = createVkAttachments(description);

    // ------------------------------------------------------------
    // Subpasses
    //
    // These vectors must stay alive until vkCreateRenderPass()
    // has returned because VkSubpassDescription contains pointers
    // into them.
    // ------------------------------------------------------------

    std::vector<std::vector<VkAttachmentReference>> colorRefs;

    std::vector<std::vector<VkAttachmentReference>> resolveRefs;

    std::vector<VkAttachmentReference> depthRefs;

    std::vector<VkSubpassDescription> vkSubpasses =
            createVkSubpasses(
                description,
                colorRefs,
                resolveRefs,
                depthRefs
            );

    // ------------------------------------------------------------
    // CreateInfo
    // ------------------------------------------------------------

    VkRenderPassCreateInfo info =
        createCreateInfo(
            vkAttachments,
            vkSubpasses,
            description.dependencies
        );

    // ------------------------------------------------------------
    // Vulkan object
    // ------------------------------------------------------------

    VkRenderPass renderPass = VK_NULL_HANDLE;

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