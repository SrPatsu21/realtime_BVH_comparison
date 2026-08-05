#include "IPipelineProvider.hpp"

class MeshPipelineProvider : public IPipelineProvider
{
public:
    void createPipelines(
        GraphicsPipelineManager& manager,
        const PipelineCreationContext& ctx
    ) override;
};