#pragma once

#include "../../graphics_pipeline/pipelines/IPipelineProvider.hpp"

class LightingPipelineProvider : public IPipelineProvider
{
public:

    void createPipelines(
        GraphicsPipelineManager& manager,
        const PipelineCreationContext& ctx
    ) override;
};