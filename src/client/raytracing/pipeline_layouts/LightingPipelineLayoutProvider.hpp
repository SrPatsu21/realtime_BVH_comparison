#pragma once

#include "../../graphics_pipeline/layouts/IPipelineLayoutProvider.hpp"

class LightingPipelineLayoutProvider final
    : public IPipelineLayoutProvider
{
public:

    GraphicsPipelineManager::PipelineFlags createPipelineLayouts(
        GraphicsPipelineManager& manager,
        const PipelineCreationContext& ctx
    ) override;
};