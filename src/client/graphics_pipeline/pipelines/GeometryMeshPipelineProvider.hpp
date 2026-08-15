#pragma once

#include "IPipelineProvider.hpp"

class GeometryMeshPipelineProvider final
    : public IPipelineProvider
{
public:

    void createPipelines(
        GraphicsPipelineManager& manager,
        const PipelineCreationContext& ctx
    ) override;
};