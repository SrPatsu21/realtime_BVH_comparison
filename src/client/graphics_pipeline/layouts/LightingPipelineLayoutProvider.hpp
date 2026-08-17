#pragma once

#include "IPipelineLayoutProvider.hpp"

class LightingPipelineLayoutProvider final
    : public IPipelineLayoutProvider
{
public:

    GraphicsPipelineManager::PipelineFlags createPipelineLayouts(
        GraphicsPipelineManager& manager,
        const PipelineCreationContext& ctx
    ) override;
};