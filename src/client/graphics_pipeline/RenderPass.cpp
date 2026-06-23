#include "RenderPass.hpp"
#include <array>
#include <stdexcept>

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
{}

RenderPass::RenderPass(
    VkDevice device,
    VkFormat swapchainImageFormat,
    VkSampleCountFlagBits msaaSamples,
    VkFormat depthFormat,
    const std::vector<IRenderPassProvider*> providers
)
: device(device)
{
    std::vector<AttachmentDesc> attachments;
    bool useMSAA = msaaSamples != VK_SAMPLE_COUNT_1_BIT;
    if(useMSAA) {
        attachments.reserve(3);
    }else {
            attachments.reserve(2);
    }
    std::vector<SubpassDesc> subpasses;
    std::vector<VkSubpassDependency> dependencies;

    // Color (MSAA)
    if (useMSAA) {
        attachments.emplace_back(
            swapchainImageFormat,
            msaaSamples,
            VK_ATTACHMENT_LOAD_OP_CLEAR,
            VK_ATTACHMENT_STORE_OP_STORE,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        );
    } else {
        attachments.emplace_back(
            swapchainImageFormat,
            msaaSamples,
            VK_ATTACHMENT_LOAD_OP_CLEAR,
            VK_ATTACHMENT_STORE_OP_STORE,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
        );
    }

    // Depth
    attachments.emplace_back(
        depthFormat,
        msaaSamples,
        VK_ATTACHMENT_LOAD_OP_CLEAR,
        VK_ATTACHMENT_STORE_OP_DONT_CARE,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    );

    // Resolve
    if (useMSAA) {
        attachments.emplace_back(
            swapchainImageFormat,
            VK_SAMPLE_COUNT_1_BIT,
            VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            VK_ATTACHMENT_STORE_OP_STORE,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
        );
    }

    // Subpass base
    SubpassDesc mainSubpass{};
    mainSubpass.colorAttachments = {0};
    mainSubpass.depthAttachment = 1;
    if (useMSAA) {
        mainSubpass.resolveAttachments = {2};
    }


    subpasses.push_back(mainSubpass);

    // Dependency base
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies.push_back(dependency);

    // mods providers
    for (auto* p : providers) {
        p->contribute(attachments, subpasses, dependencies);
    }

// pos mod fix
    deduplicateDependencies(dependencies);

    auto attachmentCount = attachments.size();

    for (size_t si = 0; si < subpasses.size(); ++si) {
        const auto& sp = subpasses[si];

        for (auto idx : sp.colorAttachments) {
            if (idx >= attachmentCount)
                throw std::runtime_error("Invalid color attachment index in subpass");
        }

        if (sp.depthAttachment.has_value() && *sp.depthAttachment >= attachmentCount)
            throw std::runtime_error("Invalid depth attachment index in subpass");

        for (auto idx : sp.resolveAttachments) {
            if (idx >= attachmentCount)
                throw std::runtime_error("Invalid resolve attachment index in subpass");
        }
    }

    for (const auto& s : subpasses) {
        if (!s.resolveAttachments.empty()) {
            for (size_t i = 0; i < s.colorAttachments.size(); ++i) {
                auto colorIdx = s.colorAttachments[i];
                auto resolveIdx = s.resolveAttachments[i];

                //! no require sample anymore
                // if (attachments[colorIdx].samples == VK_SAMPLE_COUNT_1_BIT)
                //     throw std::runtime_error("Resolve used with non-MSAA color attachment");|

                if (attachments[resolveIdx].samples != VK_SAMPLE_COUNT_1_BIT)
                    throw std::runtime_error("Resolve attachment must be SAMPLE_COUNT_1");
            }
        }
        for (auto idx : s.colorAttachments) {
            if (attachments[idx].finalLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
                throw std::runtime_error("Color attachment ends in depth layout");
        }
    }

// convert to vulkan
    // attachments
    std::vector<VkAttachmentDescription> vkAttachments;
    vkAttachments.reserve(attachments.size());

    for (const auto& a : attachments) {
        VkAttachmentDescription desc{};
        desc.format = a.format;
        desc.samples = a.samples;
        desc.loadOp = a.loadOp;
        desc.storeOp = a.storeOp;
        desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        desc.initialLayout = a.initialLayout;
        desc.finalLayout = a.finalLayout;
        vkAttachments.emplace_back(desc);
    }

    std::vector<VkSubpassDescription> vkSubpasses;
    vkSubpasses.reserve(subpasses.size());
    std::vector<std::vector<VkAttachmentReference>> colorRefs;
    std::vector<std::vector<VkAttachmentReference>> resolveRefs;
    std::vector<VkAttachmentReference> depthRefs;
    colorRefs.reserve(subpasses.size());
    resolveRefs.reserve(subpasses.size());
    depthRefs.reserve(subpasses.size());


    for (const auto& s : subpasses) {
        VkSubpassDescription sp{};
        sp.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

        // color attachment ref
        colorRefs.emplace_back();
        for (auto idx : s.colorAttachments)
            colorRefs.back().push_back({idx, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});

        sp.colorAttachmentCount = static_cast<uint32_t>(colorRefs.back().size());
        sp.pColorAttachments = colorRefs.back().data();

        // depth attachment ref
        if (s.depthAttachment.has_value()) {
            depthRefs.push_back({*s.depthAttachment, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL});
            sp.pDepthStencilAttachment = &depthRefs.back();
        }

        // color attachment resolve ref
        if (!s.resolveAttachments.empty()) {
            resolveRefs.emplace_back();
            for (auto idx : s.resolveAttachments)
                resolveRefs.back().push_back({idx, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
            sp.pResolveAttachments = resolveRefs.back().data();
        }

        vkSubpasses.emplace_back(sp);
    }

    // create render pass
    VkRenderPassCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    info.attachmentCount = static_cast<uint32_t>(vkAttachments.size());
    info.pAttachments = vkAttachments.data();
    info.subpassCount = static_cast<uint32_t>(vkSubpasses.size());
    info.pSubpasses = vkSubpasses.data();
    info.dependencyCount = static_cast<uint32_t>(dependencies.size());
    info.pDependencies = dependencies.data();

    if (vkCreateRenderPass(device, &info, nullptr, &renderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass");
    }
}

bool RenderPass::equalDependency(
    const VkSubpassDependency& a,
    const VkSubpassDependency& b
) {
    return
        a.srcSubpass == b.srcSubpass &&
        a.dstSubpass == b.dstSubpass &&
        a.srcStageMask == b.srcStageMask &&
        a.dstStageMask == b.dstStageMask &&
        a.srcAccessMask == b.srcAccessMask &&
        a.dstAccessMask == b.dstAccessMask &&
        a.dependencyFlags == b.dependencyFlags;
}

void RenderPass::deduplicateDependencies(
    std::vector<VkSubpassDependency>& deps
) {
    std::vector<VkSubpassDependency> unique;
    unique.reserve(deps.size());

    for (const auto& d : deps) {
        bool found = false;
        for (const auto& u : unique) {
            if (equalDependency(d, u)) {
                found = true;
                break;
            }
        }
        if (!found) {
            unique.push_back(d);
        }
    }

    deps.swap(unique);
}


RenderPass::~RenderPass() {
    if (this->renderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(device, this->renderPass, nullptr);
    }
}