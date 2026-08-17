#include "IPipelineLayoutProvider.hpp"

class MeshPipelineLayoutProvider final
    : public IPipelineLayoutProvider
{
public:

    GraphicsPipelineManager::PipelineFlags createPipelineLayouts(
        GraphicsPipelineManager& manager,
        const PipelineCreationContext& ctx
    ) override;
};