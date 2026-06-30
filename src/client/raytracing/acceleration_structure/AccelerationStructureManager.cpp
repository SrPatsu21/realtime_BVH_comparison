#include "AccelerationStructureManager.hpp"

template<
    typename TLBuilderType,
    typename BLBuilderType
>
void AccelerationStructureManager<
    TLBuilderType,
    BLBuilderType
>::buildTLAS(
    const std::vector<BLASInstance>& instances
)
{
    tlas.nodes.clear();

    std::vector<BLASInstance> localInstances =
        instances;

    TLBuilderType::Build(
        tlas.nodes,
        localInstances
    );
}

template<
    typename TLBuilderType,
    typename BLBuilderType
>
const BLAS<
    typename AccelerationStructureManager<
        TLBuilderType,
        BLBuilderType
    >::BLNodeType
>&
AccelerationStructureManager<
    TLBuilderType,
    BLBuilderType
>::getBLAS(
    uint32_t index
) const {
    return blas_vector[index];
}

template<
    typename TLBuilderType,
    typename BLBuilderType
>
const AccelerationStructure<
    typename AccelerationStructureManager<
        TLBuilderType,
        BLBuilderType
    >::TLNodeType
>&
AccelerationStructureManager<
    TLBuilderType,
    BLBuilderType
>::getTLAS(
) const
{
    return tlas;
}