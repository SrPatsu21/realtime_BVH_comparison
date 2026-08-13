#pragma once
#include "../../CoreVulkan.hpp"
#include "../GraphicsPipelineManager.hpp"
#include "../pipelines/IPipelineProvider.hpp"

struct IPipelineLayoutProvider
{
    virtual ~IPipelineLayoutProvider() = default;

    virtual GraphicsPipelineManager::PipelineFlags createPipelineLayouts(
        GraphicsPipelineManager& manager,
        const PipelineCreationContext& ctx
    ) = 0;
};