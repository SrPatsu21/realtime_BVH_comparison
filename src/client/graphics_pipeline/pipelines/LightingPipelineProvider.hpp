#pragma once

#include "IPipelineProvider.hpp"

class LightingPipelineProvider : public IPipelineProvider
{
public:

    void createPipelines(
        GraphicsPipelineManager& manager,
        const PipelineCreationContext& ctx
    ) override;
};