#pragma once

class GraphicsPipelineManager;
struct PipelineCreationContext;

class CompositePipelineProvider
{
public:

    void createPipelines(
        GraphicsPipelineManager& manager,
        const PipelineCreationContext& ctx
    );
};