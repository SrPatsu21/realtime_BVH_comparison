#pragma once

#include "../graphics_pipeline/pipelines/IPipelineProvider.hpp"

class ParticlePipelineProvider : public IPipelineProvider
{
public:
    void createPipelines(
        GraphicsPipelineManager& manager,
        const PipelineCreationContext& ctx
    ) override;
};