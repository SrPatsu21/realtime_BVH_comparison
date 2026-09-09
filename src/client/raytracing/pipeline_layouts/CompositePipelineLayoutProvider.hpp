#pragma once

#include "../../graphics_pipeline/GraphicsPipelineManager.hpp"

class CompositePipelineLayoutProvider
{
public:

    GraphicsPipelineManager::PipelineFlags createPipelineLayouts(
        GraphicsPipelineManager& manager,
        const PipelineCreationContext& ctx
    );
};