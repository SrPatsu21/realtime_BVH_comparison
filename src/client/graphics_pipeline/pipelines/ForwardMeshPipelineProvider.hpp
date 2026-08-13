#include "IPipelineProvider.hpp"

class ForwardMeshPipelineProvider : public IPipelineProvider
{
public:
    void createPipelines(
        GraphicsPipelineManager& manager,
        const PipelineCreationContext& ctx
    ) override;
};