#pragma once
#include "../../CoreVulkan.hpp"
#include "../GraphicsPipelineManager.hpp"
#include "PipelineCreationContext.hpp"

struct IPipelineProvider
{
    virtual ~IPipelineProvider() = default;

    virtual void createPipelines(
        GraphicsPipelineManager& manager,
        const PipelineCreationContext& ctx
    ) = 0;
};